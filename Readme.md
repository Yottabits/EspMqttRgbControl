# EspMqttRgbControl

EspMqttRgbControl is a program for the ESP8266. It allows you to control a single RGB LED or a LED Strip if you connect a few MOSFETs to your ESP. The HSV color model is used to set the color and a fading effect is implemented.


To get startet you need to
  - have access or install your own MQTT Broker (eg Mosquitto)
  - install the Arduino IDE 1.6 or higher
  - add http://arduino.esp8266.com/stable/package_esp8266com_index.json in the IDE preference Windows
  - install the ESP8266 software in the IDEs device manager
  - install the PubSubClient library

Now you should replace the following parts in the Code:
    - WIFISSID to you WiFis SSID
    - WIFIPWD to you WiFis password
    - to your MQTT Brokers address
    - USER to the user name of your MQTT Broker. Leave blank if anonymous login is allowed
    - PASSWORD to the password of your MQTT Broker. Leave blank if anonymous login is allowed
    - /YOUR/TOPIC/PATH to topic path you want to use
    - YOURDEVICENAME to the device name you want to use

If you need to change the output pins, feel free to do so.

    
