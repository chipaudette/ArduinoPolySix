
#ifndef _ChordMem_h
#define _ChordMem_h

#define N_SCALES 7
#define N_TONES 12
class ScaleTransposer {
  public:
    ScaleTranspoer(void) { init(); }
    int curScaleInd;    //zero through N_SCALES-1
    int curKey;  //zero through N_TONES-1
    
    void init(void) { curScaleInd = N_SCALES-1; curKey = 0; }
    int set_currentKey(const int key) { return curKey = key % N_TONES; }
    int set_currentScale(const int ind) { if ( (ind >= 0) && (ind < N_SCALES) ) curScaleInd = ind;  return curScaleInd;}
    int matchAndSetScaleForGivenNotes(const int nGivenNotes, int noteNumRelRoot[], int isNoteActive[]);
    
    int getNoteWithinScale(const int &curFirstNote, const int &scaleStepsAbovelRoot, bool printDebug); //this is what one generally calls when deciding what note to play
    int getClosestInScaleNoteNum(const int &note); //given a noteNum, it'll return the nearest noteNum that is in the scale.  Helper function
    int getScaleStepOfNote(const int &noteNum);    //given a noteNum, it'll return the best scaleStep for that note in the current key and scale
    
    //given a noteNum, what is the closest noteNum that is in the scale
    //                                        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    int scales_noteNum[N_SCALES][N_TONES] = { {0, 2, 2, 3, 3, 5, 7, 7, 8, 8, 10, 10}, //natural minor
                                              {0, 2, 2, 4, 4, 5, 7, 7, 9, 9, 11, 11}, //major
                                              {0, 2, 2, 3, 3, 5, 7, 7, 9, 9, 10, 10}, //dorian
                                              {0, 2, 2, 4, 4, 5, 7, 7, 9, 9, 10, 10}, //mixolydian
                                              {0, 2, 2, 3, 3, 5, 7, 7, 9, 9, 11, 11}, //harmonic minor
                                              {0, 1, 1, 4, 4, 5, 7, 7, 9, 9, 10, 10}, //phrygian dominant
                                              {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, //chromatic
                                            };

    int scaleStepsPerOctave[N_SCALES]          = { 7, 7, 7, 7, 7, 7, N_TONES };  //how many valid values in arrays below
    //given a noteNum (0-11), what step in the scale does it correspond to?
    //this array counts from one, because the root is always called the "1st" step in the scale
    int scales_scaleStep[N_SCALES][N_TONES] = { {1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7}, //natural minor
                                              {1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7}, //major
                                              {1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7}, //dorian
                                              {1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7}, //mixolydian
                                              {1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7}, //harmonic minor
                                              {1, 2, 2, 3, 3, 4, 5, 5, 6, 6, 7, 7}, //phrygian dominant
                                              {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, //chromatic
                                            };

    //given a scaleStep, what is the noteNum (ie, interal in half steps) up from the root of the scale?
    //this array counts from zero because the root is zero half-steps above the root (duh!)
    int scaleStepToNoteNum[N_SCALES][N_TONES]  = { {0, 2, 3, 5, 7, 8, 10, 1000, 1001, 1002, 1003, 1004}, //natural minor
                                              {0, 2, 4, 5, 7, 9, 11, 1000, 1001, 1002, 1003, 1004}, //major
                                              {0, 2, 3, 5, 7, 9, 10, 1000, 1001, 1002, 1003, 1004}, //dorian
                                              {0, 2, 4, 5, 7, 9, 10, 1000, 1001, 1002, 1003, 1004}, //mixolydian
                                              {0, 2, 3, 5, 7, 8, 11, 1000, 1001, 1002, 1003, 1004}, //harmonic minor
                                              {0, 1, 4, 5, 7, 8, 10, 1000, 1001, 1002, 1003, 1004}, //phrygian dominant
                                              {0, 1, 2, 3, 4, 5, 6,  7,    8,    9,    10,    11}, //chromatic
                                            };

};

class ChordMem {
  public:
    ChordMem(void) { init(); }
    int noteShift[N_POLY];
    int noteShift_scaleStep[N_POLY];
    long int noteShift_x16bits[N_POLY];
    int isNoteActive[N_POLY];
    int voiceIndexOfBase;
    int detuneFactor[N_POLY];

    void init(void);
    void setState(keybed_t &);
    long int getNewTargNoteNum_x16bits(keyPressData_t &, int);
    long int getNewTargNoteNum_x16bits(keyPressData_t &, int, bool);
    
  private:
    void setDetuneFactorsForChordMemory(void);
    int checkAndApplyMonoPolyOctaveDetune(void);
    ScaleTransposer scaleTransposer;
};

void ChordMem::init(void) {
  for (int i = 0; i < N_POLY; i++) {
    noteShift[i] = 0;
    noteShift_x16bits[i] = 0;
    isNoteActive[i] = OFF;
    detuneFactor[i] = 0;
    noteShift_scaleStep[i] = 1;
  }
  isNoteActive[0] = ON; //make one note active
  noteShift[0] = 0;  noteShift_x16bits[0] = 0;
  noteShift_scaleStep[0] = 1;
  voiceIndexOfBase = 0; //assume first voice is the one currently allocated as the base of the chord mem
}

void ChordMem::setState(keybed_t &trueKeybed) {
  keyPressData_t* keyPress;

  //First, Let's find the lowest active note.  This is the baseline by which the other notes will
  //be referenced.  We'll note the index of this lowest note in the keybed data.
  int lowestKeyInd = 0; //initialize
  int lowestNoteNum = 1000; //initialize to something stupidly large
  long int lowestNoteNum_x16bits = (1000L << 16);  //initialize to something stupidly large
  boolean anyNoteFound = false;
  for (int Ikey = 0; Ikey < N_KEY_PRESS_DATA; Ikey++) {
    if (trueKeybed.isGateActive(Ikey) == ON) {
      keyPress = trueKeybed.getKeybedDataP(Ikey);
      if (keyPress->targNoteNum_x16bits < lowestNoteNum_x16bits) {
        lowestKeyInd = Ikey;
        lowestNoteNum = keyPress->noteNum;
        lowestNoteNum_x16bits = keyPress->targNoteNum_x16bits;
        anyNoteFound = true;
      }
    }
  }
  if (!anyNoteFound) { return; }    //no note was found, do not change the chord mem state

  //define the chord mem note numbers relative to the lowest note
  voiceIndexOfBase = lowestKeyInd;

  //set the state of each voice relative to the lowest note
  for (int Ivoice = 0; Ivoice < N_POLY; Ivoice++) {
    keyPress = trueKeybed.getKeybedDataP(Ivoice);
    noteShift[Ivoice] = constrain(keyPress->noteNum - lowestNoteNum, 0, 119);
    noteShift_x16bits[Ivoice] = constrain(keyPress->targNoteNum_x16bits - lowestNoteNum_x16bits, 0, (119L << 16));
    isNoteActive[Ivoice] = trueKeybed.isGateActive(Ivoice);
  }

  //find and set the key and scale
  scaleTransposer.set_currentKey(lowestNoteNum);
  scaleTransposer.matchAndSetScaleForGivenNotes(N_POLY, noteShift, isNoteActive);
  for (int Ivoice = 0; Ivoice < N_POLY; Ivoice++) {
    if (isNoteActive[Ivoice]) {
      noteShift_scaleStep[Ivoice]=scaleTransposer.getScaleStepOfNote(noteShift[Ivoice]);
    } else {
      noteShift_scaleStep[Ivoice]=1;  //scale steps go from 1-8 (or 1-13) where "1" is the root note
    }
  }

  //set the detune factors for each voice
  setDetuneFactorsForChordMemory();

  //set the root note to be the active note by reseting its start time
  keyPress = trueKeybed.getKeybedDataP(lowestKeyInd);
  keyPress->start_millis = millis();
  keyPress->end_millis = keyPress->start_millis;

  #if 0
    Serial.println(F("ChordMem: setState: perVoice..."));
    for (int i = 0; i < N_POLY; i++) {
      Serial.print("    : ");  Serial.print(i);
      Serial.print(", shift_noteNum: ");      Serial.print(noteShift_x16bits[i] >> 16);
      Serial.print(", isActive: ");      Serial.print(isNoteActive[i]);
      Serial.print(F(", scaleStep = "));      Serial.print(noteShift_scaleStep[i]);
      Serial.println();
    }

  #endif
}

long int ChordMem::getNewTargNoteNum_x16bits(keyPressData_t &ref_keypress, int Ivoice) {
  return getNewTargNoteNum_x16bits(ref_keypress, Ivoice, false);
}
long int ChordMem::getNewTargNoteNum_x16bits(keyPressData_t &ref_keypress, int Ivoice, bool smart_transpose) {
  static int lastNoteNum[N_POLY];
  long int new_note_x16bits = 0;
  int noteShift_foo = 0;

  bool printDebug = false;
  //if (ref_keypress.noteNum != lastNoteNum[Ivoice]) {
  //  printDebug = true;
  //}

  if ((smart_transpose == false) || ((ref_keypress.noteNum - scaleTransposer.getClosestInScaleNoteNum(ref_keypress.noteNum)) != 0)) {
    //old-fashioned dumb transpose where it knows nothing of key or scale
    noteShift_foo = (int)(noteShift_x16bits[Ivoice] >> 16);
    new_note_x16bits = ref_keypress.targNoteNum_x16bits + noteShift_x16bits[Ivoice];
  } else {
    //new smart transpose where it accounts for the key and the scale
    //noteShift_foo = scaleTransposer.getNoteInScale(ref_keypress.noteNum,noteShift[Ivoice]);
    noteShift_foo = scaleTransposer.getNoteWithinScale(ref_keypress.noteNum, noteShift_scaleStep[Ivoice],printDebug); //this is in units of noteNum (ie, half steps)
    //long int purposefulOutOfTune_x16bits = 0;
    long int purposefulOutOfTune_x16bits =  ref_keypress.targNoteNum_x16bits - ((ref_keypress.targNoteNum_x16bits >> 16) << 16);
    new_note_x16bits = (((long int)noteShift_foo) << 16) + purposefulOutOfTune_x16bits;
  }


  if (printDebug) {
    lastNoteNum[Ivoice] = ref_keypress.noteNum;
    Serial.print("ChordMem: getNewTargNoteNum_x16bits: Ivoice = "); Serial.print(Ivoice);
    Serial.print(", ref note = ");Serial.print(ref_keypress.noteNum);
    //Serial.print(", note out raw = "); Serial.print((ref_keypress.targNoteNum_x16bits + noteShift_x16bits[Ivoice]) >> 16);
    //Serial.print(", noteShift final = "); Serial.print(new_note_x16bits >> 16);
    Serial.print(", note shift raw = "); Serial.print(((ref_keypress.targNoteNum_x16bits + noteShift_x16bits[Ivoice]) >> 16) - ref_keypress.noteNum);
    Serial.print(", note shift final = "); Serial.print((new_note_x16bits >> 16) - ref_keypress.noteNum);
   Serial.println();
  }
  return new_note_x16bits;
}

//set the detune factors for each note in the chord memory.
//assumes that the noteShift has already been set for each voice
void ChordMem::setDetuneFactorsForChordMemory(void) {

  //look for special case of chip's favorite 4-note Mono/Poly config
  int isMonoPolyOctaves = checkAndApplyMonoPolyOctaveDetune();

  //if not Mono/Poly octaves, distribute the detune the regular way
  if (isMonoPolyOctaves == false) {
    int nActive = 0, nPair = 0;
    int prevActiveVoiceInd = -1;
    for (int i = 0; i < N_POLY; i++) {
      if (isNoteActive[i] == true) nActive++;
    }
    if (nActive == 1) {
      //just the root note...no detune
      detuneFactor[voiceIndexOfBase] = 0;
    } else if (nActive == 2) {
      //just a pair of notes.  Are they octaves or different
      boolean isOctaves = true;
      boolean isUnison = true;
      for (int i = 0; i < N_POLY; i++) {
        if (isNoteActive[i] == true) {
          if ( (((int)(noteShift_x16bits[i] >> 16)) % 12) != 0 ) isOctaves = false;
          if (noteShift_x16bits[i] != 0) isUnison = false;
        }
      }
      if (isUnison) {
        //detune root down and octave up
        for (int i = 0; i < N_POLY; i++) {
          if (i == voiceIndexOfBase) {
            detuneFactor[i] = 2;  //positive is down, I think
          } else if (isNoteActive[i] == true) {
            detuneFactor[i] = -2;  //positive is down, I think
          }
        }
      } else if (isOctaves) {
        //detune root down and octave up
        for (int i = 0; i < N_POLY; i++) {
          if (i == voiceIndexOfBase) {
            detuneFactor[i] = 1;  //positive is down, I think
          } else if (isNoteActive[i] == true) {
            detuneFactor[i] = -1;  //positive is down, I think
          }
        }
      } else {
        //detune the non-root note only
        for (int i = 0; i < N_POLY; i++) {
          if ((i != voiceIndexOfBase) && (isNoteActive[i] == true)) {
            detuneFactor[i] = +1;  //positive is down, I think
          }
        }
      }
    } else {
      //all other configuration of notes
      Serial.println(F("stateManagement: setDetuneFactorsForChordMemory: detune method 3"));
      nActive = 0;
      for (int i = 0; i < N_POLY; i++) {
        if (i == voiceIndexOfBase) {
          detuneFactor[i] = 0; //don't detune the root
        } else {
          if (isNoteActive[i] == true) {
            nActive++; //only count the non-root note
            detuneFactor[i] = 0; //don't detune unless it's a pair
            if ((nActive % 2) == 0) {
              //it is a new pair!
              nPair++;
              detuneFactor[prevActiveVoiceInd] = nPair;
              detuneFactor[i] = -nPair;
            }
            prevActiveVoiceInd = i;
          }
        }
      }
    }
  }

  #if 0
  for (int i = 0; i < N_POLY; i++) {
    Serial.print(F("ChordMem: setDetuneFactorsForChordMemory: voice "));
    Serial.print(i);
    Serial.print(F(" : detune = "));
    Serial.println(detuneFactor[i]);
  }
  #endif
}

//does the chord mem have 2 at root and 2 at +1 octave?
int ChordMem::checkAndApplyMonoPolyOctaveDetune(void) {
  int isMonoPolyOctaves = true;
  int nRoot = 0, nOctave = 0;
  for (int i = 0; i < N_POLY; i++) {
    if (isNoteActive[i] == true) {
      if (noteShift_x16bits[i] == 0) nRoot++;
      if (noteShift_x16bits[i] == (12L << 16)) nOctave++;
      if ((noteShift_x16bits[i] != 0) && ((noteShift_x16bits[i] != (12L << 16)))) isMonoPolyOctaves = false;
    }
  }
  if (!((nRoot == 2) && (nOctave == 2))) isMonoPolyOctaves = false;

  if (isMonoPolyOctaves == true) {
    int curRootCount = 0, curCountOctave = 0;
    for (int i = 0; i < N_POLY; i++) {
      if (isNoteActive[i] == true) {
        if (noteShift_x16bits[i] == 0) {
          //detuneFactor[i] = 0; //don't detune the roots
          curRootCount++;
          if ((curRootCount % 2) == 0) {
            detuneFactor[i] = 0; //no detune
          } else {
            detuneFactor[i] = 0; //no detune
          }
        }
        if (noteShift_x16bits[i] == (12L << 16)) {
          curCountOctave++;
          if ((curCountOctave % 2) == 1) {
            detuneFactor[i] = 1;  //positive is down in pitch
          } else {
            detuneFactor[i] = -1; //negative is up in pitch
          }
        }
      }
    }
  }
  return isMonoPolyOctaves;
}



int ScaleTransposer::matchAndSetScaleForGivenNotes(const int nGivenNotes, int noteNumRelRoot[], int isNoteActive[]) {
  //find the first scale that gives no mismatches.  The last scale should be chromatic, so we'll default to that if none other fit.
  bool scale_fits = false; //initialize to false
  int scaleInd = -1; //first thing we'll do inside the loop is increment to zero

  //we're going to loop over ech scale template to find one that fits all the given notes
  while ( (scaleInd < (N_SCALES-1)) && (scale_fits == false) ) { //loop over each givn scale template until we find on ethat fits (or we end on the last one, chromatic)
    scaleInd++; //increment to the next scale
    scale_fits = true;  //assume it fits until proven otherwise by a bad note

    //loop over each given note
    for (int iGiven = 0; iGiven < nGivenNotes; iGiven++) { //loop over each note in the given note array

      //get the given note within one octave so that we can compare it to the scale templates
      int given_inOctave_noteNum = noteNumRelRoot[iGiven];
      while ( given_inOctave_noteNum < 0 ) given_inOctave_noteNum += N_TONES; //get the value within one octave
      while ( given_inOctave_noteNum >= N_TONES)  given_inOctave_noteNum -= N_TONES; //get the value within one octave
      
      if (isNoteActive[iGiven]) { //only test active notes
        bool any_fit = false; //we're looking to see if *any* of the template fit the given note, so we'll start false and flip to true whenever we find one that fits
        for (int iTest = 0; iTest < N_TONES; iTest++) { //loop over each note in the template to see if any fit the given note
          //Serial.print(F("ScaleTransposer: matchAndSetScaleForGivenNotes: iGiven = "));Serial.print(iGiven);
          //Serial.print(F(", noteRelRoot = "));Serial.print(noteNumRelRoot[iGiven]);
          //Serial.print(F(", note in Octave = ")); Serial.print(given_inOctave_noteNum);
          //Serial.print(F(", ScaleInd = "));Serial.print(scaleInd);
          //Serial.print(F(", iTest = ")); Serial.print(iTest);
          //Serial.print(F(", Scale noteNum = "));Serial.print(scales_noteNum[scaleInd][iTest]);          
          if (scales_noteNum[scaleInd][iTest] == given_inOctave_noteNum)  {  //does it match?
            //Serial.print(F(", MATCH!"));
            any_fit = true;
          } //one of the template notes does fit!
          //Serial.println();
        }
        
        if (any_fit == false) { 
          scale_fits = false; //if there is a failed fit, then we've failed to fit this scale
          //Serial.println("ScaleTransposer: matchAndSetScaleForGivenNotes: this note had no match.");
        }
      }
    }
  }

  //save the scaleInd that fits
  set_currentScale(scaleInd);

  Serial.print("ScaleTransposer: matchAndSetScaleForGivenNotes: key = ");Serial.print(curKey);
  Serial.print(", scale = "); Serial.print(curScaleInd);
  Serial.println(); 
}

/*
int ScaleTransposer::getNoteInScale(const int &curFirstNote, const int &origNoteNumRelRoot) {
  int offset = curKey;
  int foo = (curFirstNote - offset) + origNoteNumRelRoot;
  while (foo < 0) { foo += N_TONES; offset -= N_TONES; } //make sure it is zero or above
  while (foo >= N_TONES) { foo -= N_TONES; offset += N_TONES; }; //make sure it is less than N_TONES
  int new_val = scales_noteNum[curScaleInd][foo] + offset;
  while (new_val < 0) new_val += N_TONES;  //make sure it's a positive value
  return new_val;
}
*/

int ScaleTransposer::getClosestInScaleNoteNum(const int &note) {
  int offset = curKey;
  int newNote = note - curKey;
  while (newNote < 0) { newNote += N_TONES; offset -= N_TONES; };
  while (newNote >= N_TONES) { newNote -= N_TONES; offset += N_TONES; }
  return newNote = scales_noteNum[curScaleInd][newNote] + offset;
}

int ScaleTransposer::getScaleStepOfNote(const int &noteNum) {
  int inOctave_noteNum = noteNum;  //ideally, we're going to make this be within 0-11
  int offset_octaves = 0;
  while (inOctave_noteNum < 0) { inOctave_noteNum += N_TONES; offset_octaves -= 1; }  //getting this to be within 0-11
  while (inOctave_noteNum >= N_TONES) { inOctave_noteNum -= N_TONES; offset_octaves += 1; } //getting this to be within 0-11

  int scaleStep = scales_scaleStep[curScaleInd][inOctave_noteNum];
  scaleStep += ((offset_octaves * (scaleStepsPerOctave[curScaleInd])));
  return scaleStep; 
}

//here's the primary function for the rest of the PolySix to use when figuring out what notes
//to play when Chord Mem is active
int ScaleTransposer::getNoteWithinScale(const int &rootNote_noteNum, const int &scaleStepsAboveRoot, bool printDebug) {
  int root_inOctave_noteNum = rootNote_noteNum - curKey; //start with the raw notNum that we will try to fit within the octave (0-11)
  //root_inOctave_noteNum = getClosestInScaleNoteNum(root_inOctave_noteNum);  //first, push the note to one that is within the scale
  int offset_noteNum = 0; //we'll accumulate this here at the beginning, but we'll add it back in only way down at the bottom
  while (root_inOctave_noteNum < 0) { root_inOctave_noteNum += N_TONES; offset_noteNum -= N_TONES; }  //getting this to be within 0-11
  while (root_inOctave_noteNum >= N_TONES) { root_inOctave_noteNum -= N_TONES; offset_noteNum += N_TONES; } //getting this to be within 0-11
  int root_inOctave_scaleStep = scales_scaleStep[curScaleInd][root_inOctave_noteNum];  //this will be 1-7 (or 1-12) depending upon the scale
  int targ_scaleStep = root_inOctave_scaleStep + (scaleStepsAboveRoot-1);  //this is in steps of the scale.  minus one because the root (in scaleStep) is 1, so we need to not keep adding 1.  ideally, it would be 1-7 (or 1-12) but it's now probably too larget
  
  
  //force targ_scaleStep to fit within 1-7 (or 1-12, depending upon the scale)
  int offset_octaves = 0;
  while (targ_scaleStep <= 0) { targ_scaleStep += scaleStepsPerOctave[curScaleInd]; offset_octaves -= 1; }  //trying to get this within 1-7 (or 1-12) depending on scale
  while (targ_scaleStep > scaleStepsPerOctave[curScaleInd]) { targ_scaleStep -= scaleStepsPerOctave[curScaleInd]; offset_octaves += 1; }  //trying to get this within 1-7 (or 1-12) depending on scale

  //build up the outgoing note (in units of semitones...aka in units of noteNum)
  int newNote_noteNum = scaleStepToNoteNum[curScaleInd][targ_scaleStep-1]; //the subtract by one is to account for scaleSteps starting at 1 not at 0
  newNote_noteNum += (offset_octaves * N_TONES);  //assume that there are N_TONES in an octave
  newNote_noteNum += (curKey+offset_noteNum); //add the octave offset from the root note back in

  if (printDebug) {
    Serial.print(F("ScaleTransposer: getNoteWithinScale: "));
    Serial.print(F(" root = ")); Serial.print(rootNote_noteNum);
    Serial.print(F(" scaleStepsAboveRoot = ")); Serial.print(scaleStepsAboveRoot);
    Serial.print(F(" root_inOctave_noteNum = ")); Serial.print(root_inOctave_noteNum);
    Serial.print(F(" root_inOctave_scaleStep = ")); Serial.print(root_inOctave_scaleStep);
    Serial.print(F(" targ_scaleStep = ")); Serial.print(targ_scaleStep);
    Serial.print(F(" newNote_noteNum = ")); Serial.print(newNote_noteNum);
    Serial.println();
  }

  return newNote_noteNum;
}
#endif
