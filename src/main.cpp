#include <Arduino.h>

#include "Olgalimex/_config.h"
#include "Olgalimex/_ethernet.h"
#include "Olgalimex/_wifi.h"
#include "Olgalimex/_dmx.h"
#include "Olgalimex/_artnet.h"
#include "Olgalimex/_osc.h"
#include "Olgalimex/_web.h"

uint8_t actualPump1DmxValue = 0;
uint8_t actualPump2DmxValue = 0;
uint8_t actualDimmer1DmxValue = 0;
uint8_t actualDimmer2DmxValue = 0;

int outStrobe1Pin = GPIO_NUM_4;
int outStrobe2Pin = GPIO_NUM_5;

int motorUpEnablePin = GPIO_NUM_13;
int motorUpDirPin = GPIO_NUM_14;
int motorUpPulsePin = GPIO_NUM_15;

int motorDownEnablePin = GPIO_NUM_16;
int motorDownDirPin = GPIO_NUM_32;
int motorDownPulsePin = GPIO_NUM_33;

int strobe1Channel = 0;
int strobe2Channel = 2;
int motorUpChannel = 4;
int motorDownChannel = 6;

int strobeFreq = 60;
int strobe1Dimmer = 0;
int strobe2Dimmer = 0;
int motorUpFreq = 0;
int motorDownFreq = 0;


void setup() {
	// generic parameters
	oscPort = 9004;
	chipName = "Fontaine 32N";
	eepromMarker = 0xCD;
	// config : read and write parameters
	// infos : read only parameters
	addIntConfig("dmx/address", 1);
  addIntConfig("motor1/downValue", 8933);
  addIntConfig("motor1/pauseValue", 8133);
  addIntConfig("motor1/upValue", 7799);
  addIntConfig("motor2/downValue", 1733);
  addIntConfig("motor2/pauseValue", 6733);
  addIntConfig("motor2/upValue", 8733);

	Serial.begin(115200);

  pinMode(outStrobe1Pin, OUTPUT); 
  pinMode(outStrobe2Pin, OUTPUT); 

  pinMode(motorUpEnablePin, OUTPUT);
  pinMode(motorUpPulsePin, OUTPUT);
  pinMode(motorUpDirPin, OUTPUT);

  pinMode(motorDownEnablePin, OUTPUT);
  pinMode(motorDownPulsePin, OUTPUT);
  pinMode(motorDownDirPin, OUTPUT);


  digitalWrite(outStrobe1Pin, LOW);
  digitalWrite(outStrobe2Pin, LOW);
  
  digitalWrite(motorUpEnablePin, LOW);
  digitalWrite(motorUpPulsePin, LOW);
  digitalWrite(motorUpDirPin, LOW);

  digitalWrite(motorDownEnablePin, LOW);
  digitalWrite(motorDownPulsePin, LOW);
  digitalWrite(motorDownDirPin, LOW);

  
  ledcSetup(strobe1Channel, 60, 10); 
  ledcSetup(strobe2Channel, 60, 10); 
  ledcSetup(motorUpChannel, 1000, 10); 
  ledcSetup(motorDownChannel, 1000, 10); 
  
  ledcAttachPin(outStrobe1Pin, strobe1Channel);       // Associer le GPIO au canal PWM
  ledcAttachPin(outStrobe2Pin, strobe2Channel);       // Associer le GPIO au canal PWM
  ledcAttachPin(motorUpPulsePin, motorUpChannel);       // Associer le GPIO au canal PWM
  ledcAttachPin(motorDownPulsePin, motorDownChannel);       // Associer le GPIO au canal PWM
  
  ledcWrite(strobe1Channel, 0);
  ledcWrite(strobe2Channel, 0);
  ledcWrite(motorUpChannel, 0);
  ledcWrite(motorDownChannel, 0);
  


	initConfig();
	setupEthernet();
	setupWifi();
	setupOSC();
	setupWebServer();
	//setupArtnet();
	receivePin = 36;
	setupDMX();


	OscWiFi.subscribe(oscPort, "/testSerial", [](const OscMessage &m) { Serial.println("test"); });
}


void loop()
{
	vTaskDelay(5);
}

void configUpdatedMain(String key)
{
}



void onArtnetFrame(uint16_t universeRcv, uint16_t length, uint8_t sequence, uint8_t *data, IPAddress remoteIP)
{
}

void setStrobeFreq(int v) {
  ledcWriteTone(strobe1Channel, v); 
  ledcWrite(strobe1Channel, strobe1Dimmer);
}

void setStrobe1Dimmer(int v) {
  strobe1Dimmer = v;
  ledcWrite(strobe1Channel, strobe1Dimmer);
}

void setStrobe2Dimmer(int v) {
  strobe2Dimmer = v;
  ledcWrite(strobe2Channel, strobe2Dimmer);
}

void setMotorUpSpeed(int v) {
  motorUpFreq = v;
  if (v == 0) {
    digitalWrite(motorUpDirPin, motorUpFreq < 0 ? HIGH : LOW);
    ledcWriteTone(motorUpChannel, abs(motorUpFreq));
    ledcWrite(motorUpChannel,0);
  } else {
    digitalWrite(motorUpDirPin, motorUpFreq < 0 ? HIGH : LOW);
    ledcWriteTone(motorUpChannel, abs(motorUpFreq));
    ledcWrite(motorUpChannel,512);
  }
}

void setMotorDownSpeed(int v) {
  motorDownFreq = v;
  if (v == 0) {
    digitalWrite(motorDownDirPin, motorDownFreq < 0 ? HIGH : LOW);
    ledcWriteTone(motorDownChannel, abs(motorDownFreq));
    ledcWrite(motorDownChannel,0);
  } else {
    digitalWrite(motorDownDirPin, motorDownFreq < 0 ? HIGH : LOW);
    ledcWriteTone(motorDownChannel, abs(motorDownFreq));
    ledcWrite(motorDownChannel,512);
  }
}


void setPump1DMX(uint8_t val) {
  if (actualPump1DmxValue != val) {
    actualPump1DmxValue = val;
    if (val < 5) {
      setMotorUpSpeed(0);
    }
    else if (val < 127) {
  	int from = config["motor1/downValue"].as<int>();
    }
    Serial.println("new Pump 1 val : " + String(val));
  }
}

void setPump2DMX(uint8_t val) {
  if (actualPump2DmxValue != val) {
    actualPump2DmxValue = val;
    Serial.println("new Pump 2 val : " + String(val));
  }
}

void setDimmer1DMX(uint8_t val) {
  if (actualDimmer1DmxValue != val) {
    actualDimmer1DmxValue = val;
    setStrobe1Dimmer(map(val, 0, 255, 0, 1023));
  }
}

void setDimmer2DMX(uint8_t val) {
  if (actualDimmer2DmxValue != val) {
    actualDimmer2DmxValue = val;
    setStrobe2Dimmer(map(val, 0, 255, 0, 1023));
  }
}


void onDmxFrame()
{
	int ad = config["dmx/address"].as<int>();
  setDimmer1DMX(dmxData[ad+0]);
  setDimmer2DMX(dmxData[ad+1]);
  setPump1DMX(dmxData[ad+2]);
  setPump2DMX(dmxData[ad+3]);
}

void triggerTriggered(String str) {

}
