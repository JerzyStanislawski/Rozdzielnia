#include <arduino.h>
#include "webClient.h"
#include <string.h>

WebClient::WebClient(byte* mac, IPAddress ip) : server(80)
{   
    ClearHttpReq();
    req_index = 0;    

    Ethernet.begin(mac, ip);  
    server.begin();           
}

String WebClient::getRequest(String parameters[10], int & parameterCount, RespondAction customRespond)
{
    EthernetClient client = server.available();
    String endpoint = "";
	parameterCount = 0;
    
    if (client)     
    { 
        boolean currentLineIsBlank = true;
        while (client.connected()) 
        {
            if (client.available()) 
            { 
                char c = client.read();
                if (req_index < (REQ_BUF_SZ - 1)) 
                {
                    HTTP_req[req_index] = c;
                    req_index++;
                }
                if (c == '\n' && currentLineIsBlank) 
                {
                    endpoint = GetRequestedEndpoint();
                    
                    char c2 = ' ';
					parameters[0] = String("");
                    while (client.connected() && client.available() && c2 != '\n') 
                    {
                      c2 = client.read();
					  if (c2 == ';')
					  {
						  parameterCount++;
						  parameters[parameterCount] = String("");
						  continue;
					  }
                      parameters[parameterCount] += c2;
                    }
                    
                    RespondHttp(client, true);
                    customRespond(endpoint, &client);
                  
                    req_index = 0;
                    ClearHttpReq();
                    break;                    
                }
                if (c == '\n')
                    currentLineIsBlank = true;
                else if (c != '\r') 
                    currentLineIsBlank = false;
            }
        } 
        delay(1);
        client.flush();
        client.stop();
    }
    return endpoint;
}

void WebClient::RespondHttp(EthernetClient client, bool contentType)
{
  client.println("HTTP/1.1 200 OK");
  if (contentType)
    client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();  
}

String WebClient::GetRequestedEndpoint()
{
    String request(HTTP_req);
	Serial.println(request);
    int postIndex = request.indexOf("POST /");
    
    String requestStart(postIndex < 0 ? "GET /" : "POST /");
    int requestedStartLength = requestStart.length();
    int endOfResource = request.indexOf(" ", requestedStartLength);
      
    return request.substring(requestedStartLength, endOfResource);
}

void  WebClient::ClearHttpReq()
{
  for (int i = 0; i < REQ_BUF_SZ; i++) {
    HTTP_req[i] = 0;
  }
}


