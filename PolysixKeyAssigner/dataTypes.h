

#ifndef _dataTypes_H
#define _dataTypes_H

//define some global macros

#define DEBUG_TO_SERIAL (false)  //print MIDI processng debug messages to the Serial terminal
#define DEBUG_DISABLE_BUTTONS (false)
#define DEBUG_PRINT_VOICE_STATE (false)

#define ARP_AS_DETUNE false

#define ECHOMIDI (false)  
#define SHIFT_MIDI_NOTES (-36)

#define EMPTY_VOICE -999

#define N_KEY_PRESS_DATA 6   //should be 6
#define N_POLY 6    //should be 6
#define N_VOICE_SLOTS 8
#define N_KEY_LIST_LEN 16  //for sequencer
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
#define RELEASE_VELOCITY_FROM_HOLD (96)  //could be zero to 127
#define KEYPANEL_MODE_ARP (1)
#define KEYPANEL_MODE_OTHER (2)
#define KEYPANEL_MODE_TUNING (3)

#define STATE_DEFAULT (-1)
#define STATE_POLY (10)
#define STATE_UNISON (20)
#define STATE_UNISON_POLY (25)
#define STATE_CHORD (30)
#define STATE_CHORD_SMART (32)
#define STATE_CHORD_POLY (35)

#define OCTAVE_16FT (0)
#define OCTAVE_8FT (1)
#define OCTAVE_4FT (2)

#define LFO_PWM_MICROS (200)
#define LFO_BITS 10
#define LFO_MAX 1024
#define LFO_MIN 0
#define LFO_MID 511
#define LFO_TRIANGLE 1
#define LFO_SQUARE 2      //isn't really used for anything (yet)
#define LFO_SQUARE_WITH_MID 3  //isn't really used for anything (yet)
#define MAX_10BIT (1023)
#define MAX_12BIT (4095)
#define DAC_COUNTS_PER_NOTE_x8bits (8736)  //12-bit divided by 10 octaves with 12 notes per octave...all assuming 5V full scale and 0.5V/octave scaling
//define DAC_COUNTS_PER_NOTE_x8bits (8806)  //12-bit divided by 10 octaves with 12 notes per octave...all assuming 4.960V full scale and 0.5V/octave scaling
#define FULL_SCALE_INT16 (32768)
#define N_OCTAVES_TUNING (8)  //how many entries will be in the tuning table
  
typedef unsigned long millis_t;
typedef unsigned long micros_t;
typedef long int int32_t;

//define data structure for describing key presses
class keyPressData_t {
  public:
    keyPressData_t(void) { reset(); };
    int noteNum;  //target pitch, rounteded to nearest note
    long int targNoteNum_x16bits;  //target pitch, higher resolution to handle robbon
    int noteVel;
    //unsigned int noteReleaseVel;
    int isPressed=0; //ON or OFF
    int isHeld=0;    //ON or OFF
    int isLatched=0; //ON or OFF
    bool isRibbon=false;
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
      if (isGateActive() || isLatched) {
        return ON;
      } else {
        return OFF;
      }
    };
    void reset(void) {
      setNoteNum(0);  noteVel = 0;  isPressed = OFF;  isHeld = OFF; isLatched = OFF;
      isRibbon = false;
      start_millis = 0;  end_millis = 0;  forceRetrigger = true; isNewVelocity = OFF;
    };
    int setNoteNum(float noteNum_float) {
      noteNum = (int)(noteNum_float+0.5f);
      targNoteNum_x16bits = (long int)((noteNum_float * (65536.0f))+0.5f); //left shift by 16 bits and convert to long int
      return noteNum;
    }
    int setNoteNum(int _noteNum) {
      noteNum = _noteNum;
      targNoteNum_x16bits = ((long)_noteNum) << 16; //left shift by 16 bits and convert to long int
      return noteNum;
    }
    int setNoteNum(long int _noteNum_x16bits) {
      noteNum = (int)(_noteNum_x16bits >> 16);
      targNoteNum_x16bits = _noteNum_x16bits;
      return noteNum;
    }
  private:
};


//data structure defining what's happening in a voice
class voiceData_t {
  public:
    
    int keyPressID;  //index into keypress array
    int noteNum; //target note number
    long int targNoteNum_x16bits;    //target pitch of note, higher resolution (to handle in-between pitches from ribbon, for example)
    int32_t curNoteNum_x16bits; //current pitch of note (might not match the target pitch due to portamento, for instance)
    int32_t curAttackDetuneFac_x16bits;
    bool isRibbon=false;
    int noteVel;
    //unsigned int noteReleaseVel;
    int noteGate;  //high or low
    millis_t start_millis; //when the note on the voice is started
    millis_t end_millis;   //when the note on the voice is ended
    int forceRetrigger;
    int isNewVelocity;

    int setNoteNum(int _noteNum) {
      noteNum = _noteNum;
      targNoteNum_x16bits = ((long)_noteNum) << 16; //left shift by 16 bits and convert to long int
      return noteNum;
    }
    int setNoteNum(float noteNum_float) {
      noteNum = (int)(noteNum_float+0.5f);
      targNoteNum_x16bits = (long int)((noteNum_float * (65536.0f))+0.5f); //left shift by 16 bits and convert to long int
      return noteNum;
    }
    int setNoteNum(long int _noteNum_x16bits) {
      noteNum = (int)(_noteNum_x16bits >> 16);
      targNoteNum_x16bits = _noteNum_x16bits;
      return noteNum;
    }
};



typedef struct {
  int arp_latch;
  int arp_direction;
  int arp_range;
} arp_parameters_t;

#define TUNING_MODE_ALL_VOICES (0)   //I'm not sure these are used for anything
#define TUNING_MODE_ALL_OCT_OF_ONE_VOICE (1)  //I'm not sure these are used for anything
#define TUNING_MODE_ONE_OCT_OF_ONE_VOICE (2)  //I'm not sure these are used for anything
class tuningModeState_t {
  public:
    tuningModeState_t(void) {}
    int voiceCommanded = 0;  //zero to N_POLY     //I'm not sure these are used for anything
    int adjustmentMode = TUNING_MODE_ALL_VOICES;  //I'm not sure these are used for anything
};

class assignerState_t {
  public:
    assignerState_t(void) {
      init();
    }
    void init(void) {
      arp=OFF;
      poly=ON;
      unison=OFF;
      chord_mem=OFF;
      chord_mem_smart=OFF;
      hold=HOLD_STATE_OFF;
      octave = OCTAVE_16FT;
      legato = OFF;
      portamento = OFF;
      detune = OFF;
      keypanel_mode = KEYPANEL_MODE_ARP;
      velocity_sens_8bit = MAX_VELOCITY_SENS;
    };
    int arp;
    int poly;
    int unison;
    int chord_mem;
    int chord_mem_smart;
    int hold;
    int octave;
    int legato;
    int portamento;
    int detune;
    int keypanel_mode;
    int velocity_sens_8bit;
  
    static const int MIN_VELOCITY_SENS;
    static const int MAX_VELOCITY_SENS;
    
    arp_parameters_t arp_params;
} ;
const int assignerState_t::MIN_VELOCITY_SENS = 0;  //for velocity_sens_8bit
const int assignerState_t::MAX_VELOCITY_SENS = 255; //velocity_sens_8bit

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


class assignerButtonState2_t {
  public:
    assignerButtonState2_t(void) {};  //empty constructor
    
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
    pushButtonStateManager tapeEnableToggle;
};

boolean isHoldNewNote(void);  //defined in stateManagement.ino

#include "keybed.h"

void deactivateHold(keybed_t *); //defined where?

#define ARP_SORTMODE_NOTENUM 1
#define ARP_SORTMODE_STARTTIME 2
#define ARP_SORTMODE_GIVENLIST 3
class arpManager_t {
  public:
    arpManager_t(keybed_t *, keybed_t *, keybed_givenlist_t *);
    void setInputAndOutputKeybeds(keybed_t *, keybed_t *, keybed_givenlist_t *);
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
    keybed_givenlist_t *givenListKeybed;
    static const int nSortedNotes = N_KEY_LIST_LEN; //maximum of N_KEY_LIST_LEN and N_KEY_PRESS_DATA
    keyPressData_t sortedNotes[nSortedNotes]; 
    int prevSortedNoteNums[nSortedNotes];
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
    void sortArpNotesGivenOrder(void);
    void adjustArpStepToFollowCurNote(void);
    int executePostNoteUpdateFunction;
    void (*p_postNoteUpdateFunction)(void);

    int debug__call_count = 0;
  
};

#include "ChordMem.h"

#endif
