
#include "dataTypes.h"

//const int keybed_t::maxNKeyPressSlots = N_KEY_PRESS_DATA;

keybed_t::keybed_t(void) {
  //initialize each keypress
   resetAllKeybedData();
   nKeyPressSlots = maxNKeyPressSlots; //default value
}


void keybed_t::resetAllKeybedData(void) {
  int n = getMaxKeySlots();
  for (int Ikey = 0; Ikey < n; Ikey++) {
    resetKeyPress(Ikey);
  }
}
  
void keybed_t::resetKeyPress(const int &noteIndex) 
{
  int n = getMaxKeySlots();
  if (noteIndex < n) {
    //allNotes[noteIndex].noteID = 0;
    int noteNum = allKeybedData[noteIndex].noteNum; //do I really need to keep the noteNum when resetting?
    allKeybedData[noteIndex].reset();
    //allKeybedData[noteIndex].noteNum = noteNum;
    allKeybedData[noteIndex].setNoteNum(allKeybedData[noteIndex].targNoteNum_x16bits);
  }
}

//use this function to toggle between the 6 voices of poly mode, the 3 voices of 
//poly-unison and poly-chordmem
void keybed_t::set_nKeyPressSlots(const int &newVal) 
{
  int val = min(getMaxKeySlots(),newVal);
  //Serial.print("keybed.set_nKeyPressSlots: setting to ");
  //Serial.println(val);
  
  if ( val < nKeyPressSlots ) {
    //getting smaller...reduce and consolodate
    reduceAndConsolodate(val);
  }
  //now set the value
  nKeyPressSlots = val;   
}

int keybed_t::addKeyPress(const int &noteNum,const int &noteVel) {
  return addKeyPress((float)noteNum, noteVel);
}
int keybed_t::addKeyPress(const float &noteNum_float,const int &noteVel) {
  int ind;
  
  //Serial.print("keybed.addKeyPress: nKeyPressSlots = ");
  //Serial.println(nKeyPressSlots);
  
  //look for oldest empty note (not pressed nor held)
  ind = findOldestKeyPress_NotPressedNotHeld();
  
  if (ind < 0) {
    //there are no available notes, so try to get the oldest one not pressed but maybe it is held
    ind = findOldestKeyPress_NotPressed();
    
    if (ind < 0) {
      //there are no available notes, so just get oldest note
      ind = getOldestKeyPress();
    }
  }
  
//  if (DEBUG_TO_SERIAL) {
//    Serial.print("addKeyPress: note = ");
//    Serial.print(noteNum);
//    Serial.print(" going to keypress slot index = ");
//    Serial.println(ind);
//  }
  
  //create this note
  resetKeyPress(ind);
  createKeyPress(ind,noteNum_float,noteVel);

  //Serial.print("keybed::addKeyPress: vel = "); Serial.print(noteVel); Serial.print(", remapped: "); Serial.println(remapVelocityValues(noteVel,assignerState.velocity_sens_8bit));
  return ind;
}

int keybed_t::findOldestKeyPress_NotPressedNotHeld(void) {
  int return_ind = -1;
  millis_t oldest_millis = 0xFFFFFFFF;  //init to biggest number
 
  for (int i=0; i < nKeyPressSlots; i++) {
    if ((allKeybedData[i].isPressed==OFF) & (allKeybedData[i].isHeld==OFF)) {
      //this key has been released...find out which one was released the longest ago
      if (allKeybedData[i].end_millis < oldest_millis) {
        oldest_millis = allKeybedData[i].end_millis;
        return_ind = i;
      }
    }
  }   
  return return_ind;
}

int keybed_t::findOldestKeyPress_NotPressed(void) {
  int return_ind = -1;
  millis_t oldest_millis = 0xFFFFFFFF; //init to biggest number

  for (int i=0; i < nKeyPressSlots; i++) {
    if (allKeybedData[i].isPressed==OFF) {
      //this key has been released...find out which one was released the longest ago
      if (allKeybedData[i].end_millis < oldest_millis) {
        oldest_millis = allKeybedData[i].end_millis;
        return_ind = i;
      }
    }
  }   
  return return_ind;
}

int keybed_t::getOldestKeyPress(void) {
  int oldest_ind = -1;
  millis_t oldest_millis = 0xFFFFFFFF;  //init to biggest number
  
  for (int i=0; i<nKeyPressSlots; i++) {
    if (allKeybedData[i].start_millis < oldest_millis) {
      oldest_millis = allKeybedData[i].start_millis;
      oldest_ind = i;
    }
  }
  return oldest_ind;
}

int keybed_t::getNewestKeyPress(void) {
  int newest_ind = 0;
  millis_t newest_millis = 0;
  
  newest_millis = allKeybedData[newest_ind].start_millis;
  for (int i=1; i<nKeyPressSlots; i++) {
    if (allKeybedData[i].start_millis > newest_millis) {
      newest_millis = allKeybedData[i].start_millis;
      newest_ind = i;
    }
  }
  return newest_ind;
}

int keybed_t::get2ndNewestKeyPress(void) {
  int newest_ind = 0;
  int newest2_ind = 0;
  millis_t newest_millis = 0;
  millis_t newest2_millis = 0;
  
  for (int i=0; i < nKeyPressSlots; i++) {
    if (allKeybedData[i].start_millis > newest_millis) {
      //save previous newest
      if (newest_millis > newest2_millis) {  //this should always be the case, but I'll check, just to make sure
        newest2_millis=newest_millis;
        newest2_ind = newest_ind;
      }
      
      //save latest newest
      newest_millis = allKeybedData[i].start_millis;
      newest_ind = i;
    } else if (allKeybedData[i].start_millis > newest2_millis) {
      //save this one
      newest2_millis = allKeybedData[i].start_millis;
      newest2_ind = i;
    } else {
      //do nothing
    }
  }
  return newest2_ind;
}

void keybed_t::createKeyPress(const int &ind, const float &noteNum_float, const int &noteVel) {
  //static float fooFreq;
  //static unsigned long foo_dPhase;

  //allKeybedData[ind].noteNum = noteNum;
  allKeybedData[ind].setNoteNum(noteNum_float);
  allKeybedData[ind].noteVel = noteVel;
  allKeybedData[ind].isNewVelocity = true;
  allKeybedData[ind].isPressed = ON;
  allKeybedData[ind].isHeld = OFF;
  if (isHoldNewNote()) allKeybedData[ind].isHeld = ON;
  allKeybedData[ind].start_millis = millis();
  allKeybedData[ind].end_millis = allKeybedData[ind].start_millis;
}  

void keybed_t::stopKeyPress(const int &noteNum,const int &noteVel) {
  int ind=0;
  
  if (noteNum >= 120) {
      //is a stop all command?
    stopAllKeyPresses();
  } else {
    //find any and all matching noteNum and stop them all
    for (ind = 0; ind < maxNKeyPressSlots; ind++) {
      if (allKeybedData[ind].noteNum == noteNum) {
        stopKeyPressByInd(ind,noteVel);
      }
    }
  }
}

void keybed_t::stopKeyPressByInd(const int &ind, const int &noteVel) 
{
  if (allKeybedData[ind].isPressed == ON) {
    allKeybedData[ind].isPressed = OFF;
    //allKeybedData[ind].noteVel = noteVel;  //don't send note-off velocity for now.  2015-10-11
    //allKeybedData[ind].isNewVelocity = true;
    allKeybedData[ind].end_millis = millis();
  }
}
    
void keybed_t::stopAllKeyPresses(void) {
  millis_t end_time = millis();
  for (int ind=0; ind < maxNKeyPressSlots; ind++) {
    if (allKeybedData[ind].isPressed == ON) {
      allKeybedData[ind].isPressed = OFF;
      allKeybedData[ind].end_millis = end_time;
      allKeybedData[ind].isNewVelocity = false;
    }
  }
}

void keybed_t::printKeyPressState(void) {
  Serial.print("KeyPresses: ");
  for (int ind=0; ind < maxNKeyPressSlots; ind++) {
    Serial.print(ind);
    Serial.print("=");
    Serial.print(allKeybedData[ind].noteNum);
    if (allKeybedData[ind].isPressed==ON) {
      Serial.print("P");
    } else {
      Serial.print(" ");
    }
    if (allKeybedData[ind].isHeld==ON) {
      Serial.print("H");
    } else {
      Serial.print(" ");
    }
    if (allKeybedData[ind].isLatched==ON) {
      Serial.print("L");
    } else {
      Serial.print(" ");
    }
    Serial.print(", ");
  }
  Serial.println();
}


//determine whether gate for this voice should be HIGH or LOW
//int isGateActive(const keyPressData_t &keyPress) {
//  if ((keyPress.isPressed==ON) | (keyPress.isHeld==ON)) {
//    return HIGH;
//  } else {
//    return LOW;
//  }
//}

//determine whether gate for this voice should be HIGH or LOW
int keybed_t::isGateActive(const int &Ikey) {
  return allKeybedData[Ikey].isGateActive();
}

int keybed_t::isAnyGateActive(void) {
  for (int Ikey = 0; Ikey < nKeyPressSlots; Ikey++) {
    if (isGateActive(Ikey)==ON) {
      return true;
    }
  }
  return false;
}
  
  
void keybed_t::deactivateHold(void) 
{
  //step through each voice and release the artificially held notes.
  //keyPressData_t *keyPress;
  for (int i=0; i<maxNKeyPressSlots;i++) {
    allKeybedData[i].isHeld=OFF;
    if (allKeybedData[i].isPressed == OFF) {  //only release the notes that aren't still being held by the user
      //also update the release time
      stopKeyPressByInd(i,RELEASE_VELOCITY_FROM_HOLD);
    }
  }
}

//search through the keypress structure and find the newest N key presses
void keybed_t::findNewestKeyPresses(int const &nKeyPressesToFind,int newestInds[])
{
  //initialize the output
  for (int I_out = 0; I_out < nKeyPressesToFind; I_out++) {
    newestInds[I_out]=EMPTY_VOICE;
  }
  
  //find newest keys that are currently pressed or held
  int nFound = findNewestPressedOrHeldKeys(nKeyPressesToFind,newestInds);
  
  //if we've not got them all, find the remainder as the most recently released keys
  if (nFound < nKeyPressesToFind) findNewestReleasedKeys(nKeyPressesToFind,newestInds,nFound);
  
}
 
int keybed_t::findNewestPressedOrHeldKeys(const int &nKeyPressesToFind, int newestInds[])
{
  static boolean acceptanceCriteria[maxNKeyPressSlots];
  static millis_t relevantMillis[maxNKeyPressSlots];
  
  //assess the acceptance criteria for each candidate key press
  for (int I_key = 0; I_key < maxNKeyPressSlots;  I_key++) {
    acceptanceCriteria[I_key] = ((allKeybedData[I_key].isPressed==ON) | (allKeybedData[I_key].isHeld==ON));
    if (I_key >= nKeyPressSlots) acceptanceCriteria[I_key] = false;
    relevantMillis[I_key]=allKeybedData[I_key].start_millis;
  }
  
  //call newest search
  int nFound = 0;
  nFound = findNewest(nKeyPressesToFind, nFound, acceptanceCriteria, relevantMillis, newestInds);
  
  return nFound;
}

//find the keys that have been most recently released
int keybed_t::findNewestReleasedKeys(int const &nKeyPressesToFind,int newestInds[], int const &nDone)
{
  static boolean acceptanceCriteria[maxNKeyPressSlots];
  static millis_t relevantMillis[maxNKeyPressSlots];
  
  //assess the acceptance criteria for each candidate key press
  for (int I_key = 0; I_key < maxNKeyPressSlots;  I_key++) {
    acceptanceCriteria[I_key] = true;  //everything is accepted in this case!
    if (I_key >= nKeyPressSlots) acceptanceCriteria[I_key] = false; //except those that are out of contention 
    relevantMillis[I_key]=max(allKeybedData[I_key].start_millis,allKeybedData[I_key].end_millis);
  }
  
  //call newest search
  int nFound = findNewest(nKeyPressesToFind, nDone, acceptanceCriteria, relevantMillis, newestInds);
  
  return nFound;
}


// helper function that just finds the newest whatever, given the acceptanceCriteria and relevant time scale
int findNewest(const int &nKeyPressesToFind,const int &nDone, boolean acceptanceCriteria[], millis_t relevantMillis[], int newestInds[])
{
  millis_t candidate_millis = 0;
  int i;
  int candidate_ind = 0;
  int nFound=0;
  //static boolean firstTime=true;
  //static boolean alreadyGotHim[N_KEY_PRESS_DATA];
  
  //initialize
  for (i=0; i < nDone; i++) {
    acceptanceCriteria[newestInds[i]] = false; //if we've already got him, he's not acceptable anymore
  }
  
  //loop over to find the desired number of key presses
  nFound=nDone;
  for (int I_out = nDone; I_out < nKeyPressesToFind; I_out++) {
    //reset our serach for the maximum millis
    candidate_millis = 0;
    candidate_ind = 0;
    
    //loop over each keypress
    for (int I_key = 0; I_key < N_KEY_PRESS_DATA;  I_key++) {
      //have we gotten him yet?
      //if (!(alreadyGotHim[I_key])) { //folded this into acceptanceCriteria
        //no, we haven't gotten him.  Ok.  Is he acceptable?
        if (acceptanceCriteria[I_key]) {
          //yes, he is acceptable.  is he the newest?
          if (relevantMillis[I_key] > candidate_millis) {
            //this one is newer, so he's a keeper
            candidate_millis = relevantMillis[I_key];
            candidate_ind = I_key;
          }
        }
      //}
    } //close the loop through all the key presses
    
    //save the value...if it is a valid value
    //alreadyGotHim[candidate_ind]=true;
    acceptanceCriteria[candidate_ind]=false; //he's not longer acceptable
    if (candidate_millis > 0) {
      //it is valid
      newestInds[I_out]=candidate_ind;
      nFound++;
    } else {
      //it is not valid
      newestInds[I_out]=EMPTY_VOICE;
    }
  } //close the loop over the N desired key presses
  
  return nFound;
}



void keybed_t::reduceAndConsolodate(const int &newVal)
{
  //copy current keybed data
  keyPressData_t newKeybedData[maxNKeyPressSlots];
  for (int I_key = 0; I_key < maxNKeyPressSlots; I_key++) {
    newKeybedData[I_key] = allKeybedData[I_key];
  }
  
  //find newest keys pressed
  static int newestKeyInd[maxNKeyPressSlots];
  static int nToFind=min(maxNKeyPressSlots,newVal);
  findNewestKeyPresses(nToFind,newestKeyInd);
  
  //stop all notes in the main keybed structure
  stopAllKeyPresses();
  
  //copy newest notes into lowest slots of the main keybed structure
  for (int I_key = 0; I_key < nToFind; I_key++) {
    allKeybedData[I_key] = newKeybedData[newestKeyInd[I_key]];
  }
}

//int isGateActive(int const &I_key)
//{
//  if ((allKeybedData[I_key].isPressed==ON) | (allKeybedData[I_key].isHeld==ON)) {
//    return HIGH;
//  } else {
//    return LOW;
//  }
//}

int keybed_t::getMaxKeySlots(void) { return maxNKeyPressSlots; }

// /////////////////////////////////////////////////////////////////////////////////////

//const int keybed_givenlist_t::maxNKeyPressSlots = N_KEY_LIST_LEN;

keybed_givenlist_t::keybed_givenlist_t(void) {
  //initialize each keypress
   //resetAllKeybedData();
   nKeyPressSlots = maxNKeyPressSlots; //default value
}


void keybed_givenlist_t::resetAllKeybedData(void) {
  int n = getMaxKeySlots();
  for (int Ikey = 0; Ikey < n; Ikey++) resetKeyPress(Ikey);
}

void keybed_givenlist_t::resetKeyPress(const int &noteIndex) 
{
  int n = getMaxKeySlots();
  if (noteIndex < n) {
    //allNotes[noteIndex].noteID = 0;
    //int noteNum = allKeybedData[noteIndex].noteNum; //do I really need to keep the noteNum when resetting?
    allKeybedData[noteIndex].reset();
    //allKeybedData[noteIndex].noteNum = noteNum;
    allKeybedData[noteIndex].setNoteNum(allKeybedData[noteIndex].targNoteNum_x16bits);
  }
}

int keybed_givenlist_t::addKeyPress(const int &noteNum,const int &noteVel) {
  return addKeyPress((float)noteNum,noteVel);
}

int keybed_givenlist_t::addKeyPress(const float &noteNum_float,const int &noteVel) {
  int ind = next_ind;
  incrementNextInd();
  
  //create this note
  resetKeyPress(ind);
  createKeyPress(ind,noteNum_float,noteVel);

  //Serial.print("keybed::addKeyPress: vel = "); Serial.print(noteVel); Serial.print(", remapped: "); Serial.println(remapVelocityValues(noteVel,assignerState.velocity_sens_8bit));
  return ind;
}

void keybed_givenlist_t::createKeyPress(const int &ind, const float &noteNum_float, const int &noteVel) {
  //static float fooFreq;
  //static unsigned long foo_dPhase;
  
  //allKeybedData[ind].noteNum = noteNum;
  allKeybedData[ind].setNoteNum(noteNum_float);
  allKeybedData[ind].noteVel = noteVel;
  allKeybedData[ind].isNewVelocity = true;
  allKeybedData[ind].isPressed = ON;
  allKeybedData[ind].isHeld = OFF;
  if (isHoldNewNote()) allKeybedData[ind].isHeld = ON;
  allKeybedData[ind].start_millis = millis();
  allKeybedData[ind].end_millis = allKeybedData[ind].start_millis;
}  

int keybed_givenlist_t::incrementNextInd(void) { 
  next_ind++;  
  if (next_ind >= getMaxKeySlots()) next_ind = 0; 
  return next_ind;
}


void keybed_givenlist_t::stopKeyPress(const int &noteNum,const int &noteVel) {
  int ind=0;
  
  if (noteNum >= 120) {
      //is a stop all command?
    stopAllKeyPresses();
  } else {
    //find any and all matching noteNum and stop them all
    for (ind = 0; ind < maxNKeyPressSlots; ind++) {
      if (allKeybedData[ind].noteNum == noteNum) {
        stopKeyPressByInd(ind,noteVel);
      }
    }
  }
}

void keybed_givenlist_t::stopKeyPressByInd(const int &ind, const int &noteVel) 
{
  if (allKeybedData[ind].isPressed == ON) {
    allKeybedData[ind].isPressed = OFF;
    //allKeybedData[ind].noteVel = noteVel;  //don't send note-off velocity for now.  2015-10-11
    //allKeybedData[ind].isNewVelocity = true;
    allKeybedData[ind].end_millis = millis();
  }
}
    
void keybed_givenlist_t::stopAllKeyPresses(void) {
  millis_t end_time = millis();
  for (int ind=0; ind < maxNKeyPressSlots; ind++) {
    if (allKeybedData[ind].isPressed == ON) {
      allKeybedData[ind].isPressed = OFF;
      allKeybedData[ind].end_millis = end_time;
      allKeybedData[ind].isNewVelocity = false;
    }
  }
}

int keybed_givenlist_t::getActiveNoteIndByOrder(const int &active_index_in_order) { //count from zero
  if (active_index_in_order < 0) return -1;
  int count = -1;
  int foo_ind = next_ind - 1; //start on the current index that is playing (but immediately increment past...which would then be the oldest
  for (int i = 0; i < maxNKeyPressSlots; i++) {
    foo_ind ++; //increment
    if (foo_ind >= maxNKeyPressSlots) foo_ind = 0; //wrap around
    if (isPressedOrHeld(foo_ind)) count++; //is this index an active note?  if so, increment the counter
    if (count == active_index_in_order) return foo_ind; //once we've counted the requested active notes, return the index!
  }
  return -1;
}

void keybed_givenlist_t::deactivateHold(void) 
{
  //step through each voice and release the artificially held notes.
  //keyPressData_t *keyPress;
  for (int i=0; i < maxNKeyPressSlots; i++) {
    allKeybedData[i].isHeld=OFF;
    if (allKeybedData[i].isPressed == OFF) {  //only release the notes that aren't still being held by the user
      //also update the release time
      stopKeyPressByInd(i,RELEASE_VELOCITY_FROM_HOLD);
    }
  }
}

bool keybed_givenlist_t::isPressedOrHeld(const int &ind) {
  if ((ind > -1) && (ind < maxNKeyPressSlots)) {
    return ((allKeybedData[ind].isPressed==OFF) && (allKeybedData[ind].isHeld==OFF));
  } 
  return false;
}

int keybed_givenlist_t::getMaxKeySlots(void) { return maxNKeyPressSlots; }
