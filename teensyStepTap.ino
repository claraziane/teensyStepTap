/* teensyStepTap - FSR based step and tap detection

    Developed by Alex Nieva - BRAMS - 2025

    oddball() based on Clara Ziane's matlab code developed in BRAMS.
    Ported to teensyduino by Alex and Grok


    fsrRead() based on teensytap by Floris van Vugt

*/

#include <SD.h>
#include <SPI.h>
#include <Audio.h>
//#include <stdlib.h>
//#include <time.h>


// Load the samples that we will play for pureTones() and oddball()
#include "AudioSampleSndstandard.h"
#include "AudioSampleSndtargethigh.h"
#include "AudioSampleSndtargetlow.h"

// This is for the 4 digit display with the TM1637 chip.
#include <TM1637.h>

int CLK = 1;
int DIO = 0;

TM1637 tm(CLK, DIO);


void nextState(); // Forward declaration of function


#define HEIGHT_RESPONSE_ARRAY 750
#define WIDTH_RESPONSE_ARRAY 7
#define SAMPLE_RATE 44100

/*
  Setting up infrastructure for capturing responses (from a connected FSR)
*/

int responseArray[HEIGHT_RESPONSE_ARRAY][WIDTH_RESPONSE_ARRAY] = {0};
#define TAP 1
#define STEP 2

// Global Device state (we should change this variable to trial)
uint8_t state = -1;

// Definitions for the SD card
const int chipSelect = 10;
File myFile; // probably we just need one definition as each Condition is independent from the others.
char COND1[12] = "COND01.TXT";
char COND2[12] = "COND02.TXT";
char COND3[12] = "COND03.TXT";
char COND4[12] = "COND04.TXT";
char COND5[12] = "COND05.TXT";
char COND6[12] = "COND06.TXT";
char COND7[12] = "COND07.TXT";
char COND8[12] = "COND08.TXT";
char COND9[12] = "COND09.TXT";
char COND10[12] = "COND10.TXT";
char COND11[12] = "COND11.TXT";
char COND12[12] = "COND12.TXT";
char COND13[12] = "COND13.TXT";
char COND14[12] = "COND14.TXT";

// constants won't change. They're used here to set pin numbers:
const int buttonPin = 4;         // the number of the pushbutton pin. Teensy 3.2 pin.
const int startStopButton = 5;   // number of start/stop button. Teensy 3.2 pin.
//const int ledPin = A0;            // the number of the LED pin. This many not be used in the end.
const int greenLED = 9;          // LED that indicates Condition is running. Teensy 3.2 onboard LED for now.
int fsrAnalogPin = A3;            // FSR is connected to analog 4 (A4). Teensy 3.2 A3 (17) pin.
int fsrReading;                   // the analog reading from the FSR resistor divider

// Variables will change:
int ledState = LOW;                 // the current state of the output pin
int buttonState;                    // the current reading from the input pin
int lastButtonState = LOW;          // the previous reading from the input pin
int greenLEDState = LOW;            // the current state of the green LED pin
int startStopButtonState;           // the current reading from the green LED pin
int lastStartStopButtonState = LOW; // the previous reading from the green LED pin
unsigned long previousMillis = 0;   // This is for the green LED which will be constantly blinking.
// constants won't change:
const long interval = 1000;         // interval at which to blink (milliseconds)

// Main Condition interval
unsigned long previousTrialMillis = 0;
unsigned long trialStartTime = 0;
//const long trialInterval = 60000;  // 1 minute for now
const long trialInterval = 160000;  // 2.5 mins + 10 sec minute for now

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long lastDebounceTimeStartStop = 0;
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

uint8_t digitNumOn;

boolean active = false; // Whether the tap capturing & metronome and all is currently active
boolean prev_active = false; // Whether we were active on the previous loop iteration


// For interpreting taps
// All these definitions come from teensytap - FVV
int tap_onset_threshold    = 50; // the FSR reading threshold necessary to flag a tap onset
int tap_offset_threshold   = 20; // the FSR reading threshold necessary to flag a tap offset
int min_tap_on_duration    = 20; // the minimum duration of a tap (in ms), this prevents double taps
int min_tap_off_duration   = 40; // the minimum time between offset of one tap and the onset of the next taps, again this prevents double taps


int tap_phase = 0; // The current tap phase, 0 = tap off (i.e. we are in the inter-tap-interval), 1 = tap on (we are within a tap)


unsigned long current_t            = 0; // the current time (in ms)
unsigned long prev_t               = 0; // the time stamp at the previous iteration (used to ensure correct loop time)
unsigned long next_event_embargo_t = 0; // the time when the next event is allowed to happen
unsigned long trial_end_t          = 0; // the time that this trial will end
unsigned long prevBurst_t          = 0; // the time stamp at the previous iteration (used to ensure correct loop time)
unsigned long burst_onset_t        = 0; //
unsigned long burst_offset_t       = 0; //

unsigned long tap_onset_t = 0;  // the onset time of the current tap
unsigned long tap_offset_t = 0; // the offset time of the current tap
int           tap_max_force = 0; // the maximum force reading during the current tap
unsigned long tap_max_force_t = 0; // the time at which the maximum tap force was experienced


int missed_frames = 0; // ideally our script should read the FSR every millisecond. we use this variable to check whether it may have skipped a millisecond

int metronome_interval = 600; // Time between metronome clicks

unsigned long next_metronome_t            = 0; // the time at which we should play the next metronome beat
unsigned long next_feedback_t             = 0; // the time at which the next tap sound should occur


int metronome_clicks_played = 0; // how many metronome clicks we have played (used to keep track and quit)
/*
  Information pertaining to the Condition we are currently running
*/

int msg_number = 0; // keep track of how many messages we have sent over the serial interface (to be able to track down possible missing messages)
//int BPM = 120;     // calculated tap BPM. Value to be defined by Condition 2.
int BPM = 96;    // calculated cadence. Value to be defined by Condition 1.
double totalNumberOfBeats = 0;
int totalNumberOfDeviants = 0;

// Hardcoded values that come from the Python GUI

int auditory_feedback          = 1; // whether we present a tone at every tap
int auditory_feedback_delay    = 0; // the delay between the tap and the to-be-presented feedback
int metronome                  = 1; // whether to present a metronome sound
int metronome_nclicks_predelay = 10; // how many clicks of the metronome to occur before we switch on the delay (if any)
int metronome_nclicks          = 20; // how many clicks of the metronome to present on this Condition
int ncontinuation_clicks       = 20; // how many continuation clicks to present after the metronome stops

/*
  Setting up the audio
*/

float sound_volume = .55; // the volume

// Create the Audio components.
// We create two sound memories so that we can play two sounds simultaneously
AudioPlayMemory    sound0;
AudioPlayMemory    sound1;
AudioMixer4        mix1;   // one four-channel mixer (we'll only use two channels)
AudioOutputI2S     headphones;

// Create Audio connections between the components
AudioConnection c1(sound0, 0, mix1, 0);
AudioConnection c2(sound1, 0, mix1, 1);
AudioConnection c3(mix1, 0, headphones, 0);
AudioConnection c4(mix1, 0, headphones, 1); // We connect mix1 to headphones twice so that the sound goes to both ears

// Create an object to control the audio shield.
AudioControlSGTL5000 audioShield;

// Definitions coming from teensytap - FVV

int msg_number_array = 0; //
//int msg_number_buffer = 0; //

// Global variables
int* buf = NULL;         // Global pointer for buf
int nSnds = 60;          // Initial size, will change in loop
const int nTarget = 20;  // Fixed target number of placements
int* bufIndex = NULL;     // Global pointer for indices
int vector20[20];        // Static array for simplicity

// Variables for countdown
unsigned long lastCountdownMillis = 0;  // Tracks the last time we updated
const unsigned long countdownInterval = 1000;  // 1-second intervals
int countdownSeconds = 10;  // Countdown starts at 10 seconds
bool countdownActive = true;  // Flag to control countdown state

void updateCountdown() {
  unsigned long currentMillis = millis();

  // Check if 1 second has passed
  if (currentMillis - lastCountdownMillis >= countdownInterval && countdownActive) {
    if (countdownSeconds > 0) {
      Serial.printf("Starting in %d seconds...\n", countdownSeconds);
      countdownSeconds--;
    } else {
      Serial.println("Starting now!");
      countdownActive = false;  // Stop countdown after reaching 0
    }
    lastCountdownMillis = currentMillis;  // Update the last check time
  }
}

// setup
/******************************************************************************/
void setup() {
  SPI.setMOSI(7);  // Audio shield has MOSI on pin 7
  SPI.setSCK(14);  // Audio shield has SCK on pin 14

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(10);

  // turn on the output
  audioShield.enable();
  audioShield.volume(sound_volume);

  // reduce the gain on mixer channels, so more than 1
  // sound can play simultaneously without clipping
  mix1.gain(0, 0.5);
  mix1.gain(1, 0.5);

  // First, This is for card info. Probably not needed.
  //  boolean status = SD.init(SPI_FULL_SPEED, chipSelect);
  //  if (status) {
  //    Serial.println("SD card is connected :-)");
  //  } else {
  //    Serial.println("SD card is not connected or unusable :-(");
  //    return;
  //  }


  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    while (1) {
      // No SD card, so don't do anything more - stay stuck here
    }
  }
  Serial.println("card initialized.");

  pinMode(buttonPin, INPUT);
  //  pinMode(ledPin, OUTPUT);
  pinMode(startStopButton, INPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(fsrAnalogPin, INPUT);

  // set initial LED state
  //  digitalWrite(ledPin, ledState);
  digitalWrite(greenLED, greenLEDState);

  // 4-digit display setup
  tm.init();
  //set brightness; 0-7
  tm.set(2);

  Serial.begin(115200);

  nextState(); //AN: Goes to state 0 to be ready for loop.

  randomSeed(analogRead(A13)^micros()); //pad in the back of the Teensy 4.0
}

// loop
/******************************************************************************/
void loop() {

  checkStateButton();

  checkStartStopButton();

  current_t = millis(); // get current time (in ms)

  if (active) { // active True or False comes from checkStartStopButton()
    switch (state) {
      case 1: //noneWalkST
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            tap_onset_threshold    = 175;
            tap_offset_threshold   = 50;
            min_tap_on_duration    = 700;
            min_tap_off_duration   = 300;
          }
          // Here is where we put the choices for each Condition
          blinkLED();

          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(STEP);
          }
          // End of choices for each Condition
          // Here we need to calculate BPM
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            // Here we calculate BPM.
            //          Serial.println(responseArray[msg_number_array][2]);
            //          Serial.println(responseArray[1][2]);
            //          Serial.println(msg_number_array);
            BPM = 2 * round((60E3 / (responseArray[msg_number_array][2] - 1 - responseArray[10][2])) * (msg_number_array - 10));
            Serial.print("BPM: "); Serial.println(BPM);
            char msg[50];
            sprintf(msg, "BPM is %d\n", BPM);
            Serial.print("msg: "); Serial.println(msg);
            write_to_sdCard(msg, COND1); delay(50);
            //
            closeCondition(COND1);
          }
        }
        break;
      case 2:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            digitalWrite(greenLED, HIGH);
          }

          updateCountdown();
          // Here is where we put the choices for each Condition
          if (!countdownActive) {
            readFSR(TAP);
          }
          // End of choices for each Condition

          // Here we need to calculate BPM
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            // Here we calculate BPM.
            BPM = round((60E3 / (responseArray[msg_number_array][2] - 1 - responseArray[10][2])) * (msg_number_array - 10));
            Serial.print("BPM: "); Serial.println(BPM);
            char msg[50];
            sprintf(msg, "BPM is %d\n", BPM);
            Serial.print("msg: "); Serial.println(msg);
            write_to_sdCard(msg, COND2); delay(50);
            //
            closeCondition(COND2);
          }
        }
        break;
      case 3:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
          }
          // Here is where we put the choices for each Condition
          blinkLED();
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND3);
          }
        }
        break;
      case 4:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
          }
          // Here is where we put the choices for each Condition
          blinkLED();

          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(STEP);
          }
          // End of choices for each Condition

          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND4);
          }
        }
        break;
      case 5:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            digitalWrite(greenLED, HIGH);
          }
          // Here is where we put the choices for each Condition
          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(TAP);
          }
          recordAudio(); //maybe the LED needs just to stay ON.
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND5);
          }
        }
        break;
      case 6: //
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            // Generate beat sequences
            totalNumberOfBeats = (BPM / 60.0) * (trialInterval / 1000.0); // This is the total number per Condition.
            totalNumberOfDeviants = 20 * (trialInterval / 1000 / 60); // 20 per minute
            Serial.print("totalNumberOfBeats: "); Serial.println(totalNumberOfBeats);
            Serial.print("totalNumberOfDeviants: "); Serial.println(totalNumberOfDeviants);
            setupOddball((int)totalNumberOfBeats, totalNumberOfDeviants);
            nSnds = totalNumberOfBeats;
            metronome_interval = round(60E3 / BPM);
            Serial.print("metronome_interval: "); Serial.println(metronome_interval);
          }

          // Here is where we put the choices for each Condition
          blinkLED();
          playOddball(buf);
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND6);
            appendBufToSD(COND6);
          }
        }
        break;
      case 7:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            // Generate beat sequences
            totalNumberOfBeats = (BPM / 60.0) * (trialInterval / 1000.0); // This is the total number per Condition.
            totalNumberOfDeviants = 20 * (trialInterval / 1000 / 60); // 20 per minute
            Serial.print("totalNumberOfBeats: "); Serial.println(totalNumberOfBeats);
            Serial.print("totalNumberOfDeviants: "); Serial.println(totalNumberOfDeviants);
            setupOddball((int)totalNumberOfBeats, totalNumberOfDeviants);
            nSnds = totalNumberOfBeats;
            metronome_interval = round(60E3 / BPM);
            Serial.print("metronome_interval: "); Serial.println(metronome_interval);
          }
          // Here is where we put the choices for each Condition
          blinkLED();
          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(STEP);
            playOddball(buf); //maybe the LED needs just to stay ON.
          }
          
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND7);
            appendBufToSD(COND7);
          }
        }
        break;
      case 8:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            // Generate beat sequences
            totalNumberOfBeats = (BPM / 60.0) * (trialInterval / 1000.0); // This is the total number per Condition.
            totalNumberOfDeviants = 20 * (trialInterval / 1000 / 60); // 20 per minute
            Serial.print("totalNumberOfBeats: "); Serial.println(totalNumberOfBeats);
            Serial.print("totalNumberOfDeviants: "); Serial.println(totalNumberOfDeviants);
            setupOddball((int)totalNumberOfBeats, totalNumberOfDeviants);
            nSnds = totalNumberOfBeats;
            metronome_interval = round(60E3 / BPM);
            Serial.print("metronome_interval: "); Serial.println(metronome_interval);
            digitalWrite(greenLED, HIGH);
          }
          // Here is where we put the choices for each Condition
          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(TAP);
            playOddball(buf); //maybe the LED needs just to stay ON.

          }
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND8);
            appendBufToSD(COND8);
          }
        }
        break;
      case 9:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            metronome_interval = round(60E3 / BPM);
            Serial.print("metronome_interval: "); Serial.println(metronome_interval);
          }
          // Here is where we put the choices for each Condition
          blinkLED();
          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(STEP);
          }
          playPureTone(); //maybe the LED needs just to stay ON.
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND9);
          }
        }
        break;
      case 10:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            metronome_interval = round(60E3 / BPM);
            Serial.print("metronome_interval: "); Serial.println(metronome_interval);
            digitalWrite(greenLED, HIGH);
          }
          // Here is where we put the choices for each Condition
          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(TAP);
          }
          playPureTone(); //maybe the LED needs just to stay ON.
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND10);
          }
        }
        break;
      case 11:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            metronome_interval = round(60E3 / BPM);
            Serial.print("metronome_interval: "); Serial.println(metronome_interval);
          }
          // Here is where we put the choices for each Condition
          blinkLED();
          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(STEP);
          }
          playPureTone();
          recordAudio(); //maybe the LED needs just to stay ON.
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND11);
          }
        }
        break;
      case 12:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            metronome_interval = round(60E3 / BPM);
            Serial.print("metronome_interval: "); Serial.println(metronome_interval);
            digitalWrite(greenLED, HIGH);
          }
          // Here is where we put the choices for each Condition
          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(TAP);
          }
          playPureTone(); //maybe the LED needs just to stay ON.
          recordAudio();
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND12);
          }
        }
        break;
      case 13:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            // Generate beat sequences
            totalNumberOfBeats = (BPM / 60.0) * (trialInterval / 1000.0); // This is the total number per Condition.
            totalNumberOfDeviants = 20 * (trialInterval / 1000 / 60); // 20 per minute
            Serial.print("totalNumberOfBeats: "); Serial.println(totalNumberOfBeats);
            Serial.print("totalNumberOfDeviants: "); Serial.println(totalNumberOfDeviants);
            setupOddball((int)totalNumberOfBeats, totalNumberOfDeviants);
            nSnds = totalNumberOfBeats;
            metronome_interval = round(60E3 / BPM);
            Serial.print("metronome_interval: "); Serial.println(metronome_interval);
          }
          // Here is where we put the choices for each Condition
          blinkLED();
          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(STEP);
          }
          playOddball(buf);
          recordAudio(); //maybe the LED needs just to stay ON.
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND13);
            appendBufToSD(COND13);
          }
        }
        break;
      case 14:
        if (current_t > prev_t) {
          if (trialStartTime == 0) {
            trialStartTime = current_t;
            // Generate beat sequences
            totalNumberOfBeats = (BPM / 60.0) * (trialInterval / 1000.0); // This is the total number per Condition.
            totalNumberOfDeviants = 20 * (trialInterval / 1000 / 60); // 20 per minute
            Serial.print("totalNumberOfBeats: "); Serial.println(totalNumberOfBeats);
            Serial.print("totalNumberOfDeviants: "); Serial.println(totalNumberOfDeviants);
            setupOddball((int)totalNumberOfBeats, totalNumberOfDeviants);
            nSnds = totalNumberOfBeats;
            metronome_interval = round(60E3 / BPM);
            Serial.print("metronome_interval: "); Serial.println(metronome_interval);
            digitalWrite(greenLED, HIGH);
          }
          // Here is where we put the choices for each Condition
          updateCountdown();
          // Only proceed with main activity after countdown finishes
          if (!countdownActive) {
            readFSR(TAP);
          }
          playOddball(buf); //maybe the LED needs just to stay ON.
          // End of choices for each Condition
          prev_t = current_t;
          if (current_t - trialStartTime >= trialInterval + 100) {
            closeCondition(COND14);
            appendBufToSD(COND14);
          }
        }
        break;
      default:
        break;
    }

  }
  // Signal for the next loop iteration whether we were active previously.
  // For example, if we weren't active previously then we don't want to count lost frames.
  prev_active = active;

}

// nextState
/******************************************************************************/
void nextState() {
  state++;
  if (state > 14) {
    state = 0;
  }
  switch (state) {
    // CYCLE SEGMENTS
    case 0:
      Serial.println("\nReady to start Experiment. Idle state. ");
      tm.clearDisplay();
      tm.displayStr((char *)"0N");
      break;
    case 1:
      Serial.println("\nCondition 1. Spontaneus walking.");
      // Set digit on
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    case 2:
      Serial.println("\nCondition 2. Spontaneus tapping.");
      // Set digit on
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    case 3:
      Serial.println("\nCondition 3. Nothing to be done. Voice recording only");
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      break; // Check if we can erase this break and go "Fall-through"
    case 4:
      Serial.println("\nCondition 4. Voice recording + Spontaneus walking.");
      // Set digit on
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    case 5:
      Serial.println("\nCondition 5. Voice recording + Spontaneus tapping.");
      // Set digit on
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    case 6:
      Serial.println("\nCondition 6. Nothing to be done. Audio oddball");
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      break; // Check if we can erase this break and go "Fall-through"
    case 7:
      Serial.println("\nCondition 7. Audio-oddball + walking.");
      // Set digit on
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    case 8:
      Serial.println("\nCondition 8. Audio-oddball + tapping.");
      // Set digit on
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;

    /*
       Conditions 9 and 10 start with random delay. Ask Clara what this is exactly.
    */
    case 9:
      Serial.println("\nCondition 9. Audio-PureTone + walking.");
      // Set digit on
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    case 10:
      Serial.println("\nCondition 10. Audio-PureTone + tapping.");
      // Set digit on
      digitNumOn = state; // Find out how to display two digits. See other example files.
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    case 11:
      Serial.println("\nCondition 11. Audio-PureTone + VoiceRecording + walking.");
      // Set digit on
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    case 12:
      Serial.println("\nCondition 12. Audio-PureTone + VoiceRecording + tapping.");
      // Set digit on
      digitNumOn = state; // Find out how to display two digits. See other example files.
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    case 13:
      Serial.println("\nCondition 13. Audio-oddball + walking."); // Same as 7???
      // Set digit on
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    case 14:
      Serial.println("\nCondition 14. Audio-oddball + tapping."); // Same as 8???
      // Set digit on
      digitNumOn = state;
      tm.clearDisplay();
      tm.displayNum(digitNumOn);
      Serial.print("Digit "); Serial.print(digitNumOn); Serial.println(" on");
      break;
    default:
      state = -1; // Should never arrive here
      break;
  }
}

void write_to_sdCard(char *msg, char *nameOfFile) {
  // open the file.
  myFile = SD.open(nameOfFile, FILE_WRITE);

  // if the file is available, write to it:
  if (myFile) {
    myFile.println(msg);
    myFile.close();
    // print to the serial port too:
    Serial.println("I just wrote to the sdCard.");
  }
  // if the file isn't open, pop up an error:
  else {
    //    Serial.println("error opening datalog.txt");
  }
}

void send_response_to_responseArray(int type) {
  msg_number_array += 1;
  responseArray[msg_number_array][0] = msg_number_array;
  responseArray[msg_number_array][1] = type;
  responseArray[msg_number_array][2] = tap_onset_t;
  responseArray[msg_number_array][3] = tap_offset_t;
  responseArray[msg_number_array][4] = tap_max_force_t;
  responseArray[msg_number_array][5] = tap_max_force;
  responseArray[msg_number_array][6] = missed_frames;
}


void write_responseArray_to_sdCard(char *nameOfFile) {
  myFile = SD.open(nameOfFile, FILE_WRITE);
  if (myFile) {
    for (int i = 0; i < HEIGHT_RESPONSE_ARRAY; i++) {
      for (int j = 0; j < WIDTH_RESPONSE_ARRAY; j++) {
        //        Serial.print("Big data array: "); Serial.print(i); Serial.println(bigDataArray[i][j]);
        myFile.print(responseArray[i][j]);
        myFile.print(",");
      }
      myFile.print("\n");
    }
    myFile.close();
    Serial.print("I just wrote to the sdCard."); Serial.println(nameOfFile);
  }
  else {
    Serial.print("Error opening "); Serial.println(nameOfFile);
  }
}

void send_event_to_sdCard(char *type, char *nameOfFile) {
  /* Sends information about the current tap to the SD card  */
  char msg[100];
  msg_number += 1; // This is the next message
  sprintf(msg, "%d %s %lu %lu %lu %d %d\n",
          msg_number,
          type,
          tap_onset_t,
          tap_offset_t,
          tap_max_force_t,
          tap_max_force,
          missed_frames);
  write_to_sdCard(msg, nameOfFile);
}

void send_tap_to_serial(int type) {
  /* Sends information about the current tap to the PC through the serial interface */
  char msg[100];
  msg_number += 1; // This is the next message
  //  sprintf(msg, "%d tap %lu %lu %lu %d %d\n",
  sprintf(msg, "%d %d %lu %lu %lu %d %d\n",
          msg_number,
          type,
          tap_onset_t,
          tap_offset_t,
          tap_max_force_t,
          tap_max_force,
          missed_frames);
  Serial.print(msg);

}


void closeCondition(char *filename) {
  Serial.println("Active is false");
  active = false;
  trialStartTime = 0;
  next_metronome_t = 0;
  metronome_clicks_played = 0;
  digitalWrite(greenLED, LOW);
  write_responseArray_to_sdCard(filename);
  msg_number_array = 0;
  msg_number = 0;
  resetArray();
  countdownSeconds = 10;  // Tracks the last time we updated
  countdownActive = true;  // Flag to control countdown state
  Serial.println("We just reset/closed the Condition parameters");
}

// New function to append buf to SD card
void appendBufToSD(char *filename) {
  // Buffer to hold the string representation of buf
  char msg[256]; // Adjust size if nSnds grows large
  int offset = 0;

  // Convert buf to a comma-separated string
  for (int i = 0; i < nSnds; i++) {
    offset += sprintf(msg + offset, "%d", buf[i]);
    if (i < nSnds - 1) {
      offset += sprintf(msg + offset, ",");
    }
  }


  // Append to SD card
  write_to_sdCard(msg, filename);
}

void resetArray() {
  memset(responseArray, 0, sizeof(responseArray[0][0]) * HEIGHT_RESPONSE_ARRAY * WIDTH_RESPONSE_ARRAY);
}

/// END ///
