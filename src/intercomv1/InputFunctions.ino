void KeypadReader(){
   char key = keypad.getKey();
   // comprueba que se haya presionado una tecla
    if (key) {
      // envia a monitor serial la tecla presionada
      //keysTec=String(key);
      int len =keysTec.length();
      if(len<3){
         keysTec+=String(key);  
      }
      
      if(len>=2){
        PrintToDebug(keysTec);
        keysTec="";
      }
      delay(200);
    }
}

void DetectButtonPress(){

  currentState = digitalRead(BUTTON_PIN);
  if (lastState == HIGH && currentState == LOW){
    PrintToDebug("button trigger");
    PrintToDebug(keysTec);
    String houseInfo=GetHouseFromRegister(keysTec);
    PrintToDebug(houseInfo);
    if(houseInfo=="") PrintToDebug("no house found");
    StringSplitter *houseInfoSplited = new StringSplitter( houseInfo,'#',2);
    PrintToDebug(houseInfoSplited->getItemAtIndex(1));
    StringSplitter *phoneValue = new StringSplitter( houseInfoSplited->getItemAtIndex(1),'=',2);
    PrintToDebug(phoneValue->getItemAtIndex(1));
    if(phoneValue->getItemAtIndex(1).indexOf("+520000000000")>-1) MakeCall("+522288464147");

    keysTec="";
    delay(300);
   }

  lastState = currentState;
}