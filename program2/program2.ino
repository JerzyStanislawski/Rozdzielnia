#include <Event.h>
#include <Timer.h>

#include <Time.h>
#include <TimeLib.h>

#include <DS3232RTC.h>

#include <Ethernet.h>
#include <SD.h>
#include <string.h>

#include "webClient.h"
#include "lights.h"
#include "blinds.h"

byte mac[] = { 0xDE, 0xDA, 0xFE, 0xEF, 0xAE, 0xBD };
IPAddress ip(192, 168, 1, 26); 
WebClient * webClient;

Timer timer;

bool holidayMode;
bool twilightMode;
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

Lights lights;
Blinds blinds;

void setup()
{    
  Serial.begin(9600);
  
  InitLights();
  InitBlinds();
  InitPins();

  webClient = new WebClient(mac, ip);

  holidayMode = false;
  twilightMode = false;
  
  //setTime(19, 40, 0, 14, 6, 2020);
  //Serial.println(RTC.set(now()));
  //Serial.println("time set");
  //timer.every(1 * 1000UL, HandleAutoEvents);
}

void InitLights()
{
  lights.AddLight(24, 2, "sypialnia", 0);
  lights.AddLight(25, 5, "pokoj1", 0);
  lights.AddLight(34, 6, "pokoj2", 0);
  lights.AddLight(31, 11, "lazienka_gora", 0);
  lights.AddLight(33, 8, "lazienka_kinkiety", 0);
  lights.AddLight(32, 12, "lazienka_led", 0);
  lights.AddLight(35, 15, "pralnia", 0);
  lights.AddLight(37, 17, "garderoba_gora", 0);
  lights.AddLight(53, 18, "korytarz", 0);
  lights.AddLight(30, 19, "korytarz_nocne", 0);
  lights.AddLight(29, 9, "schody", 0);
  lights.AddLight(34, 14, "taras", 0);
  lights.AddLight(36, 16, "taras_punktowe", 0);
  lights.AddLight(23, 3, "taras_kolumna", 0);
}

void InitBlinds()
{
  blinds.AddBlind(43, "salon1_up", BlindDirection::UP);
  blinds.AddBlind(40, "salon1_down", BlindDirection::DOWN);
  blinds.AddBlind(38, "salon2_up", BlindDirection::UP);
  blinds.AddBlind(41, "salon2_down", BlindDirection::DOWN);
  blinds.AddBlind(42, "jadalnia_up", BlindDirection::UP);
  blinds.AddBlind(45, "jadalnia_down", BlindDirection::DOWN);
  blinds.AddBlind(47, "kuchnia_up", BlindDirection::UP);
  blinds.AddBlind(44, "kuchnia_down", BlindDirection::DOWN);
  blinds.AddBlind(46, "gabinet_up", BlindDirection::UP);
  blinds.AddBlind(49, "gabinet_down", BlindDirection::DOWN);
  blinds.AddBlind(48, "garderoba_up", BlindDirection::UP);
  blinds.AddBlind(28, "garderoba_down", BlindDirection::DOWN);
  blinds.AddBlind(50, "kotlownia_up", BlindDirection::UP);
  blinds.AddBlind(39, "kotlownia_down", BlindDirection::DOWN);
}

void InitPins()
{
  lights.DoForEach(InitPinsForLights);
  blinds.DoForEach(InitPinsForBlinds);
  
  pinMode(50, OUTPUT);
  pinMode(51, OUTPUT);
  pinMode(52, OUTPUT);
}

void loop()
{
  ProcessHttpRequest(*webClient);
  lights.CheckAndSwitchLights();

  //timer.update();
}

void InitPinsForLights(byte output, String room, byte mainSwitch, byte altSwitch)
{
  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);
  
  if (mainSwitch != 0)
    pinMode(mainSwitch, INPUT_PULLUP);  
  if (altSwitch != 0)
    pinMode(altSwitch, INPUT_PULLUP); 
}

void InitPinsForBlinds(byte output, String room)
{
  pinMode(output, OUTPUT);
}

void HttpCustomRespond(String endpoint, Client * client)
{
  if (endpoint == String("getStatus"))
  {
     lights.WriteStatus(client);
  }
  else if (endpoint == String("getTime"))
  {
    tmElements_t tm;
    RTC.read(tm);            
  
    client->print(tm.Hour, DEC);
    client->print(':');
    client->print(tm.Minute, DEC);
    client->print(':');
    client->println(tm.Second, DEC);
    client->print("Week day:");
    client->println(tm.Wday, DEC);
  }
  else if (endpoint == String("enableTwilightMode"))
    twilightMode = true;
  else if (endpoint == String("disableTwilightMode"))
    twilightMode = false;
  else if (endpoint == String("enableHolidayMode"))
    holidayMode = true;
  else if (endpoint == String("disableHolidayMode"))
    holidayMode = false;
  else if (endpoint == String("getTwilightMode"))
    client->println(holidayMode);
  else if (endpoint == String("getHolidayMode"))
    client->println(holidayMode);
}

void ProcessHttpRequest(WebClient webClient)
{
  String parameters;
  String endpoint = webClient.getRequest(parameters, HttpCustomRespond);

  if (String(endpoint) == String("impulsOswietlenie") || String(endpoint) == String("impulsRolety"))
  {
    bool value;
    String parameter = ParseHttpParameters(parameters, &value);
    
    if (String(endpoint) == String("impulsOswietlenie"))
    {
      lights.SwitchLight(parameter, value ? HIGH : LOW);
    }
    else if (String(endpoint) == String("impulsRolety"))
    {
      if (parameter == String("allRoletyUp"))
        blinds.AllBlindsUp();
      else if (parameter == String("allRoletyDown"))
        blinds.AllBlindsDown();
      else
        blinds.MoveBlind(parameters);
    }
  }
}

String ParseHttpParameters(String parameters, bool * outValue)
{
    int equalsIndex = parameters.indexOf('=');
    String parameter = parameters.substring(0, equalsIndex);
    String boolString = parameters.substring(equalsIndex + 1);

    *outValue = (boolString == "true");
    return parameter;
}

void HandleAutoEvents()
{      
  tmElements_t tm;
  RTC.read(tm);
  int hour = tm.Hour;
  int minute = tm.Minute;
  int weekday = tm.Wday;
  
  if (holidayMode == true)
    HandleAutoLight(hour);

  if (!twilightMode)
    return;

  int day = tm.Day;
  int month = tm.Month;
  int twilightTime = twilight[month - 1][day - 1];
  int twilightHour = (twilightTime / 100) + 2;
  int twilightMinute = twilightTime % 100;

  if (hour == twilightHour && minute == twilightMinute)
  {
      blinds.AllBlindsDown();
  }
  else
  {
    if (((weekday == 1 || weekday == 7) && hour == 7 && minute == 30)  || (weekday > 1 && weekday < 7 && (hour == 6 && minute == 30)))
    {
        blinds.AllBlindsUp();
    }
  }
}

void HandleAutoLight(int hour)
{
  if (hour > 20 || hour < 2)
  {
    int swiatla[] = {};
    int index = random(8);

    int swiatlo = swiatla[index] + 1;
    
    if (swiatlo == lightCurrentlyOn)
      return;

    if (lightCurrentlyOn != 0)
    {
      //Light(lightCurrentlyOn, LOW);
      delay(5000 + (random(6) * 1000));
    }

    //Light(swiatlo, HIGH);
    lightCurrentlyOn = swiatlo;
  }
  else
  {
    if (lightCurrentlyOn != 0)
    {      
      //Light(lightCurrentlyOn, LOW);
      lightCurrentlyOn = 0;
    }
  }
}


