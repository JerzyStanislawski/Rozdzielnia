#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#define REQ_BUF_SZ   1024

#include <Ethernet.h>
#include <string.h>

typedef void (*RespondAction) (String endpoint, Client * client);

class WebClient
{
  public:
    String getRequest(String parameters[10], int & parameterCount, RespondAction respond);
    void Initialize(byte* mac, IPAddress ip);
    
    WebClient(byte* mac, IPAddress ip);

  private:
    EthernetServer server;
    char HTTP_req[REQ_BUF_SZ];
    int req_index;
    
    void RespondHttp(EthernetClient client, bool contentType);
    void ClearHttpReq();
    String GetRequestedEndpoint();
};

#endif

