#include <Event.h>
#include <Timer.h>
#include <string.h>
#include <EEPROM.h>

#include <Time.h>
#include <TimeLib.h>
#include <DS3232RTC.h>

#include "scheduler.h"
#include "webClient.h"
#include "lights.h"
#include "blinds.h"
#include "board.h"

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 25); 
WebClient * webClient;

Timer timer;

Lights lights;
Blinds blinds;
Scheduler scheduler(&lights, &blinds);
Board board(&lights, &blinds, &scheduler);

bool holidayLighting;
String currentHolidayRoom;
byte currentHolidayHour;
byte currentHolidayMinute;

bool firstLoop;

void setup()
{    
  Serial.begin(9600);
  
  InitLights();
  InitBlinds();
  InitPins();

  int address = board.RestoreSettings();
  scheduler.RestoreScheduledEvents(address);
  
  webClient = new WebClient(mac, ip);
   
  timer.every(60000, HandleAutoEvents);
  
  ResetHolidaySettings();
  firstLoop = true;
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
  board.ProcessHttpRequest(*webClient);
  lights.CheckAndSwitchLights();

  if (firstLoop)
  {
    lights.AllLightsOff();
    firstLoop = false;
  }

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

void HandleAutoEvents()
{      
  tmElements_t tm;
  RTC.read(tm);
  int hour = tm.Hour;
  int minute = tm.Minute;

  scheduler.Execute(hour, minute, tm.Wday);
  board.TimerEvent(tm);
  
  if (board.GetHolidayMode())
  {
  	if (hour == 19 && minute == 50)
  	{
  		holidayLighting = true;
  		ChangeHolidayLight(hour, minute);
  	}
  	if (hour == 22 && minute == 0)
  	{
  		ResetHolidaySettings();
  		lights.AllLightsOff();
  	}
  	if (holidayLighting)
  	{
  		if (hour*60 + minute > currentHolidayHour*60 + currentHolidayMinute + 10 + random(10))
  			ChangeHolidayLight(hour, minute);
  	}
  }
}

void ChangeHolidayLight(int hour, int minute)
{
	currentHolidayHour = hour;
	currentHolidayMinute = minute;
	
	if (currentHolidayRoom != String(""))
		lights.SwitchLight(currentHolidayRoom, LOW);
	
	delay(5000 + random(10)*1000);
	
	switch (random(4))
	{
		case 0:
			currentHolidayRoom = "salon_led";
			break;
		case 1:
			currentHolidayRoom = "jadalnia";
			break;
		case 2:
			currentHolidayRoom = "salon_kominek";
			break;
		case 3:
			currentHolidayRoom = "hall";
			break;
	}
	lights.SwitchLight(currentHolidayRoom, HIGH);
}

void ResetHolidaySettings()
{
	holidayLighting = false;
	currentHolidayRoom = "";
	currentHolidayHour = 0;
	currentHolidayMinute = 0;
}
