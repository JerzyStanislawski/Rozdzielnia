#include <arduino.h>
#include "lights.h"
#include <string.h>

Light::Light(byte output, byte mainSwitch, String roomName, byte altSwitch)
{
  outputNumber = output;
  switchNumber = mainSwitch;
  room = roomName;
  altSwitchNumber = altSwitch;
  state = false;
  switchState = CheckSwitchState();
  altSwitchState = CheckAltSwitchState();
}

Lights::Lights(const Lights & old)
{
  initializedLights = old.initializedLights;
  for (byte i = 0; i < initializedLights; i++)
  {
    lights[i] = Light(old.lights[i].outputNumber, old.lights[i].switchNumber, old.lights[i].room, old.lights[i].altSwitchNumber);
  }
}

void Light::SwitchLight(byte value)
{
  digitalWrite(outputNumber, value);
}

bool Light::CheckSwitchState()
{
  return digitalRead(switchNumber) == HIGH;
}

bool Light::CheckAltSwitchState()
{
  if (altSwitchNumber != 0)
    return digitalRead(altSwitchNumber) == HIGH;
  else
    return false;
}

void Lights::AddLight(byte outputNumber, byte switchNumber, String room, byte altSwitchNumber)
{
  lights[initializedLights] = Light(outputNumber, switchNumber, room, altSwitchNumber);
  initializedLights++;
}

void Lights::AllLightsOff()
{
  for (byte i = 0; i < initializedLights; i++)
  {
    if (lights[i].state == true)
    {
      lights[i].SwitchLight(LOW);
      lights[i].state = false;
    }
  }
}

void Lights::CheckAndSwitchLights()
{
  for (byte i = 0; i < initializedLights; i++)
  {    
    bool switchState = lights[i].CheckSwitchState();
    if (lights[i].switchState != switchState)
    {
      lights[i].switchState = switchState;
      lights[i].state = !lights[i].state;
      lights[i].SwitchLight(lights[i].state);
    }
    else if (lights[i].altSwitchNumber != 0)
    {
      bool altSwitchState = lights[i].CheckAltSwitchState();
      if (lights[i].altSwitchState != altSwitchState)
      {
        lights[i].altSwitchState = altSwitchState;
        lights[i].state = !lights[i].state;
        lights[i].SwitchLight(lights[i].state);   
      }
    }
  }
}

void Lights::SwitchLight(String room, byte value)
{
  for (byte i = 0; i < initializedLights; i++)
  {
    if (lights[i].room == room)
    {
      lights[i].state = value;
      lights[i].SwitchLight(lights[i].state);
      return;
    }
  }
}

void Lights::DoForEach(LightAction action)
{
  for (byte i = 0; i < initializedLights; i++)
  {
      action(lights[i].outputNumber, lights[i].room, lights[i].switchNumber, lights[i].altSwitchNumber);
  } 
}

void Lights::WriteStatus(Client * client)
{  
  for (byte i = 0; i < initializedLights; i++)
  {    
    client->print(lights[i].outputNumber);
    client->print('=');
    client->println(lights[i].state);
  }
}



