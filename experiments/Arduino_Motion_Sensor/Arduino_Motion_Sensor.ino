#include <Ethernet.h>			          //for loading components required by the iot device object.
#include <PubSubClient.h>

#include <allthingstalk_arduino_standard_lib.h>  // SmartLiving for Makers Arduino Library
#include <SPI.h>                                  //required to have support for signed/unsigned long type.			

/*
  PIR Motion Sensor SmartLiving Makers Arduino Example
  version 1.0 09/10/2014
  
  ### Instructions

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - Grove PIR Motion sensor to D2 and grove LEd to D8
  2. Add 'iot_att' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, mac) and optionally change/add the sensor & actuator names, ids, descriptions, types
     For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Troubleshooting


*/


char deviceId[] = ""; // Your device id comes here
char clientId[] = ""; // Your client id comes here;
char clientKey[] = ""; // Your client key comes here;

ATTDevice Device(deviceId, clientId, clientKey);            //create the object that provides the connection to the cloud to manager the device.
char httpServer[] = "api.smartliving.io";                   // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";		    // MQTT Server IP Address


byte mac[] = {  0x90, 0xA2, 0xDA, 0x0D, 0x8D, 0x3D }; 	    // Adapt to your Arduino MAC Address  

int PIR_MOTION_SENSOR = 2;                                  // Define PIN number on shield & also used to construct Unique AssetID                      
int LED = 8;                                                // Define PIN number on shield & also used to construct Unique AssetID

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
         
  pinMode(PIR_MOTION_SENSOR, INPUT);
  pinMode(LED,OUTPUT);
  
  Serial.begin(9600);							         // init serial link for debugging
  
  if(Device.Connect(mac, httpServer))					         // connect the device with the IOT platform.
  {
    Device.AddAsset(PIR_MOTION_SENSOR, "PIR", "Motion Sensor", false, "bool");   // Create the asset for your device
    Device.AddAsset(LED, "LED", "Light Emitting Diode", true, "bool");           // Create the asset for your device
    Device.Subscribe(pubSub);						         // make certain that we can receive message from the iot platform (activate mqtt)
  }
  else 
    while(true);                                                                 //can't set up the device on the cloud, can't continue, so put the app in an ethernal loop so it doesn't do anything else anymore.								
}

bool Motion = false;							        
void loop()
{
  bool MotionRead = digitalRead(PIR_MOTION_SENSOR);			        // read status 
  if (Motion != MotionRead) 				                        // verify if value has changed
  {
    Motion = MotionRead;
    delay(100);                                                                 // eliminate bounce effect
    if (MotionRead == 1)
    {
       Device.Send("true", PIR_MOTION_SENSOR);
    }
    else
    {
       Device.Send("false", PIR_MOTION_SENSOR);
    }
  }
  Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  String msgString; 
  {                                                            //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
	char message_buff[length + 1];	                       //need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation.
	strncpy(message_buff, (char*)payload, length);         //copy over the data
	message_buff[length] = '\0';		               //make certain that it ends with a null			
		  
	msgString = String(message_buff);
	msgString.toLowerCase();			       //to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
  }
  int* idOut = NULL;
  {	                                                       //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
	int topicLength = strlen(topic);
	
	Serial.print("Payload: ");                             //show some debugging.
	Serial.println(msgString);
	Serial.print("topic: ");
	Serial.println(topic);
	
	if (topic[topicLength - 9] == LED+48)                  //warning: the topic will always be lowercase. The id of the actuator to use is near the end of the topic. We can only get actuator commands, so no extra check is required.
	{
	  if (msgString == "false") {
            digitalWrite(LED, LOW);		      
            idOut = &LED;		                        
	  }
	  else if (msgString == "true") {
	    digitalWrite(LED, HIGH);                   
            idOut = &LED;
	  }
	}
  }
  if(idOut != NULL)                                            //also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
    Device.Send(msgString, *idOut);    
}
