#include <Event.h>
#include <Timer.h>

#include <Time.h>
#include <TimeLib.h>

#include <DS3232RTC.h>


#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <string.h>


const int NUMER_DOMU = 22;
const int OSW_KORYTARZ_NOCNE = 30;
const int OSW_SYPIALNIA = 24;
const int OSW_POKOJ_ADAM = 25;
const int OSW_POKOJ_ULA = 26;
const int OSW_SCHODY_LED = 27;
const int ROL_GARDEROBA_DOWN = 28;
const int OSW_SCHODY = 29;
const int OSW_TARAS = 34;
const int OSW_LAZIENKA_GORA = 31;
const int OSW_TARAS_PUNKTOWE = 36;
const int OSW_LAZIENKA_KINKIETY = 33;
const int OSW_TARAS_KOLUMNA = 23; 
const int OSW_PRALNIA = 35;
const int OSW_LAZIENKA_LED = 32;
const int OSW_GARDEROBA_GORA = 37;
const int ROL_SALON2_UP = 38;
const int ROL_KOTLOWNIA_DOWN = 39;
const int ROL_SALON1_DOWN = 40;
const int ROL_SALON2_DOWN = 41;
const int ROL_JADALNIA_UP = 42;
const int ROL_SALON1_UP = 43;
const int ROL_KUCHNIA_DOWN = 44;
const int ROL_JADALNIA_DOWN = 45;
const int ROL_GABINET_UP = 46;
const int ROL_KUCHNIA_UP = 47;
const int ROL_GARDEROBA_UP = 48;
const int ROL_GABINET_DOWN = 49;
const int ROL_KOTLOWNIA_UP = 50;
const int OSW_KORYTARZ = 53;

// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   1024

// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 26); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
int req_index = 0;              // index into HTTP_req buffer

const int OSW_COUNT = 20;
int currentInputState[OSW_COUNT] = {HIGH};
int currentOutputState[OSW_COUNT] = {LOW};

Timer timer;
bool roletyState;
bool holidayMode;
int lightCurrentlyOn;

int twilight[12][31] = {
  {1534, 1535, 1536, 1537, 1538, 1540, 1541, 1542, 1543, 1544, 1546, 1547, 1548, 1550, 1551, 1553, 1554, 1555, 1557, 1558, 1600, 1601, 1603, 1605, 1606, 1608, 1609, 1611, 1613, 1614, 1616},
  {1618, 1619, 1621, 1623, 1624, 1626, 1628, 1629, 1631, 1633, 1634, 1636, 1638, 1640, 1641, 1643, 1645, 1646, 1648, 1650, 1651, 1653, 1655, 1657, 1658, 1700, 1702, 1703},
  {1705, 1707, 1708, 1710, 1712, 1714, 1715, 1717, 1719, 1720, 1722, 1724, 1725, 1727, 1729, 1730, 1732, 1734, 1736, 1737, 1739, 1741, 1742, 1744, 1746, 1747, 1749, 1751, 1753, 1754, 1756},
  {1758, 1800, 1801, 1803, 1805, 1806, 1808, 1810, 1812, 1814, 1815, 1817, 1819, 1821, 1822, 1824, 1826, 1828, 1830, 1831, 1833, 1835, 1837, 1839, 1840, 1842, 1844, 1846, 1848, 1850},
  {1851, 1853, 1855, 1857, 1859, 1900, 1902, 1904, 1906, 1908, 1909, 1911, 1913, 1915, 1916, 1918, 1920, 1921, 1923, 1925, 1926, 1928, 1930, 1931, 1933, 1934, 1936, 1937, 1938, 1940, 1941},
  {1942, 1944, 1945, 1946, 1947, 1948, 1949, 1950, 1951, 1952, 1953, 1954, 1954, 1955, 1956, 1956, 1957, 1957, 1957, 1958, 1958, 1958, 1958, 1958, 1958, 1958, 1958, 1958, 1958, 1957},
  {1957, 1956, 1956, 1955, 1955, 1954, 1953, 1953, 1952, 1951, 1950, 1949, 1948, 1947, 1945, 1944, 1943, 1942, 1940, 1939, 1937, 1936, 1934, 1933, 1931, 1930, 1928, 1926, 1925, 1923, 1921},
  {1919, 1917, 1915, 1914, 1912, 1910, 1908, 1906, 1904, 1902, 1900, 1858, 1855, 1853, 1851, 1849, 1847, 1845, 1842, 1840, 1838, 1836, 1834, 1831, 1829, 1827, 1825, 1822, 1820, 1818, 1815},
  {1813, 1811, 1808, 1806, 1804, 1801, 1759, 1757, 1754, 1752, 1750, 1747, 1745, 1743, 1740, 1738, 1736, 1733, 1731, 1729, 1726, 1724, 1722, 1719, 1717, 1715, 1713, 1710, 1708, 1706},
  {1703, 1701, 1659, 1657, 1655, 1652, 1650, 1648, 1646, 1644, 1642, 1639, 1637, 1635, 1633, 1631, 1629, 1627, 1625, 1623, 1621, 1619, 1617, 1615, 1614, 1612, 1610, 1608, 1606, 1605, 1603},
  {1601, 1559, 1558, 1556, 1555, 1553, 1552, 1550, 1549, 1547, 1546, 1544, 1543, 1542, 1541, 1539, 1538, 1537, 1536, 1535, 1534, 1533, 1532, 1531, 1531, 1530, 1529, 1528, 1528, 1527},
  {1527, 1526, 1526, 1525, 1525, 1525, 1524, 1524, 1524, 1524, 1524, 1524, 1524, 1524, 1524, 1524, 1525, 1525, 1525, 1526, 1526, 1527, 1527, 1528, 1528, 1529, 1530, 1530, 1531, 1532, 1533},
};

void setup()
{
  //disable Ethernet chip
  //pinMode(53, OUTPUT);
  //digitalWrite(53, HIGH);

  Serial.begin(9600);       // for debugging

  for (int i = 0; i < OSW_COUNT ; i++)
  {
    pinMode(i, INPUT_PULLUP);
  }
  Serial.println("Pins input set.");
 // for (int i = START_PIN + 1; i < START_PIN + (OSW_COUNT * 2); i = i + 2)
 // {
 //   pinMode(i, OUTPUT);
 //   digitalWrite(i, LOW);
 // }
  for (int i = 22; i <= 53; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  Serial.println("Pins set.");

  for (int i = 0; i < OSW_COUNT; i++)
  {
    currentInputState[i] = digitalRead(i);
    currentOutputState[i] = LOW;
  }

  Ethernet.begin(mac, ip);  // initialize Ethernet device
  server.begin();           // start to listen for clients
  Serial.println("Server initialized.");

  tmElements_t tm;
  RTC.read(tm);
  int hour = tm.Hour;
  //setTime(21, 0, 0, 6, 4, 2019);
  //RTC.set(now());
  Serial.println("Time initialized.");

  roletyState = !(hour > 19 || hour < 7);
  holidayMode = false;
  lightCurrentlyOn = 0;
  timer.every(60 * 1000UL, HandleAutoEvents);
  
  HandleAutoEvents();
}

void loop()
{
  EthernetClient client = server.available();

  if (client) 
  {
    boolean currentLineIsBlank = true;
    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read(); 
        if (req_index < (REQ_BUF_SZ - 1)) 
        {
          HTTP_req[req_index] = c;
          req_index++;
        }

        if (c == '\n' && currentLineIsBlank) 
        {
          char buf[128];
          char* fileName = GetRequestedFileName(HTTP_req, buf);

          if (String(fileName) == String("impulsRolety") || String(fileName) == String("impulsOswietlenie"))
          {
            String parameters = GetRequestParameters(client);
            if (String(fileName) == String("impulsRolety"))
            {
              HandleRoletyChange(parameters);
              RespondHttp(client, false);
            }
            else if (String(fileName) == String("impulsOswietlenie"))
            {
              bool value;
              String parameter = ParseParameters(parameters, &value);
              HandleOswietlenieChange(parameter, value ? HIGH : LOW);
            }
            RespondHttp(client, false);
          }
          else if (String(fileName) == String("getStatus"))
          {
            RespondHttp(client, true);
            for (int i = 0; i < OSW_COUNT; i++)
            {
              int output = MapInputToOutput(i);
              client.print(output);
              client.print('=');
              client.println(currentOutputState[i]);
            }
            client.print("99=");
            client.println(holidayMode);
          }
          else if (String(fileName) == String("getTime"))
          {
            RespondHttp(client, true);
            
            tmElements_t tm;
            RTC.read(tm);            
          
            client.print(tm.Hour, DEC);
            client.print(':');
            client.print(tm.Minute, DEC);
            client.print(':');
            client.println(tm.Second, DEC);
            client.print("Week day:");
            client.println(tm.Wday, DEC);
          }
          else if (String(fileName) == String("holiday"))
          {
              String parameters = GetRequestParameters(client);
              bool value;
              String parameter = ParseParameters(parameters, &value);
              if (parameter == "mode")
                holidayMode = value;
          }
          
          req_index = 0;
          StrClear(HTTP_req, REQ_BUF_SZ);
          break;
        }
        if (c == '\n')
          currentLineIsBlank = true;
        else if (c != '\r') 
          currentLineIsBlank = false;
      } // end if (client.available())
    } // end while (client.connected())
    delay(1);
    client.flush();
    client.stop();
    
  } // end if (client)
  else
  {
    for (int i = 2; i < OSW_COUNT; i++)
    {
      if (i == 4 || i == 13)
        continue;
        
      int state = digitalRead(i);
      if (currentInputState[i] != state)
      {
        int outputPin = MapInputToOutput(i);
        
        int outputValue = currentOutputState[i] == HIGH ? LOW : HIGH;
        Light(outputPin, outputValue);
        currentInputState[i] = state;
		
        if (holidayMode)
          holidayMode = false;
      }
    }
    delay(5);
  }
  timer.update();
}

void RespondHttp(EthernetClient client, bool contentType)
{
  client.println("HTTP/1.1 200 OK");
  if (contentType)
    client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();  
}

void HandleAutoEvents()
{
  Serial.println("HandleAutoEvents");

  tmElements_t tm;
  RTC.read(tm);
  int hour = tm.Hour;
  int minute = tm.Minute;
  int weekday = tm.Wday;

  int day = tm.Day;
  int month = tm.Month;
  int twilightTime = twilight[month - 1][day - 1];
  int twilightHour = (twilightTime / 100) + 2;
  int twilightMinute = twilightTime % 100;

  if ((hour == twilightHour && minute >= twilightMinute) || hour > twilightHour)
  {
    if (roletyState == false)
    {
      AllRoletyDown();
      roletyState = true;
    }
  }
  else
  {
    if (((weekday == 1 || weekday == 7) && hour >= 7 && minute >= 30)  || (weekday > 1 && weekday < 7 && ((hour == 6 && minute >= 30) || (hour > 6 && hour < 10))))
    {
      if (roletyState == true)
      {
        AllRoletyUp();
        roletyState = false;
      }
    }
  }

  if (holidayMode == true)
    HandleAutoLight(hour);
}

void HandleAutoLight(int hour)
{
  //delay(60 * (random(6) * 1000));
  Serial.print("Hour; ");
  Serial.println(hour);
  if (hour > 20 || hour < 2)
  {
    int swiatla[] = {OSW_POKOJ_ULA, OSW_POKOJ_ADAM, OSW_POKOJ_ADAM, OSW_GARDEROBA_GORA, OSW_KORYTARZ, OSW_SYPIALNIA, OSW_KORYTARZ, OSW_LAZIENKA_GORA};
    int index = random(8);

    int swiatlo = swiatla[index] + 1;
  Serial.print("swiatlo; ");
  Serial.println(swiatlo);
  Serial.print("lightCurrentlyOn; ");
  Serial.println(lightCurrentlyOn);
    
    if (swiatlo == lightCurrentlyOn)
      return;

    if (lightCurrentlyOn != 0)
    {
      Light(lightCurrentlyOn, LOW);
      delay(5000 + (random(6) * 1000));
    }

    Light(swiatlo, HIGH);
    lightCurrentlyOn = swiatlo;
  }
  else
  {
    if (lightCurrentlyOn != 0)
    {      
      Light(lightCurrentlyOn, LOW);
      lightCurrentlyOn = 0;
    }
  }
}

int MapInputToOutput(int input)
{
  if (input == 8)
    return OSW_LAZIENKA_KINKIETY;
  if (input == 2)
    return OSW_SYPIALNIA;
  if (input == 18)
    return OSW_KORYTARZ;
  if (input == 19)
    return OSW_KORYTARZ_NOCNE;
  return input + 20;
}

int MapOutputToIndex(int output)
{
  if (output == OSW_LAZIENKA_KINKIETY)
    return 8;
  if (output == OSW_SYPIALNIA)
    return 2;
  if (output == OSW_KORYTARZ)
    return 18;
  if (output == OSW_KORYTARZ_NOCNE)
    return 19;
  return output - 20;
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
  for (int i = 0; i < length; i++) {
    str[i] = 0;
  }
}

void Roleta(int roleta)
{
  digitalWrite(roleta, HIGH);
  delay(1000);
  digitalWrite(roleta, LOW);
}

void Light(int light, int value)
{
  digitalWrite(light, value);
  int index = MapOutputToIndex(light);
  currentOutputState[index] = value;
}

void AllRoletyUp()
{
  int roletyUp[] = {ROL_SALON1_UP, ROL_SALON2_UP, ROL_JADALNIA_UP, ROL_KUCHNIA_UP, ROL_GABINET_UP, ROL_GARDEROBA_UP, ROL_KOTLOWNIA_UP};
  for (int i = 0; i < 7; i++)
  {
    Roleta(roletyUp[i]);
  }
}

void AllRoletyDown()
{
  int roletyDown[] = {ROL_SALON1_DOWN, ROL_SALON2_DOWN, ROL_JADALNIA_DOWN, ROL_KUCHNIA_DOWN, ROL_GABINET_DOWN, ROL_GARDEROBA_DOWN, ROL_KOTLOWNIA_DOWN};
  for (int i = 0; i < 7; i++)
  {
      Roleta(roletyDown[i]);
  }
}

void HandleRoletyChange(String parameter)
{
  if (parameter == "allRoletyUp")
  {
    AllRoletyUp();
    return;
  }
  if (parameter == "allRoletyDown")
  {
    AllRoletyDown();
    return;
  }

  int output = 0;
  if (parameter == "salon1_up")
    output = ROL_SALON1_UP;
  if (parameter == "salon1_down")
    output = ROL_SALON1_DOWN;
  if (parameter == "salon2_up")
    output = ROL_SALON2_UP;
  if (parameter == "salon2_down")
    output = ROL_SALON2_DOWN;
  if (parameter == "jadalnia_up")
    output = ROL_JADALNIA_UP;
  if (parameter == "jadalnia_down")
    output = ROL_JADALNIA_DOWN;
  if (parameter == "kuchnia_up")
    output = ROL_KUCHNIA_UP;
  if (parameter == "kuchnia_down")
    output = ROL_KUCHNIA_DOWN;
  if (parameter == "gabinet_up")
    output = ROL_GABINET_UP;
  if (parameter == "gabinet_down")
    output = ROL_GABINET_DOWN;
  if (parameter == "garderoba_up")
    output = ROL_GARDEROBA_UP;
  if (parameter == "garderoba_down")
    output = ROL_GARDEROBA_DOWN;
  if (parameter == "kotlownia_up")
    output = ROL_KOTLOWNIA_UP;
  if (parameter == "kotlownia_down")
    output = ROL_KOTLOWNIA_DOWN;

  Roleta(output);
}

void AllLightsOff()
{
       for (int i = 0; i < OSW_COUNT; i++)
       {
          if (currentOutputState[i] == HIGH)
          {
            int outputPin = MapInputToOutput(i);
            Light(outputPin, LOW);
          }
       }
}

void HandleOswietlenieChange(String parameter, int value)
{
    if (parameter == "allOff")
    {
       AllLightsOff();
       return;
    }
  
  int output = parameter.toInt(); //0;
  if (parameter == "sypialnia")
    output = OSW_SYPIALNIA;
  if (parameter == "pokoj1")
    output = OSW_POKOJ_ADAM;
  if (parameter == "pokoj2")
    output = OSW_POKOJ_ULA;
  if (parameter == "lazienka_gora")
    output = OSW_LAZIENKA_GORA;
  if (parameter == "lazienka_kinkiety")
    output = OSW_LAZIENKA_KINKIETY;
  if (parameter == "lazienka_led")
    output = OSW_LAZIENKA_LED;
  if (parameter == "pralnia")
    output = OSW_PRALNIA;
  if (parameter == "garderoba_gora")
    output = OSW_GARDEROBA_GORA;
  if (parameter == "korytarz")
    output = OSW_KORYTARZ;
  if (parameter == "korytarz_nocne")
    output = OSW_KORYTARZ_NOCNE;
  if (parameter == "schody")
    output = OSW_SCHODY;
  if (parameter == "taras")
    output = OSW_TARAS;
  if (parameter == "taras_punktowe")
    output = OSW_TARAS_PUNKTOWE;
  if (parameter == "taras_kolumna")
    output = OSW_TARAS_KOLUMNA;

  if (output == 0)
    return;

  Light(output, value);
}

String ParseParameters(String parameters, bool * outValue)
{
  int equalsIndex = parameters.indexOf('=');
  String parameter = parameters.substring(0, equalsIndex);
  String boolString = parameters.substring(equalsIndex + 1);

  *outValue = (boolString == "true");
  return parameter;
}

String GetRequestParameters(EthernetClient client)
{  
            String parameters;
            char c2 = ' ';
            while (client.connected() && client.available() && c2 != '\n') 
            {
              c2 = client.read();
              parameters += c2;
            }

            return parameters;
}

char* GetRequestedFileName(char* httpReq, char * buf)
{
  String request(httpReq);
  int postIndex = request.indexOf("POST /");

  String requestStart(postIndex < 0 ? "GET /" : "POST /");
  int requestedStartLength = requestStart.length();
  int endOfResource = request.indexOf(" ", requestedStartLength);

  if (endOfResource == requestedStartLength)
    return "index.htm";

  String requestedFile = request.substring(requestedStartLength, endOfResource);
  requestedFile.toCharArray(buf, requestedFile.length() + 1);
  return buf;
}

