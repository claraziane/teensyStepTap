// Calibration routine for FSR
void calibrateFSR() {
    Serial.println("Starting 30-second FSR calibration...");
    Serial.println("Press the FSR periodically for 30 seconds.");
    
    unsigned long startTime = millis();
    const unsigned long duration = 30000; // 30 seconds
    const int sampleInterval = 10; // Sample every 10ms
    
    int maxValue = 0;
    int minValue = 1023; // Max ADC value on Teensy
    unsigned long lastMaxTime = 0;
    unsigned long lastMinTime = 0;
    float avgTimeBetweenMax = 0;
    float avgTimeBetweenMin = 0;
    int maxCount = 0;
    int minCount = 0;
    int prevReading = analogRead(fsrAnalogPin);
    bool rising = false;
    
    while (millis() - startTime < duration) {
        fsrReading = analogRead(fsrAnalogPin);
        
        // Detect peak (max) or valley (min)
        if (fsrReading > prevReading + 10 && !rising) { // Rising edge
            rising = true;
        } else if (fsrReading < prevReading - 10 && rising) { // Falling edge, found a max
            if (fsrReading > maxValue) {
                maxValue = fsrReading;
            }
            if (lastMaxTime != 0) {
                avgTimeBetweenMax = (avgTimeBetweenMax * maxCount + (millis() - lastMaxTime)) / (maxCount + 1);
                maxCount++;
            }
            lastMaxTime = millis();
            rising = false;
        }
        
        if (fsrReading < prevReading - 10 && !rising) { // Falling edge
            rising = false;
        } else if (fsrReading > prevReading + 10 && !rising) { // Rising edge, found a min
            if (fsrReading < minValue) {
                minValue = fsrReading;
            }
            if (lastMinTime != 0) {
                avgTimeBetweenMin = (avgTimeBetweenMin * minCount + (millis() - lastMinTime)) / (minCount + 1);
                minCount++;
            }
            lastMinTime = millis();
            rising = true;
        }
        
        prevReading = fsrReading;
        delay(sampleInterval); // Sample rate
    }
    
    // Output results
    char buffer[128];
    Serial.println("Calibration complete:");
    sprintf(buffer, "Max FSR Value: %d", maxValue);
    Serial.println(buffer);
    sprintf(buffer, "Min FSR Value: %d", minValue);
    Serial.println(buffer);
    sprintf(buffer, "Avg Time Between Max (ms): %.2f", avgTimeBetweenMax);
    Serial.println(buffer);
    sprintf(buffer, "Avg Time Between Min (ms): %.2f", avgTimeBetweenMin);
    Serial.println(buffer);
    sprintf(buffer, "Max Count: %d, Min Count: %d", maxCount, minCount);
    Serial.println(buffer);
    
    // Write to SD card
    sprintf(buffer, "FSR Calibration: Max=%d, Min=%d, AvgMaxTime=%.2f, AvgMinTime=%.2f, MaxCount=%d, MinCount=%d",
            maxValue, minValue, avgTimeBetweenMax, avgTimeBetweenMin, maxCount, minCount);
    write_to_sdCard(buffer, CALIB);
}
