#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <CircularBuffer.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <math.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define TFT_CS     D1
#define TFT_RST    D0
#define TFT_DC     D2

#define BACKCOLOR 0x0000 
#define PRINTCOLOR 0xFFFF
#define CHARBACK 0x001F

#define outputA D3
#define outputB D4
// Button is connected to Pro Mini

int AG[20];
String AG1;
int SSN[20];
String SSN1;
int SV[20];
int ST[20];
int PN[20];
int marks[5] = {2, 1, 3, 5, 2};
// Used to define which screen is displayed 1: Bar graph, 2; geheugen en gedinges, etcetera
int screen;
int inBar;
int inMonitor;
int barLength;
int yVal;
int width;
int color;
// Used for serial comunication with Pro Mini
String serialMessage;
char incomingByte;
// Used for rotary encoder
int counter = 0; 
int barCounter;
int lastBarCounter;
int selecCounter;
int aState;
int aLastState;
String buttonState;
// Pin definitions
int vibSensor = A0;
// Used for position of time
int x;
int y;
// Used for non-blocking timers
unsigned long time_current_amount = 0;     // timer for time
unsigned long time_now_step = 0;// timer for steps
unsigned long time_now_step_interval = 0; // timer for interval between steps
unsigned long old_interval = 0;
unsigned long new_interval = 0;
unsigned long time_now_reconnect = 0;
unsigned long time_now_reconnect_mqtt = 0;
unsigned long timeRefreshPeriod = 0;
String message;
// Used for monitoring time
String oldTime;
// All under used for monitoring movement
String curHour;
int oldRead;
int newRead;
CircularBuffer<int,20> moveReadings;
CircularBuffer<int,4> moveTypes;
int maxMove;
int amountOfShifts;
int amountOfInactivity;
float ratio;
int shakings = 5;
int totalMovements;
int oldTotalMovements;
int smallMovements = 5;
int normalMovements = 5;
int largeMovements = 5;
// Ints with amount of movement for current day
int h[24];
// Concatenate ints to this string to send em to pi
String hl;
// Ints with amount of shakings for current day
int t[24];
String tl; // Concatenate ints to this string to send em to pi
// Array with hours in which shakings can occur. Defined by calculations on Raspberry Pi
int shakingRiskHours[24];
// Ints with average amount of movements per hour calculated on Raspberry Pi shown on display as bars
int avgh[24] = {0,0,0,0,0,0,10,20,30,40,50,60,70,60,50,40,30,20,20,30,40,20, 40, 20};
// Ints used for the 6 bars displayed on the display
int maxAmountOfMovementsInHour; // Used for defining 100% of activity
// Used for timekeeping when not connected to internet
unsigned long timeNow; 
unsigned long timeLast;
String newTime;
long lastReconnectAttempt = 0;
int arr_index = 0;
int heartRate;
int SpO2;
int prevBarCounter;
int questionCounter;
int prevCounter;
int widthValue;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

// const char* ssid = "WemosTestNetwork";
// const char* password = "test1234";
// const char* ssid = "Dosimate";
// const char* password = "project123";
const char* ssid = "****";
const char* password = "****";
char* mqtt_server = "192.168.178.74";

WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  if (WiFi.status() != WL_CONNECTED){
    if (millis() - time_now_reconnect_mqtt > 5000) {
      time_now_reconnect_mqtt = millis();
    }
  }
}


void reconnect(){
  if (!client.connected()){
    if (client.connect("ArduinoESP")){
      client.subscribe("/Pi/DosiMate");
    } else {
      if (millis() > lastReconnectAttempt + 5000){
        lastReconnectAttempt = millis();
      }
    }
  }
}

// Listens for incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    message.concat((char)payload[i]);
  }
  if (message.startsWith("Tijd: ")    ){
    newTime = message.substring(0,10);
  } else if(message=="ARM_HOME"){
    client.publish("/woonkamer/alarm", "armed_home");
  }
    message = "";  
}

void setup() {
  pinMode (BUILTIN_LED, OUTPUT);
  pinMode (outputA,INPUT);
  pinMode (outputB,INPUT);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  setup_wifi();
  lastReconnectAttempt = 0;
  aLastState = digitalRead(outputA);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  pinMode(vibSensor, INPUT);
  WiFi.hostname("NL1ESP001");
  setTime(13,42,0,3,1,2019);
  Serial.begin(9600);
  tft.fillScreen(PRINTCOLOR);
  dispCurTime(130,3);
  drawBarChart(avgh[(newTime.substring(0,2)).toInt()], avgh[(newTime.substring(0,2)).toInt()+1], avgh[(newTime.substring(0,2)).toInt()+2], avgh[(newTime.substring(0,2)).toInt()+3], avgh[(newTime.substring(0,2)).toInt()+4], avgh[(newTime.substring(0,2)).toInt()+5]);
  realTimeRatioUpdate();
  screen = 1;
  timeClient.begin();
}

void firstQuestionScreen(){
    selecCounter  = 0;
    tft.fillScreen(PRINTCOLOR);
    dispCurTime(130,3);
    tft.setTextColor(BACKCOLOR);
    tft.setCursor(4,5);
    tft.print("Aandacht/Geheugen");
    tft.setCursor(2,15);
    tft.print("Aandacht");
    tft.setCursor(2,25);
    tft.print("Spraak");
    tft.setCursor(2,35);
    tft.print("Geheugen");
    tft.setCursor(2,45);
    tft.print("Initiatief");
    tft.setCursor(2,55);
    tft.print("Zinsopbouw");
}

void secondQuestionScreen(){
  selecCounter  = 0;
  tft.fillScreen(PRINTCOLOR);
  dispCurTime(130,3);
  tft.setTextColor(BACKCOLOR);
  tft.setCursor(4,5);
  tft.print("Slaapstoornissen");
  tft.setCursor(2,15);
  tft.print("Inslapen");
  tft.setCursor(2,25);
  tft.print("Doorslapen");
  tft.setCursor(2,35);
  tft.print("Ochtendmoe");
  tft.setCursor(2,45);
  tft.print("Dagmoe");
  tft.setCursor(2,55);
  tft.print("Indutten");
}

void thirdQuestionScreen(){
  selecCounter  = 0;
  tft.fillScreen(PRINTCOLOR);
  dispCurTime(130,3);
  tft.setTextColor(BACKCOLOR);
  tft.setCursor(4,5);
  tft.print("Spijsvertering");
  tft.setCursor(2,15);
  tft.print("Slikken");
  tft.setCursor(2,25);
  tft.print("overgeven");
  tft.setCursor(2,35);
  tft.print("Contistipatie");
  tft.setCursor(2,45);
  tft.print("Maagklachten");
  tft.setCursor(2,55);
  tft.print("Darmklachten");
}

void fourthQuestionScreen(){
  selecCounter  = 0;
  tft.fillScreen(PRINTCOLOR);
  dispCurTime(130,3);
  tft.setTextColor(BACKCOLOR);
  tft.setCursor(4,5);
  tft.print("Pijnen");
  tft.setCursor(2,15);
  tft.print("Krampen");
  tft.setCursor(2,25);
  tft.print("Pijnschokken");
  tft.setCursor(2,35);
  tft.print("Tremoren");
  tft.setCursor(2,45);
  tft.print("Hoofdpijn");
  tft.setCursor(2,55);
  tft.print("Stijfheid");
}

void dispCurTime(int x, int y){
  newTime = "";
  newTime.concat(hour());
  printDigits(minute());
  tft.setTextColor(PRINTCOLOR);
  tft.setCursor(x,y);
  tft.print(oldTime);
  tft.setTextColor(BACKCOLOR);
  tft.setCursor(x,y);
  tft.print(newTime);
  oldTime = newTime; 
}

void printDigits(int digits){
  newTime.concat(":");
  if(digits < 10)
    newTime.concat('0');
  newTime.concat(digits);
}

void drawQuestionBars(){
  // Draw the bars based on rotary encoder. So a variable int question with 1 or 0 has got to be added as condition for displayManager()
  if (inBar == 1){
    yVal = questionCounter * 10 - 3; 
    if ((int(float(float(barCounter) / float(10) * float(50))) > 0) and (int(float(float(barCounter) / float(10) * float(50))) < 100)){
      width = int(float(float(barCounter) / float(10) * float(50)));
    } else if (int(float(float(barCounter) / float(10) * float(50))) >= 100){
      width = 95;
    } else {
      width = 0;
    }
    if (width > 66){
      color = 0xFFE0;
    } else if (width > 33){
      color = (0xF81F + 0xFFE0) / 2;
    } else {
      color = 0xF81F;
    }
    if (lastBarCounter != barCounter){
      tft.fillRect(65, yVal, 100, 5, PRINTCOLOR);
      tft.fillRect(65, yVal, width, 5, color);
      lastBarCounter = barCounter;
    }
  }
}

// Detects specific movements and then calls updateHourlyMovements() with amount to add as parameter
void detectMove() {
  newRead = analogRead(vibSensor);
  if ((newRead < 900 and oldRead > 900) or (newRead > 900 and oldRead < 900)){
    moveReadings.unshift(1);
  } else if (newRead < 900 and oldRead < 900){// No movement or consistant periods of activity and inactivity (i.e. walking)
    moveReadings.unshift(0);
  } else if (newRead > 900 and oldRead > 900){// No movement or consistant periods of activity and inactivity (i.e. walking)
    moveReadings.unshift(0);
  }
  oldRead = newRead;
  for (int i=0; i<20; i++){
    if (moveReadings[i] == 1){
      amountOfShifts++;
    } else {
      amountOfInactivity++;
    }
  }
  ratio = (float(amountOfShifts) / float(amountOfInactivity));
  if (ratio > 1.20){
    moveTypes.unshift(4);
  } else if (ratio < 0.30 and ratio > 0.10){
    moveTypes.unshift(2);
  } else if (ratio > 0.30){
    moveTypes.unshift(3);
  } else if (ratio < 0.10 and ratio > 0.04){
    moveTypes.unshift(1);
  }
  if (moveTypes.isFull() == true){
    for (int i=0; i<10; i++) {
      if (moveTypes[i] > maxMove){
        maxMove = moveTypes[i];
      }
    }
    if (maxMove == 4){
      shakings++;
      totalMovements++;
      updateHourlyMovements(maxMove);
    } else if (maxMove == 3){
      largeMovements++;
      totalMovements++;
      updateHourlyMovements(maxMove);
    } else if (maxMove == 2){
      normalMovements++;
      totalMovements++;
      updateHourlyMovements(maxMove);
    } else if (maxMove == 1){
      smallMovements++;
      totalMovements++;
      updateHourlyMovements(maxMove);
    }
    moveTypes.clear();
    maxMove = 0;
  }
  amountOfShifts = 0;
  amountOfInactivity = 0;
  ratio = 0;
}

// Adds specific amount which is defined in detectMove() to the integer with the amount of movements in the current hour
void updateHourlyMovements(int amountToAdd){
  // If amountToAdd is 4, then it is a shaking so we need to add it to the int for the shakings. Not to the int for movements
  if (amountToAdd == 4){
    t[(newTime.substring(0,2)).toInt()] += amountToAdd;
    // Else, if amountToAdd is not 4, it is not a shaking so we need to add it to the ints for movements
  } else {
    h[(newTime.substring(0,2)).toInt()] += amountToAdd;
  }
}

void updateShowedBars(){
  drawBarChart(avgh[(newTime.substring(0,2)).toInt()], avgh[(newTime.substring(0,2)).toInt()+1], avgh[(newTime.substring(0,2)).toInt()+2], avgh[(newTime.substring(0,2)).toInt()+3], avgh[(newTime.substring(0,2)).toInt()+4], avgh[(newTime.substring(0,2)).toInt()+5]);;
  curHour = newTime.substring(0,2);
}

void drawBarChart(int startBar, int firstBar, int secondBar, int thirdBar, int fourthBar, int fifthBar){
  maxAmountOfMovementsInHour = 0;
  if (startBar > maxAmountOfMovementsInHour){
    maxAmountOfMovementsInHour = startBar;
  }
  if (secondBar > maxAmountOfMovementsInHour){
    maxAmountOfMovementsInHour = secondBar;
  }
  if (thirdBar > maxAmountOfMovementsInHour){
    maxAmountOfMovementsInHour = thirdBar;
  }
  if (fourthBar > maxAmountOfMovementsInHour){
    maxAmountOfMovementsInHour = fourthBar;
  }
  if (fifthBar > maxAmountOfMovementsInHour){
    maxAmountOfMovementsInHour = fifthBar;
  }
  tft.setRotation(1);
  if (startBar > 30){
    tft.fillRect(144, 6, 15, int(float(startBar) / float(maxAmountOfMovementsInHour) * float(80)), 0xFFE0);
  } else {
    tft.fillRect(144, 6, 15, int(float(startBar) / float(maxAmountOfMovementsInHour) * float(80)), 0xF81F);
  }
  if (secondBar > 30){
    tft.fillRect(127, 6, 15, int(float(secondBar) / float(maxAmountOfMovementsInHour) * float(80)), 0xFFE0);
  } else {
    tft.fillRect(127, 6, 15, int(float(secondBar) / float(maxAmountOfMovementsInHour) * float(80)), 0xF81F);
  }
  if (thirdBar > 30){
    tft.fillRect(110, 6, 15, int(float(thirdBar) / float(maxAmountOfMovementsInHour) * float(80)), 0xFFE0);
  } else {
    tft.fillRect(110, 6, 15, int(float(thirdBar) / float(maxAmountOfMovementsInHour) * float(80)), 0xF81F);
  }
  if (fourthBar > 30){
    tft.fillRect(93, 6, 15, int(float(fourthBar) / float(maxAmountOfMovementsInHour) * float(80)), 0xFFE0);
  } else {
    tft.fillRect(93, 6, 15, int(float(fourthBar) / float(maxAmountOfMovementsInHour) * float(80)), 0xF81F);
  }
  if (fifthBar > 30){
    tft.fillRect(76, 6, 15, int(float(fifthBar) / float(maxAmountOfMovementsInHour) * float(80)), 0xFFE0);
  } else {
    tft.fillRect(76, 6, 15, int(float(fifthBar) / float(maxAmountOfMovementsInHour) * float(80)), 0xF81F);
  }
  tft.setRotation(3);
  tft.setTextColor(PRINTCOLOR);
  tft.setCursor(5,70);
  tft.print((newTime.substring(0,2)).toInt());
  tft.setCursor(22,70);
  tft.print((newTime.substring(0,2)).toInt()+1);
  tft.setCursor(39,70);
  tft.print((newTime.substring(0,2)).toInt()+2);
  tft.setCursor(56,70);
  tft.print((newTime.substring(0,2)).toInt()+3);
  tft.setCursor(73,70);
  tft.print((newTime.substring(0,2)).toInt()+4);
}

void realTimeRatioUpdate(){
  for (int i = 1; i<24; i++){
      tl.concat(t[i]);
      tl.concat(",");
    }
  for (int i = 1; i<24; i++){
    hl.concat(h[i]);
    hl.concat(",");
  }
  client.publish("/Pi/DosiMate",(("Bewegingen: ",hl).c_str()));
  client.publish("/Pi/DosiMate",(("Trillingen: ",tl).c_str()));
  hl = "";
  tl = "";
  if (counter == 0 and inBar == 0 and inMonitor == 0){
    tft.fillRect(90, 10, 70, 80, PRINTCOLOR);
    tft.setRotation(1);
    tft.fillRect(4, 6, 15, (int(float(float(shakings) / ((float(smallMovements) + float(normalMovements) + float(largeMovements) + float(shakings)))) * float(70))), 0xFFE0);
    tft.fillRect(21, 6, 15, (int(float(float(largeMovements) / ((float(smallMovements) + float(normalMovements) + float(largeMovements) + float(shakings)))) * float(70))), 0xFFE0);
    tft.fillRect(38, 6, 15, (int(float(float(normalMovements) / ((float(smallMovements) + float(normalMovements) + float(largeMovements) + float(shakings)))) * float(70))), 0xFFE0);
    tft.fillRect(55, 6, 15, (int(float(float(smallMovements) / ((float(smallMovements) + float(normalMovements) + float(largeMovements) + float(shakings)))) * float(70))), 0xFFE0);
    tft.setRotation(3);
  }
  tft.setTextColor(PRINTCOLOR);
  tft.setCursor(96, 70);
  tft.print("K");
  tft.setCursor(113, 70);
  tft.print("N"); // Goed
  tft.setCursor(130, 70);
  tft.print("G"); // Goed
  tft.setCursor(147, 70);
  tft.print("T"); // Goed
}

void rotaryPosition(){
  aState = digitalRead(outputA);
   if ((aState != aLastState) and (inBar != 1)){     
     if (digitalRead(outputB) != aState) { 
       counter ++;
     } else {
       counter --;
     }
     Serial.print("Position: ");
     Serial.println(counter);
   } else if (aState != aLastState){
    if (digitalRead(outputB) != aState) { 
       barCounter ++;
     } else {
       barCounter --;
     }
     Serial.print("Position: ");
     Serial.println(counter);
   }
   aLastState = aState;
}

void serialMessages(){
  if (Serial.available() > 0) { 
    while (Serial.available() > 0){
      incomingByte = char(Serial.read());
      serialMessage.concat(String(incomingByte));
    }
  }
  if (serialMessage != ""){
    Serial.println(serialMessage);
    if (String(serialMessage) == " 1" or String(serialMessage) == "1" or serialMessage == " 1" or serialMessage == "1"){
      if (screen == 2){
        counter = 4;
        questionCounter = 1;
        inBar = 1;
        inMonitor = 1;
      } else if ((screen == 10)and(selecCounter == 18)){
        barCounter = 0;
        inMonitor = 0;
        counter = 0;
        delay(500);
        inBar = 0;
        client.publish("/Pi/DosiMate",String(AG[1]).c_str());
        client.publish("/Pi/DosiMate",String(AG[2]).c_str());
        client.publish("/Pi/DosiMate",String(AG[3]).c_str());
        client.publish("/Pi/DosiMate",String(SSN[1]).c_str());
        client.publish("/Pi/DosiMate",String(SSN[2]).c_str());
        client.publish("/Pi/DosiMate",String(SSN[3]).c_str());
        arr_index = 0;
        // Nog upload schermpje en geslaagd met 5 seconden delay en dan counter 0 zodat naar barscherm
      }
      if (inBar == 1){
        questionCounter++; // Go to next line a.k.a. question
        widthValue = width;
        barCounter = 0; // Set length of bar to 0
      }
    } else if (String(serialMessage) == " 2" or String(serialMessage) == "2" or serialMessage == " 2" or serialMessage == "2"){
      counter = 0;
      inBar = 0;
      inMonitor = 0;
    } else if (serialMessage.startsWith("HZ")){
      tft.fillScreen(BACKCOLOR);
      heartRate = (String(serialMessage).substring(2,4)).toInt();
      SpO2 = (String(serialMessage).substring(5,7)).toInt();
      client.publish("/Pi/DosiMate", String(heartRate).c_str());
      client.publish("/Pi/DosiMate", String(SpO2).c_str());
    }
  }
  serialMessage = "";
}

void defineBarLocation(){
// barCounter is defined as an int to determine which question is answered
  if (questionCounter != prevCounter){
    yVal = questionCounter * 10;
    prevCounter = questionCounter;
    drawQuestionBars();
    if (counter == 4){
      client.publish("/Pi/DosiMate",String("AG").c_str());
    } else if (counter == 6){
      client.publish("/Pi/DosiMate",String("SSN").c_str());
    } else if (counter == 8){
      client.publish("/Pi/DosiMate",String("SV").c_str());
    } else if (counter = 10){
      client.publish("/Pi/DosiMate",String("PN").c_str());
    }
    client.publish("/Pi/DosiMate",String(widthValue).c_str());

    // First question of each screen has got to be ignored. So just add that to the first index and then never do anything with it anymore...
  }
  if (questionCounter == 7 and screen == 10){
    counter = 0;
    inMonitor = 0;
    inBar = 0;
  } else if (questionCounter == 7){
    questionCounter = 2;
    counter+=2;
  }
}

// QuestionCounter als index voor array en dan de width als waarde

void displayManager(){
  if ((counter != screen)){
    if (counter == 0){
      tft.fillScreen(PRINTCOLOR);
      dispCurTime(130,3);
      updateShowedBars();
      realTimeRatioUpdate();
      screen = counter;
    } else if (counter == 2){
      screen = counter;
      tft.fillScreen(PRINTCOLOR);
      dispCurTime(130,3);
      tft.setTextColor(BACKCOLOR);
      tft.setRotation(1);
      tft.fillRect(140, 10, 15, float((float(marks[1]) / float(5)) * float(50)), 0xFFE0);
      tft.fillRect(123, 10, 15, float((float(marks[2]) / float(5)) * float(50)), 0xFFE0);
      tft.fillRect(106, 10, 15, float((float(marks[3]) / float(5)) * float(50)), 0xFFE0);
      tft.fillRect(89, 10, 15, float((float(marks[4]) / float(5)) * float(50)), 0xFFE0);
      tft.setRotation(3);
    } else if ((counter == 4) and (inMonitor == 1)){
      screen = counter;
      firstQuestionScreen();
    } else if ((counter == 6) and (inMonitor == 1)){
      screen = counter;
      secondQuestionScreen();
    } else if ((counter == 8) and (inMonitor == 1)){
      screen = counter;
      thirdQuestionScreen();
    } else if ((counter == 10) and (inMonitor == 1)){
      screen = counter;
      fourthQuestionScreen();
    }
  }
}

void loop() {
  if (millis() > time_current_amount + 30000){ // Wait 30 (non-blocking) seconds
    dispCurTime(130,3);
    time_current_amount = millis();
  }
  if (millis() > time_now_step + 50){ // Wait 50 (non-blocking) milliseconds
    time_now_step = millis();
    detectMove();
  }
  if (oldTotalMovements != totalMovements){ // Update ratio when a movement occured
    oldTotalMovements = totalMovements;
    realTimeRatioUpdate();
  }
  if (newTime.substring(0,2) != curHour){
    if (screen == 0){ // Only update the bargraph if thats the currently selected screen
      tft.fillRect(2,3,89,80,PRINTCOLOR);
      updateShowedBars();
    }
    for (int i = 1; i<24; i++){
      tl.concat(t[i]+",");
    }
    for (int i = 1; i<24; i++){
      hl.concat(h[i]+",");
    }
    client.publish("/Pi/DosiMate",hl.c_str());
    client.publish("/Pi/DosiMate",tl.c_str());
    hl = "";
    tl = "";
  }
  if (!client.connected() and (WiFi.status() == WL_CONNECTED)){
    reconnect();
  } else if (client.connected()){
    client.loop();
    if (millis() > timeRefreshPeriod + 60000 and inBar == 0 and inMonitor == 0){
      timeRefreshPeriod = millis();
      timeClient.update();
      setTime(String(timeClient.getFormattedTime()).substring(0,2).toInt()+1, String(timeClient.getFormattedTime()).substring(3,5).toInt(), 0, 0, 0, 2019);
    }
  }
  defineBarLocation();
  rotaryPosition();
  drawQuestionBars();
  displayManager();
  serialMessages();
}
