void checkStartStopButton(){
  
  // BUTTON STUFF - AN
  // read the state of the switch into a local variable:
  int reading = digitalRead(startStopButton);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastStartStopButtonState) {
    // reset the debouncing timer
    lastDebounceTimeStartStop = millis();
  }

  if ((millis() - lastDebounceTimeStartStop) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != startStopButtonState) {
      startStopButtonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (startStopButtonState == HIGH && active == false) {
//        greenLEDState = !greenLEDState;
        Serial.println("Active is true");
        active = true;
        previousTrialMillis = millis();
      }
      else if(startStopButtonState == HIGH && active == true){
        Serial.println("Active interrupted by experimenter");
        active = false; // This stops any sensor acquisition in the main loop
        trialStartTime = 0;
        next_metronome_t = 0;
        metronome_clicks_played = 0;       
        digitalWrite(greenLED, LOW);
//        write_responseArray_to_sdCard(nameOfFile); // Need to fix on getting the right txt filename
        msg_number_array = 0;
        msg_number = 0;
        resetArray();
      }
    }
  }

  // set the LED:
//  digitalWrite(greenLED, greenLEDState);

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastStartStopButtonState = reading;    
}
