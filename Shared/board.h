#ifndef BOARD_H
#define BOARD_H

#include <string.h>
#include <Print.h>

#include <Time.h>
#include <TimeLib.h>
#include <DS3232RTC.h>

#include "lights.h"
#include "blinds.h"
#include "scheduler.h"
#include "webClient.h"


class Board
{
  public:    
	void ProcessHttpRequest(WebClient & webClient);
	void TimerEvent(tmElements_t tm);
	bool GetHolidayMode();
	int RestoreSettings();
  
    Board(Lights * lights, Blinds * blinds, Scheduler * scheduler)
    {
		this->lights = lights;
		this->blinds = blinds;
		this->scheduler = scheduler;
		twilightMode = false;
		morningMode = false;
		holidayMode = false;
		morningHour = 7;
		morningMinute = 0;
		morningDays = 255;
		
		settingsEndAddress = 0;
    }

  private:
    static Lights * lights;
    static Blinds * blinds;
	static Scheduler * scheduler;
	static bool twilightMode;
	static bool morningMode;
	static bool holidayMode;
	static byte morningHour;
	static byte morningMinute;
	static byte morningDays;
	
	static int settingsEndAddress;
	
	String httpParameters[HTTP_MAX_PARAMETERS];
	
	String ParseHttpBoolParameter(String parameters, bool * outValue);
	String ParseHttpParameter(String parameters, String * outValue);
	static void HttpCustomRespond(const String & endpoint, Print * client);
	static void PrintTime(Print * client);
	static void PrintMorningMode(Print * client);
	void StoreSettings();
};
	
#endif

