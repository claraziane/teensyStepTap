void blinkLED(){
    if (current_t - previousMillis >= interval) {
      // save the last time you blinked the LED
      previousMillis = current_t;
  
      // if the LED is off turn it on and vice-versa:
      if (greenLEDState == LOW) {
        greenLEDState = HIGH;
      } else {
        greenLEDState = LOW;
      }
  
      // set the LED with the ledState of the variable:
      digitalWrite(greenLED, greenLEDState);
    }
}
