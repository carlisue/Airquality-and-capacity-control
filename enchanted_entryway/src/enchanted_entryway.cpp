/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/15059/Documents/IoT/Airquality-and-capacity-control/enchanted_entryway/src/enchanted_entryway.ino"
/*
 * Project enchanted_entryway
 * Description: Capstone Project- 
 * Author: Carli Stringfellow
 * Date: 05-12-2021
 */


#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT.h" 
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h" 
#include "Adafruit_MQTT/Adafruit_MQTT.h" 
#include "Air_Quality_Sensor.h"
// #include "Adafruit_BME280.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include "Math.h" 
#include "credentials.h"
#include "neopixel.h"

/************ Global State (you don't need to change this!) ***   ***************/ 
// SYSTEM_MODE(SEMI_AUTOMATIC); //You need to comment out to get real time
void setup();
void loop ();
bool laserRead(int pin);
void laserState ();
void airQualityCheck();
void waterPump();
void cloudData();
void dataDisplay();
float MGRead(int mg_pin);
int  MGGetPercentage(float volts, float *pcurve);
void carbonSensor ();
void neoPixel();
void MQTT_connect();
#line 23 "c:/Users/15059/Documents/IoT/Airquality-and-capacity-control/enchanted_entryway/src/enchanted_entryway.ino"
TCPClient TheClient; 

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details. 
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 

/****************************** Feeds ***************************************/ 
// Setup Feeds to publish or subscribe 
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname> 
Adafruit_MQTT_Subscribe waterButtonFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/water-the-plants");
Adafruit_MQTT_Publish moistureFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/soilmoisturelevel");
// Adafruit_MQTT_Publish temperatureFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/roomtemperature");
// Adafruit_MQTT_Publish humidityFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/roomhumidity");
Adafruit_MQTT_Publish airQualityFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/indoorairquality");
Adafruit_MQTT_Publish Mobile_CO2V = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/CO2V");
Adafruit_MQTT_Publish Mobile_CO2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/CO2");
Adafruit_MQTT_Publish currentPeople = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/peopleCount");
// /************************Hardware Related Macros************************************/
const int         MG_PIN       =             (A1);     //define which analog input channel you are going to use
const int         BOOL_PIN     =             (13);
const int         DC_GAIN      =             (8.5);  //define the DC gain of amplifier
// /***********************Software Related Macros************************************/
#define         READ_SAMPLE_INTERVAL         (5)    //define how many samples you are going to take in normal operation
#define         READ_SAMPLE_TIMES            (5)     //define the time interval(in milisecond) between each samples in
                                                     //normal operation
// /**********************Application Related Macros**********************************/
// //These two values differ from sensor to sensor. user should derermine this value.
#define         ZERO_POINT_VOLTAGE           (0.284) //define the output of the sensor in volts when the concentration of CO2 is 400PPM
#define         REACTION_VOLTGAE             (0.030) //define the voltage drop of the sensor when move the sensor from air into 1000ppm CO2
// /*****************************Globals***********************************************/
float           CO2Curve[3]  =  {2.602,ZERO_POINT_VOLTAGE,(REACTION_VOLTGAE/(2.602-3))};
//                                                      //two points are taken from the curve.
//                                                      //with these two points, a line is formed which is
//                                                      //"approximately equivalent" to the original curve.
//                                                      //data format:{ x, y, slope}; point1: (lg400, 0.324), point2: (lg4000, 0.280)
//                                                      //slope = ( reaction voltage ) / (log400 –log1000)

// Neopixel declarations
const int PIXELPIN = D6;
const int PIXELCOUNT = 12;
int i;
const int PIXEL_TYPE = WS2812B;
Adafruit_NeoPixel pixel(PIXELCOUNT, PIXELPIN, PIXEL_TYPE);

unsigned long last;
unsigned long lastTime;
bool waterValue;

//Timestamp declarations
String DateTime;
String TimeOnly;
char currentDateTime[25], currentTime[9];

const int OLED_RESET = D4; 
Adafruit_SSD1306 display(OLED_RESET);
int rot = 2;

// Air pollution sensor declarations.
AirQualitySensor airSensor(A0);
unsigned long lastTimeAir;

// Laser/photoresistor declarations.
bool laserOne, laserTwo;
const int LASERONEPIN = A2;
const int LASERTWOPIN = A3;
int counter;
int _laserState = 5;

//Capacitive soil moisture sensor v1.2 declarations.
const int SOILPIN = A4;
int soilMoisture;

//Water pump declarations.
const int WATERPIN = D3;
int pumpTimer;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  pixel.begin();
  pixel.show();
  pixel.setBrightness(255);

  pinMode(LASERONEPIN, INPUT); // Pin out for photoresistor 1
  pinMode(LASERTWOPIN, INPUT); // Pin out for photoresistor 2
  pinMode(WATERPIN, OUTPUT); // Pin out for Relay for water pump
  pinMode(SOILPIN,INPUT); // Pin out for soil sensor
  pinMode(A0, INPUT);    //air pin


  

  //  SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.display ();
  delay(300);
  display.clearDisplay(); 
  display.setRotation(rot); 

  // Sets the timezone to mountain daylight time
  Time.zone(-6);
  Particle.syncTime();

  // Serial.printf("Connecting to Internet \n");
  WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf(".");
    delay(100);
  }
  // Serial.printf("\n Connected!!!!!! \n");

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(& waterButtonFeed);

    if (airSensor.init()) {
      // Serial.println("Air Sensor ready.");
    }
    else {
      // Serial.println("Air Sensor ERROR!");
    }
} 


void loop () {
  laserState();
  MQTT_connect();
  // waterPump();   //called from cloudData function.
  
  airQualityCheck();
  cloudData();

  carbonSensor();
  neoPixel();
  dataDisplay();
  
}


bool laserRead(int pin) {
  int laserVal = analogRead(pin);
  // Serial.printf("Laser Value is: %i \n", laserVal);
  if (laserVal < 75) {
    return true;
  }
  else {
    return false; 
  }
}

void laserState () {
  laserOne = laserRead(LASERONEPIN);
  laserTwo = laserRead(LASERTWOPIN);

  if (counter < 0) {
    counter = 0;
  }

  Serial.printf("LaserState starting is: %i\n", _laserState);
  switch (_laserState) {
    case 1:
      if (laserOne && !laserTwo) {
        _laserState = 2;
        counter++;
        Serial.printf("LaserState 1++ is: %i\n", _laserState);
      }
      if (!laserOne && laserTwo) {
        _laserState = 4;
        counter--;
        Serial.printf("LaserState 1-- is: %i\n", _laserState);
      }
    break;

    case 2:
      if (!laserOne && !laserTwo) {
        _laserState = 3;
        Serial.printf("LaserState 2 is: %i\n", _laserState);
      }
    break;

    case 3:
      if (!laserOne && laserTwo) {
        _laserState = 1;
       Serial.printf("LaserState 3 is: %i\n", _laserState);
      }
    break;

    case 4:
      if(!laserOne && !laserTwo) {
        _laserState = 5;
        Serial.printf("LaserState 4 is: %i\n", _laserState);
      }
    break;

    case 5:
      if(laserOne && !laserTwo) { 
        _laserState = 1;
        Serial.printf("LaserState 5 is: %i\n", _laserState);
      }
  }
  Serial.printf("People Count: %i\n", counter); // current amount of people in room.
}

// // Checks air quality and displays whether it is high or low
void airQualityCheck() {
  int quality = airSensor.slope();
  display.display();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw WHITE text
  display.setCursor(29,56);             // Start at bootom-left corner

  Serial.print("Air Sensor value: ");
  Serial.printf("%i\n", airSensor.getValue());
  if((millis()-lastTimeAir > 5000)) {
    if (quality == AirQualitySensor::FORCE_SIGNAL) {
      Serial.printf("High pollution! Force signal active.\n");
      display.printf("High Pollution!");
    }
    else if (quality == AirQualitySensor::HIGH_POLLUTION) {
      Serial.printf("High pollution!\n");
      display.printf("High Pollution!");
    }
    else if (quality == AirQualitySensor::LOW_POLLUTION) {
      Serial.printf("Low pollution!\n");
      display.printf("Low Pollution!");
    }
    else if (quality == AirQualitySensor::FRESH_AIR) {
      Serial.printf("Fresh air.\n");
      display.printf("Fresh Air!");
    }
    display.display();
    lastTimeAir = millis();
  }
}

void waterPump() {
  soilMoisture = analogRead(SOILPIN);
  Serial.printf("moisture=%i\n", soilMoisture);
  if(soilMoisture > 3000) {
    if ((millis()-pumpTimer)>15000) {
      digitalWrite(WATERPIN,HIGH);
      // Serial.printf("Pump is ON \n");
      delay(3000);
      digitalWrite(WATERPIN,LOW);
      // Serial.printf("Pump is OFF \n");
      pumpTimer = millis();
    }
  }
}

void cloudData() {
   // Ping MQTT Broker every 2 minutes to keep connection alive
  if ((millis()-last)>120000) {
      // Serial.printf("Pinging MQTT \n");
      if(! mqtt.ping()) {
        // Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      last = millis();
  }
  // This is our 'wait for incoming subscription packets' busy subloop
  // Adafruit_MQTT_Subscribe *subscription;
  // while ((subscription = mqtt.readSubscription(1000))) {
  //   if (subscription == &waterButtonFeed) {
  //     waterValue = atoi((char *)waterButtonFeed.lastread);
  //     Serial.printf("Water Value %i \n", waterValue);
  //     if (waterValue == 1) {           //****MANUAL WATER PUMP****
  //       digitalWrite(WATERPIN,HIGH);
  //       Serial.printf("Pump is ON \n");
  //       delay(3000);
  //       digitalWrite(WATERPIN,LOW);
  //     }
  //   }
  // }

  if((millis()-lastTime > 12000)) { // PUBLISHES TO CLOUD
    if(mqtt.Update()) {
      moistureFeed.publish(soilMoisture);
      Serial.printf("Publishing Moisture %i\n", soilMoisture);
      
      airQualityFeed.publish(airSensor.getValue());
      Serial.printf("Publishing Air %i\n", airSensor.getValue() );

      currentPeople.publish(counter);
      Serial.printf("Current Count is: %i \n", counter);

      } 
    lastTime = millis();
  }
  waterPump();
}

void dataDisplay() {
  DateTime = Time.timeStr();
  TimeOnly = DateTime.substring(11,19);

  // Converts String to char arrays 
  DateTime.toCharArray(currentDateTime, 25);
  TimeOnly.toCharArray(currentTime,9);
  display.display();
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw WHITE text
  display.setCursor(0,0);             // Start at top-left corner

  display.printf("%s\n", currentTime);  // Displays current MDT
  display.printf("\n");    
  
  display.printf("People In Space: %i \n", counter); // Displays current amount of people in room.
  display.display();
}

float MGRead(int mg_pin) {
    int i;
    float v=0;
    for (i=0;i<READ_SAMPLE_TIMES;i++) {
        v += analogRead(mg_pin);
        delay(READ_SAMPLE_INTERVAL);
    }
    v = (v/READ_SAMPLE_TIMES) *5.0/4096 ;
    return v;
}

// /*****************************  MQGetPercentage **********************************
// Input:   volts   - SEN-000007 output measured in volts
//          pcurve  - pointer to the curve of the target gas
// Output:  ppm of the target gas
// Remarks: By using the slope and a point of the line. The x(logarithmic value of ppm)
//          of the line could be derived if y(MG-811 output) is provided. As it is a
//          logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic
//          value.
// ************************************************************************************/
int  MGGetPercentage(float volts, float *pcurve) {
   if ((volts/DC_GAIN )>=ZERO_POINT_VOLTAGE) {
      return -1;
    } 
   else {
      return pow(10, ((volts/DC_GAIN)-pcurve[1])/pcurve[2]+pcurve[0]);
    }
}
void carbonSensor () {
  int percentage;
  static int pastTime;
    float volts;
    volts = MGRead(MG_PIN);
    // Serial.printf( "SEN0159: %0.2fV \n",volts);
    percentage = MGGetPercentage(volts,CO2Curve);
    // Serial.print("CO2:");
    if((millis()-pastTime > 12000)) {
      if (percentage == -1) {
          Serial.printf("CO2: <400 ppm \n" );
      } else {
          Serial.printf("CO2 %d ppm \n",percentage);
      }
      if(mqtt.Update()) {
        Mobile_CO2.publish(percentage);  
      }
      pastTime = millis();
    }
}

 void neoPixel() {
   int capacity = 5; ////// CHANGE CAPACITY HERE! //////
    if(counter >= capacity) {
    //  pixel.clear();
      for(i=0; i<12; i++) {
        pixel.setPixelColor(i, 0xFF0000); 
      }
      pixel.show();
    }
    else {
      pixel.clear();
        for(i=0; i<12; i++) {
          pixel.setPixelColor(i, 0x00FF00);
        }
        pixel.show();
    }
 }

void MQTT_connect() {
  int8_t ret;
 
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
 
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}
