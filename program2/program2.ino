#include <Event.h>
#include <Timer.h>
#include <string.h>
#include <EEPROM.h>
#include <Wire.h>
#include <avr/wdt.h>

#include <Time.h>
#include <TimeLib.h>
#include <DS3232RTC.h>

#include "scheduler.h"
#include "webClient.h"
#include "lights.h"
#include "blinds.h"
#include "board.h"

byte mac[] = { 0xDE, 0xDA, 0xFE, 0xEF, 0xAE, 0xBD };
IPAddress ip(192, 168, 1, 26); 
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
bool holidayLightPending;
unsigned long holidayLightOnTime;

bool firstLoop;

void setup()
{    
  Serial.begin(9600);

  // A disturbed I2C bus must not hang the program while reading the clock.
  Wire.setWireTimeout(25000, true);
  
  InitLights();
  InitBlinds();
  InitPins();

  int address = board.RestoreSettings();
  scheduler.RestoreScheduledEvents(address);

  webClient = new WebClient(mac, ip);
   
  timer.every(60000, HandleAutoEvents);
  
  ResetHolidaySettings();
  firstLoop = true;

  // Restarts the board if anything hangs for 8 s, instead of staying dead until power cycle.
  wdt_enable(WDTO_8S);
}

void InitLights()
{
  lights.AddLight(24, 2, "sypialnia", 0);
  lights.AddLight(25, 5, "pokoj1", 0);
  lights.AddLight(34, 6, "pokoj2", 0);
  lights.AddLight(31, 11, "lazienka_gora", 0);
  lights.AddLight(33, 8, "lazienka_kinkiety", 0);
  lights.AddLight(32, 12, "lazienka_led", 0);
  lights.AddLight(35, 15, "pralnia", 0);
  lights.AddLight(37, 17, "garderoba_gora", 0);
  lights.AddLight(53, 19, "korytarz", 0);
  lights.AddLight(30, 18, "korytarz_nocne", 0);
  lights.AddLight(29, 9, "schody", 0);
  lights.AddLight(34, 14, "taras", 0);
  lights.AddLight(36, 16, "taras_punktowe", 0);
  lights.AddLight(23, 3, "taras_kolumna", 0);
}

void InitBlinds()
{
  blinds.AddBlind(43, "salon1_up", BlindDirection::UP);
  blinds.AddBlind(40, "salon1_down", BlindDirection::DOWN);
  blinds.AddBlind(38, "salon2_up", BlindDirection::UP);
  blinds.AddBlind(41, "salon2_down", BlindDirection::DOWN);
  blinds.AddBlind(42, "jadalnia_up", BlindDirection::UP);
  blinds.AddBlind(45, "jadalnia_down", BlindDirection::DOWN);
  blinds.AddBlind(47, "kuchnia_up", BlindDirection::UP);
  blinds.AddBlind(44, "kuchnia_down", BlindDirection::DOWN);
  blinds.AddBlind(46, "gabinet_up", BlindDirection::UP);
  blinds.AddBlind(49, "gabinet_down", BlindDirection::DOWN);
  blinds.AddBlind(48, "garderoba_up", BlindDirection::UP);
  blinds.AddBlind(28, "garderoba_down", BlindDirection::DOWN);
  blinds.AddBlind(50, "kotlownia_up", BlindDirection::UP);
  blinds.AddBlind(39, "kotlownia_down", BlindDirection::DOWN);
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
  wdt_reset();

  board.ProcessHttpRequest(*webClient);
  lights.CheckAndSwitchLights();
  
  if (firstLoop)
  {
    lights.AllLightsOff();
    firstLoop = false;
  }
    
  timer.update();

  if (holidayLightPending && (long)(millis() - holidayLightOnTime) >= 0)
  {
    holidayLightPending = false;
    if (board.GetHolidayMode())
      lights.SwitchLight(currentHolidayRoom, HIGH);
  }
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
  if (RTC.read(tm) != 0)
    return;
  int hour = tm.Hour;
  int minute = tm.Minute;

  scheduler.Execute(hour, minute, tm.Wday);
  board.TimerEvent(tm);
  
  if (board.GetHolidayMode())
  {
	if (hour == 20 && minute == 0)
	{
		holidayLighting = true;
		ChangeHolidayLight(hour, minute);
	}
	if (hour == 23 && minute == 30)
	{
		lights.AllLightsOff();
		ResetHolidaySettings();
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
	
	switch (random(4))
	{
		case 0:
			currentHolidayRoom = "pokoj1";
			break;
		case 1:
			currentHolidayRoom = "korytarz";
			break;
		case 2:
			currentHolidayRoom = "garderoba_gora";
			break;
		case 3:
			currentHolidayRoom = "lazienka_gora";
			break;
	}

	// The light goes on after a pause, in loop(). Waiting here would stop the HTTP server
	// and the wall switches for up to 14 s.
	holidayLightOnTime = millis() + 5000 + random(10)*1000;
	holidayLightPending = true;
}

void ResetHolidaySettings()
{
	holidayLighting = false;
	currentHolidayRoom = "";
	currentHolidayHour = 0;
	currentHolidayMinute = 0;
	holidayLightPending = false;
}
