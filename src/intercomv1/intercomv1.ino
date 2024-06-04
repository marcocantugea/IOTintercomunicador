#include <stdio.h>
#include <string.h>
#include <Arduino_JSON.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "StringSplitter.h"
#include <SD.h>

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

#define ARRAY_CMDS_SIZE 6
#define ARRAY_REGISTERCMD_SIZE 3
const String cmds[ARRAY_CMDS_SIZE]={"chgpasscode","reset","signal","getconfig","chgadminphone","resetconfig"};
const String cmdsRegister[ARRAY_REGISTERCMD_SIZE]={"adminphone","phonehouse","smshouse"};
const String configFileName="confprf.ivo";

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
    display.println("Strating...");
    display.display();
    delay(2000); 
  }
}

void PrintToDebug(String message){
  PrintToSerial(message);
  PrintOnDisplay(message);
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

void InitializeSD(){
  PrintToDebug("\nInitializing SD card...");
  
  if (!card.init(SPI_HALF_SPEED, PIN_SD_SELECT)) {
    if(DEBUG){
      PrintToDebug("initialization failed. Things to check:");
      PrintToDebug("* is a card is inserted?");
      PrintToDebug("* Is your wiring correct?");
      PrintToDebug("* did you change the chipSelect pin to match your shield or module?");
      for(;;);
    }
  } else {
    PrintToDebug("Wiring is correct and a card is present.");
  }

  if (!SD.begin(PIN_SD_SELECT)) {
    PrintToDebug("initialization failed!");
    while (1);
  }

  // print the type of card
  if(DEBUG){
    PrintToDebug("\nCard type: ");
    switch (card.type()) {
      case SD_CARD_TYPE_SD1:
        PrintToDebug("SD1");
        break;
      case SD_CARD_TYPE_SD2:
        PrintToDebug("SD2");
        break;
      case SD_CARD_TYPE_SDHC:
        PrintToDebug("SDHC");
        break;
      default:
        PrintToDebug("Unknown");
    }
  }
  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    PrintToDebug("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    //return;
  }


  // print the type and size of the first FAT-type volume
  // uint32_t volumesize;
  // PrintToDebug("\nVolume type is FAT");
  // PrintToDebug(String(volume.fatType()));

  // volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  // volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  // volumesize *= 512;                            // SD card blocks are always 512 bytes
  // PrintToDebug("Volume size (bytes): ");
  // PrintToDebug(String(volumesize));
  // PrintToDebug("Volume size (Kbytes): ");
  // volumesize /= 1024;
  // PrintToDebug(String(volumesize));
  // PrintToDebug("Volume size (Mbytes): ");
  // volumesize /= 1024;
  // PrintToDebug(String(volumesize));


  // PrintToDebug("\nFiles found on the card (name, date and size in bytes): ");
  // root.openRoot(volume);

}

void PrintToSerial(String message){
  if(DEBUG_SERIAL){
    SerialUSB.println(message);
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

void setup(){

  if(DEBUG){
    delay(5000);
  }
  InitilizeScreen();
  InitializeSetup();
  InitializeSD();
  if(!SD.exists(configFileName)) CreateInitialConfigInSD();
    
  PrintToSerial("Maduino Zero 4G Started!");
  PrintOnDisplay("Maduino Zero 4G Started!");
  sendData("AT+CGMM", 3000, DEBUG);
  

}

void loop(){
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

bool moduleStateCheck(){
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

String sendData(String command, const int timeout, boolean debug){
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
        Serial1.print(command);
        Serial1.print('\r');
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
  
    String response= sendData("AT+CSQ",3000,DEBUG);
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
    return message;
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
    }

    message.replace("==","");
    
    if(DEBUG && message!=""){
      PrintToDebug(message);
    }

    if(message=="") return;
    
    if(message.startsWith("=+CMT")){
      String commandsFound[3]={"","",""};

      //verify number for allow run cmds
      String phoneNumber=GetPhoneNumber(message);

      if(phoneNumber=="") {
        if(DEBUG){
          PrintToDebug("no phone found");  
        }
        return;
      }

      String adminPhone=GetConfig("adminPhone");
      int hasResetConfigCmd=message.indexOf("resetconfig");
      PrintToDebug(adminPhone);
      if(hasResetConfigCmd==-1){
        if(adminPhone!="+520000000000"){
          if(phoneNumber!=adminPhone) {
            PrintToDebug("Invalid admin phone");
            return;
          }
        }  
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
              PrintToDebug("CMD "+commandsFound[0]+" AC "+commandsFound[1]);
              PrintToDebug("VAL "+commandsFound[2]);
            }
            
            //String response=WaitForResponseClient(15000);
            //String response=WaitForResponseClient(15000);
            SendSMS(phoneNumber,"envia el codigo",15000);
            String response=WaitForResponseClient(15000);
            if(response=="") {
              if(DEBUG){
                PrintToDebug("no response..");  
              }
              return;
            }

            //get the response
            String passcodeSendIt=response.substring(response.length()-6,response.length());
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

  if(cmd=="cmd" && action=="getconfig"){
    String configSD=LoadConfigFile();
    SendSMS(phoneNumber,configSD,10000);
  }

  if(cmd=="cmd" && action=="chgadminphone" && value!=""){
    if(value.toInt()==0) return;
    SendSMS(phoneNumber,"confirmar guardar nuevo numero",10000);
    String response=WaitForResponseClient(15000);
    if(response=="") {
      if(DEBUG){
        PrintToDebug("no response..");  
      }
      return;
    }

    //get the response
    String confirm=response.substring(response.length()-2,response.length());
    if(confirm!="si") return;
    SaveConfiguration("adminPhone", "+52"+value);
    SendSMS(phoneNumber,"adminPhone:+52"+value,10000);
  }

  if(cmd=="cmd" && action=="resetconfig"){
    SendSMS(phoneNumber,"mkey",10000);
    String response=WaitForResponseClient(15000);
    if(response=="") {
      if(DEBUG){
        PrintToDebug("no response..");  
      }
      return;
    }

    //get the response
    String confirm=response.substring(response.length()-5,response.length());
    if(confirm!="maley") return;
    SD.remove(configFileName);
    CreateInitialConfigInSD();
    SendSMS(phoneNumber,"reset done!",10000);
  }

}

void CreateInitialConfigInSD(){

  //String config="#";
  int sizeOfInitialConfig=3;
  char *config[sizeOfInitialConfig]={"hostPhone=+520000000000","passcode=448899","adminPhone=+520000000000"};

  if(SD.exists(configFileName)){
    PrintToDebug("exists config file");
    return;
  } 
  // Declare a buffer to hold the result
  
  File configfile = SD.open(configFileName, FILE_WRITE | O_TRUNC);
    if (configfile) {
      for (int i=0; i<sizeOfInitialConfig; i++) {
        configfile.print(config[i]);
        configfile.print("\r");
      }
      configfile.close();
      PrintToDebug("Config File Created...");

   }else{
     PrintToDebug("Error writing the file");
   }
}

String LoadConfigFile(){
  
  String configSD="";
  if(!SD.exists(configFileName)) return configSD;
  File configfile = SD.open(configFileName);
  if(configfile){
    if (configfile.available()) {
      configSD=configfile.readString();
    }
    configfile.close();
    PrintToDebug("Check Configuration from file...");
  }else{
      PrintToDebug("Error read config file...");
  }
  return configSD;
}

String GetConfig(String configName){
  String configloaded=LoadConfigFile();
  StringSplitter *vsplited = new StringSplitter( configloaded,'\r',5);
  int itemCount = vsplited->getItemCount();
  String valueFound="";
  for(int i = 0; i < itemCount; i++){
     String item = vsplited->getItemAtIndex(i);
    int indexOfConfigName= item.indexOf(configName);
    if(indexOfConfigName>-1){
      StringSplitter *csplited = new StringSplitter( item,'=',2);
      valueFound=csplited->getItemAtIndex(1);
    }
  }
  return valueFound;
}

void SaveConfiguration(String configName,String Value){
  String configloaded=LoadConfigFile();
  StringSplitter *vsplited = new StringSplitter( configloaded,'\r',25);
  int itemCount = vsplited->getItemCount();
  
  if(!SD.exists(configFileName)){
    PrintToDebug("no config file exist");
    return;
  } 

  File fileOpen = SD.open(configFileName, FILE_WRITE | O_TRUNC); 
  if(fileOpen){
    for(int i = 0; i <= itemCount; i++){
      String item = vsplited->getItemAtIndex(i);
      PrintToDebug(item);
      if(item=="" || item=="\r" || item=="\n" || item=="\r\n") item="--";
      if(item!="--"){
          int indexOfConfigName= item.indexOf(configName);
          if(indexOfConfigName>-1){
            StringSplitter *csplited = new StringSplitter( item,'=',2);
            //valueFound=csplited->getItemAtIndex(1);
            fileOpen.print(csplited->getItemAtIndex(0));
            fileOpen.print("=");
            fileOpen.print(Value);
            fileOpen.print("\r");
          }else{
            fileOpen.print(item);
            fileOpen.print("\r");
          } 
      }
    }
    fileOpen.close();
    PrintToDebug("config file updated...");
  }else{
    PrintToDebug("Error read config file...");
  }
}