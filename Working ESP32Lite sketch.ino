#include <WiFi.h>
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
#include <Wire.h>

#define TFT_CS     25
#define TFT_RST    5
#define TFT_DC     33

#define BACKCOLOR 0x0000 
#define PRINTCOLOR 0xFFFF
#define CHARBACK 0x001F

#define outputA 16
#define outputB 4
// Button is connected to Pro Mini

int AG[20];
String AG1;
int SSN[20];
String SSN1;
int SV[20];
String SV1;
int PN[20];
String PN1;
String t1;
String h1;
int marks[5] = {2, 1, 3, 5, 2};
// Used to define which screen is displayed 1: Bar graph, 2; geheugen en gedinges, etcetera
int screen;
int inBar;
int inMonitor;
int barLength;
int yVal;
int width;
int color;
int color1;
int color2;
int color3;
int color4;
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
int vibSensor = 34;
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
String curHour = "13";
int oldRead;
int newRead;
CircularBuffer<int,20> moveReadings;
CircularBuffer<int,4> moveTypes;
int maxMove;
int amountOfShifts;
int amountOfInactivity;
float ratio;
int shakings = 50;
int prevShakings = 50;
int totalMovements;
int oldTotalMovements;
int smallMovements = 60;
int normalMovements = 80;
int largeMovements = 70;
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
int avgh[24] = {50,10,10,20,5,9,10,20,30,40,50,60,70,60,50,40,30,20,20,30,40,20, 40, 20};
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
int newTimePart;
unsigned long startPress;
uint32_t tsLastReport = 0;
int inWalk;
int walkCounter;
int prevWalkCounter;
unsigned long lastBeepTime = 0;
unsigned long lastBeepingMoment = 0;
unsigned long lastSilentMoment = 0;
unsigned long beepStarted;
unsigned long delayStarted;
int percentage;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

// const char* ssid = "WemosTestNetwork";
// const char* password = "test1234";
const char* ssid = "Jochem's wifi";
const char* password = "Snip238!!!";
// const char* ssid = "KraanBast2.4";
//const char* password = "Snip238!";
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
      timeClient.update();
      setTime(String(timeClient.getFormattedTime()).substring(0,2).toInt()+1, String(timeClient.getFormattedTime()).substring(3,5).toInt(), 0, 0, 0, 2019);
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
  if (message.startsWith("Marks: ")){ // Marks: 1,2,3,4
    marks[1] = message[7,8];
    marks[2] = message[9,10];
    marks[3] = message[11,12];
    marks[4] = message[13,14];
  }
  message = "";  
}

void setup() {
  pinMode (BUILTIN_LED, OUTPUT);
  pinMode (outputA,INPUT);
  pinMode (outputB,INPUT);
  pinMode(35, INPUT);
  pinMode(15, INPUT_PULLUP);
  pinMode(27,OUTPUT);
  Wire.begin(18,19, 1000000);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  setup_wifi();
  lastReconnectAttempt = 0;
  aLastState = digitalRead(outputA);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  pinMode(vibSensor, INPUT);
  setTime(13,42,0,3,1,2019);
  Serial.begin(9600);
  tft.fillScreen(PRINTCOLOR);
  drawBarChart(avgh[(newTime.substring(0,2)).toInt()], avgh[(newTime.substring(0,2)).toInt()+1], avgh[(newTime.substring(0,2)).toInt()+2], avgh[(newTime.substring(0,2)).toInt()+3], avgh[(newTime.substring(0,2)).toInt()+4], avgh[(newTime.substring(0,2)).toInt()+5]);
  realTimeRatioUpdate();
  screen = 1;
  timeClient.begin();
  dispCurPercentage();
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
  tft.print("Overgeven");
  tft.setCursor(2,35);
  tft.print("Verstopt");
  tft.setCursor(2,45);
  tft.print("Maag");
  tft.setCursor(2,55);
  tft.print("Darm");
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
  tft.print("Schokken");
  tft.setCursor(2,35);
  tft.print("Tremoren");
  tft.setCursor(2,45);
  tft.print("Hoofdpijn");
  tft.setCursor(2,55);
  tft.print("Stijfheid");
}

void walkingScreen(){
  if (inWalk == 1){
    if (walkCounter != prevWalkCounter){
      tft.fillScreen(PRINTCOLOR);
      dispCurTime(130,3);
      prevWalkCounter = walkCounter;
      tft.fillRect(40,10,60,40,PRINTCOLOR);
      tft.setTextColor(BACKCOLOR);
      tft.setCursor(2,13);
      tft.setTextSize(1);
      tft.print("Stel hier het interval in waarop wordt gepiept");
      tft.setTextSize(2);
      tft.setCursor(60,40);
      tft.setTextColor(BACKCOLOR);
      tft.print(float(float(walkCounter) / float(4)));
      tft.setTextSize(1);
      tft.setTextColor(PRINTCOLOR);
    }
  }
}

void beeper(){
  if (inWalk == 1){
    if (millis() > lastBeepTime + (float(float(walkCounter) / float(4)) * 1000)){
      lastBeepTime = millis();
      digitalWrite(27,HIGH);
      beepStarted = millis();
    }
  }
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

void dispCurPercentage(){
  tft.setCursor(3,4);
  tft.setTextColor(BACKCOLOR);
  percentage = (analogRead(35) - 1800) * 0.17;
  tft.fillRect(2,3,20,10,PRINTCOLOR);
  if (percentage < 100 and percentage > 0){
    tft.print(percentage);
    tft.setCursor(17,4);
    tft.print("%");
  } else if (percentage > 100){
    tft.print("Opladen");
  } else {
    tft.print("Acculoos");
  }
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
      barCounter = 18;
    } else {
      width = 0;
      barCounter = 0;
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
  if ((newRead < 3600 and oldRead > 3600) or (newRead > 3600 and oldRead < 3600)){
    moveReadings.unshift(1);
  } else if (newRead < 3600 and oldRead < 3600){// No movement or consistant periods of activity and inactivity (i.e. walking)
    moveReadings.unshift(0);
  } else if (newRead > 3600 and oldRead > 3600){// No movement or consistant periods of activity and inactivity (i.e. walking)
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
    tft.fillRect(144, 6, 15, int(float(startBar) / float(maxAmountOfMovementsInHour) * float(60)), 0xFFE0);
  } else {
    tft.fillRect(144, 6, 15, int(float(startBar) / float(maxAmountOfMovementsInHour) * float(60)), 0xF81F);
  }
  if (secondBar > 30){
    tft.fillRect(127, 6, 15, int(float(secondBar) / float(maxAmountOfMovementsInHour) * float(60)), 0xFFE0);
  } else {
    tft.fillRect(127, 6, 15, int(float(secondBar) / float(maxAmountOfMovementsInHour) * float(60)), 0xF81F);
  }
  if (thirdBar > 30){
    tft.fillRect(110, 6, 15, int(float(thirdBar) / float(maxAmountOfMovementsInHour) * float(60)), 0xFFE0);
  } else {
    tft.fillRect(110, 6, 15, int(float(thirdBar) / float(maxAmountOfMovementsInHour) * float(60)), 0xF81F);
  }
  if (fourthBar > 30){
    tft.fillRect(93, 6, 15, int(float(fourthBar) / float(maxAmountOfMovementsInHour) * float(60)), 0xFFE0);
  } else {
    tft.fillRect(93, 6, 15, int(float(fourthBar) / float(maxAmountOfMovementsInHour) * float(60)), 0xF81F);
  }
  if (fifthBar > 30){
    tft.fillRect(76, 6, 15, int(float(fifthBar) / float(maxAmountOfMovementsInHour) * float(60)), 0xFFE0);
  } else {
    tft.fillRect(76, 6, 15, int(float(fifthBar) / float(maxAmountOfMovementsInHour) * float(60)), 0xF81F);
  }
  tft.setRotation(3);
  tft.setTextColor(PRINTCOLOR);
  tft.setCursor(5,70);
  tft.print((newTime.substring(0,2)).toInt());
  tft.setCursor(22,70);
  tft.print((newTime.substring(0,2)).toInt() + 1);
  tft.setCursor(39,70);
  tft.print((newTime.substring(0,2)).toInt() + 2);
  tft.setCursor(56,70);
  tft.print((newTime.substring(0,2)).toInt() + 3);
  tft.setCursor(73,70);
  tft.print((newTime.substring(0,2)).toInt() + 4);
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
   } else if (aState != aLastState){
    if (digitalRead(outputB) != aState) { 
       barCounter ++;
     } else {
       barCounter --;
     }
   }
   aLastState = aState;
}

void defineBarLocation(){
// barCounter is defined as an int to determine which question is answered
  if (questionCounter != prevCounter){
    yVal = questionCounter * 10;
    prevCounter = questionCounter;
    drawQuestionBars();
    if (counter == 4){
      AG[questionCounter] = widthValue;
    } else if (counter == 6){
      SSN[questionCounter] = widthValue;
    } else if (counter == 8){
      SV[questionCounter] = widthValue;
    } else if (counter = 10){
      PN[questionCounter] = widthValue;
    }
    // First question of each screen has got to be ignored. So just add that to the first index and then never do anything with it anymore...
  }
  if (questionCounter == 7 and screen == 10){
    counter = 2;
    screen = 2;
    inMonitor = 0;
    inBar = 0;
    tft.fillScreen(PRINTCOLOR);
    tft.setCursor(3,4);
    tft.setTextColor(0xFFE0);
    if (client.connected()){
      tft.print("Uploaden begonnen");
    } else {
      tft.print("Antwoorden opslaan");
    }
    AG1.concat("AG: ");
    for (int i = 1; i < 20; i++){
      AG1.concat(AG[i]);
      AG1.concat(",");
    }
    SSN1.concat("SSN: ");
    for (int i = 1; i < 20; i++){
      SSN1.concat(SSN[i]);
      SSN1.concat(",");
    }
    SV1.concat("SV: ");
    for (int i = 1; i < 20; i++){
      SV1.concat(SV[i]);
      SV1.concat(",");
    }
    PN1.concat("PN: ");
    for (int i = 1; i < 20; i++){
      PN1.concat(PN[i]);
      PN1.concat(",");
    }
    tft.setCursor(3,4);
    dispCurTime(130,3);
    tft.setTextColor(BACKCOLOR);
    tft.setRotation(1);
    tft.fillRect(140, 10, 15, float((float(marks[1]) / float(5)) * float(50)), 0xFFE0);
    tft.fillRect(123, 10, 15, float((float(marks[2]) / float(5)) * float(50)), 0xFFE0);
    tft.fillRect(106, 10, 15, float((float(marks[3]) / float(5)) * float(50)), 0xFFE0);
    tft.fillRect(89, 10, 15, float((float(marks[4]) / float(5)) * float(50)), 0xFFE0);
    tft.setRotation(3);
    tft.setTextColor(PRINTCOLOR);
    tft.setCursor(8,66);
    tft.print("AG");
    tft.setCursor(25,66);
    tft.print("SS");
    tft.setCursor(43,66);
    tft.print("SV");
    tft.setCursor(59,66);
    tft.print("PN");
    tft.setTextColor(BACKCOLOR);
    tft.setCursor(90,20);
    tft.print("Druk om de");
    tft.setCursor(90,30);
    tft.print("monitor in");
    tft.setCursor(90,40);
    tft.print("te vullen.");
    tft.setCursor(90,50);
    tft.print("Houd inged");
    tft.setCursor(90,60);
    tft.print("rukt om te");
    tft.setCursor(90,70);
    tft.print("uploaden.");
    delay(1000);
    client.publish("/Pi/DosiMate",String(AG1).c_str());
    delay(500);
    client.publish("/Pi/DosiMate",String(SSN1).c_str());
    delay(500);
    client.publish("/Pi/DosiMate",String(SV1).c_str());
    delay(500);
    client.publish("/Pi/DosiMate",String(PN1).c_str());
    delay(1000);
    AG1 = "";
    SSN1 = "";
    SV1 = "";
    PN1 = "";
    tft.setCursor(3,4);
    tft.fillRect(3,4,120,12,PRINTCOLOR);
    tft.setTextColor(0xF81F);
    if (client.connected()){
      tft.print("Uploaden voltooid");
    } else {
      tft.print("Opgeslagen");
    }
  } else if (questionCounter == 7){
    questionCounter = 2;
    counter+=2;
  }
}

// QuestionCounter als index voor array en dan de width als waarde

void rotaryButton(){
  if (digitalRead(15) == LOW){
      startPress = millis();
      while (digitalRead(15) == LOW){
        delay(10);
      }
      if (millis() - startPress > 500){
        if (screen == 2){
          tft.fillScreen(PRINTCOLOR);
          tft.setCursor(3,4);
          tft.setTextColor(0xFFE0);
          if (client.connected()){
            tft.print("Uploaden begonnen");
          } else {
            tft.print("Antwoorden opslaan");
          }
          dispCurTime(130,3);
          tft.setTextColor(BACKCOLOR);
          tft.setRotation(1);
          tft.fillRect(140, 10, 15, float((float(marks[1]) / float(5)) * float(50)), 0xFFE0);
          tft.fillRect(123, 10, 15, float((float(marks[2]) / float(5)) * float(50)), 0xFFE0);
          tft.fillRect(106, 10, 15, float((float(marks[3]) / float(5)) * float(50)), 0xFFE0);
          tft.fillRect(89, 10, 15, float((float(marks[4]) / float(5)) * float(50)), 0xFFE0);
          tft.setRotation(3);
          tft.setTextColor(PRINTCOLOR);
          tft.setCursor(8,66);
          tft.print("AG");
          tft.setCursor(25,66);
          tft.print("SS");
          tft.setCursor(43,66);
          tft.print("SV");
          tft.setCursor(59,66);
          tft.print("PN");
          tft.setTextColor(BACKCOLOR);
          tft.setCursor(90,20);
          tft.print("Druk om de");
          tft.setCursor(90,30);
          tft.print("monitor in");
          tft.setCursor(90,40);
          tft.print("te vullen.");
          tft.setCursor(90,50);
          tft.print("Houd inged");
          tft.setCursor(90,60);
          tft.print("rukt om te");
          tft.setCursor(90,70);
          tft.print("uploaden.");
          inWalk = 0;
          t1.concat("Trillingen: 1653,");
          for (int i = 1; i<24; i++){
            tl.concat(t[i]);
            tl.concat(",");
          }
          h1.concat("Bewegingen: 1653,");
          for (int i = 1; i<24; i++){
            hl.concat(h[i]);
            hl.concat(",");
          }
          AG1.concat("AG: 1653,");
          for (int i = 1; i < 20; i++){
            AG1.concat(AG[i]);
            AG1.concat(",");
          }
          SSN1.concat("SSN: 1653,");
          for (int i = 1; i < 20; i++){
            SSN1.concat(SSN[i]);
            SSN1.concat(",");
          }
          SV1.concat("SV: 1653,");
          for (int i = 1; i < 20; i++){
            SV1.concat(SV[i]);
            SV1.concat(",");
          }
          PN1.concat("PN: 1653,");
          for (int i = 1; i < 20; i++){
            PN1.concat(PN[i]);
            PN1.concat(",");
          }
          delay(1000);
          client.publish("/Pi/DosiMate",String(AG1).c_str());
          delay(500);
          client.publish("/Pi/DosiMate",String(SSN1).c_str());
          delay(500);
          client.publish("/Pi/DosiMate",String(SV1).c_str());
          delay(500);
          client.publish("/Pi/DosiMate",String(PN1).c_str());
          delay(500);
          client.publish("/Pi/DosiMate",String(hl).c_str());
          delay(500);
          client.publish("/Pi/DosiMate",String(tl).c_str());
          delay(1000);
          hl = "";
          tl = "";
          AG1 = "";
          SSN1 = "";
          SV1 = "";
          PN1 = "";
          tft.setCursor(3,4);
          tft.fillRect(3,4,120,12,PRINTCOLOR);
          tft.setTextColor(0xF81F);
          if (client.connected()){
            tft.print("Uploaden voltooid");
          } else {
            tft.print("Opgeslagen");
          }
          screen = counter;
        } else {
          counter = 0;
          inBar = 0;
          inMonitor = 0;
        }
      } else {
      startPress = 0;
      if (screen == -2){
        if (walkCounter < 10){
          walkCounter ++;
        } else {
          walkCounter = 1;
        }
      } else if (screen == 2){
        counter = 4;
        questionCounter = 1;
        inBar = 1;
        inMonitor = 1;
      } else if ((screen == 10)and(selecCounter == 18)){
        barCounter = 0;
        inMonitor = 0;
        counter = 0;
        inBar = 0;
        arr_index = 0;
        // Nog upload schermpje en geslaagd met 5 seconden delay en dan counter 0 zodat naar barscherm
       }
       if (inBar == 1){
         questionCounter++; // Go to next line a.k.a. question
         widthValue = width;
         barCounter = 0; // Set length of bar to 0
        }
      }
    }
}

void displayManager(){
  if ((counter != screen)){
    if (counter == 0){
      tft.fillScreen(PRINTCOLOR);
      dispCurTime(130,3);
      updateShowedBars();
      dispCurPercentage();
      realTimeRatioUpdate();
      screen = counter;
      inWalk = 0;
    } else if (counter == -2){
      screen = counter;
      inWalk = 1;
      tft.fillScreen(PRINTCOLOR);
      dispCurTime(130,3);
      prevWalkCounter = walkCounter;
      tft.fillRect(40,10,60,40,PRINTCOLOR);
      tft.setTextColor(BACKCOLOR);
      tft.setCursor(2,13);
      tft.setTextSize(1);
      tft.print("Stel hier het interval in waarop wordt gepiept");
      tft.setCursor(60,40);
      tft.setTextSize(2);
      tft.print(float(float(walkCounter) / float(4)));
      tft.setTextSize(1);
      dispCurPercentage();
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
      tft.setTextColor(PRINTCOLOR);
      tft.setCursor(8,66);
      tft.print("AG");
      tft.setCursor(25,66);
      tft.print("SS");
      tft.setCursor(43,66);
      tft.print("SV");
      tft.setCursor(59,66);
      tft.print("PN");
      tft.setTextColor(BACKCOLOR);
      tft.setCursor(90,20);
      tft.print("Druk om de");
      tft.setCursor(90,30);
      tft.print("monitor in");
      tft.setCursor(90,40);
      tft.print("te vullen.");
      tft.setCursor(90,50);
      tft.print("Houd inged");
      tft.setCursor(90,60);
      tft.print("rukt om te");
      tft.setCursor(90,70);
      tft.print("uploaden.");
      inWalk = 0;
      dispCurPercentage();
    } else if ((counter == 4) and (inMonitor == 1)){
      screen = counter;
      firstQuestionScreen();
      inWalk = 0;
    } else if ((counter == 6) and (inMonitor == 1)){
      screen = counter;
      secondQuestionScreen();
      inWalk = 0;
    } else if ((counter == 8) and (inMonitor == 1)){
      screen = counter;
      thirdQuestionScreen();
      inWalk = 0;
    } else if ((counter == 10) and (inMonitor == 1)){
      screen = counter;
      fourthQuestionScreen();
      inWalk = 0;
    }
  }
}

void loop() {
  if (millis() > time_current_amount + 30000 and inMonitor == 0){ // Wait 30 (non-blocking) seconds
    dispCurTime(130,3);
    dispCurPercentage();
    time_current_amount = millis();
    if (shakings - prevShakings > 50){
      digitalWrite(27,HIGH);
      delay(300);
      digitalWrite(27,LOW);
      delay(300);
      digitalWrite(27,HIGH);
      delay(300);
      digitalWrite(27,LOW);
      prevShakings = shakings;
      tft.fillScreen(PRINTCOLOR);
      tft.setCursor(5,20);
      tft.setTextSize(2);
      tft.print("1ML in 30 min");
      tft.setTextSize(1);
      tft.setCursor(10,60);
      tft.print("Druk om te annuleren");
      delayStarted = millis();
      while (millis() < delayStarted + 20000){
        if (digitalRead(15) == LOW){
          client.publish("/Pi/DosiMate",String("Negeer volgend bericht").c_str());
          tft.setCursor(10,70);
          tft.print("Toediening geannulleerd");
        }
      }
      client.publish("/Pi/DosiMate",String("Toediening: 1ML,30").c_str());
      counter = 0;
      screen = 2;
      tft.setTextSize(1);
    }
    Serial.println(analogRead(35));
    Serial.println((analogRead(35) - 3500) * 0.17);
  }
  if (millis() > time_now_step + 50){ // Wait 50 (non-blocking) milliseconds
    time_now_step = millis();
    detectMove();
  }
  if (millis() > beepStarted + 200){ // Wait 50 (non-blocking) milliseconds
    digitalWrite(27,LOW);
  }
  if (oldTotalMovements != totalMovements){ // Update ratio when a movement occured
    oldTotalMovements = totalMovements;
    realTimeRatioUpdate();
  }
  if (newTime.substring(0,2) != curHour){
    if (screen == 0){ // Only update the bargraph if thats the currently selected screen
      tft.fillRect(2,3,89,80,PRINTCOLOR);
      updateShowedBars();
      dispCurTime(130,3);
      dispCurPercentage();
    }
    t1.concat("Trillingen: 1653,");
    for (int i = 1; i<24; i++){
      tl.concat(t[i]);
      tl.concat(",");
    }
    h1.concat("Trillingen: 1653,");
    for (int i = 1; i<24; i++){
      hl.concat(h[i]);
      hl.concat(",");
    }
    AG1.concat("AG: 1653,");
    for (int i = 1; i < 20; i++){
      AG1.concat(AG[i]);
      AG1.concat(",");
    }
    SSN1.concat("SSN: 1653,");
    for (int i = 1; i < 20; i++){
      SSN1.concat(SSN[i]);
      SSN1.concat(",");
    }
    SV1.concat("SV: 1653,");
    for (int i = 1; i < 20; i++){
      SV1.concat(SV[i]);
      SV1.concat(",");
    }
    PN1.concat("PN: 1653,");
    for (int i = 1; i < 20; i++){
      PN1.concat(PN[i]);
      PN1.concat(",");
    }
    delay(1000);
    client.publish("/Pi/DosiMate",String(AG1).c_str());
    delay(500);
    client.publish("/Pi/DosiMate",String(SSN1).c_str());
    delay(500);
    client.publish("/Pi/DosiMate",String(SV1).c_str());
    delay(500);
    client.publish("/Pi/DosiMate",String(PN1).c_str());
    delay(500);
    client.publish("/Pi/DosiMate",String(hl).c_str());
    delay(500);
    client.publish("/Pi/DosiMate",String(tl).c_str());
    delay(1000);
    hl = "";
    tl = "";
    AG1 = "";
    SSN1 = "";
    SV1 = "";
    PN1 = "";
  }
  if (!client.connected() and (WiFi.status() == WL_CONNECTED)){
    reconnect();
  } else if (client.connected()){ 
    client.loop();
    tft.fillCircle(120,5,3,0xF81F);
    if (millis() > timeRefreshPeriod + 60000 and inBar == 0 and inMonitor == 0 and counter == 0){
      timeRefreshPeriod = millis();
      timeClient.update();
      setTime(String(timeClient.getFormattedTime()).substring(0,2).toInt()+1, String(timeClient.getFormattedTime()).substring(3,5).toInt(), 0, 0, 0, 2019);
    }
  } else if (!client.connected()){
    tft.fillCircle(120,5,3,0xFFE0);
  }
  defineBarLocation();
  rotaryPosition();
  drawQuestionBars();
  displayManager();
  rotaryButton();
  walkingScreen();
  beeper();
}
