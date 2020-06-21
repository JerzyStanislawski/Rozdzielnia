#include <string.h>

#include <Event.h>
#include <Timer.h>

#include <Time.h>
#include <TimeLib.h>

#include <DS3232RTC.h>

#include "webClient.h"
#include "lights.h"
#include "blinds.h"

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 25); 
WebClient * webClient;

Timer timer;
bool wejscieOn;
bool blindsDownEarlyMorningMode;

Lights lights;
Blinds blinds;

void setup()
{    
  Serial.begin(9600);
  
  InitLights();
  InitBlinds();
  InitPins();

  webClient = new WebClient(mac, ip);
  
  wejscieOn = false;
  blindsDownEarlyMorningMode = true;

  //setTime(19, 51, 0, 14, 6, 2020);
  //Serial.println(RTC.set(now()));
  //Serial.println("time set");
  timer.every(60 * 1000UL, HandleAutoEvents);
}

void InitLights()
{
  lights.AddLight(24, 0, "garaz", 0);
  lights.AddLight(25, 23, "kotlownia", 0);
  lights.AddLight(26, 2, "wiatrolap", 3);
  lights.AddLight(28, 22, "hall", 0);
  lights.AddLight(30, 6, "salon_led", 0);
  lights.AddLight(31, 7, "jadalnia", 0);
  lights.AddLight(32, 8, "kuchnia", 0);
  lights.AddLight(33, 9, "barek", 0);
  lights.AddLight(34, 10, "spizarnia", 0);
  lights.AddLight(35, 11, "gabinet", 0);
  lights.AddLight(36, 12, "lazienka", 0);
  lights.AddLight(37, 13, "wejscie", 0);
  lights.AddLight(38, 14, "wejscie_kolumna", 0);
  lights.AddLight(39, 15, "front", 0);
  lights.AddLight(40, 16, "salon_kominek", 0);
  lights.AddLight(41, 17, "lazienka_lustro", 0);
  lights.AddLight(42, 18, "salon", 0);
  lights.AddLight(43, 19, "kuchnia_szafki", 0);
}

void InitBlinds()
{
  blinds.AddBlind(45, "sypialnia_up", BlindDirection::UP);
  blinds.AddBlind(46, "sypialnia_down", BlindDirection::DOWN);
  blinds.AddBlind(47, "pokoj1_1_up", BlindDirection::UP);
  blinds.AddBlind(48, "pokoj1_1_down", BlindDirection::DOWN);
  blinds.AddBlind(49, "pokoj1_2_up", BlindDirection::UP);
  blinds.AddBlind(27, "pokoj1_2_down", BlindDirection::DOWN);
  blinds.AddBlind(44, "pokoj2_up", BlindDirection::UP);
  blinds.AddBlind(53, "pokoj2_down", BlindDirection::DOWN);
}

void InitPins()
{
  lights.DoForEach(InitPinsForLights);
  blinds.DoForEach(InitPinsForBlinds);

  pinMode(50, OUTPUT);
  pinMode(51, OUTPUT);
  pinMode(52, OUTPUT);
}

void loop()
{
  ProcessHttpRequest(*webClient);
  lights.CheckAndSwitchLights();

   timer.update();
}

void InitPinsForLights(byte output, String room, byte mainSwitch, byte altSwitch)
{
  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);
  
  if (mainSwitch != 0)
    pinMode(mainSwitch, INPUT_PULLUP);  
  if (altSwitch != 0)
    pinMode(altSwitch, INPUT_PULLUP); 
}

void InitPinsForBlinds(byte output, String room)
{
  pinMode(output, OUTPUT);
}

void HttpCustomRespond(String endpoint, Client * client)
{
  if (endpoint == String("getStatus"))
  {
     lights.WriteStatus(client);
  }
  else if (endpoint == String("getTime"))
  {
    tmElements_t tm;
    RTC.read(tm);            
  
    client->print(tm.Hour, DEC);
    client->print(':');
    client->print(tm.Minute, DEC);
    client->print(':');
    client->println(tm.Second, DEC);
    client->print("Week day:");
    client->println(tm.Wday, DEC);
  }  
  else if (endpoint == String("enableBlindsDownEarlyMorningMode"))
    blindsDownEarlyMorningMode = true;
  else if (endpoint == String("disableBlindsDownEarlyMorningMode"))
    blindsDownEarlyMorningMode = false;
  else if (endpoint == String("getBlindsDownEarlyMorningMode"))
    client->println(blindsDownEarlyMorningMode);
}

void ProcessHttpRequest(WebClient webClient)
{
  String parameters;
  String endpoint = webClient.getRequest(parameters, HttpCustomRespond);

  if (String(endpoint) == String("impulsOswietlenie") || String(endpoint) == String("impulsRolety"))
  {
    bool value;
    String parameter = ParseHttpParameters(parameters, &value);
    
    if (String(endpoint) == String("impulsOswietlenie"))
    {
      lights.SwitchLight(parameter, value ? HIGH : LOW);
    }
    else if (String(endpoint) == String("impulsRolety"))
    {
      if (parameter == String("allRoletyUp"))
        blinds.AllBlindsUp();
      else if (parameter == String("allRoletyDown"))
        blinds.AllBlindsDown();
      else
        blinds.MoveBlind(parameters);
    }
  }
}

String ParseHttpParameters(String parameters, bool * outValue)
{
    int equalsIndex = parameters.indexOf('=');
    String parameter = parameters.substring(0, equalsIndex);
    String boolString = parameters.substring(equalsIndex + 1);

    *outValue = (boolString == "true");
    return parameter;
}

void HandleAutoEvents()
{
  tmElements_t tm;
  RTC.read(tm);
  
  if (blindsDownEarlyMorningMode && tm.Hour == 4 && tm.Minute == 0)
    blinds.AllBlindsDown();
}
//  
//  if (wejscieOn == false)
//  {
//    if (tm.Hour == 16)
//    {
//      wejscieOn = true;
//      digitalWrite(OSW_WEJSCIE_GLOWNE, HIGH);
//    }    
//  }
//  else
//  {    
//    if (tm.Hour == 23)
//    {
//      wejscieOn = false;
//      digitalWrite(OSW_WEJSCIE_GLOWNE, LOW);
//    }
//  }
//
//  if (tm.Hour == 4 && tm.Minute == 30)
//  {
//    Roleta(ROL_POKOJ1_1_DOWN);
//    Roleta(ROL_POKOJ1_2_DOWN);
//  }
//  
//}



