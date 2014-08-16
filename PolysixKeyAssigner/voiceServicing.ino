

#include "dataTypes.h"  //data type definitions and global macros
#include <KeyAssignerPinMap.ino>  //we need to know the pin mapping

#define DEBUG_THIS_FILE false
#define BIAS_PITCH_NOTES (12)

//initialization routine
void initializeVoiceData(voiceData_t voiceData[], const int &N)
{
  for (int i=0;i<N;i++) {
    voiceData[i].keyPressID=EMPTY_VOICE;  //index into keypress array
    voiceData[i].noteNum=0;
    voiceData[i].noteVel=0;
    voiceData[i].curNoteNum_x16bits = voiceData[i].noteNum << 16;
    voiceData[i].curAttackDetuneFac_x16bits = 0;
    voiceData[i].noteGate=LOW;  //high or low
    voiceData[i].start_millis=0; //when the note on the voice is started
    voiceData[i].end_millis=0;   //when the note on the voice is ended
    voiceData[i].forceRetrigger=false;
  }
}

//generic function to do the servicing
int voicePeriodIndex = 0;  //9 periods...V1-V6, Vx7, Vx8, Inter-Voice (IV)
millis_t totalMicros=0, fooMicros=0;;
int voice_count=0;
void serviceNextVoicePeriod(void)
{
  //increment to next voice
  voicePeriodIndex++;
  if (voicePeriodIndex > INDEX_LAST_PERIOD) voicePeriodIndex = 1;
    
  //decide what to do based on the voice period that we're in
  if (voicePeriodIndex <= INDEX_LAST_VOICE) {
    //a pitched voice
    serviceVoice(voicePeriodIndex);
  } else {
    //the intervoice period
    serviceInterVoice(voicePeriodIndex);
  }
    
  //once we've gone through the cycle, update the allocation of the voices
  if (voicePeriodIndex == INDEX_LAST_PERIOD) {
    updateVoiceAllocation();
    if (DEBUG_TO_SERIAL & DEBUG_THIS_FILE) trueKeybed.printKeyPressState();
    if (DEBUG_PRINT_VOICE_STATE) printVoiceState();
  }
}

//specific function for servicing a pitched voice (V1-V6, Vx7, Vx8)
void serviceVoice(const int &voiceInd)
{
  //clear the MC signal
  CLEAR_MC;  //return it to HIGH
  //delayMicroseconds(1000);
  
  //set the inhibit signal
  SET_INHIBIT;
  delayMicroseconds(5);  //added 2013-03-03
  
  //get the desired pitch value
  byte pitchByte = getPitchByte(voiceInd); // 256 note numbers
  WRITE_DB_BUS(pitchByte);
 
  //make the adjustment to the pitch value
  int32_t pitchAdjust_notes_x16bits = getPitchAdjust(voiceInd);
  setDAC_pitchNotes(pitchAdjust_notes_x16bits);
  
  delayMicroseconds(20); //was 5
  
  //update the LFO
  updateLFO(voiceInd);
  
  //set the ABC bus
  setABCbus(voiceInd);
  
  //wait for a moment before releasing the INHIBIT
  delayMicroseconds(5);
  
  //clear the inhibit signal
  CLEAR_INHIBIT;
  
  //set the MC signal, if voice Vx7
  if (voiceInd==7) SET_MC;
}

int32_t getPitchAdjust(const int &voiceInd)
//voiceInd goes from 1-9...of which 1-6 are the normal voices
{
  int32_t pitchAdjust_notes_x16bits = 0;
  
  pitchAdjust_notes_x16bits = ((int32_t)BIAS_PITCH_NOTES) << 16;  //positive drops the pitch
  pitchAdjust_notes_x16bits -= (2L<<16)/2L;  //adjust tuning...move higher by half a half-step.  Negative numbers move the pitch higher
  if (voiceInd != 7) {
    pitchAdjust_notes_x16bits -= porta_noteBend_x16bits; //negative raises the pitch
    pitchAdjust_notes_x16bits -= LFO_noteBend_x16bits; //negative raises the pitch
  }
  
  //get fixed detuning
  if ((voiceInd <= N_POLY) & (assignerState.detune == ON)) {
    if (assignerState.chord_mem == ON) {
      //detuning for chord memory notes
      pitchAdjust_notes_x16bits += (chordMemState.detuneFactor[voiceInd-1]*detune_noteBend_x16bits);
    } else if ((assignerState.poly == ON) & (assignerState.unison == ON)) {  //this is UNISON_POLY
      pitchAdjust_notes_x16bits += (unisonPolyDetuneFactors[voiceInd-1]*detune_noteBend_x16bits);
    } else if (assignerState.unison == ON) {
      pitchAdjust_notes_x16bits += (unisonDetuneFactors[voiceInd-1]*detune_noteBend_x16bits);
    } else {
      //detuning for all other states
      pitchAdjust_notes_x16bits += (defaultDetuneFactors[voiceInd-1]*detune_noteBend_x16bits);
    }   
  }
  
//  //get attack-related detuning
//  if (voiceInd <= N_POLY) {
//    pitchAdjust_notes_x16bits += getAttackDetune(voiceInd);
//  }
  
  //return the pitchAdjust
  return pitchAdjust_notes_x16bits;
}

void setDAC_pitchNotes(const int32_t &pitchAdjust_notes_x16bits)
{
  // apply the pitch correction via the DAC
  int32_t adjust_counts_x16bits = ((pitchAdjust_notes_x16bits >> 4)*((int32_t)DAC_COUNTS_PER_NOTE_x8bits)) >> (8-4);
  dac.setVoltage((int)(adjust_counts_x16bits >> 16),false);
}

//determine what byte to push out over the DB bus based on the pitch
//assigned to the current voice
byte getPitchByte(const int &voiceInd)
{
  int32_t targNoteNum_x16bits = 0;  //zero is lowest note on the Polysix
  int32_t curNoteNum_x16bits = 0; //128
  #define NoteNumOffsetForRounding (32768L)  // (2^(16 bits)-1) / 2 = (32768) 
  
  if (voiceInd == 7) {
    //always return C7 (top note of the PolySix)
    curNoteNum_x16bits = ((long)C7_NOTE_NUM) << 16;
  } else {
    if (voiceInd==8) {
      //always return the pitch byte for voice 1
      //targNoteNum=allVoiceData[0].noteNum; //target pitch
      curNoteNum_x16bits=allVoiceData[0].curNoteNum_x16bits; //this is the value last used for note 0, so that's all we need
    } else {
      //for voices 1-6, get the note number of the current voice index (where C0 on the Polysix s zero)
      targNoteNum_x16bits=((long)allVoiceData[voiceInd-1].noteNum) << 16; //target pitch
      if (assignerState.octave == OCTAVE_8FT) {
        targNoteNum_x16bits +=(12L << 16);
      } else if (assignerState.octave == OCTAVE_4FT) {
        targNoteNum_x16bits +=(24L << 16);
      }
      curNoteNum_x16bits=allVoiceData[voiceInd-1].curNoteNum_x16bits; //current pitch
      
      //compute portamento
      int32_t des_jump = targNoteNum_x16bits-curNoteNum_x16bits;
      int32_t increment_x16bits =  des_jump / porta_time_const[porta_setting_index];
      increment_x16bits = (increment_x16bits * ((int32_t)(5+voiceInd))) >> 3;  //multiply by 6/8, 7/8, 8/8, 9/8, 10/8, or 11/8 to spread the voices
      int32_t foo_porta_minstep = porta_minstep[porta_setting_index]; //twiddle the step factor per voice to let the voices diverge
      if (abs(increment_x16bits) < foo_porta_minstep) {
        if (des_jump > 0) { 
          increment_x16bits = min(foo_porta_minstep,des_jump); 
        } else { 
          des_jump = -des_jump;
          increment_x16bits = min(foo_porta_minstep,des_jump); 
          increment_x16bits = -increment_x16bits;
        }
      }
      curNoteNum_x16bits += increment_x16bits; //update the current pitch
      
      //save the pitch back into the voice data structure for next use by portamento calcs
      if (assignerState.portamento == 0) {curNoteNum_x16bits = targNoteNum_x16bits;};
      allVoiceData[voiceInd-1].curNoteNum_x16bits = curNoteNum_x16bits; //save the current pitch
    }
  }
    
  //get the byte to output
  int outNoteNum = (int)((curNoteNum_x16bits + (NoteNumOffsetForRounding-1)) >> 16);
  //int outNoteNum = (int)(curNoteNum_x16bits >> 16);
  
  //get the pitch correction to output that'll be used
  porta_noteBend_x16bits = curNoteNum_x16bits - (((int32_t)outNoteNum) << 16); //THIS IS AN OUTPUT
  
  //if ((voiceInd <= N_POLY) | (voiceInd == 8)) {
    outNoteNum += BIAS_PITCH_NOTES;  //bias to make room for other pitch shifting effects
  //}
  
  return ((byte)outNoteNum);
}
      
//specific function for servicing the inter-voice (IV) period
void serviceInterVoice(const int &voiceInd)
{
  //clear the MC signal
  CLEAR_MC;
  
  //set the inhibit signal
  SET_INHIBIT;
  
  //update the LFO
  updateLFO(voiceInd);
    
  //service the gates and pulse Latch 1
  setGatePins();  
  delayMicroseconds(5);  //added 2013-03-03
  SET_LATCH1;
  delayMicroseconds(5); //already seems to be ~5us...so comment this
  CLEAR_LATCH1;
  
  //stall a bit
  delayMicroseconds(10); //gives about 15 usec
  
  //service the LEDS and pulse Latch 2
  setLEDPins();  
  delayMicroseconds(5);  //added 2013-03-03
  SET_LATCH2;  
  delayMicroseconds(5); //already seems to be ~5us...so comment this
  CLEAR_LATCH2;
  delayMicroseconds(5);  //added 2013-03-03
  
  //Step through the DB bus to scan the keyboard
  //...we get to skip this step
  SET_DBBUS_HIGH;
  delayMicroseconds(5);  //added 2013-03-03
  
  //Read the switches (note: it changes the ABC bus state)
  if (!(DEBUG_DISABLE_BUTTONS)) readButtons(); //updates assignerButtonState and prevAssignerButtonState
  
  //Update the state of the keyAssigner based on button presses
  updateKeyAssignerStateFromButtons(assignerButtonState);
  
  //Reset (or not) the arp clock
  checkAndResetArpClock(assignerState,trueKeybed);
  
  //What now?  I'm not sure what the Polysix is doing during all of the IV period
  //...let's do nothing since I don't know what else needs to be done.
  
  //Leave the INHIBIT in its "set" state
}

void checkAndResetArpClock(const assignerState_t &localAssignerState, keybed_t &keybed)
{
  //also set the ACKR if any voices are high
  int ACKR = HIGH;
  if (localAssignerState.arp == ON) {
    ACKR = LOW;
    if (keybed.isAnyGateActive() == true) {
      ACKR = HIGH;
    }
  }
  set_ACKR(ACKR);
}

//Here's a some tricky logic.  How do we allocate the given keys to the 
//available voices?  This must also handle the case of being in the arpeggiator,
//or in chord memory, or unison, or weird combinations thereof.
void updateVoiceAllocation(void)
{
  //keyPressData_t *keybedData = trueKeybed.getKeybedDataP();
  keybed_t *keybed = &trueKeybed;
  
  if ((ARP_AS_DETUNE==false) & (assignerState.arp == ON)) {
    //update the arpeggiator
    arpManager.updateArp(assignerState);
    
    //re-point the keybed data to what the ARP is generating
    keybed = &arpGeneratedKeybed;
  }
    
  //Let's get the key presses corresponding to the 6 newest pitches
  if ((assignerState.unison == ON) & (assignerState.poly == ON)) {
    allocateVoiceForUnisonPoly(keybed);
  } else if ((assignerState.chord_mem == ON) & (assignerState.poly == ON)) {
//    allocateVoiceForChordPoly(keybed);
  } else if (assignerState.poly==ON) {
    allocateVoiceForPoly(keybed);
  } else if (assignerState.unison == ON) {
    allocateVoiceForUnison(keybed);
  } else if (assignerState.chord_mem == ON) {
    allocateVoiceForChordMem(keybed);
  }
}

//allocate the voices for poly mode
void allocateVoiceForPoly(keybed_t *keybed)
{
  boolean isNoteChanging = false;
  int newInd = 0;
  int Ikey;
  keyPressData_t *allKeybedData = keybed->getKeybedDataP();
  int nVoices = keybed->get_nKeyPressSlots();
  
  //there are only six voices and six keypress slots, so just pass them through
  for (int Ivoice=0;Ivoice < nVoices;Ivoice++) { 
    if (allVoiceData[Ivoice].noteNum == allKeybedData[Ivoice].noteNum) {
      isNoteChanging = false;
    } else { 
      isNoteChanging = true;
    }
    
    Ikey = Ivoice; //this is the key assumption for this function
    //assignKeyPressToVoice(keybedData,Ikey,Ivoice);
    assignKeyPressToVoice(allKeybedData,Ikey,Ivoice);
    
//    if (allVoiceData[Ivoice].forceRetrigger == true) {
//      //set the attack detune factor based on the velocity
//      int attackScaleFac = allKeybedData[Ivoice].noteVel;
//      //attackScaleFac = (attackScaleFac*attackScaleFac)>>7;
//      allVoiceData[Ivoice].curAttackDetuneFac_x16bits = ((int32_t)attackScaleFac)*(detune_noteBend_x16bits>>3);
//    }
    
    if (isNoteChanging) { //is the pitch of this voice changing? 
      //for the polyphonic portamento, set the "previous" pitch of this voice to the previous note played by any voice
      newInd = keybed->get2ndNewestKeyPress(); //find the previous note...this should probably search the VoiceData structure, not the keybed structure
      if (newInd >= 0) { //if it is valid, proceed
        //allVoiceData[Ivoice].curNoteNum_x16bits = ((long)allKeybedData[newInd].noteNum) << 16;  //change the portamento behavior for Poly mode
        
        //give the current voice the extended pitch value of the previous voice
        allVoiceData[Ivoice].curNoteNum_x16bits = allVoiceData[newInd].curNoteNum_x16bits;  //assume 1:1 mapping of keybed to voice
      }
    }  
  }
}

//allocate voices for unison
void allocateVoiceForUnison(keybed_t *keybed)
{
  static int newestKeyInd[1];
  static int nToFind=1;
  static boolean noteIsChanging = false;
  keyPressData_t *keybedData = keybed->getKeybedDataP();
  
  //find newest keypress
  keybed->findNewestKeyPresses(nToFind,newestKeyInd);
  
  //is this a change in voice?
  if (allVoiceData[0].noteNum == keybedData[newestKeyInd[0]].noteNum) {
    noteIsChanging = false;
  } else {
    noteIsChanging = true;
  }
  
  //allocate voices
  int newInd = 0;
  for (int Ivoice=0;Ivoice<N_POLY;Ivoice++) { 
    assignKeyPressToVoice(keybedData,newestKeyInd[0],Ivoice); 
    if (noteIsChanging) { allVoiceData[Ivoice].forceRetrigger = true; } //force retrigger if new note 
  }
}

//allocate voices for unison-poly...where 3 notes can be played, all detuned
void allocateVoiceForUnisonPoly(keybed_t *keybed)
{
  
  allocateVoiceForPoly(keybed);
  
  //copy the three voices to the other three voices
  int secondVoice = 0;
  for (int Ivoice=0; Ivoice < keybed->get_nKeyPressSlots(); Ivoice++) { //in this mode, only the bottom 3 slots are used
    secondVoice = Ivoice+3;  //copy into the upper 3 slots
    if (secondVoice < N_POLY) {
      allVoiceData[secondVoice] = allVoiceData[Ivoice];
    }
  }
  
}


void allocateVoiceForChordMem(keybed_t *keybed)
{
  static int newestKeyInd[1];
  static int nToFind=1;
  static boolean noteIsChangingOrRestarting = false;
  keyPressData_t *keybedData = keybed->getKeybedDataP();
  
  //find newest keypress
  keybed->findNewestKeyPresses(nToFind,newestKeyInd);
  
  //is this a change in voice?
  if ((allVoiceData[chordMemState.voiceIndexOfBase].noteNum != keybedData[newestKeyInd[0]].noteNum) |
      (keybedData[newestKeyInd[0]].forceRetrigger == true)) {
    noteIsChangingOrRestarting = true;
  } else {
    noteIsChangingOrRestarting = false;
  }
  
  //allocate voices
  int newNote=0;
  for (int Ivoice=0;Ivoice<N_POLY;Ivoice++) {
    assignKeyPressToVoice(keybedData,newestKeyInd[0],Ivoice); 
    newNote =  keybedData[newestKeyInd[0]].noteNum + chordMemState.noteShift[Ivoice];//shift the note
    allVoiceData[Ivoice].noteNum = constrain(newNote,0,119);  
    if (chordMemState.isNoteActive[Ivoice]==LOW) {
      allVoiceData[Ivoice].noteGate=LOW;
    } else {
      if (noteIsChangingOrRestarting) { allVoiceData[Ivoice].forceRetrigger = true; } //force retrigger if new note
    }
  }
}

//print out the current voice state
void printVoiceState(void)
{
  Serial.print("Voices    : ");
  for (int i=0;i<N_POLY;i++) {
    Serial.print(i);
    Serial.print("=");
    Serial.print(allVoiceData[i].noteNum);
    if (allVoiceData[i].noteGate == HIGH) {
      Serial.print("A");
    } else {
      Serial.print(" ");
    }
    Serial.print(" , ");
  } 
  Serial.println();
}


//now, assign a keypress to a voice
//void assignKeyPressToVoice(int const &I_key,int const &I_voice)
void assignKeyPressToVoice(keyPressData_t *keybedData,int const &I_key,int const &I_voice)
{
  //if ((I_key > -1) & (I_key < N_KEY_PRESS_DATA) & (I_voice > -1) & (I_voice < N_POLY)) {
    //is valid, go ahead
    allVoiceData[I_voice].keyPressID = I_key;  //index into keypress array
    allVoiceData[I_voice].noteNum = keybedData[I_key].noteNum;
    allVoiceData[I_voice].noteVel = keybedData[I_key].noteVel;
    allVoiceData[I_voice].noteGate = keybedData[I_key].isGateActive();
    allVoiceData[I_voice].start_millis =  keybedData[I_key].start_millis;
    allVoiceData[I_voice].end_millis = keybedData[I_key].end_millis;
    allVoiceData[I_voice].forceRetrigger = keybedData[I_key].forceRetrigger;
    
    keybedData[I_key].forceRetrigger = false; //clear this flag once it has been copied over to the voice data
  //}
}



//int32_t getAttackDetune(const int &voiceInd) 
//{
//  int32_t given_attackDetune = allVoiceData[voiceInd-1].curAttackDetuneFac_x16bits;
//  int32_t new_attackDetune = 0;
//  if (voiceInd != N_VOICE_SLOTS-1) //do this for all voices other than the high C
//  {    
//    //reduce the magnitude
//    int32_t voiceDecay = detune_decay_x16bits;
//    voiceDecay -= (allVoiceData[voiceInd-1].noteVel*20);
//    if (voiceDecay < 10L) voiceDecay = 10L;
//    new_attackDetune = abs(given_attackDetune) - voiceDecay;
//
//    //limit the range
//    if (new_attackDetune < 0) new_attackDetune = 0;
//    
//    //flip the sign...the equations above mean that it's already positive, so just
//    //implement the condition that would make it negative
//    //if (given_attackDetune > 0) new_attackDetune = -new_attackDetune;
//    
//    //assign back to the voice
//    allVoiceData[voiceInd-1].curAttackDetuneFac_x16bits = new_attackDetune;
//    return new_attackDetune;
//  } else {
//    //this is the high-C voice.  Force to zero
//    allVoiceData[voiceInd-1].curAttackDetuneFac_x16bits = 0L;
//    return 0L;
//  }
//}

void updateLFO(int voiceInd) {
  
  static micros_t cur_LFO_period_micros = 100000;
  static uint32_t phase_increment = 1000;
  static uint32_t cur_LFO_phase = 0;
  static micros_t prev_micros = 0;
  long duty = 0;
  static int callCount=0;
  
  //see if the LFO period is correct
  if (abs(cur_LFO_period_micros - ARP_period_micros) > 100)
  {
     //change the timer period
     cur_LFO_period_micros = ARP_period_micros;
     
     //change the phase increment
     long n_steps = (long)(cur_LFO_period_micros / ((micros_t)(voiceDuration_usec)));
     phase_increment = ((unsigned long)0xFFFFFFFF)/((unsigned long)n_steps);
     //Serial.println("LFO adjust");
  }
   
  //update the LFO
  cur_LFO_phase += phase_increment; //32bit number
  
  //calculate the waveform
  long l_value = (cur_LFO_phase);
  //switch (LFO_type) {
  //  case LFO_TRIANGLE:
      l_value = l_value >> 16;
      if (l_value < 0); l_value = abs(l_value);
      l_value -= 16384;
 //     break;
 // }
  
  //calculate the note bend for the mod wheel or aftertouch
  LFO_noteBend_x16bits = l_value * aftertouch_scale[aftertouch_setting_index];  //was 3, how many notes for fullscale bend
  LFO_noteBend_x16bits = (LFO_noteBend_x16bits * aftertouch_val) >> 7;  //aftertouch can be 0-127, which means divide by 127, which means 7 bits
  
  //l_aftertouch_val = (long)(aftertouch_val*2);
  //LFO_noteBend_x16bits = (LFO_noteBend_x16bits*l_aftertouch_val) >> 7;  //aftertouch can be 0-127, which means divide by 127, which means 7 bits

}

     
