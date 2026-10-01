#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include <Ethernet.h>

#define HTTP_PORT             80
#define HTTP_MAX_PARAMETERS   10
#define HTTP_ENDPOINT_SIZE    32
#define HTTP_LINE_SIZE        48
#define HTTP_BODY_SIZE       512

typedef void (*RespondAction) (const String & endpoint, Print * out);

class WebClient
{
  public:
    // Serves at most one pending request and keeps the Ethernet chip healthy.
    // Returns the requested endpoint, or an empty string when nothing was served.
    String getRequest(String parameters[HTTP_MAX_PARAMETERS], int & parameterCount, RespondAction respond);

    WebClient(byte* mac, IPAddress ip);

  private:
    EthernetServer server;
    byte* mac;
    IPAddress ip;

    char endpoint[HTTP_ENDPOINT_SIZE];
    char line[HTTP_LINE_SIZE];
    char body[HTTP_BODY_SIZE];

    unsigned long lastChipCheck;
    unsigned long lastSocketCheck;
    unsigned long socketOpenSince[MAX_SOCK_NUM];
    uint16_t socketRemotePort[MAX_SOCK_NUM];

    void StartEthernet();
    void Maintain();
    void CloseStaleSockets(unsigned long now);
    bool ReadRequest(EthernetClient & client);
    void ParseEndpoint();
    void SplitParameters(String parameters[HTTP_MAX_PARAMETERS], int & parameterCount);
};

#endif
