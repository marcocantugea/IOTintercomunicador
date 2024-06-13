bool moduleStateCheck(){
    int i = 0;
    bool moduleState = false;
    for (i = 0; i < 5; i++)
    {
        String msg = String("");
        msg = sendData("AT", 1000, DEBUG);
        if (msg.indexOf("OK") >= 0)
        {
            PrintToDebug("SIM7600 Module had turned on.");
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
        PrintToDebug("Get a 1A, input a 0x1A");

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
      PrintToDebug(response);
    }else{
      PrintToSerial(response);
    }

    return response;
}
