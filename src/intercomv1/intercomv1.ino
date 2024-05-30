#include <stdio.h>
#include <string.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "StringSplitter.h"

#define DEBUG true
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
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int unsigned displayCursor=0;

String from_usb = "";

long unsigned CheckSignalLastTime=0;
long unsigned TimeCheckSignalEvent=60000;

void InitilizeScreen(){
  PrintToSerial("Iniciando screen");
  if(DEBUG){
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      PrintToSerial("Fallo en iniciar pantalla SSD1306");
      for(;;);
    }

    display.display();
    delay(2000); 

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Iniciando...");
    display.display();
  }
}

void PrintOnDisplay(String message){
  if(DEBUG_SCREEN){
    if(display.getCursorY()>56){
      display.setCursor(0, 0);
      display.clearDisplay();
    }
    display.println(message); 
    display.display();
  }
}

void InitializeSetup(){
  // serial port for debugin
  if(DEBUG){
    SerialUSB.begin(115200);
    delay(100);
  } 

  //serial port for AT commands
  Serial1.begin(115200);

  //Reset modules
  pinMode(LTE_RESET_PIN, OUTPUT);
  digitalWrite(LTE_RESET_PIN, LOW);
  pinMode(LTE_PWRKEY_PIN, OUTPUT);
  digitalWrite(LTE_RESET_PIN, LOW);
  delay(100);
  digitalWrite(LTE_PWRKEY_PIN, HIGH);
  delay(2000);
  digitalWrite(LTE_PWRKEY_PIN, LOW);
  pinMode(LTE_FLIGHT_PIN, OUTPUT);
  digitalWrite(LTE_FLIGHT_PIN, LOW); //Normal Mode

}

void PrintToSerial(String message){
  if(DEBUG){
    SerialUSB.println(message);
  }
}

void setup()
{
  InitilizeScreen();
  InitializeSetup();

  PrintToSerial("Maduino Zero 4G Started!");
  PrintOnDisplay("Maduino Zero 4G Started!");

  sendData("AT+CGMM", 3000, DEBUG);

}

void loop()
{
  long time=millis();
 CheckSignal(time);
  while (Serial1.available() > 0)
    {
        SerialUSB.write(Serial1.read());
        yield();
    }
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

bool moduleStateCheck()
{
    int i = 0;
    bool moduleState = false;
    for (i = 0; i < 5; i++)
    {
        String msg = String("");
        msg = sendData("AT", 1000, DEBUG);
        if (msg.indexOf("OK") >= 0)
        {
            PrintToSerial("SIM7600 Module had turned on.");
            PrintOnDisplay("SIM7600 Module had turned on.");
            moduleState = true;
            return moduleState;
        }
        delay(1000);
    }
    return moduleState;
}

String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    if (command.equals("1A") || command.equals("1a"))
    {
        PrintToSerial("Get a 1A, input a 0x1A");

        Serial1.write(0x1A);
        Serial1.println();
        return "";
    }
    else
    {
        Serial1.println(command);
    }

    long int time = millis();
    while ((time + timeout) > millis())
    {
      while (Serial1.available())
        {
            char c = Serial1.read();
            if (c == '\n' || c == '\r' || c == '\r\n' || c == '\n\r' ) 
            {
              response += '=';
            }else{
              response += c;
            }
        }
    }
    //clean response
    response.replace("===","=");
    response.replace("=="," ");
    if (debug)
    {
      PrintOnDisplay(String(response));
      PrintToSerial(response);
    }
    return response;
}

void CheckSignal(long millis){
  if((millis-CheckSignalLastTime) > TimeCheckSignalEvent){
    String response= sendData("AT+CSQ",3000,false);
    StringSplitter *splitter = new StringSplitter(response, ':', 2);
    String value=splitter->getItemAtIndex(1);
    value.replace(" OK","");
    value.trim();
    StringSplitter *valuesSplited = new StringSplitter(value, ',', 2);
    float rssi=valuesSplited->getItemAtIndex(0).toFloat();
    float signal= -113+rssi*2;
    String messageQuality="Bad Signal";
    if(signal>=-30 && signal<-50) messageQuality="Perf Sig";
    if(signal<=-50 && signal>-60) messageQuality="Exce Sig";
    if(signal<=-60 && signal>-67) messageQuality="Good Sig";
    if(signal<=-67 && signal>-70) messageQuality="Min signal";
    if(signal<=-70 && signal>-80) messageQuality="No internet";
    if(signal<=-80 && signal>-90) messageQuality="No signal";
    if(signal<=-90) messageQuality="No connection";
    String message=String(signal)+"|"+ messageQuality;
    PrintToSerial(message);
    PrintOnDisplay(message);
    CheckSignalLastTime=millis;
  }
}