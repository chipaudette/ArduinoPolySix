
/*
 Polysix Key Assigner
 Author: Chip Audette
 First Created: Feb-Apr 2013
 
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

//Mapping of all the digital pins is defined in KeyAssignerPinMap
#include <KeyAssignerPinMap.ino>

/* This function places the current value of the heap and stack pointers in the
 * variables. You can call it from any place in your code and save the data for
 * outputting or displaying later. This allows you to check at different parts of
 * your program flow.
 * The stack pointer starts at the top of RAM and grows downwards. The heap pointer
 * starts just above the static variables etc. and grows upwards. SP should always
 * be larger than HP or you'll be in big trouble! The smaller the gap, the more
 * careful you need to be. Julian Gall 6-Feb-2009.
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

//define our global variables
keybed_t trueKeybed;
keybed_t arpGeneratedKeybed;
voiceData_t allVoiceData[N_VOICE_SLOTS];
assignerState_t assignerState;
arpManager_t arpManager(&trueKeybed,&arpGeneratedKeybed);
assignerButtonState2_t assignerButtonState;
chordMemState_t chordMemState;
volatile boolean newTimedAction = false; //used for basic voice timing
volatile micros_t prev_ARP_tic_micros = 0L;
volatile micros_t ARP_period_micros = 1000000L;
volatile boolean arp_period_changed = false;
int aftertouch_val = 0;
int porta_setting_index = 0;
int porta_time_const[] = {2,6,18}; //one time constant, in units of voice cycles (9*676usec)
//long porta_minstep[] = {9000L,8000L,6000L};  //7000 is good with 4, 9000 is good with 3
long porta_minstep[] = {300L,300L,300L};  //7000 is good with 4, 9000 is good with 3
int aftertouch_scale[] = {0,5,15};
int aftertouch_setting_index=1;
int32_t porta_noteBend_x16bits = 0;
int32_t LFO_noteBend_x16bits = 0;
int LFO_type = LFO_TRIANGLE;
int32_t detune_noteBend_x16bits = 3500L;
int defaultDetuneFactors[] = {0,-1,0,1,-2,2};
//int unisonDetuneFactors[] = {-1,-2,+2,-2,+2,1};
//int unisonDetuneFactors[] = {0,-1,0,-2,+1,-2};
int unisonDetuneFactors[] = {0,1,-1,2,-2,0}; //pre 2013-06-12
int unisonPolyDetuneFactors[] = {-1,-1,-1,1,1,1};
Adafruit_MCP4725 dac;
int32_t detune_decay_x16bits = 3500L;

// ///// Aftertouch curves
//4-Segment Linear with trans at 40, 80, and 110
byte aftertouch_lookup[128] = {0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 48, 50, 52, 55, 57, 59, 61, 63, 65, 67, 70, 72, 74, 76, 78, 80, 83, 85, 87, 89, 91, 93, 95, 98, 100, 102, 104, 106, 108, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127};

//define timing parameters
//#define VOICE_DURATION_USEC (676)
long voiceDuration_usec = 676; //standard is 676, human debugging is 250000

//define the timer callback function
void timer3_callback()
{
  newTimedAction = true;
}

void switchStateManager::printUpdateVals(const int &state, const int &debounceCounter)
{
  Serial.print("updateState: state ");
  Serial.print(state);
//  Serial.print(", curState ");
//  Serial.print(curState);
  Serial.print(", debounce ");
  Serial.println(debounceCounter);
}

void setup() {
  //setup the serial bus
  Serial3.begin(31200);  //set to 31200 for MIDI
  Serial.begin(115200);  //for debugging

  //setup the pins that are plugging into the empty 8049-217 socket
  setupDigitalPins();
    
  //initialize data structures
  //initAssigerButtonState(assignerButtonState);
  //copyAssignerButtonState(assignerButtonState,prevAssignerButtonState);
  initAssignerState();
  initChordMemState();
  initializeVoiceData(allVoiceData,N_POLY);
  
  //start the DAC
  dac.begin(0x62);
  
  //setup the timer to do the different voice periods
  Timer3.initialize(voiceDuration_usec);   // initialize timer3, and set its period
  Timer3.attachInterrupt(timer3_callback);  // attaches callback() as a timer overflow interrupt
  
  //setup interupt pin for arpeggiator Arpegiator and LFO
  #define PS_ACKI_INT 4  //this is interrupt #2 http://arduino.cc/en/Reference/AttachInterrupt 
  attachInterrupt(PS_ACKI_INT, measureInterruptTiming, RISING); //measure the ARP timing
  //pinMode(LFO_OUT_PIN,OUTPUT);
  //Timer4.initialize(LFO_PWM_MICROS/2);
  //Timer4.pwm(LFO_OUT_PIN,LFO_MAX,LFO_PWM_MICROS+1); //expects duty to be 10 bit (ie, 0-1023)

  //if (DEBUG_TO_SERIAL) 
  //Serial.println("Finished with Setup");
}

//unsigned long count =0;
void loop(void) {
  static boolean firstPrint = true;
  //count++;
  //if (count % 10000 == 1) Serial.println("in main loop...");
  if (newTimedAction) {
    serviceNextVoicePeriod();
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
    serviceSerial();
  }
  
}

//adjust the duration of the voice timer
void adjustVoiceTimerDuration(const long &newDuration_usec)
{
  static boolean firstTime=true;

  #if 0
    Timer3.setPeriod(newDuration_usec);
    voiceDuration_usec = newDuration_usec;
  #else
    if (firstTime) {
      if (DEBUG_TO_SERIAL) Serial.println("adjustVoiceTimerDuration: this is disabled.");
      firstTime=false;
    }
  #endif
}

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
