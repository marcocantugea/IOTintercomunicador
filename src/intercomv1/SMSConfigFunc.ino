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
      PrintToDebug(message);
    }else{
      PrintToSerial(message);
    }
    return message;
}

void ResetLTE(){
  PrintToDebug("Reseting LTE...");
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
  long time=millis();
  while ((time + 3000) > millis()){
   while (Serial1.available() > 0)
    {
       char c = Serial1.read();
        if (c == '\n' || c == '\r' || c == '\r\n' || c == '\n\r' ){
          message += '=';
        }else{
          message += c;
        }
    }
  }
    
    message.replace("==","");
    if(message=="") return;

    if(DEBUG && message!=""){
      PrintToDebug(message);
    }else{
      PrintToSerial(message);
    }

    int hasSignCommand=message.indexOf("$");

    if(message.startsWith("+CMT") && hasSignCommand>-1){
      String commandsFound[3]={"","",""};

      //verify number for allow run cmds
      String phoneNumber=GetPhoneNumber(message);
      PrintToDebug(phoneNumber);
      if(phoneNumber=="") {
        if(DEBUG){
          PrintToDebug("no phone found");  
        }
        return;
      }

      String adminPhone=GetConfig("adminPhone");
      int hasResetConfigCmd=message.indexOf("resetconfig");
      PrintToDebug(adminPhone);
      PrintToDebug(phoneNumber);
      int isAdminPhone=adminPhone.indexOf(phoneNumber);
      PrintToDebug(String(isAdminPhone));
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


        SendSMS(phoneNumber, "envia el codigo", 15000);
        String response = WaitForResponseClient(15000);
        if (response == "") {
          if (DEBUG) {
            PrintToDebug("no response..");
          }
          return;
        }

        String passcodeConfig=GetConfig("passcode");
        String passcodeSendIt=response.substring(response.length()-6,response.length());
        passcodeConfig.trim();

       if(commandsFound[0]=="cmd"){
         if(isValidCommand(commandsFound[1])){
            //triger cmd event
            if(DEBUG){
              PrintToDebug("CMD "+commandsFound[0]+" AC "+commandsFound[1]);
              PrintToDebug("VAL "+commandsFound[2]);
            }

            if(hasResetConfigCmd>-1 && passcodeSendIt==passcode){
              DoCommand(phoneNumber,commandsFound[0],commandsFound[1],commandsFound[2]);  
            }else{
              
              if(passcodeConfig=="") passcodeConfig=passcode;

              if(passcodeSendIt!=passcodeConfig) return;
              DoCommand(phoneNumber,commandsFound[0],commandsFound[1],commandsFound[2]);
            }
         }
       }
       if(commandsFound[0]=="register"){
         if(isValidCommandRegister(commandsFound[1])){
            //triger cmd event
            if(DEBUG){
              PrintToDebug("CMD "+commandsFound[0]+" AC "+commandsFound[1]);
              PrintToDebug("VAL "+commandsFound[2]);
            }

            if(passcodeConfig=="") passcodeConfig=passcode;

            if(passcodeSendIt!=passcodeConfig){ 
              PrintToDebug("invalid pass code");
              return;
            } 
            DoCommand(phoneNumber,commandsFound[0],commandsFound[1],commandsFound[2]);
         }
       }
    }
}

String WaitForResponseClient(const int timeout){
  long int time = millis();
  String message = "";
  if(DEBUG){
    PrintToDebug("Waiting for response...");
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
    PrintToDebug(message);
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

  if(containsEquals==-1){ 
    arrayToReturn[1]=valuesSplited->getItemAtIndex(1);
  }else{
    StringSplitter *vsplited = new StringSplitter(valuesSplited->getItemAtIndex(1), '#', 2);
    arrayToReturn[1]=vsplited->getItemAtIndex(0);
    arrayToReturn[2]=vsplited->getItemAtIndex(1);
  }
}

bool isValidCommand(String cmd){
  bool isValid=false;
  
  for(int i=0; i<ARRAY_CMDS_SIZE;i++){
    if(cmds[i]==cmd){
      isValid=true;
    }
  }
  return isValid;
}

bool isValidCommandRegister(String cmd){
  bool isValid=false;
  
  for(int i=0; i<ARRAY_REGISTERCMD_SIZE;i++){
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

  phone= message.substring(plusIndexOf,20);
  phone.trim();
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
      int errorValue=messageGet.indexOf("ERROR");
      lastvalue.trim();
      if(lastvalue=="OK") messageGet="OK";
      if(errorValue>-1) messageGet="OK";
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

  if(cmd=="cmd" && action=="chgpasscode" && value!=""){
    SendSMS(phoneNumber,"confirmar guardar nuevo codigo?",10000);
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
    SaveConfiguration("passcode", value);
    SendSMS(phoneNumber,"change done!",10000);
  }

  if(cmd=="cmd" && action=="chghostphone" && value!=""){
    SendSMS(phoneNumber,"confirmar guardar nuevo numero de host?",10000);
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
    SaveConfiguration("hostPhone", value);
    SendSMS(phoneNumber,"change done!",10000);
  }

  if(cmd=="register" && action=="list"){
    String registerFile=LoadRegister();
    SendSMS(phoneNumber,registerFile,10000);
  }

  if(cmd=="register" && action=="housephone"){
    SendSMS(phoneNumber,"envie el numero de casa",10000);
    String response=WaitForResponseClient(15000);
    if(response=="") {
      if(DEBUG){
        PrintToDebug("no response..");  
      }
      return;
    }

    String house=response.substring(response.length()-2,response.length());
    if(house=="") {
      if(DEBUG){
        PrintToDebug("no response house number..");  
      }
      return;
    }

    SendSMS(phoneNumber,"envie el numero de telefono",18000);
    response=WaitForResponseClient(15000);
    if(response=="") {
      if(DEBUG){
        PrintToDebug("no response..");  
      }
      return;
    }

    String phone=response.substring(response.length()-10,response.length());
    if(phone==""){
      if(DEBUG){
        PrintToDebug("no response phone number..");  
      }
      return;
    }
    
    PrintToDebug(GetHouseFromRegister(house));

    if(!AddLineToRegister(house,phone)){
      PrintToDebug("error al registar la casa y el telefono");
      SendSMS(phoneNumber,"Casa ya existente.",14000);  
      return;
    }

    SendSMS(phoneNumber,"registro agregado con exito",14000);

  }

  if(cmd=="register" && action=="rmhouse" && value!=""){
    SendSMS(phoneNumber,"desea eliminar la casa no."+ value,10000);

    String response=WaitForResponseClient(15000);
    if(response=="") {
      if(DEBUG){
        PrintToDebug("no response..");  
      }
      return;
    }

    String confirm=response.substring(response.length()-2,response.length());

    if(confirm!="si") return;

    int houseVal=value.toInt();
    if(houseVal==0) {
      SendSMS(phoneNumber,"numero de casa invalido",10000);  
      return;
    }

    DeleteLineInRegister(value);
    SendSMS(phoneNumber,"registro actualizado",10000);

  }

}