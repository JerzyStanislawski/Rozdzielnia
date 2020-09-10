#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <string.h>
#include "client.h"

#include "lights.h"
#include "blinds.h"

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
    void Clear();
	void Execute(byte hour, byte minute, byte currentDay);
	void WriteEvents(Client * client);
	void RestoreScheduledEvents();
	void Schedule(String * records, int count);
	
    Scheduler(Lights * lights, Blinds * blinds)
    {
	  this->lights = lights;
	  this->blinds = blinds;
      Clear();
    }
	
  private:
    TimeRecord records[100];
    int count;
	Lights * lights;
	Blinds * blinds;
};

#endif
