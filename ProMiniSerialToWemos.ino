#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS     30000
unsigned long startPress;

PulseOximeter pox;

uint32_t tsLastReport = 0;

void onBeatDetected()
{
   // Serial.println("Beat!");
}

void setup()
{
    Serial.begin(9600);
    pinMode(7, INPUT_PULLUP);
    Serial.print("Initializing pulse oximeter..");

    // Initialize the PulseOximeter instance
    // Failures are generally due to an improper I2C wiring, missing power supply
    // or wrong target chip
    if (!pox.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }

    // The default current for the IR LED is 50mA and it could be changed
    //   by uncommenting the following line. Check MAX30100_Registers.h for all the
    //   available options.
    // pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

    // Register a callback for the beat detection
    pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop()
{
    // Make sure to call update as fast as possible
    pox.update();

    // Asynchronously dump heart rate and oxidation levels to the serial
    // For both, a value of 0 means "invalid"
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        Serial.println("Heart rate:" + String(pox.getHeartRate()) + "bpm / SpO:" + String(pox.getSpO2()) + "%");
        tsLastReport = millis();
    }
    if (digitalRead(7) == LOW){
      pox.shutdown();
      startPress = millis();
      while (digitalRead(7) == LOW){
        delay(10);
      }
      if (millis() - startPress > 500){
        Serial.println(" 2");
        Serial.println("");
      } else {
        Serial.println(" 1");
        Serial.println("");
        delay(500);
      }
      pox.resume();
      startPress = 0;
  }
}
