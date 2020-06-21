#include <arduino.h>
#include "blinds.h"
#include <string.h>

Blind::Blind(byte output, String roomName, BlindDirection blindDirection)
{
  outputNumber = output;
  room = roomName;
  direction = blindDirection;
}

void Blinds::MoveBlind(byte outputNumber)
{  
  digitalWrite(outputNumber, HIGH);   
  delay(1000);
  digitalWrite(outputNumber, LOW);  
}


void Blinds::AddBlind(byte outputNumber, String room, BlindDirection direction)
{
  blinds[initializedBlinds] = Blind(outputNumber, room, direction);
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


