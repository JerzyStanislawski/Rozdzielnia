#include <arduino.h>
#include "lights.h"
#include "blinds.h"
#include "scheduler.h"
#include "board.h"

#include <Time.h>
#include <TimeLib.h>
#include <DS3232RTC.h>
#include <EEPROM.h>

Lights * Board::lights;
Blinds * Board::blinds;
Scheduler * Board::scheduler;
bool Board::twilightMode;
bool Board::morningMode;
bool Board::holidayMode;
byte Board::morningHour;
byte Board::morningMinute;
byte Board::morningDays;

int Board::settingsEndAddress;

// Twilight time (HHMM) for every day of the year, kept in flash to save RAM.
static const int twilight[12][31] PROGMEM = {
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

void Board::ProcessHttpRequest(WebClient & webClient)
{
  int parameterCount;
  String endpoint = webClient.getRequest(httpParameters, parameterCount, HttpCustomRespond);
  if (endpoint.length() == 0)
    return;

  if (String(endpoint) == String("impulsOswietlenie") || String(endpoint) == String("impulsRolety"))
  {
    bool value;
    String parameter = ParseHttpBoolParameter(httpParameters[0], &value);
    
    if (String(endpoint) == String("impulsOswietlenie"))
    {
      if (String(parameter) == String("allOff"))
        lights->AllLightsOff();
      else
        lights->SwitchLight(parameter, value ? HIGH : LOW);
    }
    else if (String(endpoint) == String("impulsRolety"))
    {
      if (parameter == String("allRoletyUp"))
        blinds->AllBlindsUp();
      else if (parameter == String("allRoletyDown"))
        blinds->AllBlindsDown();
      else
        blinds->MoveBlind(httpParameters[0]);
    }
	
	return;
  }  
  else if (endpoint == String("enableTwilightMode"))
    twilightMode = true;
  else if (endpoint == String("disableTwilightMode"))
    twilightMode = false;
  else if (endpoint == String("enableHolidayMode"))
    holidayMode = true;
  else if (endpoint == String("disableHolidayMode"))
    holidayMode = false;
  else if (endpoint == String("setTime"))
  {
    String timeValue;
    ParseHttpParameter(httpParameters[0], &timeValue);

    int hours = timeValue.substring(0, 2).toInt();
    int minutes = timeValue.substring(3, 5).toInt();
    int seconds = timeValue.substring(6, 8).toInt();

    int day = timeValue.substring(9, 11).toInt();
    int month = timeValue.substring(12, 14).toInt();
    int year = timeValue.substring(15, 19).toInt();
    
    setTime(hours, minutes, seconds, day, month, year);
    RTC.set(now());
  }
  else if (endpoint == String("schedule"))
  {
    String * records = new String[parameterCount];
    for (int i = 0; i < parameterCount; i++)
    {
      String record;
      ParseHttpParameter(httpParameters[i], &record);
      records[i] = record;
    }
    scheduler->Schedule(records, parameterCount, Board::settingsEndAddress);  
    delete [] records;  
  }
  else if (endpoint == String("clearSchedule"))
  {
	  scheduler->Clear(Board::settingsEndAddress);
  }
  else if (endpoint == String("setMorningMode"))
  {
	if (httpParameters[0] == String("false"))
		morningMode = false;
	else if (httpParameters[0] == String("true"))
	{
		String morningDaysValue;
		ParseHttpParameter(httpParameters[1], &morningDaysValue);
		morningDays = morningDaysValue.toInt();
		
		String morningTimeValue;
		ParseHttpParameter(httpParameters[2], &morningTimeValue);
		
		morningHour = morningTimeValue.substring(0, 2).toInt();
		morningMinute = morningTimeValue.substring(3, 5).toInt();
		
		morningMode = true;
	}
  }
  
  StoreSettings();
}

void Board::HttpCustomRespond(const String & endpoint, Print * client)
{
  if (endpoint == String("getStatus"))
  {
     Board::lights->WriteStatus(client);
  }
  else if (endpoint == String("getScheduledEvents"))
  {
    Board::scheduler->WriteEvents(client);
  }
  else if (endpoint == String("getTime"))
  {
	 Board::PrintTime(client);
  }
  else if (endpoint == String("getTwilightMode"))
  {
    client->println(Board::twilightMode);
  }
  else if (endpoint == String("getHolidayMode"))
  {
    client->println(Board::holidayMode);
  }
  else if (endpoint == String("getMorningMode"))
  {
	Board::PrintMorningMode(client);
  }
  else if (endpoint == String("getAllSettings"))
  {
	Board::PrintTime(client);
	client->print("holidayMode: ");
    client->println(Board::holidayMode);
	client->print("twilightMode: ");
    client->println(Board::twilightMode);
	Board::PrintMorningMode(client);
  }
}

void Board::PrintMorningMode(Print * client)
{
	client->print("morningMode: ");
    client->println(Board::morningMode);
	client->print("morningDays: ");
    client->println(Board::morningDays);
	client->print("morningTime: ");
	client->print(Board::morningHour, DEC);
	client->print(':');
	client->println(Board::morningMinute, DEC);
}

void Board::PrintTime(Print * client)
{
    tmElements_t tm;
    RTC.read(tm);            
  
    client->print("Time: ");
    client->print(tm.Hour, DEC);
    client->print(':');
    client->print(tm.Minute, DEC);
    client->print(':');
    client->println(tm.Second, DEC);
    client->print("Date: ");
    client->print(tm.Year + 1970, DEC);
    client->print('-');
    client->print(tm.Month, DEC);
    client->print('-');
    client->println(tm.Day, DEC);
}


String Board::ParseHttpParameter(String parameters, String * outValue)
{
    int equalsIndex = parameters.indexOf('=');
    String parameter = parameters.substring(0, equalsIndex);
    *outValue = parameters.substring(equalsIndex + 1);

    return parameter;
}

String Board::ParseHttpBoolParameter(String parameters, bool * outValue)
{
    String boolString;
    String parameter = ParseHttpParameter(parameters, &boolString);

    *outValue = (boolString == "true");
    return parameter;
}

void Board::TimerEvent(tmElements_t tm)
{		
	if (Board::twilightMode)
	{
	  int offset = tm.Month > 10 || tm.Month < 4 ? 1 : 2;
	  int twilightTime = pgm_read_word(&twilight[tm.Month - 1][tm.Day - 1]);
	  int twilightHour = (twilightTime / 100) + offset;
	  int twilightMinute = twilightTime % 100;
	  
	  if (tm.Hour == twilightHour && tm.Minute == twilightMinute)
		 blinds->AllBlindsDown();
	}
	
	if (Board::morningMode)
	{
	  if (tm.Hour == Board::morningHour && tm.Minute == morningMinute && (morningDays & (1 << tm.Wday)) == (1 << tm.Wday))
		 blinds->AllBlindsUp();	 
	}
}

bool Board::GetHolidayMode()
{
	return holidayMode;
}

void Board::StoreSettings()
{
	int eeAddress = 0;
	
	EEPROM.put(eeAddress, Board::twilightMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.put(eeAddress, Board::morningMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.put(eeAddress, Board::holidayMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.put(eeAddress, Board::morningHour);		
	eeAddress += sizeof(byte);
	
	EEPROM.put(eeAddress, Board::morningMinute);		
	eeAddress += sizeof(byte);
	
	EEPROM.put(eeAddress, Board::morningDays);		
	eeAddress += sizeof(byte);
	
}

int Board::RestoreSettings()
{
	int eeAddress = 0;
	
	EEPROM.get(eeAddress, Board::twilightMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.get(eeAddress, Board::morningMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.get(eeAddress, Board::holidayMode);		
	eeAddress += sizeof(bool);
	
	EEPROM.get(eeAddress, Board::morningHour);		
	eeAddress += sizeof(byte);
	
	EEPROM.get(eeAddress, Board::morningMinute);		
	eeAddress += sizeof(byte);
	
	EEPROM.get(eeAddress, Board::morningDays);		
	eeAddress += sizeof(byte);
	
	Board::settingsEndAddress = eeAddress;
	
	return eeAddress;
}
