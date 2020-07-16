#include <arduino.h>
#include "blinds.h"
#include <string.h>

Blind::Blind(byte output, String roomName, BlindDirection blindDirection, byte id)
{
  outputNumber = output;
  room = roomName;
  direction = blindDirection;
  this->id = id;
}

void Blinds::MoveBlind(byte outputNumber)
{  
  digitalWrite(outputNumber, HIGH);   
  delay(1000);
  digitalWrite(outputNumber, LOW);  
}


void Blinds::AddBlind(byte outputNumber, String room, BlindDirection direction)
{
  blinds[initializedBlinds] = Blind(outputNumber, room, direction, initializedBlinds);
  initializedBlinds++;
}

void Blinds::MoveBlind(String room)
{
  for (byte i = 0; i < initializedBlinds; i++)
  {
    if (blinds[i].room == room)
    {
      MoveBlind(blinds[i].outputNumber);
      return;
    }
  }
}

void Blinds::AllBlindsUp()
{
  for (byte i = 0; i < initializedBlinds; i++)
  {
    if (blinds[i].direction == BlindDirection::UP)
    {
      MoveBlind(blinds[i].outputNumber);
    }
  }
}

void Blinds::AllBlindsDown()
{
  for (byte i = 0; i < initializedBlinds; i++)
  {
    if (blinds[i].direction == BlindDirection::DOWN)
    {
      MoveBlind(blinds[i].outputNumber);
    }
  }
}

void Blinds::DoForEach(BlindAction action)
{
  for (byte i = 0; i < initializedBlinds; i++)
  {
      action(blinds[i].outputNumber, blinds[i].room);
  } 
}

byte Blinds::GetId(String name)
{
  for (byte i = 0; i < initializedBlinds; i++)
  {
	  if (blinds[i].room == name)
		  return blinds[i].id;
  }	
}

String Blinds::GetNameById(byte id)
{
  for (byte i = 0; i < initializedBlinds; i++)
  {
	  if (blinds[i].id == id)
		  return blinds[i].room;
  }		
}

