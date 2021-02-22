
#include "dataTypes.h"

#ifndef _ARP_MANAGER
#define _ARP_MANAGER

#define UP (+1)
#define DOWN (-1)

//constructor
arpManager_t::arpManager_t(keybed_t *in, keybed_t *out, keybed_givenlist_t *givenList)
{
  setInputAndOutputKeybeds(in,out,givenList);
  resetArp();
}

void arpManager_t::resetArp(void)
{
  //initialize variables
  resetArpCounters();
  arpSortMode = ARP_SORTMODE_NOTENUM;
  defaultNoteOffVel = 63;  //MIDI value from 0-127
  incrementToNextNote = false;
  //resetCountersWhenKeysPressed = false; //moved to resetArpCounters() on Jan 18, 2021
  lastIssuedNoteNumber = 0;
  lastIssuedNoteNumber_wOctave = 0;
  arpGate_x7bits = 127;
  expected_nUpdatedSinceLastNote = 100000L;
  nUpdatesSinceLastNote = 0;
  executePostNoteUpdateFunction = false;

  //clear the sorted notes
  clearSortedNotes();

  //reset the output keybed as well
  //outputKeybed->resetAllKeybedData();
  outputKeybed->stopAllKeyPresses();
}

void arpManager_t::resetArpCounters(void)
{
  curArpStep = -1;
  curArpOctave = 0;
  //incrementToNextNote = false;
  //lastIssuedNoteNumber = 0;
  curArpDirection = UP;
  resetCountersWhenKeysPressed = false; //moved here on Jan 18, 2021
}

void arpManager_t::startArp(const int &mode,const assignerState_t &localAssignerState)
{ 
  Serial.print(F("arpManager: startArp: mode = ")); Serial.println(mode);
  debug__call_count = 0; //reset this...only used for debugging

  //reset the arp counters
  resetArpCounters();
  
  //clear the sorted notes
  clearSortedNotes();
  
  //apply the latch state
  //updateLatchState(localAssignerState);
  
  //copy the given keybed to the output keybed
  if (mode == ARP_SORTMODE_GIVENLIST) {
    int source_ind = givenListKeybed->getNextInd();
    for (int Ikey=0; Ikey < outputKeybed->getMaxKeySlots() ; Ikey++) {
      source_ind--;  if (source_ind < 0) source_ind += givenListKeybed->getMaxKeySlots();
      outputKeybed->allKeybedData[Ikey] = givenListKeybed->allKeybedData[source_ind];
      outputKeybed->allKeybedData[Ikey].isHeld = OFF;
    }    
  } else {
    //this is the normal branch
    for (int Ikey=0; Ikey < outputKeybed->getMaxKeySlots() ; Ikey++) {
      outputKeybed->allKeybedData[Ikey] = inputKeybed->allKeybedData[Ikey];
      outputKeybed->allKeybedData[Ikey].isHeld = OFF;
    }
  }
  
  //stop all notes
  outputKeybed->stopAllKeyPresses();

  //set the arp's sort mode
  arpSortMode = mode;
  //arpSortMode = ARP_SORTMODE_NOTENUM; //this is the default
  //if (mode == ARP_SORTMODE_STARTTIME) {
  //  arpSortMode = ARP_SORTMODE_STARTTIME;
  //} else if (mode == ARP_SORTMODE_GIVENLIST) {
  //  arpSortMode = ARP_SORTMODE_GIVENLIST;
  //}
  
  //tell this object to stop the hold after the next update
  executePostNoteUpdateFunction = false;
  p_postNoteUpdateFunction = NULL;
  
//  Serial.println("arpManager: startArp: inputKeybed:");
//  inputKeybed->printKeyPressState();
//  Serial.println("arpManager: startArp: outputKeybed:");
//  outputKeybed->printKeyPressState();
//  
}

void arpManager_t::startArp(const int &mode, const assignerState_t &localAssignerState,void (*func)(void))
{
  startArp(mode,localAssignerState);
  executePostNoteUpdateFunction = true;
  p_postNoteUpdateFunction = func;
}


void arpManager_t::stopArp(void)
{
  int note1;
  boolean isActive;
  
  Serial.println(F("ArpManager: stopArp: stopping..."));

  //stop the last played note
  outputKeybed->stopKeyPress(lastIssuedNoteNumber_wOctave,defaultNoteOffVel);

  //clear the latching
  int n = inputKeybed->getMaxKeySlots();
  for (int Ikey=0;Ikey < n; Ikey++) {inputKeybed->allKeybedData[Ikey].isLatched = OFF; }
  n = givenListKeybed->getMaxKeySlots();
  for (int Ikey=0;Ikey < n; Ikey++) { givenListKeybed->allKeybedData[Ikey].isLatched = OFF; }
  
//  Serial.println("ArpManager: stopArp: inputKeybed:");
//  inputKeybed->printKeyPressState();
//  Serial.println("ArpManager: stopArp: outputKeybed:");
//  outputKeybed->printKeyPressState();
  
  //clear the update function...just in case
  p_postNoteUpdateFunction = NULL;
}


void arpManager_t::setInputAndOutputKeybeds(keybed_t *in, keybed_t *out, keybed_givenlist_t *givenList) 
{
  inputKeybed = in;
  outputKeybed = out;
  givenListKeybed = givenList;
}

int countActualActiveNotes(keyPressData_t keybedData[],int nKeysMax) {
  int nActive = 0;
  for (int Ikey=0;Ikey < nKeysMax; Ikey++) {
    if (keybedData[Ikey].isGateActive() == ON) {
      nActive++;
    }
  }
  return nActive;
}

int countArpActiveNotes(keyPressData_t keybedData[], int nKeysMax) {
  int nActive = 0;
  for (int Ikey=0;Ikey < nKeysMax; Ikey++) {
    if (keybedData[Ikey].isGateActiveOrLatched() == ON) {
      nActive++;
    }
  }
  return nActive;
}

void wrapOctave(int &curArpStep,int &curArpOctave,const int &nStepsPerOctave,const int &nOctaves) {
  if (curArpOctave >= nOctaves) {
    //reset to zero
    curArpOctave = 0;
  } 
  else if (curArpOctave < 0) {
    //reset to the top
    curArpOctave = nOctaves-1;
  }
}

void bounceOctave(int &curArpStep,int &curArpOctave,int &curArpDirection,const int &nStepsPerOctave,const int &nOctaves) {
  if (curArpOctave >= nOctaves) {
    //start heading down
    curArpDirection = DOWN;
    curArpStep = nStepsPerOctave-1;
    curArpOctave = nOctaves-1;
  } 
  else if (curArpOctave < 0) {
    //start heading up
    curArpDirection = UP;
    curArpStep = 0;
    curArpOctave = 0;
  }
}


void arpManager_t::incrementNoteAndOctave(const int &arpDirectionType,const int &nStepsPerOctave,const int &nOctaves)
{    
  if (nStepsPerOctave < 1) {
    curArpStep = -1;
    curArpOctave = 0;
    return;
  }

  switch (arpDirectionType) {
    case (ARP_DIR_UP):
      curArpDirection = UP;
      break;
    case (ARP_DIR_DOWN):
      curArpDirection = DOWN;
      break;
    case (ARP_DIR_UPDOWN):
      //keep whatever the previous state was
      break;
  }

  //increment the note
  if (curArpDirection == UP) {
    curArpStep++;
  } 
  else {
    curArpStep--;
  }

  //increment the octave
  if (curArpStep >= nStepsPerOctave) {
    //increase an octave
    curArpStep = 0;
    curArpOctave++;
  }
  if (curArpStep < 0) {
    //decrease an octave
    curArpStep = nStepsPerOctave-1;
    curArpOctave--;
  } 

  //wrap around on the octave
  switch (arpDirectionType) {
  case ARP_DIR_UP:
    wrapOctave(curArpStep,curArpOctave,nStepsPerOctave,nOctaves);
  case ARP_DIR_DOWN:
    wrapOctave(curArpStep,curArpOctave,nStepsPerOctave,nOctaves);
  case ARP_DIR_UPDOWN:
    bounceOctave(curArpStep,curArpOctave,curArpDirection,nStepsPerOctave,nOctaves);
  }
}

void arpManager_t::clearSortedNotes(void)
{
  for (int Inote = 1; Inote < N_KEY_PRESS_DATA; Inote++) {
    sortedNotes[Inote].reset();
  }
}

void arpManager_t::updateLatchState(const assignerState_t &localAssignerState)
{
  static int prev_latchState = false;
  int cur_latchState = localAssignerState.arp_params.arp_latch; 
  
  if (cur_latchState == OFF) {
    //latch is OFF.  Did it just transition to off?
    if (prev_latchState == ON) {
      //state changed to off, so loop through and turn off latching on each note
      int n = inputKeybed->getMaxKeySlots();
      for (int Ikey = 0; Ikey < n; Ikey++) { inputKeybed->allKeybedData[Ikey].isLatched = OFF; }
      n = givenListKeybed->getMaxKeySlots();
      for (int Ikey = 0; Ikey < n; Ikey++) { givenListKeybed->allKeybedData[Ikey].isLatched = OFF; }
    } 
  } else {
    //latch is ON, so activate the latch on each active note
    int n = inputKeybed->getMaxKeySlots();
    for (int Ikey = 0; Ikey < n; Ikey++) {
      if (inputKeybed->allKeybedData[Ikey].isGateActive() == ON) { inputKeybed->allKeybedData[Ikey].isLatched=ON; }
    }
    n = givenListKeybed->getMaxKeySlots();
    for (int Ikey = 0; Ikey < n; Ikey++) {
      if (givenListKeybed->allKeybedData[Ikey].isGateActive() == ON) { givenListKeybed->allKeybedData[Ikey].isLatched=ON; }
    }
  }
  prev_latchState = cur_latchState;
//  
//  Serial.println("arpManager: updateLatchState: inputKeybed:");
//  inputKeybed->printKeyPressState();
//  Serial.println("arpManager: updateLatchState: outputKeybed:");
//  outputKeybed->printKeyPressState();
}


void arpManager_t::checkAndResetArp(const assignerState_t &localAssignerState) 
{
  static int prev_nTrueKeysActive = 100; //some number larger than N_KEY_PRESS_DATA
  //static int firstTime_givenList = true;

  //count number of keys active
  int nTrueKeysActive;
  //if (arpSortMode == ARP_SORTMODE_GIVENLIST) {
  //  nTrueKeysActive = countActualActiveNotes(givenListKeybed->getKeybedDataP());
  //} else {
    nTrueKeysActive = countActualActiveNotes(inputKeybed->getKeybedDataP(),inputKeybed->getMaxKeySlots());
  //}

//  if (Serial) {
//    Serial.print("arpManager: checkAndResetArp: arp_latch = "); Serial.print(localAssignerState.arp_params.arp_latch);Serial.print(", arpSortMode = "); Serial.println(arpSortMode);
//    Serial.print("arpManager: checkAndResetArp: nTrueKeysActive = "); Serial.print(nTrueKeysActive);
//        Serial.print(", prevN = "); Serial.print(prev_nTrueKeysActive);
//        Serial.print(", resetCountersWhenKeysPressed = "); Serial.print(resetCountersWhenKeysPressed);
//        Serial.println();
//  }
  
  //check to see if the arp needs to be reset...due to all keys being released
  if (nTrueKeysActive == 0) {
    //now there are no keys pressed or held.  Did we just now release the keys?
    if (prev_nTrueKeysActive > 0) {
      //keys just released.  Let's reset.  How we reset depends upon the latch state.
      if (localAssignerState.arp_params.arp_latch == OFF) {
        //latch is off, so stop all the notes and reset the counters
        resetArpCounters();
        outputKeybed->stopAllKeyPresses();

        //the many-note psuedo sequencer ("given list" mode) should only be available the first time we enter arp.
        //if we're pressing more notes now, drop out of the "given list" mode back to normal starttime bode.
        if (arpSortMode == ARP_SORTMODE_GIVENLIST) {
          Serial.println(F("arpManager: checkAndResetArp 1: switching to ARP_SORTMODE_STARTTIME."));
          arpSortMode = ARP_SORTMODE_STARTTIME;
          nTrueKeysActive = countActualActiveNotes(inputKeybed->getKeybedDataP(),inputKeybed->getMaxKeySlots());
        }
      }
      
      resetCountersWhenKeysPressed = true;  //in either case, reset when a key is pressed
    }
  } else {
    //a key (or keys) are pressed
    if (resetCountersWhenKeysPressed == true) {
      Serial.println(F("checkAndResetArp: key pressed, reseting counters..."));
      
//      //the many-note psuedo sequencer ("given list" mode) should only be available the first time we enter arp.
//      //if we're pressing more notes now, drop out of the "given list" mode back to normal starttime bode.
//      if (arpSortMode == ARP_SORTMODE_GIVENLIST) {
//        if (firstTime_givenList) {
//          firstTime_givenList = false;
//        } else {
          //Serial.println("arpManager: checkAndResetArp 2: switching to ARP_SORTMODE_STARTTIME.");
          //arpSortMode = ARP_SORTMODE_STARTTIME;
          //nTrueKeysActive = countActualActiveNotes(inputKeybed->getKeybedDataP(),inputKeybed->getMaxKeySlots());
//        }
//      }
      
      //reset the Arp counters and stop all key presses
      resetArpCounters();
      outputKeybed->stopAllKeyPresses();

      int nMaxKeys = inputKeybed->getMaxKeySlots();
      for (int Ikey = 0; Ikey < nMaxKeys; Ikey++) { inputKeybed->allKeybedData[Ikey].isLatched = OFF;  }
      nMaxKeys = givenListKeybed->getMaxKeySlots();
      for (int Ikey = 0; Ikey < nMaxKeys; Ikey++) { givenListKeybed->allKeybedData[Ikey].isLatched = OFF; }

      resetCountersWhenKeysPressed = false;
    }
  }
  prev_nTrueKeysActive = nTrueKeysActive;
}

void arpManager_t::sortArpNotesByPitch(void)
{
  int Itest,Inote;
  int nBelow, endInd;
  int noteVal1, noteVal2;
  int inputKeybed_isNoteActive[N_KEY_PRESS_DATA];

  //initialize some stuff
  for (Inote=0;Inote<N_KEY_PRESS_DATA;Inote++) { 
    prevSortedNoteNums[Inote] = sortedNotes[Inote].noteNum; //make a copy of previous notes
    inputKeybed_isNoteActive[Inote] = inputKeybed->allKeybedData[Inote].isGateActiveOrLatched();
  }

  //find the correct order
  endInd=N_KEY_PRESS_DATA;
  for (Inote = 0; Inote < N_KEY_PRESS_DATA; Inote++) {
    if (inputKeybed_isNoteActive[Inote] == OFF) {
      //this note is off...just put it at the end of the list
      endInd--;
      sortedNotes[endInd] = inputKeybed->allKeybedData[Inote];  //stick on the end somewhere
    } 
    else {
      //this note is ON...find its spot at the beginning of the list
      noteVal1 = inputKeybed->allKeybedData[Inote].noteNum;
      nBelow = 0;
      for (Itest = 0; Itest < N_KEY_PRESS_DATA; Itest++) {
        if (Itest != Inote) {
          if (inputKeybed_isNoteActive[Itest] == ON) {
            noteVal2 = inputKeybed->allKeybedData[Itest].noteNum;
            if (noteVal2 < noteVal1) {
              nBelow++;
            } 
            else if (noteVal2 == noteVal1) {
              if (Itest < Inote) {
                //even though it has the same note number, if Istart is later than Inote, put Istart later.
                nBelow++;
              }
            }
          }
        }
      }
      sortedNotes[nBelow] = inputKeybed->allKeybedData[Inote];
      //sortedNotes[nBelow].isHeld = OFF; //turn off held note
    }
  }    
}

void arpManager_t::sortArpNotesByStartTime(void) {
  int Itest,Inote;
  int nBelow, endInd;
  millis_t noteVal1, noteVal2;
  int maxKeyPress = inputKeybed->getMaxKeySlots();
  int inputKeybed_isNoteActive[maxKeyPress];


  //initialize some stuff
  for (Inote=0; Inote < nSortedNotes; Inote++) { 
    prevSortedNoteNums[Inote] = sortedNotes[Inote].noteNum; //make a copy of previous notes for use by the adjustArpStepToFollowCurNote routine
  }
  for (Inote=0; Inote < maxKeyPress; Inote++) {
    inputKeybed_isNoteActive[Inote] = inputKeybed->allKeybedData[Inote].isGateActiveOrLatched();  //save this for use in this routine
  }

  //find the correct order
  endInd=maxKeyPress;
  for (Inote = 0; Inote < maxKeyPress; Inote++) {
    if (inputKeybed_isNoteActive[Inote] == OFF) {
      //this note is off...just put it at the end of the list
      endInd--;
      sortedNotes[endInd] = inputKeybed->allKeybedData[Inote];  //stick on the end somewhere
    } else {
      //this note is ON...find its spot at the beginning of the list
      noteVal1 = inputKeybed->allKeybedData[Inote].start_millis;
      nBelow = 0;
      for (Itest = 0; Itest < maxKeyPress; Itest++) {
        if (Itest != Inote) {
          if (inputKeybed_isNoteActive[Itest] == ON) {
            noteVal2 = inputKeybed->allKeybedData[Itest].start_millis;
            if (noteVal2 < noteVal1) {
              nBelow++;
            } else if (noteVal2 == noteVal1) {
              if (Itest < Inote) {
                //even though it has the same note number, if Istart is later than Inote, put Istart later.
                nBelow++;
              }
            }
          }
        }
      }
      //sortedNotes[nBelow].copyFrom(inputKeybed->allKeybedData[Inote]);
      sortedNotes[nBelow] = inputKeybed->allKeybedData[Inote];
      //if (executePostNoteUpdateFunction == true) {
        //this flag tells a different part of the code to turn off the hold state.  We should turn off the hold state of the copied notes.
        sortedNotes[nBelow].isHeld = OFF;  //turn off the hold
      //}
    }
  }    
}

void arpManager_t::sortArpNotesGivenOrder(void) {

  int maxKeyPress = givenListKeybed->getMaxKeySlots();


  bool printDebug = false;
//  if (debug__call_count < 3) {
//    debug__call_count++;
//    Serial.println("arpManager: sortArpNotesGivenOrder: copying notes over...");
//    printDebug=true;
//  }

  //find oldest active note in the list
  int start_ind = givenListKeybed->getNextInd() - 1; //start at the current active note (which is one back from "nextInd"
  if (start_ind < 0) start_ind += maxKeyPress;//wrap around, if needed
  int count = 0;
  while (((givenListKeybed->allKeybedData[start_ind]).isGateActiveOrLatched() == true) && (count < maxKeyPress)){  //find first note that is not active
    start_ind--; //step backward in time
    if (start_ind < 0) start_ind += maxKeyPress;//wrap around, if needed
    count++;
  } //should end on first note that is not active

  //correct the answer
  if (count == maxKeyPress) {
    //all notes are active...ignore everything that we just did and jump to the oldest note
    start_ind = givenListKeybed->getNextInd(); //the "next" note is also the oldest one
  } else { 
    if ((givenListKeybed->allKeybedData[start_ind]).isGateActiveOrLatched() == false) start_ind++;  //increment to get to next note, which should be active
    if (start_ind >= maxKeyPress) start_ind -= maxKeyPress;  //wrap around, if needed
  }

  //Copy notes into sortedNotes from oldest (active) to newest (active)
  int source_ind;
  for (int Inote=0; Inote < maxKeyPress; Inote++) { //walk forward, hopefully starting at the newest note
    source_ind = start_ind + Inote;   //compute the source of our key press
    if (source_ind >= maxKeyPress) source_ind -= maxKeyPress; //wrap around, if needed

    if (printDebug) {
      Serial.print(F("    : Note, = "));Serial.print(Inote); 
      Serial.print(F(", Source = ")); Serial.print(source_ind); 
      Serial.print(F(", Latched = ")); Serial.print((givenListKeybed->allKeybedData[source_ind]).isGateActiveOrLatched());
      Serial.print(F(", Note Num = ")); Serial.print((givenListKeybed->allKeybedData[source_ind]).noteNum);
      Serial.print(F(", Start = ")); Serial.print((givenListKeybed->allKeybedData[source_ind]).start_millis);
      Serial.println();
    }

    sortedNotes[Inote] = givenListKeybed->allKeybedData[source_ind]; //copy the keypress over to the sortedNotes (actually sortedKeypresses) array
    sortedNotes[Inote].isHeld = OFF;  //turn off the hold
  }
  
}

void arpManager_t::adjustArpStepToFollowCurNote(void)
{
  int bestInd=0;
  int targNoteNum = prevSortedNoteNums[curArpStep];
  boolean done = false;

  //for (bestInd = 0; bestInd < N_KEY_PRESS_DATA; bestInd++) {
  while ((bestInd < nSortedNotes) & !done) {
    //only check against arp-active notes
    if (sortedNotes[bestInd].isGateActiveOrLatched() == ON) {
      //this note is active, not check the note number
      if (sortedNotes[bestInd].noteNum == targNoteNum) {
        curArpStep = bestInd;  //adjust the current index to be on the current note
        done = true;
      }
    }
    bestInd++;
  } 
}

void arpManager_t::updateArp(const assignerState_t &localAssignerState)
{  
  //  if (Serial) { Serial.print("arpManager_t: updateArp...nUpdatesSinceLastNote = ");Serial.println(nUpdatesSinceLastNote); }
    
  //micros_t start = micros();
  nUpdatesSinceLastNote++;

  //check to see if arp needs to be reset...and do so, if necessary
  checkAndResetArp(localAssignerState);

  //update the latching state on any pressed keys
  updateLatchState(localAssignerState);

  //decide whether to turn off the note
  if (arpGate_x7bits < 127) { //only apply this criteria here if there is any gating requested
    //how many counts are needed to turn off the note
    int32_t threshold = (expected_nUpdatedSinceLastNote * arpGate_x7bits) >> 7;  //multiply by our gate than divide by 7bits
    if (nUpdatesSinceLastNote > threshold) {
      //yes, turn off the note
      if (lastIssuedNoteNumber > -1) {
        outputKeybed->stopKeyPress(lastIssuedNoteNumber_wOctave,defaultNoteOffVel); //turn off the previous note
        lastIssuedNoteNumber = -1; //is this necessary?  I'm not sure
      }
    }
  }

  //make the list of arp notes based on currently pressed notes...this translates from pressed notes to the buffer of arp notes
  int nSortedNotes;
  if (arpSortMode == ARP_SORTMODE_GIVENLIST) {
    sortArpNotesGivenOrder();   //output is in "sortedNotes"??
    nSortedNotes=givenListKeybed->getMaxKeySlots();
  } else if (arpSortMode == ARP_SORTMODE_STARTTIME) { 
    //do the notes in the order that they were pressed...a pseudo-sequencer mode
     sortArpNotesByStartTime();  //output is in "sortedNotes"??
    nSortedNotes=inputKeybed->getMaxKeySlots();
    
    //the "adjustArpStepToFollowCurNote" is not included here becuase
    //it prevents the arp from allowing the same note to exist
    //multiple times...which is important for this pseudo-sequencing
    //mode of the arp
  } else {
    sortArpNotesByPitch();   //output is in "sortedNotes"??
    nSortedNotes=inputKeybed->getMaxKeySlots();
        
    //in case the current note moved in the sort, adjust the current arp index to stay on the same noteNum
    adjustArpStepToFollowCurNote();
  }
  
  //issue the post-note-update function
  if ((executePostNoteUpdateFunction==true) & (p_postNoteUpdateFunction != NULL)) {
    p_postNoteUpdateFunction(); //usually this is the disableHoldState() function from the stateManager
    executePostNoteUpdateFunction = false;  //reset the flag so that we don't do this again next time through
  }

  //is it also time to increment to the next note?
  if (incrementToNextNote == true) {   
    //reset the flag 
    incrementToNextNote = false;

    //update the record of the amount of time between the start of the start of each arp note 
    expected_nUpdatedSinceLastNote = nUpdatesSinceLastNote;
    nUpdatesSinceLastNote = 0;  //reset the counter

    //stop the previous arp note (assuming that the gating didn't turn it off already)
    if (lastIssuedNoteNumber > -1) outputKeybed->stopKeyPress(lastIssuedNoteNumber_wOctave,defaultNoteOffVel);
  
    //increment to the next note (in the correct direction)
    int arpDirectionType = localAssignerState.arp_params.arp_direction;
    int nStepsPerOctave = countArpActiveNotes(sortedNotes,nSortedNotes); 
    int nOctaves = localAssignerState.arp_params.arp_range;
    incrementNoteAndOctave(arpDirectionType,nStepsPerOctave,nOctaves); //changes "curArpStep"

    //issue the new note
    lastIssuedNoteNumber = -1;lastIssuedNoteNumber_wOctave=-1;
    //if ((curArpStep >= 0) & (curArpStep < maxKeyPress)) {
    if ((curArpStep >= 0) & (curArpStep < nSortedNotes)) {
      //if (arpSortMode == ARP_SORTMODE_GIVENLIST) {
      //  // this is the weird branch
      //  int ind = givenListKeybed->getActiveNoteIndByOrder(curArpStep);
      //  if (ind >= 0) {
      //    lastIssuedNoteNumber = (givenListKeybed->getKeybedDataP(ind))->noteNum;
      //    lastIssuedNoteNumber_wOctave = lastIssuedNoteNumber + 12*curArpOctave;
      //    outputKeybed->addKeyPress(lastIssuedNoteNumber_wOctave,(givenListKeybed->getKeybedDataP(ind))->noteVel);
      //  }
      //} else {
        //this is the normal branch
        lastIssuedNoteNumber = sortedNotes[curArpStep].noteNum;
        lastIssuedNoteNumber_wOctave = lastIssuedNoteNumber + 12*curArpOctave;
        outputKeybed->addKeyPress(lastIssuedNoteNumber_wOctave,sortedNotes[curArpStep].noteVel,false);
      //}
    }
  }

  //micros_t total = micros() - start;
  //Serial.print("updateArp: micros = ");
  //Serial.println(total);
}

void arpManager_t::deactivateHold(void) {
  //deactivate hold on the arp's notes
  outputKeybed->deactivateHold();
}


void arpManager_t::setArpGate_x7bits(const int &val)
{
  arpGate_x7bits = constrain(val,1,127);
}

int arpManager_t::getArpGate_x7bits(void) {
  return arpGate_x7bits;
}

void arpManager_t::nextNote(void)
{
  incrementToNextNote = true;
}
#endif
