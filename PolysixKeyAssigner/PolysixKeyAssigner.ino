/*
  Polysix Key Assigner
  Author: Chip Audette
  First Created: Feb-Apr 2013
  Extended:  Oct 2015, Velocity Processing
  Extended:  Nov 2018, Voice Pitch Tuning
  Extended:  Feb 2021, Ribbon Control

  Goal: Replace the functions performed by the 8049-217 microprocessor
  used by the Korg Polysix synthesizer.  This microprocessor is the
  "Key Assigner", meaning that it scans the keybed to see which keys
  are pressed, it scans a few user control push buttons, and it generates
  the pitch and timing signals needed by the keyboard to create sounds.

  Approach: Remove the 8049 microprocessor from the socket on the KLM-366
  board that is in the Polysix.  Insert pins into the now-empty socket.
  Wire the pins back to an Arduino Mega 2560.  Write this software to
  replace the missing functions.  The major difference being that the
  Arduino will not scan the keybed -- it will instead receive MIDI commands
  over a serial port.
*/

//data type definitions and global macros
#include "dataTypes.h"
#include "KeyAssignerPinMap.h"  //Mapping of all the digital pins is defined in KeyAssignerPinMap
#include "ribbon.h"


/* This function places the current value of the heap and stack pointers in the
   variables. You can call it from any place in your code and save the data for
   outputting or displaying later. This allows you to check at different parts of
   your program flow.
   The stack pointer starts at the top of RAM and grows downwards. The heap pointer
   starts just above the static variables etc. and grows upwards. SP should always
   be larger than HP or you'll be in big trouble! The smaller the gap, the more
   careful you need to be. Julian Gall 6-Feb-2009.
*/
uint8_t * heapptr, * stackptr;
void check_mem() {
  stackptr = (uint8_t *)malloc(4);          // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);      // free up the memory again (sets stackptr to 0)
  stackptr =  (uint8_t *)(SP);           // save value of stack pointer
}


//Include Timer Library...for the Arduino MEGA!!!
//http://letsmakerobots.com/node/28278
#include "TimerThree.h"  //this is a 16-bit timer on the MEGA
#include "TimerFour.h"  //this is a 16-bit timer on the MEGA
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <EEPROM.h> //for storing the tuning factors

//define our global variables
keybed_t trueKeybed;  //this represents the keys pressed on the actual keybed (6 key limit)
keybed_givenlist_t trueKeybed_givenList;  //this holds a longer list of key presses, in the order that they were pressed
keybed_t arpGeneratedKeybed; //this represents the keys that are "pressed" by the arpeggiator
voiceData_t allVoiceData[N_VOICE_SLOTS]; //these are the 6 voices of the polysix
assignerState_t assignerState;
arpManager_t arpManager(&trueKeybed, &arpGeneratedKeybed, &trueKeybed_givenList);
assignerButtonState2_t assignerButtonState;
chordMemState_t chordMemState;
tuningModeState_t tuningModeState;
volatile boolean newTimedAction = false; //used for basic voice timing
volatile micros_t prev_ARP_tic_micros = 0L;
volatile micros_t ARP_period_micros = 1000000L;
volatile boolean arp_period_changed = false;
int aftertouch_val = 0;
int porta_setting_index = 0;
int porta_time_const[] = {2, 6, 18}; //one time constant, in units of voice cycles (9*676usec)
long porta_minstep[] = {300L, 300L, 300L};
int aftertouch_scale[] = {0, 5, 15};
int aftertouch_setting_index = 1;
int32_t porta_noteBend_x16bits = 0;
int32_t LFO_noteBend_x16bits = 0;
int LFO_type = LFO_TRIANGLE;
int32_t detune_noteBend_x16bits = 3500L;  //basic unit of detuning...combines with detune factors below
int defaultDetuneFactors[] = {0, -1, 0, 1, -2, 2};
int unisonDetuneFactors[] = {0, 1, -1, 2, -2, 0}; //pre 2013-06-12
int unisonPolyDetuneFactors[] = { -1, -1, -1, 1, 1, 1};
int32_t perVoiceTuningFactors_x16bits[N_POLY][N_OCTAVES_TUNING][1];
int32_t tuningAdjustmentFactor_x16bits = 1000L; //10,000 is about 20cents, so 1000 is about 2 cents
Adafruit_MCP4725 dac;
int32_t detune_decay_x16bits = 3500L;
Ribbon myRibbon;

// ///// Aftertouch curves
//4-Segment Linear with trans at 40, 80, and 110
byte aftertouch_lookup[128] = {0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 48, 50, 52, 55, 57, 59, 61, 63, 65, 67, 70, 72, 74, 76, 78, 80, 83, 85, 87, 89, 91, 93, 95, 98, 100, 102, 104, 106, 108, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127};

//define timing parameters
long voiceDuration_usec = 676; //standard is 676, human debugging is 250000

//define the timer callback function
void timer3_callback() { newTimedAction = true; }

//include some functions that are useful
#include "hardwareServices.h"

void switchStateManager::printUpdateVals(const int &state, const int &debounceCounter)
{
  Serial.print(F("updateState: state "));
  Serial.print(state);
  Serial.print(F(", debounce "));
  Serial.println(debounceCounter);
  return;
}

void setup() {
  //setup the serial bus
  Serial2.begin(2 * 115200); //for communication to Arduino/Teensy doing velocity processing
  Serial3.begin(31250);  //set to 31200 for MIDI  (or 31250?)
  Serial.begin(115200);  //for debugging via USB

  //setup the pins that are plugging into the empty 8049-217 socket
  setupDigitalPins();

  //initialize data structures
  assignerState.init();
  initChordMemState();
  initializeVoiceData(allVoiceData, N_POLY);
  readEEPROM();

  //start the DAC
  dac.begin(0x62);

  //setup the timer to do the different voice periods
  Timer3.initialize(voiceDuration_usec);   // initialize timer3, and set its period
  Timer3.attachInterrupt(timer3_callback);  // attaches callback() as a timer overflow interrupt

  //setup interupt pin for arpeggiator Arpegiator and LFO
  #define PS_ACKI_INT 4  //which interrupt?  On Mega 2560, #4 is on pin 19. http://arduino.cc/en/Reference/AttachInterrupt 
  attachInterrupt(PS_ACKI_INT, measureInterruptTiming, RISING); //measure the ARP timing
  //pinMode(LFO_OUT_PIN,OUTPUT);
  //Timer4.initialize(LFO_PWM_MICROS/2);
  //Timer4.pwm(LFO_OUT_PIN,LFO_MAX,LFO_PWM_MICROS+1); //expects duty to be 10 bit (ie, 0-1023)

  //setup the ribbon
  myRibbon.setup_ribbon(RIBBON_PIN, RIBBON_REF_PIN, &trueKeybed);

}

//unsigned long count =0;
void loop(void) {
  static boolean firstPrint = true;
  //count++;
  //if (count % 10000 == 1) Serial.println("in main loop...");
  if (newTimedAction) {        //newTimedAction is set to true at fixed intervals via Timer3Callback

    // this is where all the voice servicing happens.  This is a lot of work!
    serviceNextVoicePeriod();   //see voiceServicing.h
    newTimedAction = false;

    //    if ((millis() > 5000) & (firstPrint)) {
    //      check_mem();
    //      Serial.print("Memory: Heap = ");
    //      Serial.print((int)heapptr);
    //      Serial.print(", Stack = ");
    //      Serial.println((int)stackptr);
    //      firstPrint = false;
    //    }

  } else {
    //this is where incoming MIDI notes are processed.  This is also a lot of work!
    serviceSerial3();  //see serialMIDIProcessing.h

    //service USB Serial
    serviceSerial();  // see definition later in this file

    //service the ribbon
    myRibbon.service_ribbon(millis());
  }

}


//read all variables from the EEPROM
void readEEPROM(void) {
  int eeAddress = 0; //EEPROM address to start reading from

  //load tuning data for each voice
  int32_t foo;
  for (int I_voice = 0; I_voice < N_POLY; I_voice++) {
    for (int Ioct = 0; Ioct < N_OCTAVES_TUNING; Ioct++) {
      for (int Istartend = 0; Istartend < 2; Istartend++) {
        EEPROM.get(eeAddress, foo);
        perVoiceTuningFactors_x16bits[I_voice][Ioct][Istartend] = foo;
        eeAddress += sizeof(foo);
      }
    }
  }

  //read the ribbon calibration adjustment
  float foo_float;
  for (int I_cal = 0; I_cal < 2; I_cal++) { //assume 2 calibration values
    EEPROM.get(eeAddress, foo_float);
    if (isnan(foo_float)) foo_float = 0.0f;
    if (fabs(foo_float) > 3.0) foo_float = 0.0f; //reject overly large values
    myRibbon.setCal(I_cal,foo_float);
    eeAddress += sizeof(foo_float);
  }
  return;
};


//save all variables to the EPPROM
void saveEEPROM(void) {
  int eeAddress = 0; //EEPROM address to start reading from

  //save the tuning data
  int32_t foo;
  for (int I_voice = 0; I_voice < N_POLY; I_voice++) {
    for (int Ioct = 0; Ioct < N_OCTAVES_TUNING; Ioct++) {
      for (int Istartend = 0; Istartend < 2; Istartend++) {
        foo = perVoiceTuningFactors_x16bits[I_voice][Ioct][Istartend];
        EEPROM.put(eeAddress, foo);
        eeAddress += sizeof(foo);
      }
    }
  }

  //write the ribbon calibration adjustment
  float foo_float;
  for (int I_cal = 0; I_cal < 2; I_cal++) { //assume 2 calibration values
    foo_float = myRibbon.cal_adjust[I_cal];
    EEPROM.put(eeAddress, foo_float);
    eeAddress += sizeof(foo_float);
  }

  //Serial.println("saveEEPROM: complete.");
  return;
}


void printTuningFactors(void) {
  Serial.println(F("printTuningFactors(): Tuning factors..."));
  for (int I_voice = 0; I_voice < N_POLY; I_voice++) {
    Serial.print("  : V"); Serial.print(I_voice); Serial.print(": ");
    for (int Ioct = 0; Ioct < N_OCTAVES_TUNING; Ioct++) {
      Serial.print("[");
      Serial.print(perVoiceTuningFactors_x16bits[I_voice][Ioct][0]);
      Serial.print(", ");
      Serial.print(perVoiceTuningFactors_x16bits[I_voice][Ioct][1]);
      Serial.print("], ");
    }
    Serial.println();
  }
  return;
}


void clearTuningFactors(void) {
  for (int I_voice = 0; I_voice < N_POLY; I_voice++) {
    for (int Ioct = 0; Ioct < N_OCTAVES_TUNING; Ioct++) {
      perVoiceTuningFactors_x16bits[I_voice][Ioct][0] = 0;
      perVoiceTuningFactors_x16bits[I_voice][Ioct][1] = 0;
    }
  }
  return;
}

// print menu of options to USB serial
void printHelpMenu(void) {
  Serial.println(F("PolySixKeyAssigner: USB Serial Menu Options:"));
  Serial.println(F("    : h/?: Print this help menu."));
  Serial.println(F("    : `/~: Read/Save EEPROM"));
  Serial.println(F("    : t/T: Print/Clear all tuning factors in RAM"));
  Serial.println(F("    : p/P: Lower/Raise tuning factor of this octave of this voice"));
}

// service the USB serial link
void serviceSerial(void) {
  if (Serial.available() > 0)
  {
    //read the byte
    char byte1 = Serial.read();

    //interpret the received byte
    switch (byte1)
    {
      case '?':
        printHelpMenu(); break;
      case 'h':
        printHelpMenu(); break;
      case '~':
        saveEEPROM();
        Serial.println(F("serviceSerial: saveEEPROM complete."));
        break;
      case '`':
        readEEPROM();
        Serial.println(F("serviceSerial: readEEPROM complete."));
        break;
      case 't':
        printTuningFactors(); break;
      case 'T':
        clearTuningFactors();
        Serial.println(F("serviceSerial: clearTuningFactors() complete."));
        break;
      case 'p':
        //lower pitch of this voice
        //Serial.print(F("Adjusted Pitch.  Adjustment is now: "));
        //Serial.println(adjustTuningThisVoiceOrRibbon(-1,true));
        adjustTuningThisVoiceOrRibbon(-1, true);
        break;
      case 'P':
        //raise pitch this voice
        //Serial.print(F("Adjusted Pitch.  Adjustment is now: "));
        //Serial.println(adjustTuningThisVoiceOrRibbon(+1,true));
        adjustTuningThisVoiceOrRibbon(1, true);
        break;
    }
  }
  return;
}

void adjustTuningThisVoiceOrRibbon(int adjustmentFactor, bool printDebug) {
//adjustment factor will be +/-1 or +/-5 (or whatever) representing a small or large tuning adjustment
  
  //find which voice is the most recent (assumed currently active)
  int curVoiceInd = getNewestActiveVoiceInd(); //counts voices starting from zero
  if (curVoiceInd < 0) return 0;


  if (allVoiceData[curVoiceInd].isRibbon == 1) {
    //adjust the ribbon's calibration
    float rib_cal_adjust_factor = 0.02f;  //1.0 is a half step?

    //find which end of the ribbon are we playing?
    int calInd = 0;  //zero will adjust the low end of the ribbon cal.  One will adjust the high end of the ribbon cal
    int curNote = getCurrentNoteOfVoice(curVoiceInd); //this isn't great.  we should ask the keyPress or ask the ribbon where it is
    if (curNote > 36) { //assume the ribbon is notes 12-48...though, because we're using voice data instead of keypress, the octave switch on the synth could mess this allup?!?
      calInd = 1;  //let's adjust the high end of the ribbon cal
    }
    myRibbon.incrementCal(calInd,adjustmentFactor*rib_cal_adjust_factor);
   
  } else {
    //adjust the synth voice's calibration

    //which note are we playing?
    int curNote = getCurrentNoteOfVoice(curVoiceInd);
    int curOctaveInd = min(max(0, curNote / 12), N_OCTAVES_TUNING - 1);
    int Istartend = 0;
    int noteRelCurOct = curNote - curOctaveInd * 12;
    if (noteRelCurOct == 11) Istartend = 1;
  
    if (tuningModeState.adjustmentMode == TUNING_MODE_ALL_VOICES) {
      for (int I_voice = 0; I_voice < N_POLY; I_voice++) {
        perVoiceTuningFactors_x16bits[I_voice][curOctaveInd][Istartend] += \
            ((int32_t)adjustmentFactor) * tuningAdjustmentFactor_x16bits;
      }
    } else {
      perVoiceTuningFactors_x16bits[curVoiceInd][curOctaveInd][Istartend] += \
          ((int32_t)adjustmentFactor) * tuningAdjustmentFactor_x16bits;
    }
  
    if (printDebug) {
      Serial.print(F("adjustTuningThisVoiceOrRibbon: voice = ")); Serial.print(curVoiceInd);
      Serial.print(F(", note = ")); Serial.print(curNote);
      Serial.print(F(", octave ind = ")); Serial.print(curOctaveInd);
      Serial.print(F(", Istartend  = ")); Serial.print(Istartend);
      Serial.println();
      Serial.print(F("adjustTuningThisVoiceOrRibbon: new tuning factor = ")); Serial.println(perVoiceTuningFactors_x16bits[curVoiceInd][curOctaveInd][Istartend]);
    }
  }
}

////adjust the duration of the voice timer
//void adjustVoiceTimerDuration(const long &newDuration_usec)
//{
//  static boolean firstTime=true;
//
//  #if 0
//    Timer3.setPeriod(newDuration_usec);
//    voiceDuration_usec = newDuration_usec;
//  #else
//    if (firstTime) {
//      if (DEBUG_TO_SERIAL) Serial.println("adjustVoiceTimerDuration: this is disabled.");
//      firstTime=false;
//    }
//  #endif
//}

//here is the callback for pin attached to the Polysix's arp clock
void measureInterruptTiming(void)
{
  //tell the Arp that a pulse has come
  arpManager.nextNote();

  //now measure the timing of the pulses for use by other functions (like the LFO)
  micros_t cur_micros = micros();
  ARP_period_micros = cur_micros - prev_ARP_tic_micros;
  prev_ARP_tic_micros = cur_micros;
}
