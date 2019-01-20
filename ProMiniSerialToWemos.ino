unsigned long startPress;

void setup() {
  pinMode(11, INPUT_PULLUP);
  Serial.begin(115200);
}

void rotaryPushButton(){
  if (digitalRead(11) == LOW){
    startPress = millis();
    while (digitalRead(11) == LOW){
      delay(10);
    }
    if (millis() - startPress > 500){
      Serial.println(" longPress");
    } else {
      Serial.println(" shortPress");
    }
    startPress = 0;
  }
}

void loop() {
  rotaryPushButton();
}


