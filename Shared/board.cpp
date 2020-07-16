#include <arduino.h>
#include "lights.h"
#include "blinds.h"
#include "scheduler.h"
#include "board.h"

#include <Time.h>
#include <TimeLib.h>
#include <DS3232RTC.h>

Lights * Board::lights;
Blinds * Board::blinds;
Scheduler * Board::scheduler;
bool Board::twilightMode;

void Board::ProcessHttpRequest(WebClient webClient)
{
  int parameterCount;
  String endpoint = webClient.getRequest(httpParameters, parameterCount, HttpCustomRespond);

  if (String(endpoint) == String("impulsOswietlenie") || String(endpoint) == String("impulsRolety"))
  {
    bool value;
    String parameter = ParseHttpBoolParameter(httpParameters[0], &value);
    
    if (String(endpoint) == String("impulsOswietlenie"))
    {
      if (String(parameter) == String("allOff"))
        lights->AllLightsOff();
      else
        lights->SwitchLight(parameter, value ? HIGH : LOW);
    }
    else if (String(endpoint) == String("impulsRolety"))
    {
      if (parameter == String("allRoletyUp"))
        blinds->AllBlindsUp();
      else if (parameter == String("allRoletyDown"))
        blinds->AllBlindsDown();
      else
        blinds->MoveBlind(httpParameters[0]);
    }
  }  
  else if (endpoint == String("enableTwilightMode"))
    twilightMode = true;
  else if (endpoint == String("disableTwilightMode"))
    twilightMode = false;
  else if (endpoint == String("setTime"))
  {
    String timeValue;
    ParseHttpParameter(httpParameters[0], &timeValue);

    int hours = timeValue.substring(0, 2).toInt();
    int minutes = timeValue.substring(3, 5).toInt();
    int seconds = timeValue.substring(6, 8).toInt();

    int day = timeValue.substring(9, 11).toInt();
    int month = timeValue.substring(12, 14).toInt();
    int year = timeValue.substring(15, 19).toInt();
    
    setTime(hours, minutes, seconds, day, month, year);
    RTC.set(now());
  }
  else if (endpoint == String("schedule"))
  {
    String * records = new String[parameterCount];
    for (int i = 0; i < parameterCount; i++)
    {
      String record;
      ParseHttpParameter(httpParameters[i], &record);
      records[i] = record;
    }
    scheduler->Schedule(records, parameterCount);  
    delete [] records;  
  }
  else if (endpoint == String("clearSchedule"))
  {
	  scheduler->Clear();
  }
}

void Board::HttpCustomRespond(String endpoint, Client * client)
{
  if (endpoint == String("getStatus"))
  {
     Board::lights->WriteStatus(client);
  }
  else if (endpoint == String("getScheduledEvents"))
  {
    Board::scheduler->WriteEvents(client);
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
  else if (endpoint == String("getTwilightMode"))
    client->println(Board::twilightMode);
}


String Board::ParseHttpParameter(String parameters, String * outValue)
{
    int equalsIndex = parameters.indexOf('=');
    String parameter = parameters.substring(0, equalsIndex);
    *outValue = parameters.substring(equalsIndex + 1);

    return parameter;
}

String Board::ParseHttpBoolParameter(String parameters, bool * outValue)
{
    String boolString;
    String parameter = ParseHttpParameter(parameters, &boolString);

    *outValue = (boolString == "true");
    return parameter;
}
