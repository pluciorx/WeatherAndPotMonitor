/*
Name:		KwiatekMonitor ESP32 V/M/T/P
Created:	22/04/2022 2:58:36 PM
Author:		pluciorx@gmail.com
Version: 1.0b
Based on: https://github.com/educ8s/ESP8266-Weather-Display-with-Art-Deco-enclosure 
*/


#include <ThingSpeak.h>
#include <DallasTemperature.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <JSON_Decoder.h>
#include <OpenWeather.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <OneWire.h>

// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
#define GREY     0xC618

#define cs   D2  
#define dc   D3   
#define rst  D4 

//DS onboard temp sensor
OneWire oneWire(D1);
DallasTemperature sensors(&oneWire);

OW_Weather OWC; // Weather forecast library instance

const char* ssid = "www.smarttip.ch";      // SSID of local network
const char* password = "xxxxx";  // Password on network
char* APIKEY = "xxxxxxxx"; //OpenWeather API Key
char* CityID = "2658524"; //Spreitenbach but we will use IP

//ThingSpeak channel
const unsigned long myChannelNumber = 1711365;
const char* myWriteAPIKey = "xxxxxxxx";

const long utcOffsetInHours = 2; //UTC + 2h

// Define NTP Client to get time
WiFiUDP ntpUDP;
WiFiClient client;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * utcOffsetInHours);

boolean night = false;
int counter = 900;

float Temperature;
float intTemperature;
String timeS;
String date;
byte last_second, second_, minute_, hour_, day_, month_;
int year_;
char Date[] = "00/00/2000";
const int iconYoffset = -7;

extern  unsigned char  cloud[];
extern  unsigned char  thunder[];
extern  unsigned char  wind[];

Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

void setup() {
	Serial.begin(115200);
	tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
	tft.fillScreen(BLACK);
	sensors.begin();

	Serial.println("Connecting");
	WiFi.begin(ssid, password);

	tft.setCursor(30, 80);
	tft.setTextColor(WHITE);
	tft.setTextSize(1);
	tft.print("Connecting...");

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}
	timeClient.begin();
	ThingSpeak.begin(client);  // Initialize ThingSpeak

	clearScreen();
	tft.setCursor(30, 80);
	tft.setTextColor(WHITE);
	tft.setTextSize(1);
	tft.print("Connected...");
	Serial.println("Connected");
	delay(1000);
}

void loop() {

	if (counter == 900) //Get new data every 30 minutes
	{
		counter = 0;
		getWeatherData();
		printDate();
	}
	else
	{
		counter++;
		delay(1000);
		printClock();

		printIntTemp();
		Serial.println(counter);
	}
}

void updateIntTemp()
{
	sensors.requestTemperatures();
	intTemperature = sensors.getTempCByIndex(0);
	intTemperature = intTemperature - 5;
	Serial.print(intTemperature);
	Serial.println("ºC");
}

void getWeatherData() 
{
	// Create the structures that hold the retrieved weather
	OW_current* current = new OW_current;
	// OW_hourly* hourly = new OW_hourly;
	//OW_daily* daily = new OW_daily;
	Serial.println("Getting Weather from OpenWeatherMap");
	OWC.getForecast(current, nullptr, nullptr, APIKEY, "47.42016", "8.36301", "metric", "en", false);
	int potHumidity = 0;

	potHumidity = ThingSpeak.readIntField(myChannelNumber, 5);
	if (potHumidity > 99) potHumidity = 99;
	float battVoltage = ThingSpeak.readFloatField(myChannelNumber, 4);
	int status = ThingSpeak.getLastReadStatus();
	if ( status == 200)
	{
		Serial.print("Pot:");
		Serial.println(potHumidity);

		Serial.print("VBatt:");
		Serial.println(battVoltage);
	}
	else {
		Serial.println("No Weather Data");
		Serial.println(status);
	}

	printData(current.temp, current.id, current.humidity, potHumidity, battVoltage);

	delete current;
}

void printDate()
{
	printClock(); // to get the time updated. 

	unsigned long unix_epoch = timeClient.getEpochTime();
	day_ = day(unix_epoch);
	month_ = month(unix_epoch);
	year_ = year(unix_epoch);

	Date[0] = day_ / 10 + 48;
	Date[1] = day_ % 10 + 48;
	Date[3] = month_ / 10 + 48;
	Date[4] = month_ % 10 + 48;
	Date[8] = (year_ / 10) % 10 + 48;
	Date[9] = year_ % 10 % 10 + 48;
	
	tft.fillRect(15, 5, 75, 10, BLACK);

	tft.setCursor(15, 10);
	tft.setTextColor(WHITE);
	tft.setTextSize(1);
	tft.print(Date);

}

void printClock()
{
	timeClient.update();
	timeS = timeClient.getFormattedTime();

	setDayOrNight(timeS);

	tft.setCursor(15, 20);
	tft.fillRect(15, 20, 100, 20, BLACK);

	tft.setCursor(15, 20);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print(timeS);
}

void printIntTemp()
{
	updateIntTemp();

	tft.fillRect(10, 135, 50, 20, BLACK);
	//DS temp
	tft.setCursor(10, 135);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print(String(intTemperature, 1));

	tft.setCursor(58, 135);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print("C");
}

void printPotHumidity(int potHumidity)
{
	
	tft.fillRect(83, 135, 50, 20, BLACK);
	//ThingSpeak humidity
	tft.setCursor(83, 135);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print(potHumidity);

	tft.setCursor(106, 135);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print("%");
}

void printLastBattVolt(float battVolt)
{
	tft.fillRect(83,152,40, 10, BLACK);
	//ThingSpeak humidity
	tft.setCursor(83, 152);
	tft.setTextColor(WHITE);
	tft.setTextSize(1);
	tft.print(String(battVolt,3));

	tft.setCursor(113, 152);
	tft.setTextColor(WHITE);
	tft.setTextSize(1);
	tft.print("V");
}

void printData(float outTemp, int weatherID, int humidity, int potHumidity,float potVbatt)
{
	clearScreen();
	printClock();
	printWeatherIcon(weatherID);
	//OW temp
	tft.setCursor(10, 115);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print(String(outTemp, 1));

	tft.setCursor(58, 115);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print("C");

	//OW humidity
	tft.setCursor(83, 115);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print(humidity);

	tft.setCursor(106, 115);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print("%");

	printPotHumidity(potHumidity);
	printLastBattVolt(potVbatt);
}

void printWeatherIcon(int id)
{
	switch (id)
	{
	case 800: drawClearWeather(); break;
	case 801: drawFewClouds(); break;
	case 802: drawFewClouds(); break;
	case 803: drawCloud(); break;
	case 804: drawCloud(); break;

	case 200: drawThunderstorm(); break;
	case 201: drawThunderstorm(); break;
	case 202: drawThunderstorm(); break;
	case 210: drawThunderstorm(); break;
	case 211: drawThunderstorm(); break;
	case 212: drawThunderstorm(); break;
	case 221: drawThunderstorm(); break;
	case 230: drawThunderstorm(); break;
	case 231: drawThunderstorm(); break;
	case 232: drawThunderstorm(); break;

	case 300: drawLightRain(); break;
	case 301: drawLightRain(); break;
	case 302: drawLightRain(); break;
	case 310: drawLightRain(); break;
	case 311: drawLightRain(); break;
	case 312: drawLightRain(); break;
	case 313: drawLightRain(); break;
	case 314: drawLightRain(); break;
	case 321: drawLightRain(); break;

	case 500: drawLightRainWithSunOrMoon(); break;
	case 501: drawLightRainWithSunOrMoon(); break;
	case 502: drawLightRainWithSunOrMoon(); break;
	case 503: drawLightRainWithSunOrMoon(); break;
	case 504: drawLightRainWithSunOrMoon(); break;
	case 511: drawLightRain(); break;
	case 520: drawModerateRain(); break;
	case 521: drawModerateRain(); break;
	case 522: drawHeavyRain(); break;
	case 531: drawHeavyRain(); break;

	case 600: drawLightSnowfall(); break;
	case 601: drawModerateSnowfall(); break;
	case 602: drawHeavySnowfall(); break;
	case 611: drawLightSnowfall(); break;
	case 612: drawLightSnowfall(); break;
	case 615: drawLightSnowfall(); break;
	case 616: drawLightSnowfall(); break;
	case 620: drawLightSnowfall(); break;
	case 621: drawModerateSnowfall(); break;
	case 622: drawHeavySnowfall(); break;

	case 701: drawFog(); break;
	case 711: drawFog(); break;
	case 721: drawFog(); break;
	case 731: drawFog(); break;
	case 741: drawFog(); break;
	case 751: drawFog(); break;
	case 761: drawFog(); break;
	case 762: drawFog(); break;
	case 771: drawFog(); break;
	case 781: drawFog(); break;

	default:break;
	}
}

void setDayOrNight(String timeS)
{
	int length = timeS.length();
	String hour = timeS.substring(length - 8, length - 6);
	int time = timeS.toInt();

	if (time > 21 || time < 7)
	{
		night = true;
	}
	else
	{
		night = false;
	}
}

void clearScreen()
{
	tft.fillScreen(BLACK);
}

void drawClearWeather()
{
	if (night)
	{
		drawTheMoon();
	}
	else
	{
		drawTheSun();
	}
}

void drawFewClouds()
{
	if (night)
	{
		drawCloudAndTheMoon();
	}
	else
	{
		drawCloudWithSun();
	}
}

void drawTheSun()
{
	tft.fillCircle(64, 80 + iconYoffset, 26, YELLOW);
}

void drawTheFullMoon()
{
	tft.fillCircle(64, 80 + iconYoffset, 26, GREY);
}

void drawTheMoon()
{
	tft.fillCircle(64, 80 + iconYoffset, 26, GREY);
	tft.fillCircle(75, 73 + iconYoffset, 26, BLACK);
}

void drawCloud()
{
	tft.drawBitmap(0, 35 + iconYoffset, cloud, 128, 90, GREY);
}

void drawCloudWithSun()
{
	tft.fillCircle(73, 70 + iconYoffset, 20, YELLOW);
	tft.drawBitmap(0, 36 + iconYoffset, cloud, 128, 90, BLACK);
	tft.drawBitmap(0, 40 + iconYoffset, cloud, 128, 90, GREY);
}

void drawLightRainWithSunOrMoon()
{
	if (night)
	{
		drawCloudTheMoonAndRain();
	}
	else
	{
		drawCloudSunAndRain();
	}
}

void drawLightRain()
{
	tft.drawBitmap(0, 35 + iconYoffset, cloud, 128, 90, GREY);
	tft.fillRoundRect(50, 105 + iconYoffset, 3, 13, 1, BLUE);
	tft.fillRoundRect(65, 105 + iconYoffset, 3, 13, 1, BLUE);
	tft.fillRoundRect(80, 105 + iconYoffset, 3, 13, 1, BLUE);
}

void drawModerateRain()
{
	tft.drawBitmap(0, 35 + iconYoffset, cloud, 128, 90, GREY);
	tft.fillRoundRect(50, 105 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(57, 102 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(65, 105 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(72, 102 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(80, 105 + iconYoffset, 3, 15, 1, BLUE);
}

void drawHeavyRain()
{
	tft.drawBitmap(0, 35 + iconYoffset, cloud, 128, 90, GREY);
	tft.fillRoundRect(43, 102 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(50, 105 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(57, 102 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(65, 105 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(72, 102 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(80, 105 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(87, 102 + iconYoffset, 3, 15, 1, BLUE);
}

void drawThunderstorm()
{
	tft.drawBitmap(0, 40 + iconYoffset, thunder, 128, 90, YELLOW);
	tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
	tft.fillRoundRect(48, 102 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(55, 102 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(74, 102 + iconYoffset, 3, 15, 1, BLUE);
	tft.fillRoundRect(82, 102 + iconYoffset, 3, 15, 1, BLUE);
}

void drawLightSnowfall()
{
	tft.drawBitmap(0, 30 + iconYoffset, cloud, 128, 90, GREY);
	tft.fillCircle(50, 100 + iconYoffset, 3, GREY);
	tft.fillCircle(65, 103 + iconYoffset, 3, GREY);
	tft.fillCircle(82, 100 + iconYoffset, 3, GREY);
}

void drawModerateSnowfall()
{
	tft.drawBitmap(0, 35 + iconYoffset, cloud, 128, 90, GREY);
	tft.fillCircle(50, 105 + iconYoffset, 3, GREY);
	tft.fillCircle(50, 115 + iconYoffset, 3, GREY);
	tft.fillCircle(65, 108 + iconYoffset, 3, GREY);
	tft.fillCircle(65, 118 + iconYoffset, 3, GREY);
	tft.fillCircle(82, 105 + iconYoffset, 3, GREY);
	tft.fillCircle(82, 115 + iconYoffset, 3, GREY);
}

void drawHeavySnowfall()
{
	tft.drawBitmap(0, 35 + iconYoffset, cloud, 128, 90, GREY);
	tft.fillCircle(40, 105 + iconYoffset, 3, GREY);
	tft.fillCircle(52, 105 + iconYoffset, 3, GREY);
	tft.fillCircle(52, 115 + iconYoffset, 3, GREY);
	tft.fillCircle(65, 108 + iconYoffset, 3, GREY);
	tft.fillCircle(65, 118 + iconYoffset, 3, GREY);
	tft.fillCircle(80, 105 + iconYoffset, 3, GREY);
	tft.fillCircle(80, 115 + iconYoffset, 3, GREY);
	tft.fillCircle(92, 105 + iconYoffset, 3, GREY);
}

void drawCloudSunAndRain()
{
	tft.fillCircle(73, 70 + iconYoffset, 20, YELLOW);
	tft.drawBitmap(0, 32 + iconYoffset, cloud, 128, 90, BLACK);
	tft.drawBitmap(0, 35 + iconYoffset, cloud, 128, 90, GREY);
	tft.fillRoundRect(50, 105 + iconYoffset, 3, 13, 1, BLUE);
	tft.fillRoundRect(65, 105 + iconYoffset, 3, 13, 1, BLUE);
	tft.fillRoundRect(80, 105 + iconYoffset, 3, 13, 1, BLUE);
}

void drawCloudAndTheMoon()
{
	tft.fillCircle(94, 60 + iconYoffset, 18, GREY);
	tft.fillCircle(105, 53 + iconYoffset, 18, BLACK);
	tft.drawBitmap(0, 32 + iconYoffset, cloud, 128, 90, BLACK);
	tft.drawBitmap(0, 35 + iconYoffset, cloud, 128, 90, GREY);
}

void drawCloudTheMoonAndRain()
{
	tft.fillCircle(94, 60 + iconYoffset, 18, GREY);
	tft.fillCircle(105, 53 + iconYoffset, 18, BLACK);
	tft.drawBitmap(0, 32 + iconYoffset, cloud, 128, 90, BLACK);
	tft.drawBitmap(0, 35 + iconYoffset, cloud, 128, 90, GREY);
	tft.fillRoundRect(50, 105 + iconYoffset, 3, 11, 1, BLUE);
	tft.fillRoundRect(65, 105 + iconYoffset, 3, 11, 1, BLUE);
	tft.fillRoundRect(80, 105 + iconYoffset, 3, 11, 1, BLUE);
}

void drawWind()
{
	tft.drawBitmap(0, 35 + iconYoffset, wind, 128, 90, GREY);
}

void drawFog()
{
	tft.fillRoundRect(45, 60 + iconYoffset, 40, 4, 1, GREY);
	tft.fillRoundRect(40, 70 + iconYoffset, 50, 4, 1, GREY);
	tft.fillRoundRect(35, 80 + iconYoffset, 60, 4, 1, GREY);
	tft.fillRoundRect(40, 90 + iconYoffset, 50, 4, 1, GREY);
	tft.fillRoundRect(45, 100 + iconYoffset, 40, 4, 1, GREY);
}

void clearIcon()
{
	tft.fillRect(0, 40 + iconYoffset, 128, 100, BLACK);
}
