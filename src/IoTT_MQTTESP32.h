/*
IoTT_MQTTESP32.h

MQTT interface to send and receive Loconet messages to and from an MQTT broker

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef IoTT_MQTTESP32_h
#define IoTT_MQTTESP32_h

#include <stdlib.h>
#include <arduino.h>
#include <Math.h>
#include <inttypes.h>
#include <WiFi.h>
#include <IoTTCommDef.h>
#include <ArduinoJson.h> //standard JSON library, can be installed in the Arduino IDE
#include <PubSubClient.h> //standard library, install using library manager


#define reconnectInterval 5000  //if not connected, try to reconnect every 5 Secs
#define queBufferSize 50 //messages that can be written in one burst before buffer overflow

class MQTTESP32 : public PubSubClient
{
public:
   MQTTESP32();
   ~MQTTESP32();
   MQTTESP32(Client& client);
   void processLoop();
   uint16_t lnWriteMsg(lnTransmitMsg txData);
   uint16_t lnWriteMsg(lnReceiveBuffer txData);
   void setNodeName(char * newName, bool newUseMAC = true);
   void setBCTopicName(char * newName);
   void setEchoTopicName(char * newName);
   void setPingTopicName(char * newName);
   void setPingFrequency(uint16_t pingSecs);
   bool connectToBroker();
   void setMQTTCallback(cbFct newCB);
   
  
private:
   // Member functions
   bool sendMQTTMessage(lnReceiveBuffer txData);
   bool sendPingMessage();

   static void psc_callback(char* topic, byte* payload, unsigned int length);


   // Member variables
   lnReceiveBuffer transmitQueue[queBufferSize];
   uint8_t que_rdPos, que_wrPos;
   lnReceiveBuffer lnInBuffer;
   
   char nodeName[50] = "IoTT-MQTT";	
   bool useMAC = true;

   uint8_t numWrite, numRead;
   
   uint32_t nextPingPoint;
   uint32_t pingDelay = 300000; //5 Mins
   uint32_t respTime;
   uint8_t  respOpCode;
   uint16_t respID;
   
   uint16_t lastReconnectAttempt = millis();
};

//this is the callback function. Provide a function of this name and parameter in your application and it will be called when a new message is received
extern void onMQTTMessage(lnReceiveBuffer * recData) __attribute__ ((weak));

#endif
