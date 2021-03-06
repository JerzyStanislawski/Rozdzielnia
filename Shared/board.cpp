#include <arduino.h>
#include "lights.h"
#include "blinds.h"
#include "scheduler.h"
#include "board.h"

#include <Time.h>
#include <TimeLib.h>
#include <DS3232RTC.h>
#include <EEPROM.h>

Lights * Board::lights;
Blinds * Board::blinds;
Scheduler * Board::scheduler;
bool Board::twilightMode;
bool Board::morningMode;
bool Board::holidayMode;
byte Board::morningHour;
byte Board::morningMinute;
byte Board::morningDays;

int Board::settingsEndAddress;

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
	
	return;
  }  
  else if (endpoint == String("enableTwilightMode"))
    twilightMode = true;
  else if (endpoint == String("disableTwilightMode"))
    twilightMode = false;
  else if (endpoint == String("enableHolidayMode"))
    holidayMode = true;
  else if (endpoint == String("disableHolidayMode"))
    holidayMode = false;
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
    scheduler->Schedule(records, parameterCount, Board::settingsEndAddress);  
    delete [] records;  
  }
  else if (endpoint == String("clearSchedule"))
  {
	  scheduler->Clear(Board::settingsEndAddress);
  }
  else if (endpoint == String("setMorningMode"))
  {
	if (httpParameters[0] == String("false"))
		morningMode = false;
	else if (httpParameters[0] == String("true"))
	{
		String morningDaysValue;
		ParseHttpParameter(httpParameters[1], &morningDaysValue);
		morningDays = morningDaysValue.toInt();
		
		String morningTimeValue;
		ParseHttpParameter(httpParameters[2], &morningTimeValue);
		
		morningHour = morningTimeValue.substring(0, 2).toInt();
		morningMinute = morningTimeValue.substring(3, 5).toInt();
		
		morningMode = true;
	}
  }
  
  StoreSettings();
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
	 Board::PrintTime(client);
  }
  else if (endpoint == String("getTwilightMode"))
  {
    client->println(Board::twilightMode);
  }
  else if (endpoint == String("getHolidayMode"))
  {
    client->println(Board::holidayMode);
  }
  else if (endpoint == String("getMorningMode"))
  {
	Board::PrintMorningMode(client);
  }
  else if (endpoint == String("getAllSettings"))
  {
	Board::PrintTime(client);
	client->print("holidayMode: ");
    client->println(Board::holidayMode);
	client->print("twilightMode: ");
    client->println(Board::twilightMode);
	Board::PrintMorningMode(client);
  }
}

void Board::PrintMorningMode(Client * client)
{
	client->print("morningMode: ");
    client->println(Board::morningMode);
	client->print("morningDays: ");
    client->println(Board::morningDays);
	client->print("morningTime: ");
	client->print(Board::morningHour, DEC);
	client->print(':');
	client->println(Board::morningMinute, DEC);
}

void Board::PrintTime(Client * client)
{
    tmElements_t tm;
    RTC.read(tm);            
  
    client->print("Time: ");
    client->print(tm.Hour, DEC);
    client->print(':');
    client->print(tm.Minute, DEC);
    client->print(':');
    client->println(tm.Second, DEC);
    client->print("Date: ");
    client->print(tm.Year + 1970, DEC);
    client->print('-');
    client->print(tm.Month, DEC);
    client->print('-');
    client->println(tm.Day, DEC);
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

void Board::TimerEvent(tmElements_t tm)
{		
	if (Board::twilightMode)
	{
	  int offset = tm.Month > 10 || tm.Month < 4 ? 1 : 2;
	  int twilightTime = twilight[tm.Month - 1][tm.Day - 1];
	  int twilightHour = (twilightTime / 100) + offset;
	  int twilightMinute = twilightTime % 100;
	  
	  if (tm.Hour == twilightHour && tm.Minute == twilightMinute)
		 blinds->AllBlindsDown();
	}
	
	if (Board::morningMode)
	{
	  if (tm.Hour == Board::morningHour && tm.Minute == morningMinute && (morningDays & (1 << tm.Wday)) == (1 << tm.Wday))
		 blinds->AllBlindsUp();	 
	}
}

bool Board::GetHolidayMode()
{
	return holidayMode;
}

void Board::StoreSettings()
{
	int eeAddress = 0;
	
	EEPROM.put(eeAddress, Board::twilightMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.put(eeAddress, Board::morningMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.put(eeAddress, Board::holidayMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.put(eeAddress, Board::morningHour);		
	eeAddress += sizeof(byte);
	
	EEPROM.put(eeAddress, Board::morningMinute);		
	eeAddress += sizeof(byte);
	
	EEPROM.put(eeAddress, Board::morningDays);		
	eeAddress += sizeof(byte);
	
}

int Board::RestoreSettings()
{
	int eeAddress = 0;
	
	EEPROM.get(eeAddress, Board::twilightMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.get(eeAddress, Board::morningMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.get(eeAddress, Board::holidayMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.get(eeAddress, Board::morningHour);		
	eeAddress += sizeof(byte);
	
	EEPROM.get(eeAddress, Board::morningMinute);		
	eeAddress += sizeof(byte);
	
	EEPROM.get(eeAddress, Board::morningDays);		
	eeAddress += sizeof(byte);
	
	Board::settingsEndAddress = eeAddress;
	
	return eeAddress;
}
