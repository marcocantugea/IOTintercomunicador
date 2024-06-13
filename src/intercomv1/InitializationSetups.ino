void InitializeSetup(){
  // serial port for debugin
  if(DEBUG_SERIAL){
    SerialUSB.begin(115200);
    delay(100);
  } 

  //serial port for AT commands
  Serial1.begin(115200);
  
  //key pad pin
  //pinMode(BUTTON_PIN,INPUT);
  
  //button 
  pinMode(BUTTON_PIN, INPUT_PULLUP);

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
