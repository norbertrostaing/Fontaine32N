#include <Arduino.h>
#include <mcpwm.h>

StrobePWM strobes;

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

bool pleaseDrop = false;

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
    const float run = in_max - in_min;
    if(run == 0) { return out_min;}
    const float rise = out_max - out_min;
    const float delta = x - in_min;
    return (delta * rise) / run + out_min;
}

void onArtnetFrame(uint16_t universeRcv, uint16_t length, uint8_t sequence, uint8_t *data, IPAddress remoteIP)
{
}

void setStrobeFreq(float v) {
  strobes.setFrequency(v);  
}

void setStrobe1Dimmer(int v) {
  v = constrain(v, 0, 1023);
  strobes.setDuty1(v);
}

void setStrobe2Dimmer(int v) {
  v = constrain(v, 0, 1023);
  strobes.setDuty2(v);
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


void setValuesDMX(float val) {
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

    float upStrobe = config["strobeFreq/reverseValue"].as<float>();
    float pauseStrobe = config["strobeFreq/pauseValue"].as<float>();
    float downStrobe = config["strobeFreq/flowValue"].as<float>();
    float speedStrobe = 0;

    float boundaries[4] = {0.48, 0.52, 0.73, 0.77};

    
   if (val < boundaries[0]) {
      speed1 = fmap(val, 0, boundaries[0], 0, up1);
      speed2 = fmap(val, 0, boundaries[0], 0, up2);
      speedStrobe = upStrobe;
    }
    else if (val < boundaries[1]) {
      speed1 = up1;
      speed2 = up2;
      speedStrobe = upStrobe;
    }
    else if (val < boundaries[2]) {
      speed1 = fmap(val, boundaries[1], boundaries[2], up1, pause1);
      speed2 = fmap(val, boundaries[1], boundaries[2], up2, pause2);
      speedStrobe = fmap(val, boundaries[1], boundaries[2], upStrobe, pauseStrobe);
    }
    else if (val < boundaries[3]) {
      speed1 = pause1;
      speed2 = pause2;
      speedStrobe = pauseStrobe;
    }
    else  {
      speed1 = fmap(val, boundaries[3], 1, pause1, down1);
      speed2 = fmap(val, boundaries[3], 1, pause2, down2);
      speedStrobe = fmap(val, boundaries[3], 1, pauseStrobe, downStrobe);
    }
    setMotorUpSpeed(speed1);
    setMotorDownSpeed(speed2);
    setStrobeFreq(speedStrobe);
  }
}

void setDimmer1DMX(float val) {
  if (actualDimmer1DmxValue != val) {
    actualDimmer1DmxValue = val;
    int max = config["strobe1/maxDim1023"].as<int>();
    int dim = max*val;
    setStrobe1Dimmer(dim);
    }
}

void setDimmer2DMX(float val) {
  if (actualDimmer2DmxValue != val) {
    actualDimmer2DmxValue = val;
    int max = config["strobe2/maxDim1023"].as<int>();
    int dim = max*val;
    setStrobe2Dimmer(dim);
  }
}


void onDmxFrame()
{
	int ad = config["dmx/address"].as<int>();
  setDimmer1DMX(dmxData[ad+0]);
  setDimmer2DMX(dmxData[ad+1]);
  setValuesDMX(dmxData[ad+2]);
}

void triggerTriggered(String str) {

}


void configUpdatedMain(String key)
{
}

void setup() {
	// generic parameters
	oscPort = 9004;
	chipName = "Fontaine 32N";
	eepromMarker = 0x56;
	// config : read and write parameters
	// infos : read only parameters
	addIntConfig("dmx/address", 1);
  addIntConfig("motor1/flowValue", 8000);
  addIntConfig("motor1/pauseValue", 8000);
  addIntConfig("motor1/reverseValue", 8000);

  addIntConfig("motor2/flowValue", 1900);
  addIntConfig("motor2/pauseValue", 2800);
  addIntConfig("motor2/reverseValue", 8000);

  addFloatConfig("strobeFreq/flowValue", 65);
  addFloatConfig("strobeFreq/pauseValue", 60);
  addFloatConfig("strobeFreq/reverseValue", 55);


  addIntConfig("strobe1/maxDim1023", 102);
  addIntConfig("strobe2/maxDim1023", 102);

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

  ledcSetup(motorUpChannel, 1000, 10); 
  ledcSetup(motorDownChannel, 1000, 10); 
  
  ledcAttachPin(motorUpPulsePin, motorUpChannel);       // Associer le GPIO au canal PWM
  ledcAttachPin(motorDownPulsePin, motorDownChannel);       // Associer le GPIO au canal PWM
  
  ledcWrite(motorUpChannel, 0);
  ledcWrite(motorDownChannel, 0);
  
  strobes.begin(
      outStrobe1Pin,
      outStrobe2Pin,
      config["strobeFreq/flowValue"].as<float>()
  );

	setupEthernet();
	setupWifi();
	setupOSC();
	setupWebServer();
	//setupArtnet();
	receivePin = 36;
	setupDMX();

  oscSubscribe("/strobeFreq",     [](const OscMessage& m) { if (m.size()>0) { setStrobeFreq(m.arg<float>(0)); } });
  oscSubscribe("/strobe1Dimmer",  [](const OscMessage& m) { if (m.size()>0) { setStrobe1Dimmer(m.arg<float>(0)*1023); } });
  oscSubscribe("/strobe2Dimmer",  [](const OscMessage& m) { if (m.size()>0) { setStrobe2Dimmer(m.arg<float>(0)*1023); } });
  oscSubscribe("/motorUpSpeed",   [](const OscMessage& m) { if (m.size()>0) { setMotorUpSpeed(m.arg<int>(0)); } });
  oscSubscribe("/motorDownSpeed", [](const OscMessage& m) { if (m.size()>0) { setMotorDownSpeed(m.arg<int>(0)); } });
  oscSubscribe("/saveReverseData", [](const OscMessage& m) { writeConfig("motor1/reverseValue", motorUpFreq); writeConfig("motor2/reverseValue", motorDownFreq); });
  oscSubscribe("/savePauseData", [](const OscMessage& m) {  writeConfig("motor1/pauseValue", motorUpFreq); writeConfig("motor2/pauseValue", motorDownFreq); });
  oscSubscribe("/saveFlowData", [](const OscMessage& m) { writeConfig("motor1/flowValue", motorUpFreq); writeConfig("motor2/flowValue", motorDownFreq); });
  oscSubscribe("/saveDropData", [](const OscMessage& m) { writeConfig("motor1/dropValue", motorUpFreq); writeConfig("motor2/dropValue", motorDownFreq); });
  oscSubscribe("/DMX1", [](const OscMessage& m) { setDimmer1DMX(m.arg<float>(0)); });
  oscSubscribe("/DMX2", [](const OscMessage& m) { setDimmer2DMX(m.arg<float>(0)); });
  oscSubscribe("/DMX3", [](const OscMessage& m) { setValuesDMX(m.arg<float>(0)); });
  oscSubscribe("/drop", [](const OscMessage& m) { pleaseDrop = true; });

}


void loop()
{
	vTaskDelay(5);
  if (pleaseDrop) {
    pleaseDrop = false;
    if (motorUpFreq == 0) {
      setMotorUpSpeed(2666);
      delay(100);
      setMotorUpSpeed(0);
    }
  }
}
