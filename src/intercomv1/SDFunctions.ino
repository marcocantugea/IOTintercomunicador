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

void CreateInitialRegisterInSD(){
  int sizeOfInitialConfig=1;
  char *config[sizeOfInitialConfig]={"house=01#phone=+520000000000"};

  if(SD.exists(registerFileName)){
    PrintToDebug("exists register file");
    return;
  } 
  // Declare a buffer to hold the result
  
  File registerFile = SD.open(registerFileName, FILE_WRITE | O_TRUNC);
    if (registerFile) {
      for (int i=0; i<sizeOfInitialConfig; i++) {
        registerFile.print(config[i]);
        registerFile.print("\r");
      }
      registerFile.close();
      PrintToDebug("Register File Created...");

   }else{
     PrintToDebug("Error writing the Register file");
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

String LoadRegister(){
  String registerLines="";
  if(!SD.exists(registerFileName)) return registerLines;
  File registerFile = SD.open(registerFileName);
  if(registerFile){
    if (registerFile.available()) {
      registerLines=registerFile.readString();
    }
    registerFile.close();
    PrintToDebug("Checking register file...");
  }else{
      PrintToDebug("Error read register file...");
  }
  return registerLines;
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
  int newline=valueFound.indexOf("\r");
  int newlineN=valueFound.indexOf("\n");
  if(newline!=-1) valueFound.remove(newline);
  if(newlineN!=-1) valueFound.remove(newlineN);
  valueFound.trim();
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

bool AddLineToRegister(String house,String phone){
  bool added=true;
  int houseVal= house.toInt();
  if(houseVal==0) {
    PrintToDebug("invalid house");
    return false;
  }

  int phoneVal= phone.toInt();
  if(phoneVal==0){
    PrintToDebug("invalid phone");
    return false;
  }

  String registerLine=GetHouseFromRegister(house);
  if(registerLine!=""){
    return false;
  }

  File fileOpen = SD.open(registerFileName, FILE_WRITE); 
  if(fileOpen){
    fileOpen.print("house=");
    fileOpen.print(house);
    fileOpen.print("#phone=+52");
    fileOpen.print(phone);
    fileOpen.print("\r");
    fileOpen.close();
    PrintToDebug("register file updated...");
  }else{
    PrintToDebug("Error read config file...");
    return false;
  }
  
  return added;
}

bool UpdateLineInRegister(String house,String phone){
  bool savedSuccess=true;
  String houseSaved= GetHouseFromRegister(house);
  if(houseSaved!=""){
    DeleteLineInRegister(house);
  }

  if(AddLineToRegister(house,phone)){
    savedSuccess= true;
  }else{
    savedSuccess= false;
  }

  return savedSuccess;
}

void DeleteLineInRegister(String house){
  String registerFilebuff=LoadRegister();
  StringSplitter *vsplited = new StringSplitter( registerFilebuff,'\r',50);
  int itemCount = vsplited->getItemCount();
  String valueFound="";
  String newRegister="";
  for(int i = 0; i < itemCount; i++){
    int houseVal=vsplited->getItemAtIndex(i).indexOf(house);
    if(houseVal==-1){
      String row=vsplited->getItemAtIndex(i);
      if(row!="" ||  row!="\r") newRegister+=row;
    }
  }

  File registerFile = SD.open(registerFileName, FILE_WRITE | O_TRUNC);
  if(registerFile){
    registerFile.print(newRegister);
    registerFile.print("\r");
    registerFile.close();
     PrintToDebug("register file updated...");
  }else{
    PrintToDebug("Erro to open register file");
  }

}

String GetHouseFromRegister(String house){
  String line="";
   if(!SD.exists(registerFileName)) return line;
  File registerFile = SD.open(registerFileName);
  if(registerFile){
    if (registerFile.available()) {
      String linebuff=registerFile.readString();
      if(linebuff.indexOf(house)>-1){
        line=linebuff;
      }
    }
    registerFile.close();
    PrintToDebug("Checking register file...");
  }else{
      PrintToDebug("Error read register file...");
  }
  return line;
}