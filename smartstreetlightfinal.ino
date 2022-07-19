/**************SMART STREET LIGHT**************
 * 
 *                                                    .................This project is made by PRIYA NATH.
 * 
 * This is a Smart Street Light which can automatically turn on and off and go from low power mode to high 
 * power mode automatically. The motivation for this project is that most street lights waste power all night 
 * even if there is no traffic present but with this Smart Street Light the power wastage is really low and 
 * we can cutomize it in the code.
 * 
 * ### Features ###
 * 1. Automatic on-off according to environment light(automatically turns on when it gets dark).
 * 2. Automatic power consumtion mode (when object is detected it will go to maximum power mode and 
 *      automatically comes out of it).
 * 3. customizable color and color changing effect.
 * 4. Integrated emergency stop light (pressing the button to cross the street).
 * 5. Integrated USB charging for mobile devices.
 * 6. Automatic charging with solar panels.
 * 
 * **********************************************************************************************************
 * You need to change the following values before use
 * 1. IO_USERNAME
 * 2. IO_KEY
 * 3. WIFI_SSID
 * 4. WIFI_PASS
 * CONFIGURE THE SAME IN ADAFRUIT IO DASHBOARD
 * 
 *                                                                                  .................Sep-2021
 */
#include <FastLED.h>                    //FastLED library for ws2812b LEDs

#include "AdafruitIO_WiFi.h"            //Aafruit IO dasboard libraray for posting and getting data

#define LED_PIN     14                  //LED pin for ws2812b LEDs
#define NUM_LEDS    16                  //Total number of LEDs in all the lamps 
#define BRIGHTNESS  255                 //brightness of FastLED
#define LED_TYPE    WS2812B             //LED type
#define COLOR_ORDER GRB                 //color oredr of LED diode
#define LED         4                   //Total number of Lamps
#define NUM_LAMPS   NUM_LEDS/LED        //Total number of LEDs in one Lamp

#define tmr         5000                //Timer in ms after it will go to Low power mode

#define IRSen       5                   //IR sensor pin for object detection

#define LDRPin      16                  //LDR pin for auto on-off

#define SWPin       12                  //Switch pin as emergency crossing light
#define wait        15000               //Wait for ms after the light turns green

#define IO_USERNAME  "Priya21"          //Aadafruit IO username (can be found in the website after login)
#define IO_KEY       "aio_mhLW15jgVmT1ck0wZtOmEsZzjmUK"  //Adafruit IO key (can be found in the website after login)
#define WIFI_SSID   "Airtel"            //Your WiFi SSID
#define WIFI_PASS   "mamata@123456"     //Your WiFi password

CRGB leds[NUM_LEDS];                    //Initializing array of LEDs

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS); //configure WiFi and Adafruit IO credentials
AdafruitIO_Feed *lmph[LED] = {io.feed("lamp1h"), io.feed("lamp2h"), io.feed("lamp3h"), io.feed("lamp4h")}; //slider H value names in Adafruit IO
AdafruitIO_Feed *lmps[LED] = {io.feed("lamp1s"), io.feed("lamp2s"), io.feed("lamp3s"), io.feed("lamp4s")}; //slider S value names in Adafruit IO
AdafruitIO_Feed *toggle = io.feed("toggle");  //Toggle button name in Adafruit IO
AdafruitIO_Feed *modes = io.feed("mode");     //Mode button name in Adafruit IO

int H[LED] = {0, 0, 0, 0};              //Initializing H values
int S[LED] = {0, 0, 0, 0};              //Initializing S values
int hsv[LED] = {0, 0, 0, 0};            //Initializing H values for multicolor mode

int t = 0;                              //timer value for OnOff timer
bool prevst = 0;                        //previous state of On/Off

int low = 100;                          //low value when in power saving mode (0 to 255)
int high = 255;                         //high value when in high power mode (0 to 255)
int curv = 255;                         //current brightness of Lamps

bool LDRst = 1;                         //LDR state when it is day the value is 1 otherwise 0

bool OnOff = 1;                         //OnOff switch value

bool mono = 1;                          //mono mode (1) or multicolor rainbow mode (0)

void setup() {                          //setup of program
    WiFi.setOutputPower(0);             //set wifi power to lowest to avoid crosstalking (0 - 20.5)
    
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);  //Fastled initialization
    FastLED.setBrightness(  BRIGHTNESS ); //Set brightness of LEDs

    pinMode(IRSen, INPUT);              //Pinmode of IR sensor
    
    pinMode(LDRPin, INPUT);             //Pinmode of LDR pin

    pinMode(SWPin, INPUT);              //Pinmode of switch pin

    Serial.begin(115200);               //initialize serial monitor

    io.connect();                       //connect to Adafruit IO
    for(int i=0; i<LED; i++)            //loop to get message values for H and S values of each Lamp
    {
      lmph[i]->onMessage(handleMessage);  //handle message to extract values from message
      lmps[i]->onMessage(handleMessage);
    }
    toggle->onMessage(handleMessage);   //handle message of toogle switch
    modes->onMessage(handleMessage);    //handle message of modes switch 
    while(io.status() < AIO_CONNECTED) { //Till esp8266 dont connect to Adafruit IO
      Serial.print(".");
      delay(500);
    }
    Serial.println();
    Serial.println(io.statusText());    //Status after connecting to Adafruit IO
    for(int i=0;i<LED;i++)              //get latest values of slider from Adafruit IO
    {
      lmph[i]->get();
      lmps[i]->get();
    }
    toggle->get();                      //get updated value of toggle switch
    modes->get();                       //get updated value of modes switch
    
    rainbow();                          //flash rainbow
    delay(2000);                        //wait for 2 seconds
}

void handleMessage(AdafruitIO_Data *data) { //to retrieve data from received message from Adafruit IO

  int val = data->toInt();              //convert received data to int
  String nam = data->feedName();        //get name of variable
  String tmp;                           //to convert variable index
  int l = nam.length();                 //length of string name
  int k = 0;                            //for storing index
  if(nam.charAt(l-1) == 'h')            //received H value
  {
    tmp = nam.substring(4);
    k = tmp.toInt();
    H[k-1] = val;                       //save the received data
    Serial.print("h ");
    Serial.print(k);
    Serial.print(" ");
  }
  else if(nam.charAt(l-1) == 's')       //received S value
  {
    tmp = nam.substring(4);
    k = tmp.toInt();
    S[k-1] = val;
    Serial.print("s ");
    Serial.print(k);
    Serial.print(" ");
  }
  else if(nam == "mode")                //mode value received
  {
    mono = val;                         //update mono value
    if(mono == 0)                       //if mono is 0 then initialize rainbow colors
    {
      rainbow();                        //initilize rainbow colors
    }
  }
  else if(nam == "toggle")              //toggle value received
  {
    OnOff = val;                        //update onoff value
  }
  LEDs(k-1, H[k-1], S[k-1], curv);      //to fluctuate the change
  Serial.println(val);
}

void multicolor(){                      //for rainbow color chase
  for(int j=0;j<LED;j++)                //cycle through every lamp
  {
    if(hsv[j] > 255)                    //if hsv is greater than 255 reset it
    {
      hsv[j] = 0;
    }
    LEDs(j, hsv[j]++, 255, 255);        //update to LEDs
  }
}

void rainbow(){                         //for initializing rainbow colors
  for(int i=0;i<LED;i++)                //cycle through every lamp
    {
      hsv[i] = 255*i/LED;               //set divided points of value 0 to 255 in 4 points
      LEDs(i, hsv[i], 255, 255);
    }
}

void LEDs(int led, int h, int s, int v){  //for updating each lamps values (lamp_no, H_value, S_value, v_value)
  for (int i=led*LED;i<(led*LED)+LED;i++) //cycle through every led in lamp
  {
    leds[i] = CHSV(h, s, v);              //assign each led the hsv values
  }
  FastLED.show();                         //update to LEDs
  curv = v;                               //update current V value
}

void turnOn(int low, int high){           //for truning on Lamp
  for(int i=0;i<NUM_LAMPS;i++)            //cycle through every lamp
  {
    for(int j=low;j<=high;j+=2)           //increase brightness of all panels from low to high
    {
      LEDs(i, H[i], S[i], j);             //update Lamps
    }
  }
}

void turnOff(int high, int low){          //for turning off Lamp
  for(int i=0;i<NUM_LAMPS;i++)
  {
    for(int j=high;j>=low;j-=2)           //decrease brightness of all Lamps from high to low
    {
      LEDs(i, H[i], S[i], j);
    }
  }
}

void detect(bool LDR){                    //for detecting object
  if(t <= tmr){                           //check timer
    if((digitalRead(IRSen) || LDR) && prevst == 0) //check if IR sensor detects any object and the if LDR detects low light
    {
      Serial.println("on");
      turnOn(low, high);                  //trun on light
      prevst = 1;                         //set previous state to true
      t=0;                                //reset timer
    }
    if(digitalRead(IRSen))               //if an object is present for long time in front of the sensor reset timer till no object is detected
    {
      Serial.println("object");
      t=0;
    }
    t+=100;                               //increment timer value
  }
  else{                                   //if timer is over reset timer and tutn off Lamps and reset previous state
    t = 0;
    if(prevst == 1)
    {
      turnOff(high, low);
      prevst = 0;
    }
    Serial.println("off");
  }
}

void stopLight(){                         //for emergency road cross light
  if(!digitalRead(SWPin))                 //if switch press is detected
  {
    for(int i=0;i<100;i++)                //if switch is pressed more than 1 seconds then continue otherwise return to main
    {
      if(digitalRead(SWPin))
      {
        return;
      }
      delay(10);
    }
    for(int i=0;i<LED;i++)                //update Lamps to red color
    {
      LEDs(i, 0, 255, 255);
    }
    delay(wait);                          //wait some time
    for(int j=0;j<4;j++)                  //flash green 4 times
    {
      for(int i=0;i<LED;i++)
      {
        LEDs(i, 90, 255, 255);
      }
      delay(500);
      for(int i=0;i<LED;i++)
      {
        LEDs(i, 0, 255, 0);
      }
      delay(500);
    }
    detect(1);                            //return to force detect object
  }
}

void loop()                               //void loop of program
{
  io.run();                               //this should be called in every loop so that every variable of Adafruit IO is updated
  yield();                                //to reset watchdog timer
  if(digitalRead(LDRPin) && OnOff)        //if LDR detects no light
  {
    FastLED.setBrightness(BRIGHTNESS);    //set max brightness
    FastLED.show();                       //update values
    if(mono)                              //if mono mode is on detect object
    {
      detect(LDRst);
      LDRst = 0;
    }                                     //if mono mode is not on then dorainbow color chase
    else{
      multicolor();
      LDRst = 1;
    }
    stopLight();                          //cross light detection
    FastLED.show();
  }
  else{
    FastLED.setBrightness(0);             //if toggle is off then force brightness to 0 of all strips
    FastLED.show();
    LDRst = 1;
  }
}
