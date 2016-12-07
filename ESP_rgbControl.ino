#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>


//WiFi credentials of your network
#define wifi_ssid "WIFISSID"
#define wifi_pwd "WIFIPWD"

//Credentials of your MQTT Broker
#define mqtt_srv "BROKER"
#define mqtt_usr "USER"
#define mqtt_pwd "PASSWORD"

//Set topics
#define topic_will "/YOUR/TOPIC/PATH/will"
#define topic_hue "/YOUR/TOPIC/PATH/hue"
#define topic_brightness "/YOUR/TOPIC/PATH/brightness"
#define topic_saturation "/YOUR/TOPIC/PATH/saturation"

//Output pins
#define pinRChannel 14
#define pinGChannel 12
#define pinBChannel 13


//Hue [0, 1]
double hue = 0;
//Saturation [0, 1]
double saturation = 1;
//Brightness [0, 1]
double brightness = 0;

WiFiClient espClient;
PubSubClient client(espClient);

//something change is true when... something changed... obviously
//But serious: somethingChanged is set when new data arrives via MQTT
bool somethingChanged = true;
bool fadeFinished = false;

//rgb is the final color set that is directly calculated when new data arrives
int rgb[3] = {0, 0, 0};
//oldRGB is used to generate a fade from the previous values to the new rgb values
int oldRgb[3] = {0, 0, 0};
//ledRGB is the current set of values that are written to the leds
int ledRgb[3] = {0,0,0};

//changeCounter is the counter used for the fading effect between two sets of data
byte chaneCounter = 0;

void setup()
{
	//Debugging
	Serial.begin(9600);
	
	//Set pinModes
	pinMode(pinRChannel, OUTPUT);
	pinMode(pinGChannel, OUTPUT);
	pinMode(pinBChannel, OUTPUT);
	//Show red while setup and connecting to WiFi and Broker
	analogWrite(pinRChannel, 1023);
	
	
	setup_wifi();
	client.setServer(mqtt_srv, 1883);
	client.setCallback(callback);
}

void loop()
{
	if(!client.connected()){
		reconnect();
	}
	client.loop();
	
	if(somethingChanged){
		hsvToRgb(hue, saturation, brightness, rgb);
		somethingChanged = false;
		//writeToChannels();
	}
	fade();
}


//Connect to WiFi and show debugging message
void setup_wifi(){
	delay(10);
	Serial.print("Connecting...");
	WiFi.begin(wifi_ssid, wifi_pwd);
	while(WiFi.status() != WL_CONNECTED){
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("Connected");
}

//Connect to MQTT Broker and subscribe to topics
void reconnect(){
	while (!client.connected()){
		Serial.print("Connect to MQTT");
		
		//Try to connect to MQTT
		if(client.connect("YOURDEVICENAME", "", "", topic_will, 0, 0, "false")){
			Serial.println("Connected");
			//Set will to true to show that the device is available
			client.publish(topic_will, "true");
			
			//Subscribe to topics
			client.subscribe(topic_brightness);
			client.subscribe(topic_hue);
			client.subscribe(topic_saturation);
			
			//Show one second green to show a succesful setup
			analogWrite(pinRChannel, 0);
			analogWrite(pinGChannel, 1023);
			//Used for fading to black after the green led
			oldRgb[1] = 1023;
			delay(1000);
		}
		else{
			Serial.println("Fail");
			delay(5000);
		}
	}
}


//Callback when a message arrives via MQTT
void callback(char* topic, byte* payload, unsigned int length) {
	//If a topic is at least part of the changed topic, do stuff
	//There were several complications with the amount of backslashes in the topics, thats why
	if(strstr(topic, topic_brightness) != NULL){
		brightness = payloadToFloat(payload, length);
	}
	if(strstr(topic, topic_hue) != NULL){
		hue = payloadToFloat(payload, length);
	}
	if(strstr(topic, topic_saturation) != NULL){
		saturation =  payloadToFloat(payload, length);
	}
	
	somethingChanged = true;
	
	//If new values arrived while a fade is in progress,
	//set the current output as the new beginning for a fade and reset the fading counter
	if(!fadeFinished){
		oldRgb[0] = ledRgb[0];
		oldRgb[1] = ledRgb[1];
		oldRgb[2] = ledRgb[2];
		chaneCounter = 0;
	}
	fadeFinished = false;

}

//Please don't ask
//Found this on the web somewhere
//It's using ASCII conversion... And i don't like this...
//If you have a better solution to convert a byte array into a float 
//please! Tell me!
double payloadToFloat(byte * payload, unsigned int length){
	float f = 0.0;
	int x = 0;
	int index = -1;
	for (int i = 0; i < length; i++) {
		if (payload[i] >= 48 && payload[i] <= 57) {
			x = x * 10 + (int)(payload[i] - 48);
			} else if (payload[i] == 46) {
			f += x;
			x = 0;
			index = i;
		}
	}
	if (index != -1) {	
		int p = 1;
		for (int i = 0; i < length - index - 1; i++) {
			p = p * 10;
		}
		f += (x / (float)p);
	}
	return f;
}

//Timer used to run a part of the fading every 10 milliseconds
unsigned long fadeTimer = 0;
void fade(){
	//
	if(millis() - fadeTimer > 10 && !fadeFinished){
		fadeTimer = millis();
		ledRgb[0] = oldRgb[0] + ((rgb[0]-oldRgb[0])/50.0f)*chaneCounter;
		ledRgb[1] = oldRgb[1] + ((rgb[1]-oldRgb[1])/50.0f)*chaneCounter;
		ledRgb[2] = oldRgb[2] + ((rgb[2]-oldRgb[2])/50.0f)*chaneCounter;
		
		analogWrite(pinRChannel, ledRgb[0]);
		analogWrite(pinGChannel, ledRgb[1]);
		analogWrite(pinBChannel, ledRgb[2]);
		
		chaneCounter++;
	}
	
	//When the fading finished, write the final values to the output and refresh oldRgb
	if(chaneCounter == 50 ){
		analogWrite(pinRChannel, rgb[0]);
		analogWrite(pinGChannel, rgb[1]);
		analogWrite(pinBChannel, rgb[2]);
		oldRgb[0] = rgb[0];
		oldRgb[1] = rgb[1];
		oldRgb[2] = rgb[2];
		
		//Reset counter and Finished flag
		fadeFinished = true;
		chaneCounter = 0;
	}
}

//In original from ratkins/RGBConverter 
//Only a manipulated version of the HsvToRgb function was needed
void hsvToRgb(double h, double s, double v, int rgb[]) {
	double r, g, b;

	int i = int(h * 6);
	double f = h * 6 - i;
	double p = v * (1 - s);
	double q = v * (1 - f * s);
	double t = v * (1 - (1 - f) * s);

	switch(i % 6){
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
	}

	rgb[0] = r * 1023;
	rgb[1] = g * 1023;
	rgb[2] = b * 1023;
}