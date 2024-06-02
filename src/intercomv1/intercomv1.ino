#include <stdio.h>
#include <string.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "StringSplitter.h"

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
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int unsigned displayCursorYStart=0;

String from_usb = "";

long unsigned CheckSignalLastTime=0;
long unsigned TimeCheckSignalEvent=60000;

long unsigned ResetTimerDebug=0;
long unsigned TimeResetDebug=420000; //change to 30,000 for release
#define ARRAY_CMDS_SIZE 4
#define ARRAY_REGISTERCMD_SIZE 3
const String cmds[ARRAY_CMDS_SIZE]={"chgpassadmin","reset","signal"};
const String cmdsRegister[ARRAY_REGISTERCMD_SIZE]={"adminphone","phonehouse","smshouse"};

String passcode = "448899";  

void InitilizeScreen(){
  if(DEBUG_SCREEN){
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
      PrintToSerial("fail to load SSD1306");
      for(;;);
    }

    display.display();
    delay(2000); 

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, displayCursorYStart);
    display.println("Iniciando...");
    display.display();
  }
}

void PrintOnDisplay(String message){
  if(DEBUG_SCREEN){
    if(display.getCursorY()>56){
      display.setCursor(0, displayCursorYStart);
      display.clearDisplay();
    }
    display.println(message); 
    display.display();
  }
}

void InitializeSetup(){
  // serial port for debugin
  if(DEBUG_SERIAL){
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
  if(DEBUG_SERIAL){
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

  //check for serial messages event
  CheckForSerialCMD();
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
      PrintOnDisplay(response);
      PrintToSerial(response);
    }
    return response;
}

String CheckSignal(long millis){
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

    if(DEBUG){
      PrintToSerial(message);
      PrintOnDisplay(message);
    }
    
    CheckSignalLastTime=millis;
    return message;
  }
}

void ResetLTE(){
  PrintToSerial("Reseting LTE...");
  PrintOnDisplay("Reseting LTE...");
  digitalWrite(LTE_RESET_PIN, HIGH);
  digitalWrite(LTE_RESET_PIN, LOW);
  delay(100);
  digitalWrite(LTE_PWRKEY_PIN, HIGH);
  delay(2000);
  digitalWrite(LTE_PWRKEY_PIN, LOW);
  delay(500);
  digitalWrite(LTE_FLIGHT_PIN, HIGH); //Normal Mode
  delay(1000);
  digitalWrite(LTE_FLIGHT_PIN, LOW); //Normal Mode
  delay(1000);
  sendData("AT+CGMM", 3000, DEBUG);
}

void CheckForSerialCMD(){
  String message = "";
   while (Serial1.available() > 0)
    {
       char c = Serial1.read();
         if (c == '\n' || c == '\r' || c == '\r\n' || c == '\n\r' ) 
            {
              message += '=';
            }else{
              message += c;
            }
       //SerialUSB.write(Serial1.read());
       //yield();
    }

    //clean message
    message.replace("==","");

    if(DEBUG && message!=""){
      PrintOnDisplay(message);
      PrintToSerial(message);
    }

    if(message=="") return;
    
    if(message.startsWith("=+CMT")){
      String commandsFound[3]={"","",""};

      //verify number for allow run cmds
      String phoneNumber=GetPhoneNumber(message);
      PrintToSerial("phone: "+ phoneNumber);  
      if(phoneNumber=="") {
        if(DEBUG){
          PrintToSerial("no phone found");  
          PrintOnDisplay("no phone found");
        }
        return;
      }

      CheckCommand(message,commandsFound);
      
      if(commandsFound[0]=="") return;
      commandsFound[0].trim();
      commandsFound[1].trim();
      commandsFound[2].trim();
       if(commandsFound[0]=="cmd"){
         if(isValidCommand(commandsFound[1])){
            //triger cmd event
            if(DEBUG){
              PrintToSerial("CMD "+commandsFound[0]+" AC "+commandsFound[1]);
              PrintToSerial("VAL "+commandsFound[2]);
              PrintOnDisplay("CMD "+commandsFound[0]+" AC "+commandsFound[1]);
              PrintOnDisplay("VAL "+commandsFound[2]);
            }
            
            //String response=WaitForResponseClient(15000);
            //String response=WaitForResponseClient(15000);
            SendSMS(phoneNumber,"input code",15000);
            String response=WaitForResponseClient(15000);
            if(response=="") {
              if(DEBUG){
                PrintToSerial("no response..");  
                PrintOnDisplay("no response..");
              }
              return;
            }

            //get the response
            String passcodeSendIt=response.substring(response.length()-6,response.length());
            PrintToSerial(passcodeSendIt);
            if(passcodeSendIt!=passcode) return;

            DoCommand(phoneNumber,commandsFound[0],commandsFound[1],commandsFound[2]);

            //PrintToSerial(response);
         }
       }
       if(commandsFound[0]=="register"){
         if(isValidCommandRegister(commandsFound[1])){
            //triger cmd event
            PrintToSerial(commandsFound[1]);
         }
       }
    }
}

String WaitForResponseClient(const int timeout){
  long int time = millis();
  String message = "";
  if(DEBUG){
    PrintToSerial("Waiting for response...");
    PrintOnDisplay("Waiting for response...");
  }
  
  while ((time + timeout) > millis()){
    while (Serial1.available() > 0)
    {
       char c = Serial1.read();
         if (c == '\n' || c == '\r' || c == '\r\n' || c == '\n\r' ) 
            {
              message += '=';
            }else{
              message += c;
            }
    }
  }
  //clean message
  message.replace("==","");

  if(DEBUG && message!=""){
    PrintOnDisplay(message);
    PrintToSerial(message);
  }

  return message;
}

void  CheckCommand(String message,String * arrayToReturn){
  //=+CMT: "+522288464147","","24/05/30,10:17:10-24"$cmd:setadmin
  int index= message.indexOf("$");
  if(index==-1) new StringSplitter("", ':', 2);

  String verb= message.substring(index+1,message.length());
  int indexValues=verb.indexOf(":");
  if(indexValues==-1) return;
  
  StringSplitter *valuesSplited = new StringSplitter(verb, ':', 2);
  arrayToReturn[0]=valuesSplited->getItemAtIndex(0);

  int containsEquals= valuesSplited->getItemAtIndex(1).indexOf('#');
  PrintToSerial(String(containsEquals));

  if(containsEquals==-1){ 

    arrayToReturn[1]=valuesSplited->getItemAtIndex(1);
  }else{
    StringSplitter *vsplited = new StringSplitter(valuesSplited->getItemAtIndex(1), '#', 2);
    arrayToReturn[1]=vsplited->getItemAtIndex(0);
    arrayToReturn[2]=vsplited->getItemAtIndex(1);
  }

  PrintToSerial(arrayToReturn[0]);
  PrintToSerial(arrayToReturn[1]);
}

bool isValidCommand(String cmd){
  bool isValid=false;
  //PrintToSerial(String(sizeof(cmds)));
  for(int i=0; i<ARRAY_CMDS_SIZE;i++){
    if(cmds[i]==cmd){
      isValid=true;
    }
  }
  return isValid;
}

bool isValidCommandRegister(String cmd){
  bool isValid=false;
  //PrintToSerial(String(sizeof(cmds)));
  for(int i=0; i<ARRAY_CMDS_SIZE;i++){
    if(cmdsRegister[i]==cmd){
      isValid=true;
    }
  }
  return isValid;
}

String GetPhoneNumber(String message){
  String phone="";
  //=+CMT: "+522288464147","","24/05/30,18:45:31-24"$cmd:signal
  int plusIndexOf= message.lastIndexOf("+");
  if(plusIndexOf==-1) return phone;

  phone= message.substring(plusIndexOf,21);

  return phone;
}

void SendSMS(String phoneNumber,String message,int  timeout){
  //sendData("AT+CNMI=2,1,0,0,0",500,DEBUG);
  sendData("AT+CMGS=\""+phoneNumber+"\"",timeout/2,DEBUG);
  sendData(message,timeout/2,DEBUG);
  sendData("1A",0,DEBUG);
  delay(3000);
  long time=millis();
  String messageGet="";
  //while ((time + 15000) > millis()){
  while (messageGet!="OK"){
    while (Serial1.available() > 0)
    {
       char c = Serial1.read();
       //|| c == '\r\n' || c == '\n\r' 
         if (c == '\n' && c == '\r') 
            {
              messageGet += '=';
            }else{
              messageGet += c;
            }
    }
    messageGet.replace("=","");
    messageGet.trim();
    SerialUSB.println(messageGet);
    if(messageGet!="") {
      String lastvalue=messageGet.substring(messageGet.length()-2,messageGet.length());
      lastvalue.trim();
      if(lastvalue=="OK") messageGet="OK";
    }
  }
  //sendData("AT+CNMI=2,2,0,0,0",500,DEBUG);
  //WaitForResponseClient(800);

}

void DoCommand(String phoneNumber,String cmd,String action,String value ){
  if(cmd=="cmd" && action=="signal"){
      long time=millis();
      String response=CheckSignal(time);
      SendSMS(phoneNumber,response,10000);
  }

  if(cmd=="cmd" && action=="reset"){
    ResetLTE();
    delay(1000);
    SendSMS(phoneNumber,"Reset Done!",10000);
  }

}