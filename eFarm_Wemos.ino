#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SimpleTimer.h>

//simple timer
SimpleTimer timer;

// disable mux for now...
//for multiple analog inputs
//#define MUX_A D7
//#define MUX_B D6
//#define MUX_C D5

//digital pins
#define PIN_WATER D2
#define PIN_RAIN D0

#define PIN_RED D10
#define PIN_GREEN D9
#define PIN_BLUE D8

#define PIN_PUMP_BUTTON D7

//analog pin
#define ANALOG_INPUT A0

//sea level pressure for altitude
#define SEALEVELPRESSURE_HPA (1013.25)

//bme sensor for temp, humidity and pressure
Adafruit_BME280 bme;

//settings
int deviceId;
int moistureMin;
int moistureMax;
int temperatureMin;
int temperatureMax;
bool waterPumpOn;

//sensors data
String water;
float moisture;
int rain;
float temperature;
float pressure;
float humidity;
float altitude;

//wifi and http
const char* ssid = "wifi_name";
const char* password = "password";
const String url = "http://192.168.0.101/api/device";
HTTPClient httpPost;
HTTPClient httpGet;
int i = 0;
    
//ssd1306 display
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void connectToWiFi(void) {
  int i = 0;
  resetDisplay();
  WiFi.begin(ssid, password);
  Serial.print("Wifi: Connecting...");
  display.println("WiFi: Connecting : ");
  display.println("");
  display.println("");
  lightGreen(200);
  display.display();
  while (WiFi.status() != WL_CONNECTED && i < 20) {
    i++;
    lightRed(1000);
    Serial.print(".");
    display.print(".");
    display.display();
  }
  Serial.println();
  return;
}

void updatePump(boolean newState) {
  httpPut.begin(url + "/settings?waterPump=" + (newState ? "true" : "false"));

  if (WiFi.status() == WL_CONNECTED) {      
      
      httpPut.addHeader("Authorization", "Basic AUTHORIZATION_TOKEN"); 

      int httpCode = httpPut.sendRequest("PUT", "");
      
      if (httpCode > 0) {
        if (httpCode >= 200 && httpCode < 300) {      
            Serial.println();  
            Serial.println("PUT request status: " + String(httpCode));
            Serial.println("OK");         

        } else if (httpCode >= 400 && httpCode <500) {
            Serial.println("PUT request status: " + String(httpCode));
            Serial.println("PUT: Client error");
        } else if (httpCode >= 500 && httpCode <600) {
            Serial.println("PUT request status: " + String(httpCode));
            Serial.println("PUT: Server error");
        } else {
            Serial.println("PUT request status: " + String(httpCode));
            Serial.println("PUT: Unkwnown");
        }       
      } else {
            Serial.println("PUT request status: " + String(httpCode));
            Serial.println("PUT: Unknown error");
      }
  } else {
      Serial.println("");
      Serial.println("[PUT] Wifi: Not connected");
      Serial.println("Trying again...");
      connectToWiFi();
  }
}

void readSettings(void) {

    httpGet.begin(url + "/settings");
    
    if (WiFi.status() == WL_CONNECTED) {
      
      httpGet.addHeader("Authorization", "Basic AUTHORIZATION_TOKEN");
      int httpCode = httpGet.GET();

      if (httpCode > 0) {
        if (httpCode >= 200 && httpCode < 300) {
            const size_t capacity = JSON_OBJECT_SIZE(7) + 90;
            DynamicJsonDocument doc(capacity);

            const String json = httpGet.getString();

            deserializeJson(doc, json);

            deviceId = doc["DeviceId"]; // 1
            moistureMin = doc["MoistureMin"]; // 0
            moistureMax = doc["MoistureMax"]; // 0
            temperatureMin = doc["TemperatureMin"]; // 0
            temperatureMax = doc["TemperatureMax"]; // 0
            waterPumpOn = doc["WaterPump"]; // true

            Serial.println("");
            Serial.println("Get request status: " + String(httpCode));
            
            Serial.println("DeviceID: " + String(deviceId));
            Serial.println("Moisture (Min-Max) %: " + String(moistureMin) + " - " + String(moistureMax));
            Serial.println("Temperature (Min-Max) *C: " + String(temperatureMin) + " - " + String(temperatureMax));
            Serial.println("Water pump status: " + String(waterPumpOn));  
                
        } else if (httpCode >= 400 && httpCode <500) {
            Serial.println("Get request status: " + String(httpCode));
            Serial.println("GET: Client error");
        } else if (httpCode >= 500 && httpCode <600) {
            Serial.println("Get request status: " + String(httpCode));
            Serial.println("GET: Server error");
        } else {
            Serial.println("Get request status: " + String(httpCode));
            Serial.println("GET: Unkwnown");
        }       
      } else {
            Serial.println("Get request status: " + String(httpCode));
            Serial.println("GET: Unknown error");
      }
  } else {
      Serial.println("");
      Serial.println("[GET] Wifi: Not connected");
      Serial.println("Trying again....");
      connectToWiFi();
  }
}

void postData(void) {
  //securing that initial empty data when device starts up won't be sent to the server
  if (moisture == 0 && temperature==0 && pressure==0 && altitude==0 && humidity==0 && rain==0) {
    return;
  }
  
  httpPost.begin(url + "/post?SoilMoisture=" +
                  String(moisture) + "&Rain=" + String(rain) + "&Water=" + water + 
                  "&Temperature=" + String(temperature) + 
                  "&Pressure=" + String(pressure) + 
                  "&Humidity=" + String(humidity) + "&Altitude=" + String(altitude));
  if (WiFi.status() == WL_CONNECTED) {      
      
      httpPost.addHeader("Authorization", "Basic AUTHORIZATION_TOKEN"); 

      int httpCode = httpPost.sendRequest("POST", "");
      
      if (httpCode > 0) {
        if (httpCode >= 200 && httpCode < 300) {      
            Serial.println();  
            Serial.println("Post request status: " + String(httpCode));
            Serial.println("OK");         

        } else if (httpCode >= 400 && httpCode <500) {
            Serial.println("Post request status: " + String(httpCode));
            Serial.println("Post: Client error");
        } else if (httpCode >= 500 && httpCode <600) {
            Serial.println("Post request status: " + String(httpCode));
            Serial.println("Post: Server error");
        } else {
            Serial.println("Post request status: " + String(httpCode));
            Serial.println("Post: Unkwnown");
        }       
      } else {
            Serial.println("Get request status: " + String(httpCode));
            Serial.println("Post: Unknown error");
      }
  } else {
      Serial.println("");
      Serial.println("[POST] Wifi: Not connected");
      Serial.println("Trying again...");
      connectToWiFi();
  }
}

void lightGreen(int seconds) {
      digitalWrite(PIN_RED, LOW);
      digitalWrite(PIN_GREEN, HIGH);
      digitalWrite(PIN_BLUE, LOW);
      delay(seconds);
      digitalWrite(PIN_RED, LOW);
      digitalWrite(PIN_GREEN, LOW);
      digitalWrite(PIN_BLUE, LOW);  
}
void lightRed(int seconds) {
      digitalWrite(PIN_RED, HIGH);
      digitalWrite(PIN_GREEN, LOW);
      digitalWrite(PIN_BLUE, LOW);
      delay(seconds);
      digitalWrite(PIN_RED, LOW);
      digitalWrite(PIN_GREEN, LOW);
      digitalWrite(PIN_BLUE, LOW);   
}

void checkPump() {
  if (waterPumpOn) {
      digitalWrite(PIN_RED, LOW);
      digitalWrite(PIN_GREEN, HIGH);
      digitalWrite(PIN_BLUE, LOW);
  } else {
      digitalWrite(PIN_RED, LOW);
      digitalWrite(PIN_GREEN, LOW);
      digitalWrite(PIN_BLUE, LOW);  
  }
}

void readWater(void) {
  int intWater = 1;
  intWater = digitalRead(PIN_WATER);
  water = intWater == 0 ? "true" : "false";
}

void readWeather() {
  temperature = bme.readTemperature();
  pressure = (bme.readPressure() / 100.0F);
  humidity = bme.readHumidity();
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
}

void readMoisture(void) {

  //disable for now
  //changeMux(LOW, LOW, LOW);
 
  int analogValue;
  float sum = 0;

 int n = 5;
  for (int i=0; i<n; i++){
    analogValue = analogRead(A0);
    sum += ( 100 - ( (analogValue/1024.00) * 100 ) );
    delay(150);
  }
  moisture=0;
  moisture = sum/n;
}

void readRain(void) {
  if(digitalRead(PIN_RAIN) == LOW) 
  {
    Serial.println("Rain sensor: Rain detected"); 
    rain = 1;
    delay(10); 
  }
    else
  {
    Serial.println("Rain sensor: No rain");
    rain = 0;
    delay(10); 
  }

}

void resetDisplay(void) {
  display.clearDisplay();       
  display.setCursor(0,0);             
}

void displayData(void) {
  resetDisplay();
  String strRain;
    switch (rain) {
     case 0: 
      strRain = "No rain";
      break;
     case 1:
      strRain = "Rain";
      break;
  }
  display.println("Temperature: " + String(temperature) + " *C ");
  display.println("Humidity: " + String(humidity) + " %");
  display.println("Pressure: " + String(pressure) + " hPa");
  display.println("Moisture: " + String(moisture) + " %");
  display.println("Rain: " + String(rain) + "-" + strRain);
  display.print("Water leakage: ");
  display.println(water == "true" ? "YES" : "No");
  display.print("Pump: ");
  display.print(waterPumpOn ? "ON" : "OFF");
  display.print(", Alt: " + String(int(altitude)) + " m");
  display.display();
}

void displayInformation(void) {
  resetDisplay();
  display.println("DeviceID: " + String(deviceId));
  display.println();
  if (moisture < moistureMin || moisture>moistureMax) {
    display.println("Moisture is critical!");
  } else {
    display.println("Moisture is ok!");
  }
  display.println();
  if (temperature < temperatureMin || temperature > temperatureMax) {
    display.println("Temperature is critical!");
  } else {
    display.println("Temperature is ok!");
  }
  display.display();
  delay(1000);
}

// code used from https://github.com/Mjrovai/ArduFarmBot-2/tree/master/ArduFarmBot2_Local_Automatic_Ctrl_V2
// debouncing prevents false readings
// very usefull stuff
// who would say
boolean debounce(int pin)
{
  boolean state;
  boolean previousState;
  const int debounceDelay = 30;
  
  previousState = digitalRead(pin);
  for(int counter=0; counter< debounceDelay; counter++)
  {
    delay(1);
    state = digitalRead(pin);
    if(state != previousState)
    {
      counter = 0;
      previousState = state;
    } 
  }
  return state;
}

void readPumpButton(void) {
  boolean pumpState = debounce(PIN_PUMP_BUTTON);
  if (pumpState) {
    if (waterPumpOn) {
      updatePump(false);
    } else {
      updatePump(true);
    }
    readSettings();
    checkPump();
    displayData();
  }
}

void setup() {
  
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  display.display(); //displays adafruit logo :)))
  display.clearDisplay();
  display.setTextSize(1);             
  display.setTextColor(SSD1306_WHITE);       
  display.setCursor(0,0); 

  delay(1000); 

  // disable for now...
  //pinMode(MUX_A, OUTPUT);
  //pinMode(MUX_B, OUTPUT);
  //pinMode(MUX_C, OUTPUT);
  
  pinMode(PIN_WATER, INPUT);
  pinMode(PIN_RAIN, INPUT);

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);

  pinMode(PIN_PUMP_BUTTON, INPUT_PULLUP);
  
  connectToWiFi();

  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.mode(WIFI_STA);

  timer.setInterval(1000, readSettings);
  timer.setInterval(1000, postData);
  timer.setInterval(1000, readWater);
  timer.setInterval(10000, readWeather);
  timer.setInterval(1000, checkPump);  
  timer.setInterval(1000, readMoisture);
  timer.setInterval(1000, readRain);
  timer.setInterval(1000, displayData);
  timer.setInterval(1000, readPumpButton);
  timer.setInterval(10000, displayInformation);
  
  
  bool bmeState = bme.begin(0x76);
  
  if (!bmeState) {
    Serial.println("Could not find BME280");
    while(1);
  }
  
  httpPost.setReuse(true);
  httpGet.setReuse(true);
  httpPut.setReuse(true);
}

//disable for now
//void changeMux(int c, int b, int a) {
//  digitalWrite(MUX_A, a);
//  digitalWrite(MUX_B, b);
//  digitalWrite(MUX_C, c);
//}

void loop() {  
  timer.run();
}
