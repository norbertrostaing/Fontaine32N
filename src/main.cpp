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





void onArtnetFrame(uint16_t universeRcv, uint16_t length, uint8_t sequence, uint8_t *data, IPAddress remoteIP)
{
}

void setStrobeFreq(int v) {
  ledcWriteTone(strobe1Channel, v); 
  ledcWrite(strobe1Channel, strobe1Dimmer);
  ledcWriteTone(strobe2Channel, v); 
  ledcWrite(strobe2Channel, strobe2Dimmer);
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


void setPumpsDMX(uint8_t val) {
  if (actualPump1DmxValue != val) {
    actualPump1DmxValue = val;
    int stop = 0;
    int up1 = config["motor1/reverseValue"].as<int>();
    int pause1 = config["motor1/pauseValue"].as<int>();
    int down1 = config["motor1/flowValue"].as<int>();
    int speed1 = 0;
    int up2 = config["motor2/reverseValue"].as<int>();
    int pause2 = config["motor2/pauseValue"].as<int>();
    int down2 = config["motor2/flowValue"].as<int>();
    int speed2 = 0;
    if (val < 81) {
      speed1 = map(val, 0, 81, stop, up1);
      speed2 = map(val, 0, 81, stop, up2);
    }
    else if (val < 87) {
      speed1 = up1;
      speed2 = up2;
    }
    else if (val < 168) {
      speed1 = map(val, 87, 167, up1, pause1);
      speed2 = map(val, 87, 167, up2, pause2);
    }
    else if (val < 174) {
      speed1 = pause1;
      speed2 = pause2;
    }
    else  {
      speed1 = map(val, 175, 255, pause1, down1);
      speed2 = map(val, 175, 255, pause2, down2);
    }
    setMotorUpSpeed(speed1);
    setMotorDownSpeed(speed2);
  }
}

// void setPump2DMX(uint8_t val) {
//   if (actualPump2DmxValue != val) {
//     actualPump2DmxValue = val;
//     int stop = 0;
//     int up = config["motor2/reverseValue"].as<int>();
//     int pause = config["motor2/pauseValue"].as<int>();
//     int down = config["motor2/flowValue"].as<int>();
//     int speed = 0;
//     if (val < 81) {
//       speed = map(val, 0, 81, stop, up);
//     }
//     else if (val < 87) {
//       speed = up;
//     }
//     else if (val < 168) {
//       speed = map(val, 87, 167, up, pause);
//     }
//     else if (val < 174) {
//       speed = pause;
//     }
//     else  {
//       speed = map(val, 175, 255, pause, down);
//     }
//     setMotorDownSpeed(speed);
//   }
// }

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
  setPumpsDMX(dmxData[ad+2]);
}

void triggerTriggered(String str) {

}


void configUpdatedMain(String key)
{
  if (key == "strobeFreq") {
    setStrobeFreq(config["strobeFreq"].as<int>());
  }
}

void setup() {
	// generic parameters
	oscPort = 9004;
	chipName = "Fontaine 32N";
	eepromMarker = 0xAB;
	// config : read and write parameters
	// infos : read only parameters
	addIntConfig("dmx/address", 1);
  addIntConfig("strobeFreq", 60);
  addIntConfig("motor1/flowValue", 8933);
  addIntConfig("motor1/pauseValue", 8133);
  addIntConfig("motor1/reverseValue", 7799);
  addIntConfig("motor2/flowValue", 1733);
  addIntConfig("motor2/pauseValue", 6733);
  addIntConfig("motor2/reverseValue", 8733);

	Serial.begin(115200);
	initConfig();


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

  
  ledcSetup(strobe1Channel, config["strobeFreq"].as<int>(), 10); 
  ledcSetup(strobe2Channel, config["strobeFreq"].as<int>(), 10); 
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
  


	setupEthernet();
	setupWifi();
	setupOSC();
	setupWebServer();
	//setupArtnet();
	receivePin = 36;
	setupDMX();

  OscWiFi.subscribe(oscPort, "/strobeFreq",     [](const OscMessage& m) { if (m.size()>0) { setStrobeFreq(m.arg<int>(0)); } });
  OscWiFi.subscribe(oscPort, "/strobe1Dimmer",  [](const OscMessage& m) { if (m.size()>0) { setStrobe1Dimmer(m.arg<float>(0)*1023); } });
  OscWiFi.subscribe(oscPort, "/strobe2Dimmer",  [](const OscMessage& m) { if (m.size()>0) { setStrobe2Dimmer(m.arg<float>(0)*1023); } });
  OscWiFi.subscribe(oscPort, "/motorUpSpeed",   [](const OscMessage& m) { if (m.size()>0) { setMotorUpSpeed(m.arg<int>(0)); } });
  OscWiFi.subscribe(oscPort, "/motorDownSpeed", [](const OscMessage& m) { if (m.size()>0) { setMotorDownSpeed(m.arg<int>(0)); } });
  OscWiFi.subscribe(oscPort, "/saveReverseData", [](const OscMessage& m) { writeConfig("motor1/reverseValue", motorUpFreq); writeConfig("motor2/reverseValue", motorDownFreq); });
  OscWiFi.subscribe(oscPort, "/savePauseData", [](const OscMessage& m) {  writeConfig("motor1/pauseValue", motorUpFreq); writeConfig("motor2/pauseValue", motorDownFreq); });
  OscWiFi.subscribe(oscPort, "/saveFlowData", [](const OscMessage& m) { writeConfig("motor1/flowValue", motorUpFreq); writeConfig("motor2/flowValue", motorDownFreq); });
}


void loop()
{
	vTaskDelay(5);
}
