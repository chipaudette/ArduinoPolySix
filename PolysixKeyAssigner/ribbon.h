

#ifndef _Ribbon_h
#define _Ribbon_h

#include "dataTypes.h"

#define DEBUG_RIBBON false
#define PRINT_CAL_TEXT false

#define RIBBON_SPAN (1023)
//define BEND_DEFAULT 0
//define BEND_FOR_RIBBON 1
//define RIB_NOTE_NUM_IF_NOTE_OFF -1.0
#define RIB_NOTE_ON_VEL 127
#define RIB_NOTE_OFF_VEL 127
#define RIB_NOTE_OFFSET (12.0f)

class Ribbon {
  public:
    Ribbon(void) { }
    
    //int ribbon_max_val = 339,ribbon_min_val = 13; //for my Arduino UNO
    //int ribbon_max_val = 520,ribbon_min_val = 22; //for my Arduino UNO, External 3.3V ref
    int ribbon_max_val = 333,ribbon_min_val = 13; //for my Arduino Micro and Arduino Mega
    float R_pullup_A3_kOhm = 0.0;
    float R_ribbon_min_kOhm = 0.0;
    const float R_ribbon_max_kOhm = 17.45;
    const float ribbon_span_half_steps_float = 36;
    const float ribbon_span_extra_half_steps_float = 0.25;
    //const int max_allowed_jump_steps = 5;
    //boolean flag_bend_range_is_for_ribbon = false;
    int toggle_counter = 0;
    const int filt_reset_thresh = ribbon_max_val + max(1,(RIBBON_SPAN - ribbon_max_val)/6);
    int ribbonPin = A0;

    void setup_ribbon(int ribPin, keybed_base_t *keybed);
    void service_ribbon(const unsigned long &cur_millis);
    float read_ribbon(void);
    float process_ribbon(float);
    int prev_raw_ribbon_value = 0;
  private:
    bool is_note_on = false;
    int keybed_voice_ind = -1;
    keybed_base_t *thisKeybed;

};
    
  
void Ribbon::setup_ribbon(int ribPin, keybed_base_t *keybed) {
  ribbonPin = ribPin;
  pinMode(ribbonPin, INPUT_PULLUP);
  //set scaling for ribbon
  R_pullup_A3_kOhm = R_ribbon_max_kOhm * ( ((float)RIBBON_SPAN) / ((float)ribbon_max_val) - 1.0 );
  R_ribbon_min_kOhm = ((float)ribbon_min_val) / ((float)ribbon_max_val) * R_ribbon_max_kOhm;
  thisKeybed = keybed;
}

void Ribbon::service_ribbon(const unsigned long &cur_millis = 0) {

  //make sure that we're not calling this too often
  static unsigned long int prev_millis = 0L; 
  if ((cur_millis > 0) && (cur_millis >= prev_millis) && ((cur_millis - prev_millis) < 5)) { return; } //called too fast, so just return
  prev_millis = cur_millis; //save for next time

  //calculate the average sample rate for the ribbon...hopefully, it's around 200 Hz (because of the 5 msec stalling from above)
  #if (DEBUG_RIBBON)
    static int call_count = 0;
    static unsigned long start_millis = 0;
    call_count++;
    if (call_count == 200) {
      float samples_per_second = ((float)call_count)/((float)(cur_millis - start_millis))*1000.0f;
      Serial.print("Ribbon: service_ribbon: service_ribbon: ave ribbon sample (Hz) = ");
      Serial.println(samples_per_second);

      //reset for next count
      call_count = 0;
      start_millis = cur_millis;
    }
  #endif

  //Now, start to do real work
  float rib_value_float, note_num_float;
  
  //read input and do any noise-reduction filtering
  rib_value_float = read_ribbon(); //this is now done in the interrupt service routine!

  //is a note active
  bool was_note_on = is_note_on;
  is_note_on = (((int)rib_value_float) < (ribbon_max_val + 10));  //it is on if the value is withinthe allowed range of th ribbon
  if (is_note_on) note_num_float = process_ribbon(rib_value_float);

  //add in any offset
  note_num_float += RIB_NOTE_OFFSET;

  #if (DEBUG_RIBBON)
    if ((is_note_on) && (call_count % 5 == 0)) {
      Serial.print("Ribbon: service_ribbon: rib_value_float = ");
      Serial.print(rib_value_float);
      Serial.print(", was_note_on = ");
      Serial.print(was_note_on);
      Serial.print(", is_note_on = ");
      Serial.print(is_note_on);
      if (is_note_on) {
        Serial.print(", note_num_float = "); 
        Serial.print(note_num_float);
      }
      Serial.println();
    }
  #endif

  //update the note info depending upon whether note is starting, sustaining, or stopping
  if (was_note_on == false) {
    if (is_note_on == true) {
      //note starting
      keybed_voice_ind = thisKeybed->addKeyPress(((int)note_num_float+0.5),RIB_NOTE_ON_VEL);
    } else {
      //not is still off.  do nothing.
    }
  } else {
    // note has been on
    if (is_note_on == true) {
      //note is sustaining...update its ptich
      if (keybed_voice_ind >= 0) (thisKeybed->getKeybedDataP(keybed_voice_ind))->noteNum = ((int)(note_num_float+0.5));
    } else {
      // note was on but is now off...stop the note
      thisKeybed->stopKeyPressByInd(keybed_voice_ind,RIB_NOTE_OFF_VEL);
    }
  }
}


float Ribbon::read_ribbon(void) {

  //read the ribbon value
  int raw_ribbon_value = analogRead(ribbonPin);
  int ribbon_value = raw_ribbon_value;
  float ribbon_value_float = (float)ribbon_value;
  //if (drivePin_state == LOW) ribbon_value_float = RIBBON_SPAN - ribbon_value_float;


//  //invert the drive pin
//  toggle_counter++;
//  if (toggle_counter >= 1) {
//    toggle_counter = 0;
//    drivePin_state = !drivePin_state;
//    digitalWrite(drivePin,drivePin_state);    
//  }
  
//  //if the ribbon value is above some limit (ie, not being touched), don't apply the filters.  
//  //This is to prevent the internal filter states getting spun up onto these non-real values.  
//  //Hopefully, this'll improve the pitch transitions on short, sharp, stacato notes.
//  if (ribbon_value < filt_reset_thresh) {
//    //apply notch filtering
//    if (DO_NOTCH_FILTERING) {
//      //apply filtering
//      ribbon_value_float = notch_filter.process(ribbon_value_float); //60 Hz
//      //ribbon_value_float = notch_filter2.process(ribbon_value_float); //120 Hz
//    }
//  
//    //apply low-pass filtering (for decimation)
//    if (PROCESSING_DECIMATION > 1) {
//      ribbon_value_float = lp_filter.process(ribbon_value_float);
//    }
//
//    //if previously the note was off, force the system to update now for max responsiveness
//    if (prev_raw_ribbon_value >= filt_reset_thresh) {
//      decimation_counter = PROCESSING_DECIMATION;
//    }
//  } else {
//    //if note was previously on, now it's off, so force the systme to update now for max responsiveness
//    if (prev_raw_ribbon_value <= ribbon_max_val) {
//      //this is the note turning off.  Force the system to update now
//      decimation_counter = PROCESSING_DECIMATION;
//    }
//  }
//  prev_raw_ribbon_value = raw_ribbon_value;

/*
 * 
  //print some debugging info
  if (PRINT_RAW_RIBBON_VALUE) {
    if (flag_cannot_keep_up) {
      Serial.print(-ribbon_value); //enable this to get value at top and bottom of ribbon to calibrate
    } else {
      Serial.print(ribbon_value); //enable this to get value at top and bottom of ribbon to calibrate
    }
    Serial.print(", "); //enable this to get value at top and bottom of ribbon to calibrate
    Serial.print((int)ribbon_value_float); //enable this to get value at top and bottom of ribbon to calibrate
    //Serial.print(compare_value); //enable this to get value at top and bottom of ribbon to calibrate
    Serial.println();
    flag_cannot_keep_up = false;
  }
  */

  //save for next time
  //prev_raw_ribbon_value = raw_ribbon_value;

  return ribbon_value_float;
}
float Ribbon::process_ribbon(float ribbon_value_float) {

  // filter if note is a legitimate value
  static float prev_ribbon_value_float = 0.0f;
  //int ribbon_value = (int)ribbon_value_float;
  if (prev_ribbon_value_float > (float)(ribbon_max_val + 10)) {//was it previously not a note?
    //if was previously not a note, so set previous value to the current value
    prev_ribbon_value_float = ribbon_value_float; //save unfiltered value for next time     
  }
  //filter
  const float new_fac = 0.5f, old_fac = 0.999f*(1.0f-new_fac); //a smaller "new_fac" results in more low-pass filtering
  float filt_ribbon_value_float = new_fac * ribbon_value_float + old_fac * prev_ribbon_value_float;
  prev_ribbon_value_float = filt_ribbon_value_float; //save for next time
  ribbon_value_float = filt_ribbon_value_float;

  //process ribbon value
  float foo_float = 1.0 / (((float)RIBBON_SPAN) / ribbon_value_float - 1.0);
  float ribbon_frac = (R_pullup_A3_kOhm / R_ribbon_max_kOhm) * foo_float;
  float ribbon_R_kOhm = R_pullup_A3_kOhm * foo_float;
  float ribbon_span_frac = (ribbon_R_kOhm - R_ribbon_min_kOhm) / (R_ribbon_max_kOhm - R_ribbon_min_kOhm);
  ribbon_span_frac = max(0.0, ribbon_span_frac);
  float note_num_float = ribbon_span_frac * (ribbon_span_half_steps_float + ribbon_span_extra_half_steps_float);
  note_num_float = note_num_float - ribbon_span_extra_half_steps_float/2.0;

  //apply a calibration (of sorts) to better linearize the response
  if (1) { 
    #define N_CAL 8
    float input_vals[N_CAL] = {-0.11, 0.3, 1.20, 2.2,  3.0, 10.53, 22.93, 35.9}; //calibraiton table...input values measured from ribbon
    float output_vals[N_CAL] = {0.0,  1.0, 2.0,  3.0,  4.0, 12.0,  24.0, 36.0};    //calibraiton table...output values desired for the input values above

    if (PRINT_CAL_TEXT) {
      if ((note_num_float > -10.0) & (note_num_float < 38.0)) {
        Serial.print("Applying Cal: raw_ribbon_val = ");
        Serial.print(ribbon_value_float);
        Serial.print(", note_num_float = ");
        Serial.print(note_num_float);
      }
    }

    //search through calibration table and apply
    int iHigh = 1;
    while ((iHigh < (N_CAL-1)) && (note_num_float > input_vals[iHigh])) {  iHigh++; }  
    int iLow = iHigh-1;
    note_num_float = (note_num_float - input_vals[iLow])/(input_vals[iHigh]-input_vals[iLow])*(output_vals[iHigh]-output_vals[iLow])+output_vals[iLow];


//    float bottom_transition = 1.5;
//    float first_transition = 8.0;
//    float second_transition = -2.0; //-10
//    float third_transition = 1.0; //-5  
    //float cal_amount = 1.0; // Uno
//    float cal_amount = 1.0; // micro
//    if (note_num_float > bottom_transition) {
//      if (note_num_float < first_transition) {
//        note_num_float += (cal_amount*((note_num_float-bottom_transition)/(first_transition-bottom_transition)));
//      } else if (note_num_float < (ribbon_span_half_steps_float+second_transition)) {
//        note_num_float += cal_amount;
//      } else if (note_num_float < (ribbon_span_half_steps_float+third_transition)) {
//        float foo = note_num_float - (ribbon_span_half_steps_float+second_transition); //how many above the transition
//        float full_segment = -second_transition + third_transition; //positive number
//        note_num_float += cal_amount*(1.0 - foo/full_segment);
//      }
//    }

    if (PRINT_CAL_TEXT) {
      if ((note_num_float > -10.0) & (note_num_float < ribbon_span_half_steps_float+2.0)) { 
        Serial.print(", final note_num_float = ");
        Serial.println(note_num_float);
      }
    }

    
  }


  /* Process to get the note num + bend (as for issuing a MIDI command)
  note_num = ((int)(note_num_float + 0.5)); //round

  //look for special case...decide to change note or just bend a lot
  closest_note_num = note_num;
  if (0) {
    //add some hysteresis to prevent fluttering between two notes
    //if you keep you finger on the line between them
    if (abs(note_num - prev_note_num) == 1) {  //is it a neighboring note?
      if (note_num > prev_note_num) {
        note_num = ((int)(note_num_float + 0.35)); //need extra to get up to next note
      } else if (note_num < prev_note_num) {
        note_num = ((int)(note_num_float + 0.65)); //next extra to get down to next note
      }
    }
  } else {
    //if it can bend the note, just let it bend the note
    if ( ((prev_is_note_on==true) && (abs(note_num_float - ((float)prev_note_num)) < (synth_bend_half_steps-0.5))) ||  //bend if we can
         ((prev_is_note_on==false) && (abs(note_num - prev_note_num) < min(2,synth_bend_half_steps))) ) { //if new note, only bend if we're off by one note
      //don't change note, step back to original note...the rest of the code will just bend more
      note_num = prev_note_num;
    }
  }
  */

/*
  //calc the desired brightness CC value (as a value of 0.0 to 1.0)
  brightness_frac = min(1.0,max(0.0,note_num_float / ribbon_span_half_steps_float));
  */

  return note_num_float;
}

#endif
