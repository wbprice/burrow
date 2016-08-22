#include <IRremoteESP8266.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

/*
 * Wifi Configuration
 */

const char* ssid     = "whiskey";
const char* password = "oreganojumpsuit";
const char* host = "burrow-server.herokuapp.com";

/*
 * Uses the IRremoteESP8266 library to interact with the Haier AC unit in my window.
 */

IRsend irsend(0); //an IR led is connected to GPIO pin 0

/*
 * Hex codes corresponding to different actions performed by the remote.
 */

const unsigned long POWER_BUTTON = 0x19F69867;
const unsigned long MODE_BUTTON = 0x19F610EF;
const unsigned long SPEED_BUTTON = 0x19F620DF;
const unsigned long TIMER_BUTTON = 0x19F658A7;
const unsigned long TEMP_UP_BUTTON = 0x19F6A05F;
const unsigned long TEMP_DOWN_BUTTON = 0x19F6906F;

void connectToWifi() {
  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // We start by connecting to a WiFi network

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

String fetchDesiredTemperature(String url) {

  String response;
  int bodyIndex;

    // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
  }

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
    }
  }

  while(client.available()) {
    response += client.readStringUntil('\r');
  }

  // Get the index of the start of the body 
  // (including space occupied by newline characters)
  bodyIndex = response.indexOf("\n\n") + 2;

  return response.substring(bodyIndex);
  
}

int getTemperatureFromResponse(String response) {

  StaticJsonBuffer<200> jsonBuffer;

  // Length (with one extra character for the null terminator)
  int str_len = response.length() + 1; 
  
  // Prepare the character array (the buffer) 
  char char_array[str_len];
  
  // Copy it over 
  response.toCharArray(char_array, str_len);

  JsonObject& body = jsonBuffer.parseObject(char_array);

  if (!body.success()) {
    Serial.println("parseObject() failed");
    return 0;
  }

  const int temperature = body["temperature"];

  return temperature;
     
}

void setup() {
  irsend.begin();
  Serial.begin(9600);
  connectToWifi();
}

int currentTemperature = 78;

void loop() {
  delay(5000);

  String url = "/api/v1/thermostat/1";
  int temperature = getTemperatureFromResponse(fetchDesiredTemperature(url));

  if (currentTemperature != temperature) {
    Serial.println("Temperature needs to be changed");

    if (currentTemperature > temperature) {
      while (currentTemperature > temperature) {
        irsend.sendNEC(TEMP_DOWN_BUTTON, 32);
        currentTemperature -= 1;
        delay(300);
      }
    }

    else if (temperature > currentTemperature) {
      Serial.println("Temperature needs to be raised");
      while (temperature > currentTemperature) {
        irsend.sendNEC(TEMP_UP_BUTTON, 32);
        currentTemperature += 1;
        delay(300);
      }
    }

    Serial.println("Done adjusting temperature to: ");
    Serial.println(currentTemperature);
    
  }

  else {
    Serial.println("No change is needed\n\n");
  }

}