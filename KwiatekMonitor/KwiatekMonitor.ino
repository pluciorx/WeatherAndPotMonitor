/*
Name:		KwiatekMonitor ESP32 V/M/T/P
Created:	22/04/2022 2:58:36 PM
Author:		pluciorx@gmail.com
Version: 1.0b
*/

#include <ThingSpeak.h>
#include <WiFi.h>
#include <Adafruit_BME280.h>
#include <Pangodream_18650_CL.h>
#include <driver/adc.h>

//BME Sensor setup
#define SEALEVELPRESSURE_HPA (1013.25)
#define SENSORS_VCCPIN 12
#define BME_TEMP_COMPENSATION -3
Adafruit_BME280 bme; // I2C
float air_temperatureC = 0;
float air_pressure = 0;
float altitude = 0;
float air_humidity = 0;

//Handy stuff for voltate read
Pangodream_18650_CL BL;
double battery_volt = 0;

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1700        //just a little less than 30 min's 
//#define TIME_TO_SLEEP  20        //for debug lets do the stuff every 20S

//Soil Moisture Setup
const int SensorPin = 33;
int soilMoistureValue = 0;
int soilmoisturepercent = 0;
const int AirValue = 2400;   //take sensour out of water 
const int WaterValue = 712;  //you need to replace this value with Value_2

const char* ssid = "www.smarttip.ch";   // your network SSID (name) 
const char* password = "xxxxxx";   // your network password

WiFiClient client;

//ThingSpeak channel details
unsigned long myChannelNumber = 1711365;
const char* myWriteAPIKey = "xxxxx";


enum EState
{
	INITIATING,
	SENDING,
	SENSORS_READ,
	SLEEPING
};
EState _state;
//uint32_t Freq = 0;

void setup() {
	Serial.begin(115200);
	
	pinMode(SENSORS_VCCPIN, OUTPUT);
	pinMode(13, OUTPUT);
	digitalWrite(13, LOW); //power down sensors
	analogSetPinAttenuation(34, ADC_11db);
	analogSetPinAttenuation(33, ADC_11db);
	digitalWrite(SENSORS_VCCPIN, LOW); //power down sensors
	btStop();
	WiFi.mode(WIFI_OFF);
	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

	_state = INITIATING;
}

void collectValues() {

	digitalWrite(SENSORS_VCCPIN, HIGH); //power up sensors
	delay(100);

	while (!bme.begin(0x76)) {
		Serial.println("Could not find a valid BME280 sensor, check wiring!");
	}
	bme.setTemperatureCompensation(BME_TEMP_COMPENSATION);
	Serial.println("");
	air_temperatureC = bme.readTemperature();
	Serial.print("Temperature = ");
	Serial.print(air_temperatureC);
	Serial.println(" *C");

	air_pressure = bme.readPressure() / 100.0F;
	Serial.print("Pressure = ");
	Serial.print(air_pressure);
	Serial.println(" hPa");

	altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
	Serial.print("Approx. Altitude = ");
	Serial.print(altitude);
	Serial.println(" m");
	air_humidity = bme.readHumidity();
	Serial.print("Humidity = ");
	Serial.print(air_humidity);
	Serial.println(" %");

	//Soil Moisture Readings filter from the edge data
	while (readSoilMoistureSensor() > 99 || soilmoisturepercent < 1) {
		delay(10);
	}
	
	Serial.print("Moist Value:");
	Serial.print(soilMoistureValue);
	Serial.println(" V");
	Serial.print("Moisture:");
	Serial.print(soilmoisturepercent);
	Serial.println("%");

	//Battery level.
	Serial.print("Raw Value: ");
	Serial.println(BL.pinRead());
	Serial.print("Volts: ");
	battery_volt = BL.getBatteryVolts();
	Serial.println(battery_volt);
	Serial.print("Charge level: ");
	Serial.println(BL.getBatteryChargeLevel());
	Serial.println("");

	digitalWrite(SENSORS_VCCPIN, LOW); //power down bme and Soil moisture before using WiFi
	
}

int readSoilMoistureSensor() {
	soilMoistureValue = analogRead(SensorPin);  
	soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
	
	return soilmoisturepercent;             
}

// the loop function runs over and over again until power down or reset
void loop() {

	DoWork();
}

void DoWork()
{
	switch (_state)
	{
	case INITIATING: {
		Serial.println("ESP32 Initialised...");
		adc_power_on();
		_state = SENSORS_READ;

	}break;
	case SLEEPING: {

		Serial.println("Going Sleep...");
		Serial.flush();
		digitalWrite(SensorPin, LOW);
		digitalWrite(DEF_PIN, LOW);
		WiFi.disconnect(true);
		WiFi.mode(WIFI_OFF);

		adc_power_off();
		esp_deep_sleep_start();

	}break;
	case SENSORS_READ: {

		Serial.println("Reading Sensors...");
		collectValues();
		_state = SENDING;

	}break;
	case SENDING:
	{
		Serial.println("Connecting and sending data");
		WiFi.mode(WIFI_STA);
		while (WiFi.status() != WL_CONNECTED) {
			WiFi.begin(ssid, password);
			delay(1000);
		}
		
		ThingSpeak.begin(client);  // Initialize ThingSpeak
		ThingSpeak.setField(1, air_temperatureC);
		ThingSpeak.setField(2, air_pressure);
		ThingSpeak.setField(3, air_humidity);
		ThingSpeak.setField(4, String(battery_volt, 3));
		ThingSpeak.setField(5, soilmoisturepercent);
		ThingSpeak.setField(6, altitude);
		
		int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

		if (x == 200) {
			Serial.println("Channel update successful.");
			_state = SLEEPING;
		}
		else {
			Serial.println("Problem updating channel. HTTP error code " + String(x));
			delay(2500);
		}
	}break;

	}
}