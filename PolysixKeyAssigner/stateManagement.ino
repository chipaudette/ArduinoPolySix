
#include "dataTypes.h"
//include <KeyAssignerPinMap.ino>  //we need to know the pin mapping

#define DEBUG_THIS_FILE false

//only used locally for tracking *changes* in button state
//static assignerButtonState_t prev_but_state;
//static assignerButtonState_t but_state_changed;

//void initAssignerState(void)
//{
//  assignerState.arp=OFF;
//  assignerState.poly=ON;
//  assignerState.unison=OFF;
//  assignerState.chord_mem=OFF;
//  assignerState.hold=HOLD_STATE_OFF;
//  assignerState.octave = OCTAVE_16FT;
//  assignerState.legato = OFF;
//  assignerState.portamento = OFF;
//  assignerState.detune = OFF;
//  assignerState.keypanel_mode = KEYPANEL_MODE_ARP;
//  assignerState.velocity_sensitivity = ON;
//};

void initChordMemState(void)
{
  for (int i=0;i<N_POLY;i++) {
    chordMemState.noteShift_x16bits[i]=0;
    chordMemState.isNoteActive[i]=OFF;
    chordMemState.detuneFactor[i]=0;
  }
  chordMemState.isNoteActive[0]=ON; //make one note active
  chordMemState.noteShift_x16bits[0]=0;
  chordMemState.voiceIndexOfBase = 0; //assume first voice is the one currently allocated as the base of the chord mem
}

void updateKeyAssignerStateFromButtons(assignerButtonState2_t &cur_but_state)
{
  static micros_t prev_knob_value = 0L;
  static int diff_knob_thresh = 100;
  int prePedal_porta_state = OFF;
  int prePedal_porta_setting_index = 0;

  //update the keypanel mode...interpret as arp mode or as standard/other mode?
  updateKeyPanelMode(cur_but_state); //one thing this does is update assignerState.keypanel_mode

  if (assignerState.keypanel_mode == KEYPANEL_MODE_TUNING) {
    interpretButtonsAndSwitches_KEYPANEL_TUNING(cur_but_state);  
  } else {
    //interpret the buttons and switches for the other modes
    
    //update arpeggiator on/off status
    updateArpOnOffAndParameters(cur_but_state);  
  
    //update the poly/unison/chord state
    updatePolyUnisonChordState(cur_but_state);
  
    //update the hold state
    updateHoldState(cur_but_state);
  
    //if not in ARP mode, listen to all the switches (not the buttons, the switches)
    if (assignerState.keypanel_mode == KEYPANEL_MODE_OTHER) {
      interpretSwitches_KEYPANEL_OTHER(cur_but_state);
    }
    
    //update the portamento from the pedal
    if (cur_but_state.portamentoPedal.state == ON) {
      prePedal_porta_state = assignerState.portamento;
      prePedal_porta_setting_index = porta_setting_index;
      porta_setting_index = 2;
      assignerState.portamento = ON;
  
      //do labor for looking at the ARP speed knob as a method for controlling the amount of portamento
      if (cur_but_state.portamentoPedal.didStateChange() == true) prev_knob_value = ARP_period_micros;
      if ((ARP_period_micros - prev_knob_value) > diff_knob_thresh) {
        prev_knob_value = ARP_period_micros;
        setNewPortamentoValue(prev_knob_value,porta_setting_index);
      }
    } else if (cur_but_state.portamentoPedal.wasJustReleased() == true) {
      porta_setting_index = prePedal_porta_setting_index;
      assignerState.portamento = prePedal_porta_state;
    }
  }
}

void interpretButtonsAndSwitches_KEYPANEL_TUNING(assignerButtonState2_t &cur_but_state)
{   
    //Interpret switch determining how much is getting tuned
    switch (cur_but_state.arp_range.state) {
      case (ARP_RANGE_FULL):
        tuningModeState.adjustmentMode = TUNING_MODE_ALL_VOICES;
        break;
      case (ARP_RANGE_2OCT):
        tuningModeState.adjustmentMode = TUNING_MODE_ALL_OCT_OF_ONE_VOICE;
        break;
      case (ARP_RANGE_1OCT):
        tuningModeState.adjustmentMode = TUNING_MODE_ONE_OCT_OF_ONE_VOICE;
        break;
    }


    //Interpret switches determining which voice to be tuned
    switch (cur_but_state.arp_dir.state) {
      case (ARP_DIR_UP):
        if (cur_but_state.arp_latch.state == ON) {
          tuningModeState.voiceCommanded = 0;
        } else {
          tuningModeState.voiceCommanded = 3;
        }
        break;
      case (ARP_DIR_DOWN):
        if (cur_but_state.arp_latch.state == ON) {
          tuningModeState.voiceCommanded = 0+1;
        } else {
          tuningModeState.voiceCommanded = 3+1;
        }
        break;
      case (ARP_DIR_UPDOWN):
        if (cur_but_state.arp_latch.state == ON) {
          tuningModeState.voiceCommanded = 0+2;
        } else {
          tuningModeState.voiceCommanded = 3+2;
        }
        break;
    } 

    //interpret buttons to determine how much tuning to do
    bool printDebug = true;
    if (cur_but_state.hold.wasJustReleased()==true) {
       adjustTuningThisVoiceOrRibbon(-5,printDebug);
    }
    if (cur_but_state.chord_mem.wasJustReleased()==true) {
      adjustTuningThisVoiceOrRibbon(-1,printDebug);
    }
    if (cur_but_state.unison.wasJustReleased()==true) {
      adjustTuningThisVoiceOrRibbon(+1,printDebug);
    }
    if (cur_but_state.poly.wasJustReleased()==true) {
      adjustTuningThisVoiceOrRibbon(+5,printDebug);
    }
}

void interpretSwitches_KEYPANEL_OTHER(assignerButtonState2_t &cur_but_state)
{
  //if arp is off, just take the current switch's states.  If arp is on, look for changes in switchs' states
  int update_now = true;
  if (assignerState.arp == ON) {
    update_now = false;
    if (cur_but_state.arp_dir.didStateChange() == true) {
      update_now = true;
    }
  }
  if (update_now == true) {
    switch (cur_but_state.arp_dir.state) { //temporary way to control the portamento
    case ARP_DIR_UP:
      porta_setting_index = 0;
      break;
    case ARP_DIR_DOWN:
      porta_setting_index = 1;
      break;
    case ARP_DIR_UPDOWN:
      porta_setting_index = 2;  
      break;
    default:
      porta_setting_index = 1;
    }
  }
  
  //turn portamento on or off.
  update_now = true;
  if (assignerState.arp == ON) {
    update_now = false;
    if (cur_but_state.arp_latch.didStateChange() == true) {
      update_now = true;
    }
  }
  if (update_now == true) assignerState.portamento = cur_but_state.arp_latch.state; //temporary way to control the portamento
  
  //now update the aftertouch settings
  //if arp is off, just take the current switch buttons.  If arp is on, look for changes
  update_now = true;
  if (assignerState.arp == ON) {
    update_now = false;
    if (cur_but_state.arp_range.didStateChange() == true) {
      update_now = true;
    }
  }
  if (update_now == true) {
    switch (assignerButtonState.arp_range.state) { //temporary way to control the aftertouch
    case ARP_RANGE_FULL:
      aftertouch_setting_index = 0;
      break;
    case ARP_RANGE_2OCT:
      aftertouch_setting_index = 1;
      break; 
    case ARP_RANGE_1OCT:
      aftertouch_setting_index = 2;
      break;  
    default:
      porta_setting_index = 1; //??  This probably never gets called, but isn't it wrong?  should it be aftertouch_setting_index = 0;
    }
  }
  return;
}


//update the keypanel mode
void updateKeyPanelMode(assignerButtonState2_t &cur_but_state)
{
  //look first to see if coming out of tuning mode
  if (assignerState.keypanel_mode == KEYPANEL_MODE_TUNING) {
    //are we switching out of this mode?
    if (cur_but_state.tapeEnableToggle.state == LOW) {
      //switch out of tuning mode
      Serial.println(F("stateManagement: updateKeyPanelMode: switching out of tuning mode..."));
      Serial.println(F("stateManagement: updateKeyPanelMode: saving tuning state to EEPROM..."));
      saveEEPROM();
            
      if (cur_but_state.fromTapeToggle.state == HIGH) {
        //has the panel mode state changed?
        assignerState.keypanel_mode = KEYPANEL_MODE_OTHER;
      } else {   
        assignerState.keypanel_mode = KEYPANEL_MODE_ARP;
      }
    }

  } else {
    //look to see if going into tuning mode
    if ( (cur_but_state.tapeEnableToggle.state == HIGH) && (cur_but_state.tapeEnableToggle.didStateChange()) ) {
      switchToTuningMode();
      assignerState.keypanel_mode = KEYPANEL_MODE_TUNING;
      Serial.println(F("stateManagement: updateKeyPanelMode: switching to tuning mode..."));
        
      //make sure that we don't kick into ARP mode when we let go of the ARP button  (THIS WAS THE OLD WAY OF GETTING INTO TUNING MODE)
      //cur_but_state.arp_on.set_user_beenPressedAgainToFalse(); //this means that the arp on/off won't toggle when you finally release the arp button

    } else if (cur_but_state.fromTapeToggle.state == HIGH) {
      //has the panel mode state changed?
      assignerState.keypanel_mode = KEYPANEL_MODE_OTHER;
    } else {   
      assignerState.keypanel_mode = KEYPANEL_MODE_ARP;
    }
  }
}

//switch the unit to tuning mode
void switchToTuningMode(void) {

  //turn off arp (if active)
  if (assignerState.arp == ON) arpManager.stopArp();
  
  //turn off detune
  assignerState.detune = OFF;

}

  
//Update the Poly/Unison/Chord state...
//Note: ignore all button presses if the arp button is pressed.  If the arp button is pressed,
//It means that we're in some complex arp-button-related command mode
void updatePolyUnisonChordState(assignerButtonState2_t &cur_but_state)
{
  static int prevState = STATE_POLY;  //static variable is saved throughout time.  Initialize on first time to poly mode.
  int newState = 0;
  boolean stateChanged = true;
  //static boolean stateButtonPreviouslyReleased = true;

  //ignore all button presses if arp button is pressed
  if (cur_but_state.arp_on.state == ON) {
    return;
  }

  //count how many buttons are pressed
  int nButtonsPressed = 0;
  if (cur_but_state.poly.state) nButtonsPressed++;
  if (cur_but_state.unison.state) nButtonsPressed++;
  if (cur_but_state.chord_mem.state) nButtonsPressed++;
  
  //start assessing what might have happened
  switch (nButtonsPressed) {
    case 0:
      newState = noButtonsPressed_lookForRelease(cur_but_state);
      break;
    case 1:
      newState = oneButtonPressed_lookForRelease(cur_but_state);
      break;
    default:
      //no action
      newState = STATE_DEFAULT;
      break;
  }
  if (newState == STATE_DEFAULT) newState = prevState;
  
  
  //if assigner state has changed, do any de-activation of previous states
  //prior to activating the new state
  stateChanged=false; if (newState != prevState) stateChanged=true;

  //if the state has changed, deactivate the previous state
  if (stateChanged==true){
    
    //de-activate the previous state
    if (prevState == STATE_UNISON) {
      Serial.println(F("updatePolyUnisonChordState: Disabling Unison"));
      deactivateUnison();
    } else if (prevState == STATE_CHORD) {
      Serial.println(F("updatePolyUnisonChordState: Disabling Chord Mem"));
      deactivateChordMemory();
    } else if (prevState == STATE_UNISON_POLY) {
      Serial.println(F("updatePolyUnisonChordState: Disabling Unison Poly"));
      deactivateUnisonPoly();
    } else if (prevState == STATE_CHORD_POLY) {
      Serial.println(F("updatePolyUnisonChordState: Disabling Chord Poly"));
      deactivateChordPoly();
    }
  
    //activate the new state
    switch (newState) {
      case STATE_POLY:
        Serial.println(F("updatePolyUnisonChordState: activate Poly mode."));
        activatePoly(stateChanged);
        //if (newState != prevState) cur_but_state.poly.user_beenReleased = false;  //because button was just pressed
        //if (stateChanged) cur_but_state.poly.user_beenReleased = false;  //because button was just pressed
        break;
      case STATE_UNISON:
        Serial.println(F("updatePolyUnisonChordState: activate Unison mode."));
        activateUnison(stateChanged);
        //if (newState != prevState) cur_but_state.unison.user_beenReleased = false;//because button was just pressed
        //if (stateChanged) cur_but_state.unison.user_beenReleased = false;//because button was just pressed
        break;
      case STATE_CHORD:     
        Serial.println(F("updatePolyUnisonChordState: activate Chord_Mem mode."));
        activateChordMemory(stateChanged);
        //if (newState != prevState) cur_but_state.chord_mem.user_beenReleased = false;//because button was just pressed
        //if (stateChanged) cur_but_state.chord_mem.user_beenReleased = false;//because button was just pressed
        break;
      case STATE_UNISON_POLY:
        Serial.println(F("updatePolyUnisonChordState: activate UnisonPoly mode."));
        activateUnisonPoly(stateChanged);
        break;
      case STATE_CHORD_POLY:
        Serial.println(F("updatePolyUnisonChordState: activate ChordPoly mode."));
        activateChordPoly(stateChanged);
        break;
    }
  }

  //check to see if legato state can be toggled...based on whether the button for the current state was pressed again
  updateLegatoState(cur_but_state,newState,prevState);

  //save the state for the next time around
  prevState = newState;
}


//update the arpeggiator state based on the arp button presses and the switches
void updateArpOnOffAndParameters(assignerButtonState2_t &cur_but_state) 
{
  static micros_t prev_knob_value = 0L;
  static int diff_knob_thresh = 100;

  //only process on release of the button
  int cur_but_arp = cur_but_state.arp_on.state;
  int but_didChange = cur_but_state.arp_on.didStateChange();
  int user_beenReleased = cur_but_state.arp_on.user_beenReleased;
  int user_beenPressedAgain = cur_but_state.arp_on.user_beenPressedAgain;

  //look for release of a button that didn't have other actions during this press 
  //(user_beenPressedAgain would be false if other actions had taken place during this press)
  if ((cur_but_state.arp_on.wasJustReleased()==true) & (user_beenPressedAgain == true)) {
    if (assignerState.keypanel_mode == KEYPANEL_MODE_ARP) {
      //toggle the arp state
      assignerState.arp = !assignerState.arp;

      //let's try to run the arpeggiator
      if (assignerState.arp == ON) {
        //start arpeggiator
        Serial.print(F("updateArpOnOffState: Arp On. Hold = "));Serial.print(assignerState.hold);Serial.print(F(", latch = "));Serial.println(assignerState.arp_params.arp_latch);
        int mode = ARP_SORTMODE_NOTENUM;
        if (assignerState.hold == ON) {
          if (assignerState.arp_params.arp_latch == OFF) {
            mode = ARP_SORTMODE_STARTTIME;
          } else { 
            mode = ARP_SORTMODE_GIVENLIST;
          }
          arpManager.startArp(mode,assignerState,deactivateHoldState); //tell the arp to deactivate the hold after it updates the notes for the ARP
        } else {
          arpManager.startArp(mode,assignerState); //the basic start arp command
        }

      } else {
        //stop arpeggiator
        Serial.println(F("updateArpOnOffState: Arp Off"));
        arpManager.stopArp();
      }
    } else if (assignerState.keypanel_mode == KEYPANEL_MODE_OTHER) {
      //in this mode, we're controlling other functions...like DETUNE
      //toggle the detune state
      assignerState.detune = !assignerState.detune;   
    } 
  } 

  //look to see if any other state buttons were pressed while ARP is pressed...control panel must be in ARP mode
  if (assignerState.keypanel_mode == KEYPANEL_MODE_ARP) {
    if (cur_but_state.arp_on.state == ON) {
      //check to see if any of the other four assigner state buttons are pressed
      int val = -1;
      if (cur_but_state.hold.state == ON) val = 31;  //one quarter of 127
      if (cur_but_state.chord_mem.state == ON) val = 63;
      if (cur_but_state.unison.state == ON) val = 95;
      if (cur_but_state.poly.state == ON) val = 127;
  
      if (val > -1) {
        arpManager.setArpGate_x7bits(val);
        cur_but_state.arp_on.set_user_beenPressedAgainToFalse(); //this means that the arp on/off won't toggle when you finally release the arp button
      }
    }
  } else { //assigner state is in "other" (non-ARP) mode....in this mode, combo presses with Arpeggio controls velocity sensitivity
    if (cur_but_state.arp_on.state == ON) {
       //check to see if any of the other four assigner state buttons are pressed
      int val = -1;
      if (cur_but_state.hold.state == ON) val = assignerState_t::MIN_VELOCITY_SENS;  //one quarter of 127
      if (cur_but_state.chord_mem.state == ON) val = (int)map(35, 0, 100, assignerState_t::MIN_VELOCITY_SENS, assignerState_t::MAX_VELOCITY_SENS);
      if (cur_but_state.unison.state == ON) val = (int)map(60, 0, 100, assignerState_t::MIN_VELOCITY_SENS, assignerState_t::MAX_VELOCITY_SENS);
      if (cur_but_state.poly.state == ON) val = assignerState_t::MAX_VELOCITY_SENS;
 
      if (val > -1) {
        assignerState.velocity_sens_8bit = val;
      }
    }
  }


  //do labor for looking at the ARP speed knob as a method for controlling the amount of portamento
  if (assignerState.keypanel_mode == KEYPANEL_MODE_OTHER) {
    if (cur_but_arp == true) {
      //button is pressed
      if (but_didChange == true) {
        //and this is the first time through with it pressed
        prev_knob_value = ARP_period_micros;
      }

      //has the speed knob been turned while the button has been pressed?
      if ((ARP_period_micros - prev_knob_value) > diff_knob_thresh) {
        //knob has been moved
        cur_but_state.arp_on.user_beenReleased = false;
        cur_but_state.arp_on.set_user_beenPressedAgainToFalse();
        prev_knob_value = ARP_period_micros;
        if (assignerState.keypanel_mode == KEYPANEL_MODE_OTHER) {
          setNewDetuneValue(prev_knob_value);
        }
      }
    }
  }

  //interpret the ARP parameters from the switches
  if (assignerState.keypanel_mode == KEYPANEL_MODE_ARP) {
    if (assignerState.arp == ON) {
      //if arp is on, just look for changes in state
      if (cur_but_state.arp_latch.didStateChange() == true) assignerState.arp_params.arp_latch = cur_but_state.arp_latch.state;
      if (cur_but_state.arp_dir.didStateChange() == true) assignerState.arp_params.arp_direction = cur_but_state.arp_dir.state;
      if (cur_but_state.arp_range.didStateChange() == true) assignerState.arp_params.arp_range = cur_but_state.arp_range.state;
    } else {
      //if arp is off, just take the state as shown
      assignerState.arp_params.arp_latch = cur_but_state.arp_latch.state;
      assignerState.arp_params.arp_direction = cur_but_state.arp_dir.state;
      assignerState.arp_params.arp_range = cur_but_state.arp_range.state;
    }
  }
}
//
////Update the Poly/Unison/Chord state...
////Note: ignore all button presses if the hold button is pressed.  If the hold button is pressed,
////It means that we're in some complex hold-related command mode
//void updatePolyUnisonChordState_old(assignerButtonState2_t &cur_but_state)
//{
//  static int prevState = STATE_POLY;
//  int newState = 0;
//  boolean stateChanged = true;
//  //static boolean stateButtonPreviouslyReleased = true;
//
//  //ignore all button presses if arp button is pressed
//  if (cur_but_state.arp_on.state == ON) {
//    return;
//  }
//
//  //is any state button pressed at all?
//  if ((cur_but_state.poly.state == ON) | (cur_but_state.unison.state == ON) | (cur_but_state.chord_mem.state == ON)) {
//    //a state button is pressed
//
//    //decide what state the current button presses demand
//    if ((cur_but_state.poly.state==ON) & (cur_but_state.hold.state==OFF)) {
//      newState = STATE_POLY;
//    } else if ((cur_but_state.unison.state==ON) & (cur_but_state.hold.state==OFF)) {
//      newState = STATE_UNISON;
//    } else if ((cur_but_state.chord_mem.state==ON) & (cur_but_state.hold.state==OFF)) {
//      newState = STATE_CHORD;
//    } 
//
//    //if assigner state has changed, do any de-activation of previous states
//    //prior to activating the new state
//    stateChanged=false;
//    if (newState != prevState) stateChanged=true;
//
//    //if the state has changed, deactivate the previous state
//    if (stateChanged==true){
//      if (prevState == STATE_UNISON) {
//        deactivateUnison();
//      } else if (prevState == STATE_CHORD) {
//        deactivateChordMemory();
//      }
//    }
//
//    //activate the new state
//    switch (newState) {
//      case STATE_POLY:
//        activatePoly(stateChanged);
//        //if (newState != prevState) cur_but_state.poly.user_beenReleased = false;  //because button was just pressed
//        if (stateChanged) cur_but_state.poly.user_beenReleased = false;  //because button was just pressed
//        break;
//      case STATE_UNISON: 
//        activateUnison(stateChanged);
//        //if (newState != prevState) cur_but_state.unison.user_beenReleased = false;//because button was just pressed
//        if (stateChanged) cur_but_state.unison.user_beenReleased = false;//because button was just pressed
//        break;
//      case STATE_CHORD:
//        activateChordMemory(stateChanged);
//        //if (newState != prevState) cur_but_state.chord_mem.user_beenReleased = false;//because button was just pressed
//        if (stateChanged) cur_but_state.chord_mem.user_beenReleased = false;//because button was just pressed
//        break;
//    }
//  } else {  //no button has been pressed
//    //no button is pressed, so the current state is the previous state
//    newState = prevState;
//  }
//
//  //check to see if legato state can be toggled...based on whether the button for the current state was pressed again
//  updateLegatoState(cur_but_state,newState,prevState);
//
//  //save the state for the next time around
//  prevState = newState;
//}

//update the Hold state
void updateHoldState(assignerButtonState2_t &cur_but_state)
{
  //int but_hold = cur_but_state.hold.state;
  //int but_poly = cur_but_state.poly.state;
  //buttonStateReport_t holdButReport;
  //cur_but_state.hold.getStateReport(holdButReport);
  //int holdButtonChanged = cur_but_state.hold.didStateChange();

  //if arp button is on, ignore the hold press because the arp is doing something with the hold
  if ((cur_but_state.arp_on.state == ON) | (cur_but_state.arp_on.wasJustReleased()==true)) return;  //arp button is currently pressed
 
  //  //slightly more advanced logic...while the button is held down, latch in all held notes
  //  if (but_hold==ON) {
  //    //activate hold...flip the isHeld to ON
  //    if (DEBUG_TO_SERIAL & DEBUG_THIS_FILE) Serial.println("updateHoldState: activiating based on but_hold = ON");
  //    boolean enableHoldState=false; //this will hold the current notes without holding future notes
  //    if (holdButtonChanged==true) activateHold(enableHoldState);  //added the "if" 2013-06-05 
  //  }
  //
  //  //check to see if poly button is pushed...this puts us into a different hold state
  //  if ((but_hold==ON) & (but_poly==ON)) {
  //    if (DEBUG_TO_SERIAL & DEBUG_THIS_FILE) Serial.println("updateHoldState: activiating but no new notes");
  //    assignerState.hold = HOLD_STATE_ON_BUT_NO_NEW;
  //  }

  //check to see if button is released...if state is OFF and button is released, go into normal hold mode
  if (cur_but_state.hold.wasJustReleased() == true) {
    if (assignerState.hold == HOLD_STATE_OFF) {
      activateHoldState(); //hold current and future notes
    } else {
      deactivateHoldState();
    }
  }
}

//actiavte hold.  default is to activate current and future notes.  
//  enableHoldState=false only does current notes, not future
void activateHoldState(void) {
  activateHoldState(true);
}
void activateHoldState(boolean enableHoldState) {
  keyPressData_t *keyPress;

  Serial.print(F("stateManagement: activate hold..."));Serial.println(enableHoldState);
  
  if (enableHoldState) { //added Feb 20, 2021
    //loop over each key and activate hold...flip the isHeld to ON
    for (int i; i < trueKeybed.getMaxKeySlots(); i++) {
      //if (allKeybedData[i].isPressed==ON) allKeybedData[i].isHeld=ON;
      keyPress = trueKeybed.getKeybedDataP(i);
      if (keyPress->isPressed==ON) keyPress->isHeld=ON;
    }
  
    //loop over each key and activate hold...flip the isHeld to ON
    for (int i; i < trueKeybed_givenList.getMaxKeySlots(); i++) {
      keyPress = trueKeybed_givenList.getKeybedDataP(i);
      if (keyPress->isPressed==ON) keyPress->isHeld=ON;
    }
  }

  //activate the global hold state
  if (enableHoldState) assignerState.hold = HOLD_STATE_ON;
}

void deactivateHoldState(void) {

  Serial.println(F("stateManagement: deactivate hold."));

  //change the assigner state
  assignerState.hold = HOLD_STATE_OFF;

  //change the hold state on keybed notes
  trueKeybed.deactivateHold();
  trueKeybed_givenList.deactivateHold();
  arpManager.deactivateHold(); //stop the arpeggiator's held notes, too

}


//update the hold state from MIDI...basically just off and on
void updateHoldStateMIDI(int const &desired_hold_state) {
  if (DEBUG_TO_SERIAL & DEBUG_THIS_FILE) {
    Serial.print(F("stateManagement: updateHoldStateMIDI, desired_hold state = "));
    Serial.println(desired_hold_state);
  }
  if (desired_hold_state==ON) {
    activateHoldState();
  } else {
    //Serial.println("stateManagement: updateHoldStateMIDI: deactivateHold...");
    deactivateHoldState();
  }
}

void updateFootswitchHoldState(int const &desired_hold_state,int const &didStateChange) {
  if (didStateChange == true) {
    if (desired_hold_state == ON) {
      if (assignerState.hold == HOLD_STATE_OFF) {
        activateHoldState();
      }
    } else {
      if (assignerState.hold == HOLD_STATE_ON) {
        //Serial.println("stateManagement: updateFootswitchHoldState: deactivateHold");
        deactivateHoldState();
      }
    }
  }
}

//return true of false whether a new note should be held
boolean isHoldNewNote(void)
{
  if (assignerState.hold == HOLD_STATE_ON) {
    return true;
  } else {
    return false;
  }
}

void activatePoly(boolean const &stateChanged) 
{
  assignerState.poly = ON;
  assignerState.unison = OFF;
  assignerState.chord_mem = OFF;

  if (stateChanged==true) {
    //Serial.println("activatePoly: stateChanged is true.");
    assignerState.legato = OFF;
    assignerState.detune = OFF; 
  }
}

void activateUnison(boolean const &stateChanged)
{
  assignerState.poly = OFF;
  assignerState.unison = ON;
  assignerState.chord_mem = OFF;

  //change the number of voices that are tracked by the keybed to one
  //trueKeybed.set_nKeyPressSlots(1);

  if (stateChanged==true) {
    //Serial.println("activateUnison: stateChanged is true.");
    assignerState.legato = OFF;
  }
}

void deactivateUnison(void)
{
  //if not in arp mode, set each keypress to the note of the current voice so that it responds OK when going back out of chord memory
  if (assignerState.arp == OFF) {
    for (int i=0;i<N_POLY;i++) {
      (trueKeybed.getKeybedDataP(i))->noteNum = allVoiceData[i].noteNum;
      (trueKeybed.getKeybedDataP(i))->noteVel = allVoiceData[i].noteVel;
      (trueKeybed.getKeybedDataP(i))->isNewVelocity = 1;
    }
  }

  //tell the keybed to track N_POLY notes again
  trueKeybed.set_nKeyPressSlots(N_KEY_PRESS_DATA);
}

void activateUnisonPoly(boolean const &stateChanged)
{
  assignerState.poly = ON;
  assignerState.unison = ON;
  assignerState.chord_mem = OFF;
  
  //change the number of voices that are tracked by the keybed to three
  trueKeybed.set_nKeyPressSlots(3);
  //Serial.print("activateUnisonPoly: activated!  nKeyPressSlots = ");
  //Serial.println(trueKeybed.get_nKeyPressSlots());

  if (stateChanged==true) {
    //Serial.println("activaeUnisonPoly: stateChanged is true.");
    assignerState.legato = OFF;
  }
}

void deactivateUnisonPoly(void)
{
  //if not in arp mode, set each keypress to the note of the current voice so that it responds OK when going back out of chord memory
  if (assignerState.arp == OFF) {
    for (int i=0;i<N_POLY;i++) {
      (trueKeybed.getKeybedDataP(i))->noteNum = allVoiceData[i].noteNum;
      (trueKeybed.getKeybedDataP(i))->noteVel = allVoiceData[i].noteVel;
      (trueKeybed.getKeybedDataP(i))->isNewVelocity = 1;
    }
  }
  
  //tell the keybed to track N_POLY notes again
  trueKeybed.set_nKeyPressSlots(N_KEY_PRESS_DATA);
}


void activateChordMemory(boolean const &stateChanged)
{
  //if arp is off, lock in the currently pressed notes for the chord
  if (assignerState.arp == OFF) {
    if (stateChanged) {
      Serial.println(F("activaeChordMemory: stateChanged is true."));
      //lock in chord voicing of all pressed or held notes
      setChordMemState();
    }
  }

  assignerState.poly = OFF;
  assignerState.unison = OFF;
  assignerState.chord_mem = ON;

  if (stateChanged==true) {
    assignerState.legato = OFF;
  }
}

 
void deactivateChordMemory(void) 
{
  //if arp is not on, set each keypress to the current voice so that it responds OK when going back out of chord memory
  if (assignerState.arp == OFF) {
    for (int i=0;i<N_POLY;i++) {
      if (chordMemState.isNoteActive[i]==HIGH) {
        //allKeybedData[i].noteNum = allVoiceData[i].noteNum;
        //(trueKeybed.getKeybedDataP(i))->noteNum = allVoiceData[i].noteNum;
        (trueKeybed.getKeybedDataP(i))->setNoteNum(allVoiceData[i].targNoteNum_x16bits);
        (trueKeybed.getKeybedDataP(i))->noteVel = allVoiceData[i].noteVel;
        (trueKeybed.getKeybedDataP(i))->isNewVelocity = 1;
      }
    }
  }
}

void activateChordPoly(boolean const &stateChanged)
//chord memory should already have been active
{ 
  activateChordMemory(stateChanged);
  
  assignerState.poly = ON;
  assignerState.unison = OFF;
  assignerState.chord_mem = ON;

  if (stateChanged==true) {
    Serial.println(F("activateChordPoly: stateChanged is true."));
    assignerState.legato = OFF;
  }

  //all the keybed to track only 3 notes
  trueKeybed.set_nKeyPressSlots(3);
}

void deactivateChordPoly(void)
{
  deactivateChordMemory();
  
  //all the keybed to track only 3 notes
  trueKeybed.set_nKeyPressSlots(N_KEY_PRESS_DATA);
}

void setChordMemState(void)
{
  keyPressData_t* keyPress;

  //find lowest active note and note index in the keybed data
  int lowestKeyInd = 0; //initialize
  long int lowestNoteNum_x16bits = (1000L << 16);  //initialize to something stupidly large
  boolean anyNoteFound = false;
  for (int Ikey=0;Ikey<N_KEY_PRESS_DATA;Ikey++) {     
    if (trueKeybed.isGateActive(Ikey) == ON) {     
      keyPress = trueKeybed.getKeybedDataP(Ikey);
      if (keyPress->targNoteNum_x16bits < lowestNoteNum_x16bits) {
        lowestKeyInd = Ikey;
        lowestNoteNum_x16bits = keyPress->targNoteNum_x16bits;
        anyNoteFound=true;
      }
    }
  }
  if (!anyNoteFound) {
    //no note was found, do not change the chord mem state
    return;
  }

  //define the chord mem note numbers relative to the lowest note
  chordMemState.voiceIndexOfBase = lowestKeyInd;

  //set the state of each voice relative to the lowest note
  //static int noteShift = 0;
  long int noteShift_x16bits = 0;
  for (int Ivoice=0;Ivoice<N_POLY;Ivoice++) {
    keyPress = trueKeybed.getKeybedDataP(Ivoice);
    //noteShift = keyPress->noteNum - lowestNoteNum;
    //noteShift = constrain(noteShift,0,119);
    noteShift_x16bits = keyPress->targNoteNum_x16bits - lowestNoteNum_x16bits;
    noteShift_x16bits = constrain(noteShift_x16bits,0, (119L << 16));
    chordMemState.noteShift_x16bits[Ivoice]=noteShift_x16bits;
    chordMemState.isNoteActive[Ivoice] = trueKeybed.isGateActive(Ivoice);
    //chordMemState.isNoteActive[Ivoice] = isGateActive(allKeybedData[Ivoice]);
  }

  //set the detune factors for each voice
  setDetuneFactorsForChordMemory();

  //set the root note to be the active note by reseting its start time
  keyPress = trueKeybed.getKeybedDataP(lowestKeyInd);
  keyPress->start_millis = millis();
  keyPress->end_millis = keyPress->start_millis;

  if (DEBUG_TO_SERIAL & DEBUG_THIS_FILE) {
    Serial.print(F("setChordMemState: Shift: "));
    for (int i=0;i<N_POLY;i++) {
      Serial.print(chordMemState.noteShift_x16bits[i] >> 16);
      Serial.print(" ");
      delayMicroseconds(20);
    }
    Serial.print(F("isActive: "));
    for (int i=0;i<N_POLY;i++) {
      Serial.print(chordMemState.isNoteActive[i]);
      Serial.print(" ");
      delayMicroseconds(20);
    }
    Serial.println();
  }
}

//set the detune factors for each note in the chord memory.
//assumes that the noteShift has already been set for each voice
void setDetuneFactorsForChordMemory(void) {

  //look for special case of chip's favorite 4-note Mono/Poly config
  int isMonoPolyOctaves = checkAndApplyMonoPolyOctaveDetune();

  //if not Mono/Poly octaves, distribute the detune the regular way
  if (isMonoPolyOctaves == false) {
    int nActive=0,nPair=0;
    int prevActiveVoiceInd=-1;
    for (int i=0; i<N_POLY; i++) { 
      if (chordMemState.isNoteActive[i]==true) nActive++; 
    }
    if (nActive == 1) {
      //just the root note...no detune
      chordMemState.detuneFactor[chordMemState.voiceIndexOfBase] = 0;
    } else if (nActive == 2) {
      //just a pair of notes.  Are they octaves or different
      boolean isOctaves = true;
      boolean isUnison = true;
      for (int i=0; i<N_POLY; i++) { 
        if (chordMemState.isNoteActive[i]==true) { 
          if ( (((int)(chordMemState.noteShift_x16bits[i] >> 16)) % 12) != 0 ) isOctaves = false;
          if (chordMemState.noteShift_x16bits[i] != 0) isUnison = false;
        }
      }
      if (isUnison) {
        //detune root down and octave up
        for (int i=0; i<N_POLY; i++) {
          if (i == chordMemState.voiceIndexOfBase) {
            chordMemState.detuneFactor[i] = 2;  //positive is down, I think
          } else if (chordMemState.isNoteActive[i]==true) { 
            chordMemState.detuneFactor[i] = -2;  //positive is down, I think
          }
        }
      } else if (isOctaves) {
        //detune root down and octave up
        for (int i=0; i<N_POLY; i++) {
          if (i == chordMemState.voiceIndexOfBase) {
            chordMemState.detuneFactor[i] = 1;  //positive is down, I think
          } else if (chordMemState.isNoteActive[i]==true) { 
            chordMemState.detuneFactor[i] = -1;  //positive is down, I think
          }
        }
      } else {
        //detune the non-root note only
        for (int i=0; i<N_POLY; i++) {
          if ((i != chordMemState.voiceIndexOfBase) & (chordMemState.isNoteActive[i]==true)) { 
            chordMemState.detuneFactor[i] = +1;  //positive is down, I think
          }
        }
      }
    } else {
      //all other configuration of notes
      Serial.println(F("stateManagement: setDetuneFactorsForChordMemory: detune method 3"));
      nActive=0;
      for (int i=0; i<N_POLY; i++) {
        if (i == chordMemState.voiceIndexOfBase) {
          chordMemState.detuneFactor[i] = 0; //don't detune the root
        } else {
          if (chordMemState.isNoteActive[i]==true) {
            nActive++; //only count the non-root note
            chordMemState.detuneFactor[i]=0;  //don't detune unless it's a pair
            if ((nActive % 2) == 0) {
              //it is a new pair!
              nPair++;
              chordMemState.detuneFactor[prevActiveVoiceInd] = nPair;
              chordMemState.detuneFactor[i] = -nPair;
            }
            prevActiveVoiceInd = i;
          }
        }
      }
    }
  }
  
  for (int i = 0; i < N_POLY; i++) {
    Serial.print(F("stateManagement: setDetuneFactorsForChordMemory: voice "));
    Serial.print(i);
    Serial.print(F(" : detune = "));
    Serial.println(chordMemState.detuneFactor[i]); 
  }
}

//does the chord mem have 2 at root and 2 at +1 octave?
int checkAndApplyMonoPolyOctaveDetune(void) {
  int isMonoPolyOctaves = true;
  int nRoot = 0, nOctave = 0;
  for (int i=0; i<N_POLY; i++) {
    if (chordMemState.isNoteActive[i]==true) {
      if (chordMemState.noteShift_x16bits[i] == 0) nRoot++;
      if (chordMemState.noteShift_x16bits[i] == (12L << 16)) nOctave++;
      if ((chordMemState.noteShift_x16bits[i] != 0) & ((chordMemState.noteShift_x16bits[i] != (12L << 16)))) isMonoPolyOctaves=false;
    }
  }
  if (!((nRoot==2) & (nOctave==2))) isMonoPolyOctaves=false;

  if (isMonoPolyOctaves==true) {
    int curRootCount=0,curCountOctave = 0;
    for (int i=0; i<N_POLY; i++) {
      if (chordMemState.isNoteActive[i]==true) {
        if (chordMemState.noteShift_x16bits[i] == 0) {
          //chordMemState.detuneFactor[i] = 0; //don't detune the roots
          curRootCount++;
          if ((curRootCount % 2) == 0) {
            chordMemState.detuneFactor[i] = 0; //no detune
          } else {
            chordMemState.detuneFactor[i] = 0; //no detune
          }
        }
        if (chordMemState.noteShift_x16bits[i] == (12L << 16)) {
          curCountOctave++;
          if ((curCountOctave % 2) == 1) {
            chordMemState.detuneFactor[i] = 1;  //positive is down in pitch
          } else {
            chordMemState.detuneFactor[i] = -1; //negative is up in pitch
          }
        }
      }
    }
  }
  return isMonoPolyOctaves;
}


int noButtonsPressed_lookForRelease(assignerButtonState2_t &cur_but_state) 
{
  int newState = STATE_DEFAULT;
   
  //find which button was released and which had not been part of a 2-button combo before
  if ((cur_but_state.poly.wasJustReleased()==true) & (cur_but_state.poly.user_beenPressedAgain == true)) {
    newState = STATE_POLY;
    cur_but_state.poly.set_user_beenPressedAgainToFalse(); //in effect, it disables this button until it is pressed again
  } else if ((cur_but_state.unison.wasJustReleased()==true) & (cur_but_state.unison.user_beenPressedAgain == true))  {
    newState = STATE_UNISON;
    cur_but_state.unison.set_user_beenPressedAgainToFalse();//in effect, it disables this button until it is pressed again
  } else if ((cur_but_state.chord_mem.wasJustReleased()==true) & (cur_but_state.chord_mem.user_beenPressedAgain == true))  {
    newState = STATE_CHORD; 
    cur_but_state.chord_mem.set_user_beenPressedAgainToFalse();//in effect, it disables this button until it is pressed again
  }
 
  return newState;
} 
  
int oneButtonPressed_lookForRelease(assignerButtonState2_t &cur_but_state) 
{
  int newState = STATE_DEFAULT;
  
  //find which button is still held
  if (cur_but_state.poly.state == ON) {
    
    //find if any button was just released
    if (cur_but_state.unison.wasJustReleased()==true) {
      newState = STATE_UNISON_POLY;
    } else if (cur_but_state.chord_mem.wasJustReleased()==true) {
//      newState = STATE_CHORD_POLY; 
    }
    if (newState > STATE_DEFAULT) cur_but_state.poly.set_user_beenPressedAgainToFalse();  //this help us ignore this button when it is eventually released
    
  } else if (cur_but_state.unison.state == ON) {
 
    //find if any button was just released
    if (cur_but_state.poly.wasJustReleased()==true)  {
      newState = STATE_UNISON_POLY;
    } else if (cur_but_state.chord_mem.wasJustReleased()==true)  {
      /////NO ACTION HERE!
      //newState = STATE_CHORD_UNISON; 
    }
    if (newState > STATE_DEFAULT) cur_but_state.unison.set_user_beenPressedAgainToFalse();  //this help us ignore this button when it is eventually released  
    
  } else if (cur_but_state.chord_mem.state == ON) {
    
    //find if any button was just released
    if (cur_but_state.poly.wasJustReleased()==true)  {
      //newState = STATE_CHORD_POLY;
    } else if (cur_but_state.unison.wasJustReleased()==true) {
      /////NO ACTION HERE!
      //newState = STATE_CHORD_UNISON
    }
    if (newState > STATE_DEFAULT) cur_but_state.chord_mem.set_user_beenPressedAgainToFalse();  //this help us ignore this button when it is eventually released  
     
  }
  
  return newState;
  
}
 

void toggleLegatoState(void)
{
  assignerState.legato = !(assignerState.legato);
  //Serial.print("toggleLegatoState: exiting with legato = ");
  //Serial.println(assignerState.legato);
}

// Assumes that you've already checked that all three buttons are currently NOT pressed!
void updateLegatoState(assignerButtonState2_t &but_state, const int &state, const int &prevState)
{
  //to change legato state, the current state's button must be pressed and released, 
  //after already having been pressed the fist time.

  //legato requires that the assigner state state the same
  if ( !(state == prevState) ) {
    //assigner state is not the same as it was.  this is not a legato command.  return;
    return;
  }

  buttonStateReport_t unisonButReport;
  buttonStateReport_t chordButReport;
  buttonStateReport_t polyButReport;
  but_state.unison.getStateReport(unisonButReport);
  but_state.chord_mem.getStateReport(chordButReport);
  but_state.poly.getStateReport(polyButReport);

  if ( ((state == STATE_UNISON) & (isOKtoToggleLegato(unisonButReport)==true)) |
    ((state == STATE_CHORD) & (isOKtoToggleLegato(chordButReport)==true)) |
    ((state == STATE_POLY) & (isOKtoToggleLegato(polyButReport)==true)) )
  {
    toggleLegatoState();
  }  
}

//OK to toggle legato state when the button has been released, when it had been pressed, and when it has already been released before
int isOKtoToggleLegato(buttonStateReport_t &butStateReport)
{
  if ((butStateReport.state == OFF) & (butStateReport.stateChanged == true) & (butStateReport.prev_user_beenReleased == true)) 
  {
    return true;
  } else {
    return false;
  }
}

void setNewPortamentoValue(const micros_t &value_micros, const int &porta_setting_index) 
{
  int new_value = (int) (value_micros / 20000L);  //I think that the fastest is 50 Hz, which is 20000 micros, which I want to be 1
  porta_time_const[porta_setting_index] = constrain(new_value,0,60);
}

void setNewDetuneValue(const micros_t &prev_knob_value) 
{
  //I think that the fastest is 50 Hz, which is 20000 micros, which I want to be 1
  detune_noteBend_x16bits = (1000000L / prev_knob_value)*1000L;
}
