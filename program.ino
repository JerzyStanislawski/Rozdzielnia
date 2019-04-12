
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <string.h>

#include <Event.h>
#include <Timer.h>

#include <Time.h>
#include <TimeLib.h>

#include <DS3232RTC.h>

const int OSW_GARAZ = 24;
const int OSW_KOTLOWNIA = 25;
const int OSW_WIATROLAP = 26;
const int OSW_WOLNE1 = 27;
const int OSW_KORYTARZ = 28;
const int OSW_WOLNE2 = 29;
const int OSW_SALON_LED = 30;
const int OSW_JADALNIA = 31;
const int OSW_KUCHNIA = 32;
const int OSW_BAREK = 33;
const int OSW_SPIZARNIA = 34;
const int OSW_GABINET = 35;
const int OSW_LAZIENKA = 36;
const int OSW_WEJSCIE_GLOWNE = 37;
const int OSW_WEJSCIE_KOLUMNA = 38;
const int OSW_FRONT = 39;
const int OSW_SALON_KOMINEK = 40;
const int OSW_LAZIENKA_LUSTRO = 41;
const int OSW_SALON = 42;
const int OSW_KUCHNIA_SZAFKI = 43;
const int ROL_POKOJ2_UP = 44;
const int ROL_SYPIALNIA_UP = 45;
const int ROL_SYPIALNIA_DOWN = 46;
const int ROL_POKOJ1_1_UP = 47;
const int ROL_POKOJ1_1_DOWN = 48;
const int ROL_POKOJ1_2_UP = 49;
const int ROL_POKOJ1_2_DOWN = 27;
const int ROL_POKOJ2_DOWN = 53;


// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   1024

// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 25); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
int req_index = 0;              // index into HTTP_req buffer

const int OSW_COUNT = 24;
int currentInputState[OSW_COUNT] = {HIGH};
int currentOutputState[OSW_COUNT] = {LOW};

Timer timer;
bool wejscieOn;

void setup()
{
    // disable Ethernet chip
    // pinMode(53, OUTPUT);
    // digitalWrite(53, HIGH);
    
    Serial.begin(9600);       // for debugging
    
    // initialize SD card
//    Serial.println("Initializing SD card...");
//    if (!SD.begin(4)) {
//        Serial.println("ERROR - SD card initialization failed!");
//        return;    // init failed
//    }
//    Serial.println("SUCCESS - SD card initialized.");
//    // check for index.htm file
//    if (!SD.exists("index.htm")) {
//        Serial.println("ERROR - Can't find index.htm file!");
//        return;  // can't find index file
//    }
//    Serial.println("SUCCESS - Found index.htm file.");

  for (int i = 0; i < OSW_COUNT ; i++)
  {
    pinMode(i, INPUT_PULLUP);
  }
  Serial.println("Pins input set.");
  Serial.println();
  
   for (int i = 24; i <= 53; i++)
   {
      pinMode(i, OUTPUT);
      digitalWrite(i, LOW);
   }
    Serial.println("Pins set.");

    for (int i = 0; i < OSW_COUNT; i++)
    {
      currentInputState[i] = HIGH;
      currentOutputState[i] = LOW;
    }
                            
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients
    
  tmElements_t tm;
  RTC.read(tm);
  int hour = tm.Hour;
  int minute = tm.Minute;
  //setTime(hour, 8, 58, 18, 2, 2018);
  //RTC.set(now());
  Serial.println(hour);
  Serial.println(minute);
  wejscieOn = false;
}

void loop()
{
    EthernetClient client = server.available();

    if (client) {  // got client?
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
                    char buf[128];
                    char* fileName = GetRequestedFileName(HTTP_req, buf);

                    if (String(fileName) == String("impulsRolety") || String(fileName) == String("impulsOswietlenie"))
                    {
                      String parameters;
                      char c2 = ' ';
                      while (client.connected() && client.available() && c2 != '\n') {
                        c2 = client.read();
                        parameters += c2;
                      }
                      if (String(fileName) == String("impulsRolety"))
                        HandleRoletyChange(parameters);
                      else if (String(fileName) == String("impulsOswietlenie"))
                      {
                        bool value;
                        String parameter = ParseParameters(parameters, &value);
                        HandleOswietlenieChange(parameter, value ? HIGH : LOW);
                      }
                      RespondHttp(client, true);
                    }
                    else if (String(fileName) == String("getStatus"))
                    {
                      RespondHttp(client, true);
                      for (int i = 0; i < OSW_COUNT; i++)
                      {
                        int output = MapInputToOutput(i);
                        client.print(output);
                        client.print('=');
                        client.println(currentOutputState[i]);
                      }
                    }

                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;                    
                }
                if (c == '\n')
                    currentLineIsBlank = true;
                else if (c != '\r') 
                    currentLineIsBlank = false;
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);
        client.flush();
        client.stop();
    } // end if (client)
    else
    {
      for (int i = 0; i < OSW_COUNT; i++)
      {
        if (i == 1 || i == 13 || i == 20 || i == 21)
          continue;
          
        int state = digitalRead(i);
        if (currentInputState[i] != state)
        {         
          int outputPin = MapInputToOutput(i);          
          int outputValue = currentOutputState[i] == HIGH ? LOW : HIGH;
          Light(outputPin, outputValue);
          
          currentInputState[i] = state;
        }
      }
      delay(10);
    }        
  //timer.update();
}

void RespondHttp(EthernetClient client, bool contentType)
{
  client.println("HTTP/1.1 200 OK");
  if (contentType)
    client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();  
}

void HandleAutoEvents()
{
  Serial.println("HandleAutoEvents");

  tmElements_t tm;
  RTC.read(tm);
  
  Serial.println(tm.Second);
  if (wejscieOn == false)
  {
    if (tm.Hour == 16)
    {
      wejscieOn = true;
      digitalWrite(OSW_WEJSCIE_GLOWNE, HIGH);
    }    
  }
  else
  {    
    if (tm.Hour == 23)
    {
      wejscieOn = false;
      digitalWrite(OSW_WEJSCIE_GLOWNE, LOW);
    }
  }
  
}

int MapInputToOutput(int input)
{
  if (input == 22)
    return OSW_KORYTARZ;
  if (input == 23)
    return OSW_KOTLOWNIA;
  if (input == 3)
    return OSW_WIATROLAP;
  return input + 24;
}

int MapOutputToIndex(int output)
{
  if (output == OSW_KORYTARZ)
    return 22;
  if (output == OSW_KOTLOWNIA)
    return 23;
  return output - 24;
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

void Roleta(int roleta)
{      
    digitalWrite(roleta, HIGH);   
    delay(1000);
    digitalWrite(roleta, LOW);  
}

void Light(int light, int value)
{  
    digitalWrite(light, value);
    int index = MapOutputToIndex(light);
    currentOutputState[index] = value;

    MultipleInputsToOneOutput(index);
}

void MultipleInputsToOneOutput(int index)
{
  if (index == 2)
    currentOutputState[3] = currentOutputState[2];
  if (index == 3)
    currentOutputState[2] = currentOutputState[3];
}

void AllRoletyUp()
{
    int roletyUp[] = {ROL_SYPIALNIA_UP, ROL_POKOJ1_1_UP, ROL_POKOJ1_2_UP, ROL_POKOJ2_UP};
    for (int i = 0; i < 4; i++)
    {
      Roleta(roletyUp[i]);
    }  
}


void AllRoletyDown()
{
    int roletyDown[] = {ROL_SYPIALNIA_DOWN, ROL_POKOJ1_1_DOWN, ROL_POKOJ1_2_DOWN, ROL_POKOJ2_DOWN};
    for (int i = 0; i < 4; i++)
    {
      Roleta(roletyDown[i]);
    }
}

void HandleRoletyChange(String parameter)
{ 
    if (parameter == "allRoletyUp")
    {
      AllRoletyUp();
      return;
    }
    if (parameter == "allRoletyDown")
    {
      AllRoletyDown();
      return;
    }
    
    int output = 0;
    if (parameter == "sypialnia_up")
      output = ROL_SYPIALNIA_UP;
    if (parameter == "sypialnia_down")
      output = ROL_SYPIALNIA_DOWN;
    if (parameter == "pokoj1_1_up")
      output = ROL_POKOJ1_1_UP;
    if (parameter == "pokoj1_1_down")
      output = ROL_POKOJ1_1_DOWN;
    if (parameter == "pokoj1_2_up")
      output = ROL_POKOJ1_2_UP;
    if (parameter == "pokoj1_2_down")
      output = ROL_POKOJ1_2_DOWN;
    if (parameter == "pokoj2_up")
      output = ROL_POKOJ2_UP;
    if (parameter == "pokoj2_down")
      output = ROL_POKOJ2_DOWN;
    
    Roleta(output);
}

void AllLightsOff()
{
       for (int i = 0; i < OSW_COUNT; i++)
       {
          if (currentOutputState[i] == HIGH)
          {
            int outputPin = MapInputToOutput(i);
            Light(outputPin, LOW);
          }
       }
}

void HandleOswietlenieChange(String parameter, int value)
{
    if (parameter == "allOff")
    {
       AllLightsOff();
       return;
    }
    
    int output = 0;
    if (parameter == "garaz")
      output = OSW_GARAZ;
    if (parameter == "kotlownia")
      output = OSW_KOTLOWNIA;
    if (parameter == "wiatrolap")
      output = OSW_WIATROLAP;
    if (parameter == "hall")
      output = OSW_KORYTARZ;
    if (parameter == "salon")
      output = OSW_SALON;
    if (parameter == "salon_led")
      output = OSW_SALON_LED;
    if (parameter == "jadalnia")
      output = OSW_JADALNIA;
    if (parameter == "barek")
      output = OSW_BAREK;
    if (parameter == "kuchnia")
      output = OSW_KUCHNIA;
    if (parameter == "spizarnia")
      output = OSW_SPIZARNIA;
    if (parameter == "gabinet")
      output = OSW_GABINET;
    if (parameter == "lazienka")
      output = OSW_LAZIENKA;
    if (parameter == "wejscie")
      output = OSW_WEJSCIE_GLOWNE;
    if (parameter == "wejscie_kolumna")
      output = OSW_WEJSCIE_KOLUMNA;
    if (parameter == "front")
      output = OSW_FRONT;
    if (parameter == "salon_kominek")
      output = OSW_SALON_KOMINEK;
    if (parameter == "kuchnia_szafki")
      output = OSW_KUCHNIA_SZAFKI;
    if (parameter == "lazienka_lustro")
      output = OSW_LAZIENKA_LUSTRO;

    if (output == 0)
      return;
      
    Light(output, value);
}

String ParseParameters(String parameters, bool * outValue)
{
    int equalsIndex = parameters.indexOf('=');
    String parameter = parameters.substring(0, equalsIndex);
    String boolString = parameters.substring(equalsIndex + 1);

    *outValue = (boolString == "true");
    return parameter;
}

char* GetRequestedFileName(char* httpReq, char * buf)
{
    String request(httpReq);
    int postIndex = request.indexOf("POST /");
    
    String requestStart(postIndex < 0 ? "GET /" : "POST /");
    int requestedStartLength = requestStart.length();
    int endOfResource = request.indexOf(" ", requestedStartLength);

    if (endOfResource == requestedStartLength)
      return "index.htm";
      
    String requestedFile = request.substring(requestedStartLength, endOfResource);
    requestedFile.toCharArray(buf, requestedFile.length() + 1);
    return buf;
}

