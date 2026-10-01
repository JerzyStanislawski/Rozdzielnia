#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <string.h>
#include <Print.h>

#include "lights.h"
#include "blinds.h"

#define MAX_SCHEDULED_EVENTS 100

enum RecordType 
{
  LIGHTS,
  BLINDS
};

struct TimeRecord
{
	byte roomId;
	RecordType type;
	byte hour;
	byte minute;
	byte days;
	bool onOrUp;

  TimeRecord(byte roomId, RecordType type, byte hour, byte minute, byte days, bool onOrUp);
  TimeRecord() {}
};

class Scheduler
{
  public:
	TimeRecord Add(String room, RecordType type, byte hour, byte minute, byte days, bool onOrUp);
    void Clear(int startAddress);
	void Execute(byte hour, byte minute, byte currentDay);
	void WriteEvents(Print * client);
	void RestoreScheduledEvents(int startAddress);
	void Schedule(String * records, int count, int startAddress);
	
    Scheduler(Lights * lights, Blinds * blinds)
    {
	  this->lights = lights;
	  this->blinds = blinds;
    }
	
  private:
    TimeRecord records[MAX_SCHEDULED_EVENTS];
    int count;
	Lights * lights;
	Blinds * blinds;
};

#endif
