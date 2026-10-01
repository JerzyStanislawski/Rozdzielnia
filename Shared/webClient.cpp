#include <arduino.h>
#include <avr/wdt.h>
#include "webClient.h"
#include <utility/w5100.h>

// The whole request has to arrive within this time, otherwise it is dropped.
#define HTTP_REQUEST_TIMEOUT       1500
// Requests are answered and closed right away, so a connection open this long is dead.
#define HTTP_STALE_SOCKET_TIMEOUT  5000
#define SOCKET_CHECK_INTERVAL       500
#define CHIP_CHECK_INTERVAL        5000

// Collects the response and sends it in chunks. Printing straight to EthernetClient
// sends a separate TCP packet for every print() call and waits for the chip after each.
class ResponseWriter : public Print
{
  public:
    ResponseWriter(EthernetClient & client) : client(client), length(0), failed(false) {}

    virtual size_t write(uint8_t c)
    {
        if (length == sizeof(buffer))
            Flush();
        buffer[length++] = c;
        return 1;
    }

    void Flush()
    {
        if (length > 0 && !failed)
        {
            // Writing to a peer that stopped answering blocks until the chip gives up (~3 s).
            wdt_reset();
            failed = client.write(buffer, length) != length;
        }
        length = 0;
    }

    using Print::write;

  private:
    EthernetClient & client;
    uint8_t buffer[128];
    uint8_t length;
    bool failed;
};

WebClient::WebClient(byte* mac, IPAddress ip) : server(HTTP_PORT)
{
    this->mac = mac;
    this->ip = ip;
    lastChipCheck = 0;
    lastSocketCheck = 0;
    for (uint8_t i = 0; i < MAX_SOCK_NUM; i++)
        socketRemotePort[i] = 0;

    // Deselect the SD card slot of the Ethernet shield, a card left in it would disturb SPI.
    digitalWrite(4, HIGH);
    pinMode(4, OUTPUT);

    StartEthernet();
    server.begin();
}

void WebClient::StartEthernet()
{
    Ethernet.begin(mac, ip);

    // Give up on a peer that stopped answering after ~3 s (200+400+800+1600 ms) instead of
    // the chip default of ~32 s. Until then every write to that peer blocks the whole program.
    Ethernet.setRetransmissionTimeout(200);
    Ethernet.setRetransmissionCount(3);
}

String WebClient::getRequest(String parameters[HTTP_MAX_PARAMETERS], int & parameterCount, RespondAction customRespond)
{
    parameterCount = 0;
    Maintain();

    EthernetClient client = server.available();
    if (!client)
        return String();

    String requested;
    if (ReadRequest(client))
    {
        wdt_reset();
        SplitParameters(parameters, parameterCount);
        requested = endpoint;

        ResponseWriter response(client);
        response.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n"));
        customRespond(requested, &response);
        response.Flush();
    }

    wdt_reset();
    client.stop();
    return requested;
}

void WebClient::Maintain()
{
    unsigned long now = millis();

    // A power glitch or interference can reset the W5x00 on its own while the Arduino keeps
    // running. The chip then forgets its IP address and closes all sockets.
    if (now - lastChipCheck >= CHIP_CHECK_INTERVAL)
    {
        lastChipCheck = now;
        if (!(Ethernet.localIP() == ip))
            StartEthernet();  // server.available() opens a new listening socket by itself
    }

    if (now - lastSocketCheck >= SOCKET_CHECK_INTERVAL)
    {
        lastSocketCheck = now;
        CloseStaleSockets(now);
    }
}

// The Ethernet library hands out only connections that have data to read. A client that
// connects and goes silent (phone leaving Wi-Fi, port scanner, browser pre-connect) keeps
// its socket forever. The W5100 has only 4 sockets, so a few of those stop the server for good.
void WebClient::CloseStaleSockets(unsigned long now)
{
    uint8_t sockets = W5100.getChip() == 51 ? 4 : MAX_SOCK_NUM;
    for (uint8_t i = 0; i < sockets; i++)
    {
        EthernetClient socket(i);
        uint8_t status = socket.status();
        if (status == SnSR::CLOSED || status == SnSR::LISTEN || socket.localPort() != HTTP_PORT)
        {
            socketRemotePort[i] = 0;
            continue;
        }
        if (socket.available() > 0)
            continue;  // a request is waiting, server.available() hands it out next

        // Remote port tells apart a new connection that reused the socket.
        uint16_t remotePort = socket.remotePort();
        if (socketRemotePort[i] != remotePort)
        {
            socketRemotePort[i] = remotePort;
            socketOpenSince[i] = now;
        }
        else if (now - socketOpenSince[i] > HTTP_STALE_SOCKET_TIMEOUT)
        {
            socket.setConnectionTimeout(100);
            socket.stop();
            socketRemotePort[i] = 0;
        }
    }
}

// Reads the request line, the headers that matter and the body (one line of parameters).
// Returns false when the client disconnects or does not send the whole request in time.
bool WebClient::ReadRequest(EthernetClient & client)
{
    unsigned long start = millis();
    long contentLength = -1;
    bool expectContinue = false;
    bool requestLine = true;
    byte lineLength = 0;

    endpoint[0] = 0;
    body[0] = 0;

    while (true)
    {
        if (millis() - start > HTTP_REQUEST_TIMEOUT)
            return false;

        int c = client.read();
        if (c < 0)
        {
            if (!client.connected())
                return false;
            continue;
        }
        if (c == '\r')
            continue;
        if (c != '\n')
        {
            if (lineLength < HTTP_LINE_SIZE - 1)
                line[lineLength++] = c;
            continue;
        }

        line[lineLength] = 0;
        if (requestLine)
        {
            if (lineLength == 0)
                continue;
            ParseEndpoint();
            requestLine = false;
        }
        else if (lineLength == 0)
            break;
        else if (strncasecmp_P(line, PSTR("Content-Length:"), 15) == 0)
            contentLength = atol(line + 15);
        else if (strncasecmp_P(line, PSTR("Expect:"), 7) == 0)
            expectContinue = true;
        lineLength = 0;
    }

    if (expectContinue && contentLength > 0)
    {
        ResponseWriter response(client);
        response.print(F("HTTP/1.1 100 Continue\r\n\r\n"));
        response.Flush();
    }

    // Without Content-Length only what has already arrived is taken.
    int bodyLength = 0;
    while (contentLength != 0)
    {
        if (millis() - start > HTTP_REQUEST_TIMEOUT)
            return false;

        int c = client.read();
        if (c < 0)
        {
            if (contentLength < 0)
                break;
            if (!client.connected())
                return false;
            continue;
        }
        if (contentLength > 0)
            contentLength--;
        if (c == '\n')
            break;
        if (c != '\r' && bodyLength < HTTP_BODY_SIZE - 1)
            body[bodyLength++] = c;
    }
    body[bodyLength] = 0;
    return true;
}

// "POST /impulsOswietlenie HTTP/1.1" -> "impulsOswietlenie"
void WebClient::ParseEndpoint()
{
    byte length = 0;
    char * path = strchr(line, ' ');
    if (path != NULL && path[1] == '/')
    {
        path += 2;
        while (path[length] != 0 && path[length] != ' ' && length < HTTP_ENDPOINT_SIZE - 1)
            length++;
        memcpy(endpoint, path, length);
    }
    endpoint[length] = 0;
}

// "a=1;b=2;" -> parameters "a=1", "b=2", parameterCount 2 (the count of ';').
void WebClient::SplitParameters(String parameters[HTTP_MAX_PARAMETERS], int & parameterCount)
{
    parameterCount = 0;
    char * start = body;
    while (true)
    {
        char * separator = strchr(start, ';');
        if (separator != NULL)
            *separator = 0;
        parameters[parameterCount] = start;

        if (separator == NULL || parameterCount == HTTP_MAX_PARAMETERS - 1)
            break;
        parameterCount++;
        start = separator + 1;
    }

    for (int i = parameterCount + 1; i < HTTP_MAX_PARAMETERS; i++)
        parameters[i] = "";
}
