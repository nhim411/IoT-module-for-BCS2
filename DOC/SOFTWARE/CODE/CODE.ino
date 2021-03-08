#include "libraries.h"
#include "variables.h"
#include "functions.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
LiquidCrystal_PCF8574 lcd;
SPIClass spiSD(HSPI);
RTC_DS3231 rtcDS;
File sdFile;
Button pushButton_1(PUSHBUTTON_1);
Button pushButton_2(PUSHBUTTON_2);

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setup()
{
  Wire.begin(SDA_PIN, SCL_PIN);
  EEPROM.begin(5);
  lcd.begin(LCD_COLUMNS, LCD_ROWS, LCD_ADDRESS, BACKLIGHT);
  lcd.clear();                          // Clear LCD screen.
  //EEPROM.begin(EEPROM_SIZE);
  delay(100);
  //demo OTA Update
  //    lcd.clear();
  //    lcd.setCursor(0, 0);
  //    lcd.print("      DEMO      ");
  //    lcd.setCursor(0, 1);
  //    lcd.println("   OTA UPDATE   ");
  //    delay(5000);
  //demo OTA Update
  onDisplay();
  //  lcd.setCursor(0, 0);
  //  lcd.print("LAN: -SD: -RTC: ");                   // Print print String to LCD on first line
  //lcdI2C.selectLine(2);                    // Set cursor at the begining of line 2
  //  lcd.setCursor(0, 1);
  //  lcd.print("SIM: -WIFI: -W:");                     // Print print String to LCD on second line
  // Serial port for debugging purposes
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXD2_PIN, TXD2_PIN);
  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  else {
    //update data from SPIFFS
    baud = readFile(SPIFFS, "/baud.txt");
    data_bit = readFile(SPIFFS, "/data_bit.txt");
    stop_bit = readFile(SPIFFS, "/stop_bit.txt");
    parity = readFile(SPIFFS, "/parity.txt");
    instrument = readFile(SPIFFS, "/instrument.txt");
    set_sv = readFile(SPIFFS, "/set_sv.txt");
    ftp_server = readFile(SPIFFS, "/ftp_server.txt");
    user1 = readFile(SPIFFS, "/user1.txt");
    user2 = readFile(SPIFFS, "/user2.txt");
    pass1 = readFile(SPIFFS, "/pass1.txt");
    pass2 = readFile(SPIFFS, "/pass2.txt");
    time_update_ftp = readFile(SPIFFS, "/time_update_ftp.txt");
    time_update_sd = readFile(SPIFFS, "/time_update_sd.txt");
    set_sv = readFile(SPIFFS, "/set_sv.txt");
    wf_ssid = readFile(SPIFFS, "/wf_ssid.txt");
    wf_password = readFile(SPIFFS, "/wf_password.txt");
  }
  pushButton_1.init();
  pushButton_2.init();

  Serial1.begin(baud.toInt(), SERIAL_8N1, RXD1_PIN, TXD1_PIN);
  //RS3231
  if (! rtcDS.begin()) {
    Serial.println("Couldn't find RTC");
  } else {
    onRTS = true;
  }
  if (rtcDS.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtcDS.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtcDS.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  spiSD.begin(CLK_PIN, MISO_PIN, MOSI_PIN, SDFILE_PIN_CS); //CLK,MISO,MOIS,SS
  pinMode(SDFILE_PIN_CS, OUTPUT);
  while (!SD.begin(SDFILE_PIN_CS, spiSD)) {
    onDisplay();
    Serial.println(F("Card failed, or not present"));
  }
  onSD = true;
  WiFi.begin(wf_ssid.c_str(), wf_password.c_str());
  Serial.println("Connecting to WiFi..");
  uint32_t t = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("w");
    bool justOne = false;
    if ( ((millis() - t) > 5000) && (justOne == false) )  //if not connect try connect LAN
    {
      justOne = true;
      Serial.println("Retry");
      //WiFi.mode(WIFI_OFF);        //stop wifi
      initEthernet();
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    // Print ESP32 Local IP Address
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" wifi connected ");
    lcd.setCursor(0, 1);
    lcd.println(WiFi.localIP().toString().c_str());
    onWIFI = true;
  }
  if (!MDNS.begin("esp32"))
  {
    Serial.println("Error starting mDNS");
    return;
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html");
  });
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  server.on("/bttn.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/bttn.css", "text/css");
  });
  server.on("/modbus", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/modbus.html", String(), false, processor);
  });
  server.on("/ftp", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/ftp.html", String(), false, processor);
  });
  server.on("/device", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/device.html", String(), false, processor);
  });
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest * request) {
    String user, pass;
    if (choose_user == "1") {
      user = user1;
      pass = pass1;
    } else {
      user = user2;
      pass = pass2;
    }
    //update server
    int ftp_len = ftp_server.length() + 1;
    char ftp_array[ftp_len];
    ftp_server.toCharArray(ftp_array, ftp_len);
    //update user
    int user_len = user.length() + 1;
    char user_array[user_len];
    user.toCharArray(user_array, user_len);
    //update pass
    int pass_len = pass.length() + 1;
    char pass_array[pass_len];
    pass.toCharArray(pass_array, pass_len);
    ESP32_FTPClient ftp (ftp_array, user_array, pass_array);
    ftp.OpenConnection();
    ftp.ChangeWorkDir("/");
    download_ftp = "";
    ftp.InitFile("Type A");
    ftp.DownloadString("demo.txt", download_ftp);
    Serial.println("The file content is: ");
    Serial.println(download_ftp);
    ftp.CloseConnection();
    request->send(SPIFFS, "/download.html", String(), false, processor);
  });
  server.on("/show", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/show.html", String(), false, processor);
  });
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/download.html", String(), false, processor);
  });
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/run.html", String(), false, processor);
  });
  //wf-config?wf_ssid=Hoa+Nam&wf_password=123456
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/wifi.html", String(), false, processor);
  });
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    ESP.restart();
  });
  // Send a GET request to /ftp-config?server=<server-id>&user1=<user1>&pass1=<pass1>&user2=<user2>&pass2=<pass2>&time=<time>
  server.on("/modbus-config", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("baud"))
    {
      baud = request->getParam("baud")->value();
      writeFile(SPIFFS, "/baud.txt", baud.c_str());
      Serial.print("Baud: ");
      Serial.println(baud);
    }
    if (request->hasParam("data_bit"))
    {
      data_bit = request->getParam("data_bit")->value();
      writeFile(SPIFFS, "/data_bit.txt", data_bit.c_str());
      Serial.print("Bit data: ");
      Serial.println(data_bit);
    }
    if (request->hasParam("stop_bit"))
    {
      stop_bit = request->getParam("stop_bit")->value();
      writeFile(SPIFFS, "/stop_bit.txt", stop_bit.c_str());
      Serial.print("Bit Stop: ");
      Serial.println(stop_bit);
    }
    if (request->hasParam("parity"))
    {
      instrument = request->getParam("parity")->value();
      writeFile(SPIFFS, "/parity.txt", parity.c_str());
      Serial.print("Parity: ");
      Serial.println(parity);
    }
    if (request->hasParam("instrument"))
    {
      instrument = request->getParam("instrument")->value();
      writeFile(SPIFFS, "/instrument.txt", instrument.c_str());
      Serial.print("instrument: ");
      Serial.println(instrument);
    }
    if (request->hasParam("set_sv"))
    {
      instrument = request->getParam("set_sv")->value();
      writeFile(SPIFFS, "/set_sv.txt", set_sv.c_str());
      Serial.print("Set SV Value: ");
      Serial.println(set_sv);
    }
    request->send(SPIFFS, "/modbus.html", String(), false, processor);
  });
  // Send a GET request to /ftp-config?server=<server-id>&user1=<user1>&pass1=<pass1>&user2=<user2>&pass2=<pass2>&time=<time>
  server.on("/ftp-config", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("server"))
    {
      ftp_server = request->getParam("server")->value();
      writeFile(SPIFFS, "/ftp_server.txt", ftp_server.c_str());
      Serial.print("Server IP: ");
      Serial.println(ftp_server);
    }
    if (request->hasParam("user1"))
    {
      user1 = request->getParam("user1")->value();
      writeFile(SPIFFS, "/user1.txt", user1.c_str());
      Serial.print("Username 1: ");
      Serial.println(user1);
    }
    if (request->hasParam("pass1"))
    {
      pass1 = request->getParam("pass1")->value();
      writeFile(SPIFFS, "/pass1.txt", pass1.c_str());
      Serial.print("Password 1: ");
      Serial.println(pass1);
    }
    if (request->hasParam("user2"))
    {
      instrument = request->getParam("user2")->value();
      writeFile(SPIFFS, "/user2.txt", user2.c_str());
      Serial.print("Username 2: ");
      Serial.println(user2);
    }
    if (request->hasParam("pass2"))
    {
      pass2 = request->getParam("pass2")->value();
      writeFile(SPIFFS, "/pass2.txt", pass2.c_str());
      Serial.print("Password 2: ");
      Serial.println(pass2);
    }
    if (request->hasParam("time_update_ftp"))
    {
      time_update_ftp = request->getParam("time_update_ftp")->value();
      writeFile(SPIFFS, "/time_update_ftp.txt", time_update_ftp.c_str());
      Serial.print("Update time FTP: ");
      Serial.println(time_update_ftp);
    }

    request->send(SPIFFS, "/ftp.html", String(), false, processor);
  });

  //Route to device config
  server.on("/device-config", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("time_update_sd"))
    {
      time_update_sd = request->getParam("time_update_sd")->value();
      writeFile(SPIFFS, "/time_update_sd.txt", time_update_sd.c_str());
      Serial.print("Time update SD: ");
      Serial.println(time_update_sd);
    }
    if (request->hasParam("sv"))
    {
      set_sv = request->getParam("sv")->value();
      writeFile(SPIFFS, "/set_sv.txt", set_sv.c_str());
      Serial.print("Set SV value: ");
      Serial.println(set_sv);// response: 01 03 02 00 1c b9 8d
      setDone = true;
      request->send(SPIFFS, "/device.html", String(), false, processor);
      //01 06 0001 0258 D890 set 600 do
    }
  });
  server.on("/wf-config", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("wf_ssid"))
    {
      wf_ssid = request->getParam("wf_ssid")->value();
      writeFile(SPIFFS, "/wf_ssid.txt", wf_ssid.c_str());
      Serial.print("Wifi SSID: ");
      Serial.println(wf_ssid);
    }
    if (request->hasParam("wf_password"))
    {
      wf_password = request->getParam("wf_password")->value();
      writeFile(SPIFFS, "/wf_password.txt", wf_password.c_str());
      Serial.print("Wifi Password: ");
      Serial.println(wf_password);// response: 01 03 02 00 1c b9 8d
      request->send(SPIFFS, "/wifi.html", String(), false, processor);
      //01 06 0001 0258 D890 set 600 do
    }
  });

  server.on("/setting", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasParam("run"))
    {
      isRun = request->getParam("run")->value();
      //writeFile(SPIFFS, "/run.txt", isRun.c_str());
      Serial.print("Run: ");
      Serial.println(isRun);
    }
    if (request->hasParam("choose-user"))
    {
      choose_user = request->getParam("choose-user")->value();
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/sv", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", sv.c_str());
  });
  server.on("/pv", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", pv.c_str());
  });
  // Start server
  server.onNotFound(notFound);
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  if (eth_connected == true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" wifi connected ");
    lcd.setCursor(0, 1);
    lcd.println(ETH.localIP().toString().c_str());
  }
  if ( (onWIFI == true) || (eth_connected == true))
  {
    onServer = true;
    server.begin();
    onServer = true;
  }

}

void loop()
{
  bool pushButton_1Val = pushButton_1.onPress();
  bool pushButton_2Val = pushButton_2.onPress();
  if ( (unsigned long) (millis() - time1) > 7000 )
  {
    onDisplay();
    time1 = millis();
  }
  unsigned long currentMillis = millis();
  interval_ftp = time_update_ftp.toInt() * 10000; //60000
  interval_sd = time_update_sd.toInt() * 10000 + 1000;
  if (isRun == "on") {
    //SV
    for (i = 0 ; i < 8 ; i++) {
      Serial1.write(msg_sv[i]);
    }
    int a = 0;
    while (Serial1.available())
    {
      ByteArray1[a] = Serial1.read();
      a++;
    }
    int Data1;
    Data1 = ByteArray1[3] * 256 + ByteArray1[4];
    sv = String(Data1);
    delay(100);


    //PV
    for (i = 0 ; i < 8 ; i++) {
      Serial1.write(msg_pv[i]);
    }
    int c = 0;
    while (Serial1.available())
    {
      ByteArray2[c] = Serial1.read();
      c++;
    }
    int Data2;
    Data2 = ByteArray2[3] * 256 + ByteArray2[4];
    pv = String(Data2);
    delay(100);


    //upload file to server
    if ((currentMillis - previousMillis_ftp >= interval_ftp) && (openFile == false)) {
      openFile = true;
      sdFile = SD.open("/420ma.txt", FILE_READ);
      if (sdFile) {
        String fileContent;
        while (sdFile.available()) {
          fileContent += String((char)sdFile.read());
        }
        sdFile.close();
        //        Serial.println("READ FILE 420ma.txt:");
        //        Serial.println(fileContent);
        String user, pass;
        if (choose_user == "1") {
          user = user1;
          pass = pass1;
        } else {
          user = user2;
          pass = pass2;
        }
        //update server
        if (gMode == false) {
          int ftp_len = ftp_server.length() + 1;
          char ftp_array[ftp_len];
          ftp_server.toCharArray(ftp_array, ftp_len);
          //update user
          int user_len = user.length() + 1;
          char user_array[user_len];
          user.toCharArray(user_array, user_len);
          //update pass
          int pass_len = pass.length() + 1;
          char pass_array[pass_len];
          pass.toCharArray(pass_array, pass_len);
          ESP32_FTPClient ftp (ftp_array, user_array, pass_array);
          ftp.OpenConnection();
          ftp.ChangeWorkDir("/");
          ftp.InitFile("Type A");
          ftp.NewFile("420ma.txt");
          ftp.Write(fileContent.c_str());
          ftp.CloseFile();
          ftp.CloseConnection();
          Serial.println("UPLOAD TO SERVER DONE");
        } else {
          Serial2.println("AT+CFTPPORT=21");
          delay(1000);
          Serial2.println("AT+CFTPMODE=1");
          delay(1000);
          Serial2.println("AT+CFTPTYPE=A");
          delay(1000);
          Serial2.println("AT+CFTPSERV=\"103.97.125.251\"");
          delay(1000);
          Serial2.println("AT+CFTPUN=\"nguyendinhdung@lhnam.net\"");
          delay(1000);
          Serial2.println("AT+CFTPPW=\"nguyendinhdung\"");
          delay(1000);
          Serial2.println("AT+CFTPPUT=\"/uploadbysim.txt\",0");
          delay(5000);
          Serial2.println(fileContent);
          delay(1000);
          Serial2.print(26, HEX); //print 0x1A
        }
      } else {
        Serial.println("error opening the 420ma.txt,UPLOAD FAIL");
      }
      previousMillis_ftp = currentMillis;
      openFile = false;
    }
    //write file to sd
    if ((currentMillis - previousMillis_sd >= interval_sd) && (openFile == false)) {
      //do something
      openFile = true;
      sdFile = SD.open("/420ma.txt", FILE_READ);
      if (sdFile) {
        fileContent = "";
        while (sdFile.available()) {
          fileContent += String((char)sdFile.read());
        }
        sdFile.close();
      }
      delay(100);
      sdFile = SD.open("/420ma.txt", FILE_WRITE);
      // if the file is available, write to it:
      if (sdFile) {
        DateTime now = rtcDS.now();
        String upload_data;
        upload_data = sv + "|" + pv + "|";
        upload_data += now.hour();
        upload_data += "|";
        upload_data += now.minute();
        upload_data += "|";
        upload_data += now.day();
        upload_data += "|";
        upload_data += now.month();
        upload_data += "|";
        upload_data += now.year();
        sdFile.print(fileContent);
        sdFile.println(upload_data);
        sdFile.close();
        Serial.println("Write to sd:");
        //        Serial.print(fileContent);
        //        Serial.println(upload_data);
      }    else {
        // if the file didn't open, print an error
        Serial.println(F("error opening file 420ma.txt, WRITE FAIL"));
      }
      previousMillis_sd = currentMillis;
      openFile = false;
    }
  }
  if (setDone == true) {
    setDone = false;
    String set_sv_hex = decToHex(set_sv.toInt());
    msg_set_sv[4] = StrtoByte(set_sv_hex.substring(0, 2));
    msg_set_sv[5] = StrtoByte(set_sv_hex.substring(2, 4));

    String msg_sv_string = "01060001";
    msg_sv_string = msg_sv_string + set_sv_hex;
    String calculated_crc = ModRTU_CRC(msg_sv_string);
    msg_set_sv[6] = StrtoByte(calculated_crc.substring(0, 2));
    msg_set_sv[7] = StrtoByte(calculated_crc.substring(2, 4));
    msg_sv_string = msg_sv_string + calculated_crc;
    Serial.print("Set sv value massage:");
    Serial.println(msg_sv_string);
    for (int i = 0 ; i < 8; i++) {
      Serial1.write(msg_set_sv[i]);
    }
  }
  if ( pushButton_1Val == true) {
    //4G Mode, Sending data with 4G
    if (gMode == true) {
      gMode = false;
      Serial.println();
      Serial.println("4G MODE: OFF");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("4G MODE: OFF");
    } else {
      gMode = true;
      Serial.println();
      Serial.println("4G MODE: ON");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("4G MODE: ON");
    }
  }
  if ( pushButton_2Val == true) {
    //Reset
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  Rebooting...");
    ESP.restart();
  }
  AsyncElegantOTA.loop();
}

void onDisplay() {
  lcd.setCursor(0, 0);
  lcd.print("LAN: -SD: -RTC: ");                   // Print print String to LCD on first line
  //lcdI2C.selectLine(2);                    // Set cursor at the begining of line 2
  lcd.setCursor(0, 1);
  lcd.print("SIM: -WIFI: -W:");
  if (onRTS == false) {
    lcd.setCursor(15, 0);
    lcd.print("E");
  }
  else {
    lcd.setCursor(15, 0);
    lcd.print("A");
  }
  if (onSD == false) {
    lcd.setCursor(9, 0);
    lcd.print("E");
  }
  else {
    lcd.setCursor(9, 0);
    lcd.print("A");
  }
  if (onWIFI == false) {
    lcd.setCursor(11, 1);
    lcd.print("E");
  }
  else {
    lcd.setCursor(11, 1);
    lcd.print("A");
  }
  if (eth_connected == false) {
    lcd.setCursor(4, 0);
    lcd.print("E");
  }
  else {
    lcd.setCursor(4, 0);
    lcd.print("A");
  }
  if (onServer == false) {
    lcd.setCursor(15, 1);
    lcd.print("E");
  }
  else {
    lcd.setCursor(15, 1);
    lcd.print("A");
  }
  if (gMode == false) {
    lcd.setCursor(4, 1);
    lcd.print("E");
  }
  else {
    lcd.setCursor(4, 1);
    lcd.print("A");
  }
}

uint32_t serialConfigFromString(const char* config) {
  if ( 0 == strcmp(config, "SERIAL_8N1"))
    return SERIAL_8N1;
  else if ( 0 == strcmp(config, "SERIAL_8N2"))
    return SERIAL_8N2;
  else if ( 0 == strcmp(config, "SERIAL_8N2"))
    return SERIAL_8N2;
}
