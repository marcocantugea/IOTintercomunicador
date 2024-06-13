void MakeCall(String phoneNumber){
  sendData("ATD"+phoneNumber+";",1000,DEBUG);
  long int time = millis();
  while ((time + 15000) > millis()){
    PrintToDebug("call in progress");
  }
  sendData("AT+CHUP",1000,DEBUG);

}