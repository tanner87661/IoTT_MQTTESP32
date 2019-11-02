/*

MQTT Access library to send LocoNet commands to an MQTT broker and recieve tem
* 
* The library makes use of two topics to deal with LocoNet commands:
* 
* Broadcast command (default is lnIn) is used for regular message flow. A Loconet device within the MQTT network can send a message using the broadcast topic to send it
* to the gateway. The gateway will send it to LocoNet. 
* 
* Echo topic (default is nlEcho) is used by teh gateway to send confirmation that a received message has been sent to the gateway. This way any application can have positive confirmation that a 
* sent message was indeed sent to Loconet.

Concept and implementation by Hans Tanner. See https://youtu.be/e6NgUr4GQrk for more information
See Digitrax LocoNet PE documentation for more information

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

#include <IoTT_MQTTESP32.h>


#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())

char lnPingTopic[100] = "lnPing";  //ping topic, do not change. This is helpful to find Gateway IP Address if not known. 
char lnBCTopic[100] = "lnIn";  //default topic, can be specified in mqtt.cfg. Useful when sending messages from 2 different LocoNet networks
char lnEchoTopic[100] = "lnEcho"; //default topic, can be specified in mqtt.cfg

char thisNodeName[60] = ""; //default topic, can be specified in mqtt.cfg

cbFct mqttCallback = NULL;


MQTTESP32::MQTTESP32():PubSubClient()
{
	setCallback(psc_callback);
	nextPingPoint = millis() + pingDelay;
}

MQTTESP32::~MQTTESP32()
{
}

MQTTESP32::MQTTESP32(Client& client):PubSubClient(client)
{
	setCallback(psc_callback);
	nextPingPoint = millis() + pingDelay;
}

void MQTTESP32::psc_callback(char* topic, byte* payload, unsigned int length)
{
  DynamicJsonDocument doc(4 * length);
  DeserializationError error = deserializeJson(doc, payload);
  if (!error)
  {
	  if (doc.containsKey("From"))
	  {
			lnReceiveBuffer recData;
			String sentFrom = doc["From"];
//			Serial.println(sentFrom);
			if ((strcmp(topic, lnPingTopic) == 0) && (strcmp(sentFrom.c_str(), thisNodeName) != 0))
			{
				//this is a ping command from another node
			}
			if (strcmp(topic, lnBCTopic) == 0) //can be new message or echo from our own message
			{
				if (doc.containsKey("Data"))
				{
					recData.lnMsgSize = doc["Data"].size();
					for (int j=0; j < recData.lnMsgSize; j++)  
						recData.lnData[j] = doc["Data"][j];
					if (doc.containsKey("ReqID"))
						recData.reqID = (uint16_t) doc["ReqID"];
					else
						recData.reqID = 0;
					if (doc.containsKey("ReqRespTime"))
						recData.reqRespTime = doc["ReqRespTime"];
					else
						recData.reqRespTime = 0;
					if (doc.containsKey("ReqRecTime"))
						recData.reqRecTime = doc["ReqRecTime"];
					else
						recData.reqRecTime = 0;
					if (doc.containsKey("EchoTime"))
						recData.echoTime = doc["EchoTime"];
					else
						recData.echoTime = 0;
					recData.errorFlags = 0;
					if (strcmp(sentFrom.c_str(), thisNodeName) == 0)
					{
						recData.errorFlags |= msgEcho;
						recData.echoTime = micros() - recData.reqRecTime;
					}
					if (mqttCallback != NULL)
						mqttCallback(&recData);
					else
						if (onMQTTMessage) 
							onMQTTMessage(&recData);
				}
			}
	  }
  }
	
}

void MQTTESP32::setNodeName(char * newName, bool newUseMAC)
{
	strcpy(&nodeName[0], newName);
	useMAC = newUseMAC;
	strcpy(&thisNodeName[0], nodeName);
	if (useMAC)
	{
		String hlpStr = String(ESP_getChipId());
	    strcpy(&thisNodeName[strlen(thisNodeName)], hlpStr.c_str());
	}
}

void MQTTESP32::loadMQTTCfgJSON(DynamicJsonDocument doc)
{
	if (doc.containsKey("MQTTServer"))
		strcpy(&mqtt_server[0], doc["MQTTServer"]);
    if (doc.containsKey("MQTTPort"))
        mqtt_port = doc["MQTTPort"];
    if (doc.containsKey("MQTTUser"))
		strcpy(&mqtt_user[0], doc["MQTTUser"]);
    if (doc.containsKey("MQTTPassword"))
        strcpy(&mqtt_password[0], doc["MQTTPassword"]);
    if (doc.containsKey("NodeName"))
        strcpy(&nodeName[0], doc["NodeName"]);
    if (doc.containsKey("inclMAC"))
        includeMAC = doc["inclMAC"];
    if (doc.containsKey("pingDelay"))
        setPingFrequency(doc["pingDelay"]);
    if (doc.containsKey("BCTopic"))
        strcpy(&appBCTopic[0], doc["BCTopic"]);
    if (doc.containsKey("EchoTopic"))
        strcpy(&appEchoTopic[0], doc["EchoTopic"]);
    if (doc.containsKey("PingTopic"))
        strcpy(&appPingTopic[0], doc["PingTopic"]);

    setServer(mqtt_server, mqtt_port);
    setNodeName(nodeName);
}

void MQTTESP32::setMQTTCallback(cbFct newCB)
{
	mqttCallback = newCB;
}

void MQTTESP32::setBCTopicName(char * newName)
{
	strcpy(&lnBCTopic[0], newName);
}

void MQTTESP32::setEchoTopicName(char * newName)
{
	strcpy(&lnEchoTopic[0], newName);
}

void MQTTESP32::setPingTopicName(char * newName)
{
	strcpy(&lnPingTopic[0], newName);
}

bool MQTTESP32::connectToBroker()
{
	Serial.println(thisNodeName);
	if (connect(thisNodeName)) 
	{
		// ... and resubscribe
		subscribe(lnBCTopic);
		subscribe(lnPingTopic);
		subscribe(lnEchoTopic);
	}
	else
	{
      Serial.print("Connection failed, rc= ");
      Serial.println(state());
    }
	return connected();
}


uint16_t MQTTESP32::lnWriteMsg(lnTransmitMsg txData)
{
    uint8_t hlpQuePtr = (que_wrPos + 1) % queBufferSize;
    if (hlpQuePtr != que_rdPos) //override protection
    {
		transmitQueue[hlpQuePtr].lnMsgSize = txData.lnMsgSize;
		transmitQueue[hlpQuePtr].reqID = txData.reqID;
		transmitQueue[hlpQuePtr].reqRecTime = micros();
		memcpy(transmitQueue[hlpQuePtr].lnData, txData.lnData, txData.lnMsgSize);
		transmitQueue[hlpQuePtr].reqRespTime = 0;
		transmitQueue[hlpQuePtr].echoTime = 0;
		transmitQueue[hlpQuePtr].errorFlags = 0;
		que_wrPos = hlpQuePtr;
		return txData.lnMsgSize;
	}
	else
	{	
		Serial.println("MQTT Write Error. Too many messages in queue");
		return -1;
	}
}

uint16_t MQTTESP32::lnWriteMsg(lnReceiveBuffer txData)
{
    uint8_t hlpQuePtr = (que_wrPos + 1) % queBufferSize;
    if (hlpQuePtr != que_rdPos) //override protection
    {
		transmitQueue[hlpQuePtr].lnMsgSize = txData.lnMsgSize;
		transmitQueue[hlpQuePtr].reqID = txData.reqID;
		transmitQueue[hlpQuePtr].reqRecTime = micros();
		memcpy(transmitQueue[hlpQuePtr].lnData, txData.lnData, txData.lnMsgSize);
		transmitQueue[hlpQuePtr].reqRespTime = txData.reqRespTime;
		transmitQueue[hlpQuePtr].echoTime = txData.echoTime;
		transmitQueue[hlpQuePtr].errorFlags = txData.errorFlags;
		que_wrPos = hlpQuePtr;
		return txData.lnMsgSize;
	}
	else
	{	
		Serial.println("MQTT Write Error. Too many messages in queue");
		return -1;
	}
}

void MQTTESP32::setPingFrequency(uint16_t pingSecs)
{
	pingDelay = 1000 * pingSecs;
	nextPingPoint = millis() + pingDelay;
}

bool MQTTESP32::sendPingMessage()
{
    DynamicJsonDocument doc(1200);
    char myMqttMsg[200];
    String hlpStr = thisNodeName;
    doc["From"] = hlpStr; //NetBIOSName + "-" + ESP_getChipId();
	doc["IP"] = WiFi.localIP().toString();
	long rssi = WiFi.RSSI();
	doc["SigStrength"] = rssi;
	doc["Mem"] = ESP.getFreeHeap();
	doc["Uptime"] = round(millis()/1000);
    serializeJson(doc, myMqttMsg);
    if (connected())
    {
      if (!publish(lnPingTopic, myMqttMsg)) 
      {
//        	Serial.println(F("Ping Failed"));
			return false;
      } else 
      {
//        	Serial.println(F("Ping OK!"));
			return true;
      }
    }
    else
		return false;
}

bool MQTTESP32::sendMQTTMessage(lnReceiveBuffer txData)
{
    DynamicJsonDocument doc(1200);
    char myMqttMsg[400];
    String jsonOut = "";
    String hlpStr = thisNodeName;
    doc["From"] = hlpStr; //NetBIOSName + "-" + ESP_getChipId();
    doc["ReqRecTime"] = txData.reqRecTime;
    doc["ReqRespTime"] = txData.reqRespTime;
    doc["EchoTime"] = txData.echoTime;
    doc["ReqID"] = txData.reqID;
    doc["ErrorFlags"] = txData.errorFlags;
    doc["Valid"] = 1; //legacy data, do not use in new designs
    JsonArray data = doc.createNestedArray("Data");
    for (byte i=0; i < txData.lnMsgSize; i++)
      data.add(txData.lnData[i]);
    serializeJson(doc, myMqttMsg);
    if (connected())
    {
      if ((txData.errorFlags & msgEcho) > 0)  //send echo message if echo flag is set 
        if (!publish(lnEchoTopic, myMqttMsg))
        {
          return false;
        } else 
        {
			return true;
        }
      else  //otherwise send BC message (in direct mode, meaning the command came in via lnOutTopic)
        if (!publish(lnBCTopic, myMqttMsg))
        {
          return true;
        } else 
        {
			return true;
        }
    }
	return true;
}
void MQTTESP32::processLoop()
{
    if (!connected()) 
    {
      long now = millis();
      if (now - lastReconnectAttempt > reconnectInterval) 
      {
        lastReconnectAttempt = now;
        // Attempt to reconnect
        Serial.println("Reconnect MQTT Broker");
        if (connectToBroker()) 
        {
          lastReconnectAttempt = 0;
          Serial.println("Success");
        }
        else
          Serial.println("Failure");
      }
    } 
    else
    {
      // Client connected
		loop();
		if (que_wrPos != que_rdPos)
		{
			int hlpQuePtr = (que_rdPos + 1) % queBufferSize;
			if (sendMQTTMessage(transmitQueue[hlpQuePtr]))
				que_rdPos = hlpQuePtr; //if not successful, we keep trying
		}
		if (pingDelay > 0)
			if (millis() > nextPingPoint)
			{
				if (sendPingMessage())
					nextPingPoint += (pingDelay);
			}
	}
	yield();
}

