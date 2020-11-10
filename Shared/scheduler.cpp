#include <arduino.h>
#include "scheduler.h"
#include <string.h>
#include <EEPROM.h>
#include "lights.h"
#include "blinds.h"


TimeRecord::TimeRecord(byte roomId, RecordType type, byte hour, byte minute, byte days, bool onOrUp)
{
  this->roomId = roomId;
  this->type = type;
  this->hour = hour;
  this->minute = minute;
  this->onOrUp = onOrUp;
  this->days = days;
}

TimeRecord Scheduler::Add(String room, RecordType type, byte hour, byte minute, byte days, bool onOrUp)
{
  byte id = type == RecordType::LIGHTS ? lights->GetId(room) : blinds->GetId(room);
  records[count] = TimeRecord(id, type, hour, minute, days, onOrUp);
  count++;
  
  return records[count - 1];
}

void Scheduler::Clear(int startAddress)
{
	count = 0;
	EEPROM.put(startAddress, count);
}

void Scheduler::Execute(byte hour, byte minute, byte currentDay)
{
	for (byte i = 0; i < count; i++)
	{
		TimeRecord record = records[i];
		
		if (record.hour == hour && record.minute == minute && (record.days & (1 << currentDay)) == (1 << currentDay))
		{
			if (record.type == RecordType::LIGHTS)
				lights->SwitchLight(lights->GetNameById(record.roomId), record.onOrUp ? HIGH : LOW);
			else
				blinds->MoveBlind(blinds->GetNameById(record.roomId));
		}	
	}
}

void Scheduler::Schedule(String * records, int partialCount, int startAddress)
{
  int eeAddress = startAddress;
  EEPROM.put(eeAddress, count + partialCount);
  eeAddress += sizeof(int);
  eeAddress += count * sizeof(TimeRecord);
  
  for (int i = 0; i < partialCount; i++)
  {
      int comma = records[i].indexOf(',');
      String room = records[i].substring(0, comma);
      String type = records[i].substring(comma + 1, comma + 2);
      byte hours = (byte)records[i].substring(comma + 3, comma + 5).toInt();
      byte minutes = (byte)records[i].substring(comma + 6, comma + 8).toInt();
      byte days = (byte)records[i].substring(comma + 9, comma + 12).toInt();
      byte onOrUp = (byte)records[i].substring(comma + 13, comma + 14).toInt();

      TimeRecord record = Add(room, type == "L" ? RecordType::LIGHTS : RecordType::BLINDS, hours, minutes, days, onOrUp == 1);

      EEPROM.put(eeAddress, record);
      eeAddress += sizeof(TimeRecord);
  }  	
}

void Scheduler::WriteEvents(Client * client)
{  
  for (byte i = 0; i < count; i++)
  {    
	TimeRecord record = records[i];
	
    client->print(record.type == RecordType::LIGHTS ? lights->GetNameById(record.roomId) : blinds->GetNameById(record.roomId));
    client->print(',');
    client->print(record.type);
    client->print(',');
    client->print(record.hour);
    client->print(':');
    client->print(record.minute);
    client->print(',');
    client->print(record.days);
    client->print(',');
    client->print(record.onOrUp);
    client->println(';');
  }
}

void Scheduler::RestoreScheduledEvents(int startAddress)
{
	int eeAddress = startAddress;
	EEPROM.get(eeAddress, count);
		
	eeAddress += sizeof(int);
	for (int i = 0; i < count; i++)
	{
		TimeRecord record;
		EEPROM.get(eeAddress, record);
		records[i] = record;
		eeAddress += sizeof(TimeRecord);
	}
}