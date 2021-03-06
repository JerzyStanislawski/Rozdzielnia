#ifndef LIGHTS_H
#define LIGHTS_H

#include <string.h>
#include "client.h"

typedef void (*LightAction) (byte outputNumber, String room, byte mainSwitch, byte altSwitch);

class Light
{
  public:
    byte outputNumber;
    byte switchNumber;
    byte altSwitchNumber;
    String room;
    bool state;
    bool switchState;
    bool altSwitchState;
	byte id;
    
    Light(byte output, byte mainSwitch, String roomName, byte altSwitch, byte id);
    Light() {}
    
    void SwitchLight(byte value);
    bool CheckSwitchState();
    bool CheckAltSwitchState();
};

class Lights
{
  public:
    void AddLight(byte outputNumber, byte switchNumber, String room, byte altSwitchNumber);
    bool CheckAndSwitchLights();
    void SwitchLight(String room, byte value);
    void AllLightsOff();
    void DoForEach(LightAction action);
    void WriteStatus(Client * client);
	String GetNameById(byte id);
	byte GetId(String name);
    
    Lights()
    {
      initializedLights = 0;
    }
    Lights(const Lights &);

  private:
    Light lights[25];
    byte initializedLights;
};

#endif

