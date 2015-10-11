

//define some global macros

#define DEBUG_TO_SERIAL (false)
#define DEBUG_DISABLE_BUTTONS (false)
#define DEBUG_PRINT_VOICE_STATE (false)

#define ARP_AS_DETUNE false

#define ECHOMIDI (false)
#define SHIFT_MIDI_NOTES (-36)

#define EMPTY_VOICE -999

#define N_KEY_PRESS_DATA 5   //should be 6...changed temporarily 2015-10-11
#define N_POLY 5    //should be 6...changed temporarily 2015-10-11
#define N_VOICE_SLOTS 8
#define INDEX_LAST_VOICE (8)    //index of the last voice period
#define INDEX_LAST_PERIOD (9)  //index of whatever is the last period
#define ON (1)
#define OFF (0)
#define IGNORE (-999)
#define ARP_RANGE_FULL (4)
#define ARP_RANGE_2OCT (2)
#define ARP_RANGE_1OCT (1)
#define ARP_DIR_UP  (1)
#define ARP_DIR_DOWN (2)
#define ARP_DIR_UPDOWN (3)
#define HOLD_STATE_OFF (0)
#define HOLD_STATE_ON (1)
#define HOLD_STATE_ON_BUT_NO_NEW (2)
#define DEFAULT_ON_VEL 128;
#define RELEASE_VELOCITY_FROM_HOLD (96)  //could be zero to 128
#define KEYPANEL_MODE_ARP (1)
#define KEYPANEL_MODE_OTHER (2)

#define STATE_DEFAULT (-1)
#define STATE_POLY (10)
#define STATE_UNISON (20)
#define STATE_UNISON_POLY (25)
#define STATE_CHORD (30)
#define STATE_CHORD_POLY (35)

#define OCTAVE_16FT (0)
#define OCTAVE_8FT (1)
#define OCTAVE_4FT (2)

#define PS_ACKI_PIN  (19)      //this is attached to an interrupt pin http://arduino.cc/en/Reference/AttachInterrupt 
#define LFO_OUT_PIN (7) //pins 6, 7, 8 are controlled by Timer4 on the Mega 2560
#define LFO_PWM_MICROS (200)
#define LFO_BITS 10
#define LFO_MAX 1024
#define LFO_MIN 0
#define LFO_MID 511
#define LFO_TRIANGLE 1
#define LFO_SQUARE 2
#define LFO_SQUARE_WITH_MID 3
#define MAX_10BIT (1023)
#define MAX_12BIT (4095)
#define DAC_COUNTS_PER_NOTE_x8bits (8736)  //12-bit divided by 10 octaves with 12 notes per octave...all assuming 5V full scale and 0.5V/octave scaling
//define DAC_COUNTS_PER_NOTE_x8bits (8806)  //12-bit divided by 10 octaves with 12 notes per octave...all assuming 4.960V full scale and 0.5V/octave scaling
#define FULL_SCALE_INT16 (32768)
  
typedef unsigned long millis_t;
typedef unsigned long micros_t;
typedef long int int32_t;

//define data structure for describing key presses
//define UNASSIGNED_TO_VOICE -1
#ifndef _KEYPRESS_TYPES
#define _KEYPRESS_TYPES
class keyPressData_t {
  public:
    keyPressData_t(void) { reset(); };
    int noteNum;
    int noteVel;
    //unsigned int noteReleaseVel;
    int isPressed; //ON or OFF
    int isHeld;    //ON or OFF
    int isLatched; //ON or OFF
    millis_t start_millis;  //when the key is pressed
    millis_t end_millis;    //when the key is released
    //int allocatedToVoice;   //index into voice structure holding this note
    int forceRetrigger;
    int isNewVelocity;
    int isGateActive(void) {
      if ((isPressed==ON) | (isHeld==ON)) {
        return ON;
      } else {
        return OFF;
      }
    };
    int isGateActiveOrLatched(void) {
      if (isGateActive() | isLatched) {
        return ON;
      } else {
        return OFF;
      }
    };
    void reset(void) {
      noteNum = 0;  noteVel = 0;  isPressed = OFF;  isHeld = OFF; isLatched = OFF;
      start_millis = 0;  end_millis = 0;  forceRetrigger = true; isNewVelocity = OFF;
    };
//    void copyFrom(const keyPressData_t &input) {
//      noteNum = input.noteNum;      noteVel = input.noteVel;      isPressed = input.isPressed;      isHeld = input.isHeld;
//      start_millis = input.start_millis;      end_millis = input.end_millis;      forceRetrigger = input.forceRetrigger;
//    } 
  private:
};


//data structure defining what's happening in a voice
typedef struct {
  int keyPressID;  //index into keypress array
  int noteNum;
  int32_t curNoteNum_x16bits;
  int32_t curAttackDetuneFac_x16bits;
  int noteVel;
  //unsigned int noteReleaseVel;
  int noteGate;  //high or low
  millis_t start_millis; //when the note on the voice is started
  millis_t end_millis;   //when the note on the voice is ended
  int forceRetrigger;
  int isNewVelocity;
} voiceData_t;

typedef struct {
  int arp_latch;
  int arp_direction;
  int arp_range;
} arp_parameters_t;

typedef struct {
  int arp;
  int poly;
  int unison;
  int chord_mem;
  int hold;
  int octave;
  int legato;
  int portamento;
  int detune;
  int keypanel_mode;
  arp_parameters_t arp_params;
} assignerState_t;

typedef struct {
  int state;
  int stateChanged;
  int prev_user_beenReleased;
} buttonStateReport_t;

void copyStateReport(const buttonStateReport_t &source, buttonStateReport_t &target) {
  target.state = source.state;
  target.stateChanged = source.stateChanged;
  target.prev_user_beenReleased = source.prev_user_beenReleased;
} 

#define buttonDebounceCycles (2) //was 2
#define WAITING_FOR_RELEASE -1
#define WAITING_FOR_PRESS false
#define BEEN_PRESSED true
class switchStateManager {
  public:
    int state;
    switchStateManager(void) { resetState(); };
    void printUpdateVals(const int &, const int &);
    void printUpdateVals(void) { printUpdateVals(state,debounceCounter); }
    void updateState(const int &curState) {
      if (debounceCounter < 0) {
        //do nothing...we're in the debounce period
        debounceCounter++;
        stateReport.state = state;
        stateReport.stateChanged= false;       
      } else {
        //has the state changed?
        stateReport.stateChanged = false;
        if (state != curState) stateReport.stateChanged = true; 
        
        //update the state
        state = curState;
        stateReport.state = state;
          
        //reset the debounce counter
        if (stateReport.stateChanged==true) debounceCounter = -buttonDebounceCycles;
      }
    };
    void resetState(void) { 
      state = OFF; 
      debounceCounter=0; 
      resetStateReport(); 
    };
    void getStateReport(buttonStateReport_t &targetReport) { copyStateReport(stateReport,targetReport);}
    int didStateChange(void) { return stateReport.stateChanged; };      
    int wasJustReleased(void) { return ((state==OFF) & (didStateChange()==true)); }
  protected:
    int debounceCounter;
    int userBeenPressedAgain_processing_state;
    buttonStateReport_t stateReport;
    void resetStateReport(void) {
        stateReport.state=OFF;
        stateReport.stateChanged=OFF;
        stateReport.prev_user_beenReleased=true;
    }
};
class pushButtonStateManager : public switchStateManager {
  public:
    //int state; //ON or OFF
    int user_beenReleased;  //the user sets this false, this object sets it true when the button is released
    int user_beenPressedAgain; //the user sets this false, this object sets it true when it has been pressed
    pushButtonStateManager(void) { resetState(); };
    pushButtonStateManager(const int &initState) { resetState(); state = initState; user_beenReleased=true; };
    pushButtonStateManager(const int &initState,const int &initReleased) { resetState(); state = OFF; user_beenReleased=initReleased; };
    void updateState(const int &curState) {
      switchStateManager::updateState(curState);
      if (debounceCounter >= 0) {
        stateReport.prev_user_beenReleased = user_beenReleased;
   
        //take care of the user flags
        if (curState == OFF) {
          user_beenReleased = true;  //now that it is released, set this to true
          userBeenPressedAgain_processing_state = WAITING_FOR_PRESS;
        } else if (curState == ON) {
          if (userBeenPressedAgain_processing_state == WAITING_FOR_PRESS) {
            user_beenPressedAgain = true;
            userBeenPressedAgain_processing_state = BEEN_PRESSED;
          }
        }
      }
    };
    void set_user_beenPressedAgainToFalse(void) { user_beenPressedAgain = false;  userBeenPressedAgain_processing_state = WAITING_FOR_RELEASE;};
    void resetState(void) { 
      switchStateManager::resetState();
      user_beenReleased=true; 
      user_beenPressedAgain=true; 
      userBeenPressedAgain_processing_state = BEEN_PRESSED;
    }
  private:
    int userBeenPressedAgain_processing_state;
};


typedef struct {
  switchStateManager arp_range;
  switchStateManager arp_dir;
  switchStateManager arp_latch;
  pushButtonStateManager arp_on;
  pushButtonStateManager poly;
  pushButtonStateManager unison;
  pushButtonStateManager chord_mem;
  pushButtonStateManager hold;
  pushButtonStateManager sustainPedal;
  pushButtonStateManager portamentoPedal;
  pushButtonStateManager fromTapeToggle;
} assignerButtonState2_t;

typedef struct {
  int noteShift[N_POLY];
  int isNoteActive[N_POLY];
  int voiceIndexOfBase;
  int detuneFactor[N_POLY];
} chordMemState_t;

class keybed_t {
  public:
    keybed_t(void);
    void resetAllKeybedData(void);
    void resetKeyPress(const int &);
    void addKeyPress(const int &,const int &);
    void createKeyPress(const int &, const int &, const int &);
    void stopKeyPress(const int &,const int &);
    void stopKeyPressByInd(const int &ind, const int &noteVel); 
    void stopAllKeyPresses(void);
    void printKeyPressState(void);
    keyPressData_t* getKeybedDataP(void) { return allKeybedData; }
    keyPressData_t* getKeybedDataP(const int &Ikey) { return &(allKeybedData[Ikey]); }
    int isGateActive(const int &Ikey);
    int isAnyGateActive(void);
    keyPressData_t allKeybedData[N_KEY_PRESS_DATA];
    void deactivateHold(void);
    int get_nKeyPressSlots(void) { return nKeyPressSlots; };
    void set_nKeyPressSlots(const int &);
    
    int findOldestKeyPress_NotPressedNotHeld(void);
    int findOldestKeyPress_NotPressed(void);
    int getOldestKeyPress(void);
    int getNewestKeyPress(void);
    int get2ndNewestKeyPress(void);
    void findNewestKeyPresses(int const &,int []);
    int findNewestPressedOrHeldKeys(const int &, int []);
    int findNewestReleasedKeys(int const &,int [], int const &);
    
  private:
    int nKeyPressSlots;
    void reduceAndConsolodate(const int &);
};

void deactivateHold(keybed_t *);

#define ARP_SORTMODE_NOTENUM 1
#define ARP_SORTMODE_STARTTIME 2
class arpManager_t {
  public:
    arpManager_t(keybed_t *, keybed_t *);
    void setInputAndOutputKeybeds(keybed_t *, keybed_t *);
    void startArp(const int &,const assignerState_t &);
    void startArp(const int &,const assignerState_t &,void (*)(void));
    void updateArp(const assignerState_t &);
    void incrementNoteAndOctave(const int &,const int &,const int &);
    void stopArp(void);
    void resetArp(void);
    void resetArpCounters(void);
    void nextNote(void);
    void updateLatchState(const assignerState_t &);
    void setArpGate_x7bits(const int &);
    int getArpGate_x7bits(void);
    void deactivateHold(void);

  private:
    keybed_t *inputKeybed;  //usually this is the "trueKeybed"
    keybed_t *outputKeybed;
    keyPressData_t sortedNotes[N_KEY_PRESS_DATA];
    int prevSortedNoteNums[N_KEY_PRESS_DATA];
    int incrementToNextNote;
    int arpSortMode;
    int curArpStep;
    int curArpOctave;
    int lastIssuedNoteNumber;
    int lastIssuedNoteNumber_wOctave;
    int curArpDirection;
    int defaultNoteOffVel;
    int resetCountersWhenKeysPressed;
    int arpGate_x7bits;  //1 to 127 where 127 is full length (zero not allowed) 
    int32_t expected_nUpdatedSinceLastNote;
    int32_t nUpdatesSinceLastNote;
    void checkAndResetArp(const assignerState_t &);
    void clearSortedNotes(void);
    void sortArpNotesByPitch(void);
    void sortArpNotesByStartTime(void);
    void adjustArpStepToFollowCurNote(void);
    int executePostNoteUpdateFunction;
    void (*p_postNoteUpdateFunction)(void);
  
};

#endif

