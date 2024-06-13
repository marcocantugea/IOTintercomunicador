void PrintToDebug(String message){
  PrintToSerial(message);
  PrintOnDisplay(message);
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