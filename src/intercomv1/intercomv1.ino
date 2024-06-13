#include <stdio.h>
#include <string.h>
#include <Arduino_JSON.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "StringSplitter.h"
#include <SD.h>
#include <Keypad.h>

#define DEBUG true
#define DEBUG_SERIAL true
#define DEBUG_SCREEN true
#define MODE_1A

#define DTR_PIN 9
#define RI_PIN 8

#define LTE_PWRKEY_PIN 5
#define LTE_RESET_PIN 6
#define LTE_FLIGHT_PIN 7

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

const int PIN_SD_SELECT = 4; //sd seleced pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int unsigned displayCursorYStart=0;

String from_usb = "";

long unsigned ResetTimerDebug=0;
long unsigned TimeResetDebug=420000; //change to 30,000 for release

Sd2Card card;
SdVolume volume;
SdFile root;

#define ARRAY_CMDS_SIZE 7
#define ARRAY_REGISTERCMD_SIZE 4
const String cmds[ARRAY_CMDS_SIZE]={"chgpasscode","reset","signal","getconfig","chgadminphone","resetconfig","chghostphone"};
const String cmdsRegister[ARRAY_REGISTERCMD_SIZE]={"housephone","housesms","list","rmhouse"};
const String configFileName="confprg.ivo";
const String registerFileName="regprb.ivo";
const String logFile="loga.log";

String passcode = "448899";

//keypad conf
const uint8_t ROWS = 4;
const uint8_t COLS = 4;

 char keys[ROWS][COLS] = {
   { '1', '2', '3', 'A' },
   { '4', '5', '6', 'B' },
   { '7', '8', '9', 'C' },
   { '*', '0', '#', 'D' }
 };

 uint8_t rowPins[ROWS] = { 2,9,11,13 };
 uint8_t colPins[COLS] = { 3,10,12,A0 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String keysTec;

//button setting 2 3 4* 9 13 12 11 10 A0 A1 A3 A4 A5
const int BUTTON_PIN = A5;
int currentState;
int lastState = HIGH;  


void setup(){

  if(DEBUG){
    delay(5000);
  }
  InitilizeScreen();
  InitializeSetup();
  InitializeSD();
  if(!SD.exists(configFileName)) CreateInitialConfigInSD();
  if(!SD.exists(registerFileName)) CreateInitialRegisterInSD();
    
  PrintToDebug("Maduino Zero 4G Started!");
  sendData("AT+CGMM", 3000, DEBUG);
  

}

void loop(){

  //check for serial messages event
  if(Serial1.available() > 0) CheckForSerialCMD();

  //check for key inputs
  KeypadReader();

  //detec button press
  DetectButtonPress();

  
  //check signal event  
  //CheckSignal(time);
  

  //reset lte event
  // if((time-ResetTimerDebug)> TimeResetDebug){
  //   ResetLTE();
  //   ResetTimerDebug=time;
  // }
  
  
  // while (Serial1.available() > 0)
  //   {
  //       SerialUSB.write(Serial1.read());
  //       yield();
  //   }
  if(DEBUG){
    while (SerialUSB.available() > 0 && DEBUG)
    {
    #ifdef MODE_1A
            int c = -1;
            c = SerialUSB.read();
            if (c != '\n' && c != '\r')
            {
                from_usb += (char)c;
            }
            else
            {
                if (!from_usb.equals(""))
                {
                    sendData(from_usb, 300, DEBUG);
                    from_usb = "";
                }
            }
    #else
            Serial1.write(SerialUSB.read());
            yield();
    #endif
      }
  }
    
}


