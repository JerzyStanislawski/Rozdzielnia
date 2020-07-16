#ifndef BLINDS_H
#define BLINDS_H

#include <string.h>

typedef void (*BlindAction) (byte outputNumber, String room);

enum BlindDirection 
{
  UP,
  DOWN
};

class Blind
{
  public:
    byte outputNumber;
    String room;  
    BlindDirection direction;
	byte id;
	
    Blind(byte output, String roomName, BlindDirection blindDirection, byte id);
    Blind() {}
};

class Blinds
{
  public:
    void AddBlind(byte outputNumber, String room, BlindDirection direction);
    void MoveBlind(String room);
    void AllBlindsUp();      
    void AllBlindsDown();
    void DoForEach(BlindAction action);
	String GetNameById(byte id);
	byte GetId(String name);
	
    Blinds()
    {
      initializedBlinds = 0;
    }

  private:
    void MoveBlind(byte outputNumber);
    Blind blinds[20];
    byte initializedBlinds;
};

#endif

