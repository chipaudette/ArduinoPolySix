

#ifndef _Ribbon_h
#define _Ribbon_h

#include "dataTypes.h"

#define DEBUG_RIBBON  false
#define PRINT_CAL_TEXT false

#define RIBBON_SPAN (1023)
#define RIB_NOTE_ON_VEL 127
#define RIB_NOTE_OFF_VEL 127
#define RIB_NOTE_OFFSET (12.0f)    //the bottom of the ribbon would normally issue note zero.  This adds an offset to issue a different note

class Ribbon {
  public:
    Ribbon(void) { setCalToDefaultPlusAdjustment(); }
    
    //int ribbon_max_val = 339,ribbon_min_val = 13; //for my Arduino UNO
    //int ribbon_max_val = 520,ribbon_min_val = 22; //for my Arduino UNO, External 3.3V ref
    //int ribbon_max_val = 333,ribbon_min_val = 13; //for my Arduino Micro and Arduino Mega
    int ribbon_max_val = 798,ribbon_min_val = 33; //using analog reference via 
    float ribbon_max_val_scaled=0.0f, ribbon_min_val_scaled = 0.0f; //to be set later
    
    float R_pullup_A3_kOhm = 0.0; //to be set later
    float R_ribbon_min_kOhm = 0.005;
    const float R_ribbon_max_kOhm = 17.3;  
    const float ribbon_span_half_steps_float = 36.0; //total span of the ribbon (3 octaves, C to C)
    const float ribbon_span_extra_half_steps_float = 0.25;
    float Vin_Vref = 1.0; //to be set later
    float Vin_Vref_SPAN = 1.0; //to be set later
    
    //const int max_allowed_jump_steps = 5;
    //boolean flag_bend_range_is_for_ribbon = false;
    int toggle_counter = 0;
    //const int filt_reset_thresh = ribbon_max_val + max(1,(RIBBON_SPAN - ribbon_max_val)/6);
    int ribbonPin = A0;
    int ribbonRefPin = A1;

    void setup_ribbon(int ribPin, int ribRefPin, keybed_base_t *keybed);
    void service_ribbon(const unsigned long &cur_millis);
    float read_ribbon(void);
    float process_ribbon(float);
    int prev_raw_ribbon_value = 0;

    #define N_CAL 12
    float default_cal_input_vals[N_CAL] = {0.0, 1.02, 2.4, 3.8, 4.81, 8.55, 13.93, 19.31, 25.9, 30.35, 35.1, 35.9};
    float cal_output_vals[N_CAL] = {0.0, 1.0, 2.0,  3.0, 4.0,  7.0,  12.0,  17.0,  24.0,  29.0, 35.0, 36.0};    //calibraiton table...output values desired for the input values above
    float cal_input_vals[N_CAL];
    #define N_ADJUST_CAL 2
    float cal_adjust[N_ADJUST_CAL] = {0.0, 0.0}; //adjustment to cal. Low end of ribbon and high end of ribbon.  Negative values lowers the pitch
    void setCalToDefaultPlusAdjustment(void) {
      //if (Serial) Serial.println("setCalToDefaultPlusAdjustment: adjusting...");
      for (int i=0; i<N_CAL; i++) { 
        cal_input_vals[i] = default_cal_input_vals[i]; //default value for the calibration
         //if (Serial) { Serial.print("    "); Serial.print(i); Serial.print(", default = "); Serial.print(cal_input_vals[i]); }

        //now, adjust the calibration by the two-point adjustment
        float cur_val = cal_input_vals[i];
        float fracOfSpan = (cur_val - default_cal_input_vals[0])/(default_cal_input_vals[N_CAL-1]-default_cal_input_vals[0]);
        float adjustment = fracOfSpan * (cal_adjust[1]-cal_adjust[0]) + cal_adjust[0];
        cal_input_vals[i] -= adjustment;
        //if (Serial) { Serial.print(", adjust = "); Serial.print(adjustment);Serial.print(", final = "); Serial.println(cal_input_vals[i]);}
      }      
    }
    float setCal(int calAdjustInd, float newVal) {
      if (calAdjustInd <= N_ADJUST_CAL) {
        cal_adjust[calAdjustInd] = newVal;
        setCalToDefaultPlusAdjustment(); //update the combined table of the default cal plus the adjustment
        return cal_adjust[calAdjustInd];
      }
      return -999.999f;
    }
    float incrementCal(int calAdjustInd, float addedVal) {
      //Serial.print("ribbon: incrementCal: calAdjustInd, addedVal = "); Serial.print(calAdjustInd); Serial.print(", "); Serial.println(addedVal);
      if (calAdjustInd <= N_ADJUST_CAL) { 
        float foo_val = setCal(calAdjustInd, cal_adjust[calAdjustInd]+addedVal);
        //Serial.print("ribbon: incrementCal: newVal = "); Serial.println(foo_val);
      }
      return -999.999f;
    }
    
  private:
    bool is_note_on = false;
    int keybed_voice_ind = -1;
    keybed_base_t *thisKeybed;

};
    
  
void Ribbon::setup_ribbon(int ribPin, int ribRefPin, keybed_base_t *keybed) {
  //set the analog read pins
  ribbonPin = ribPin;
  ribbonRefPin = ribRefPin;
  pinMode(ribbonPin, INPUT_PULLUP);
  pinMode(ribbonRefPin, INPUT_PULLUP);
  analogReference(EXTERNAL);  //output of ribbonRefPin (pullup ~35K?) through 20K to ground.  Does this pin also have ~35K to ground?
  //analogReference(INTERNAL2V56);
  
  //set scaling for ribbon
  //R_pullup_A3_kOhm = R_ribbon_max_kOhm * ( ((float)RIBBON_SPAN) / ((float)ribbon_max_val) - 1.0 );
  //R_ribbon_min_kOhm = ((float)ribbon_min_val) / ((float)ribbon_max_val) * R_ribbon_max_kOhm;
  ribbon_max_val_scaled = ((float)ribbon_max_val)/((float)RIBBON_SPAN);
  ribbon_min_val_scaled = ((float)ribbon_min_val)/((float)RIBBON_SPAN);
  Vin_Vref = (R_ribbon_max_kOhm - R_ribbon_min_kOhm) /( (R_ribbon_max_kOhm/ribbon_max_val_scaled) - (R_ribbon_min_kOhm/ribbon_min_val_scaled) );
  R_pullup_A3_kOhm = -R_ribbon_max_kOhm + Vin_Vref*(R_ribbon_max_kOhm/ribbon_max_val_scaled);
  Vin_Vref_SPAN = Vin_Vref * ((float)RIBBON_SPAN);

//  delay(1000);
//  Serial.print("Ribbon: setup_ribon: ribbon_max_val_scaled = "); Serial.println(ribbon_max_val_scaled);
//  Serial.print("Ribbon: setup_ribon: ribbon_min_val_scaled = "); Serial.println(ribbon_min_val_scaled);
//  Serial.print("Ribbon: setup_ribon: Vin_Vref = "); Serial.println(Vin_Vref);
//  Serial.print("Ribbon: setup_ribon: R_pullup_A3_kOhm = "); Serial.println(R_pullup_A3_kOhm);
//  Serial.print("Ribbon: setup_ribon: Vin_Vref_SPAN = "); Serial.println(Vin_Vref_SPAN);  
  
  //set keybed to which we will transmit keypresses
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
      Serial.print(F("Ribbon: service_ribbon: service_ribbon: ave ribbon sample (Hz) = "));
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
  is_note_on = (((int)rib_value_float) < (ribbon_max_val + 10));  //it is on if the value is within the allowed range of th ribbon
  if (is_note_on) note_num_float = process_ribbon(rib_value_float);

  //add in any offset
  note_num_float += RIB_NOTE_OFFSET;

  #if (DEBUG_RIBBON)
    if ((is_note_on) && (call_count % 5 == 0)) {
      Serial.print(F("Ribbon: service_ribbon: rib_value_float = "));
      Serial.print(rib_value_float);
      Serial.print(F(", was_note_on = "));
      Serial.print(was_note_on);
      Serial.print(F(", is_note_on = "));
      Serial.print(is_note_on);
      if (is_note_on) {
        Serial.print(F(", note_num_float = ")); 
        Serial.print(note_num_float);
      }
      Serial.println();
    }
  #endif

  //update the note info depending upon whether note is starting, sustaining, or stopping
  if (was_note_on == false) {
    if (is_note_on == true) {
      //note starting
      keybed_voice_ind = thisKeybed->addKeyPress(note_num_float,RIB_NOTE_ON_VEL,true);
    } else {
      //not is still off.  do nothing.
    }
  } else {
    // note has been on
    if (is_note_on == true) {
      //note is sustaining...update its ptich
      if (keybed_voice_ind >= 0) (thisKeybed->getKeybedDataP(keybed_voice_ind))->setNoteNum(note_num_float);
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


  return ribbon_value_float;
}
float Ribbon::process_ribbon(float ribbon_value_float) {
  static int count = 0;
  bool printDebug = false;

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
  float note_num_float=0.0;
  #if 0
    //variable resistor below fixed resistor
    float foo_float = 1.0f / (((float)RIBBON_SPAN) / ribbon_value_float - 1.0f);
    float ribbon_frac = (R_pullup_A3_kOhm / R_ribbon_max_kOhm) * foo_float;
    float ribbon_R_kOhm = R_pullup_A3_kOhm * foo_float;
    float ribbon_span_frac = (ribbon_R_kOhm - R_ribbon_min_kOhm) / (R_ribbon_max_kOhm - R_ribbon_min_kOhm);
    //ribbon_span_frac = max(0.0, ribbon_span_frac);
    note_num_float = ribbon_span_frac * (ribbon_span_half_steps_float + ribbon_span_extra_half_steps_float);
    note_num_float = note_num_float - ribbon_span_extra_half_steps_float/2.0;
  #else

    #if 1
      //basic linear interpolation
      note_num_float = (ribbon_value_float - ribbon_min_val)/(ribbon_max_val - ribbon_min_val)*(ribbon_span_half_steps_float+ribbon_span_extra_half_steps_float);
      note_num_float -= (0.5f*ribbon_span_extra_half_steps_float); //shift over a smidge
    #else   
      //new resistor network method
      //float ribbon_value_scaled = ribbon_value_float / ((float)RIBBON_SPAN);
      //float foo_denom = Vin_Vref / ribbon_value_scaled - 1.0f;
      float foo_denom = Vin_Vref_SPAN / ribbon_value_float - 1.0f;
      float R_kOhm = R_pullup_A3_kOhm / foo_denom;
      note_num_float = (R_kOhm - R_ribbon_min_kOhm) / (R_ribbon_max_kOhm + R_ribbon_min_kOhm); //should span 0.0 to 1.0
      note_num_float *= (ribbon_span_half_steps_float + ribbon_span_extra_half_steps_float);  //scale up to full length of the ribbon (half steps)
      note_num_float -= (0.5f*ribbon_span_extra_half_steps_float); //shift over a smidge
    #endif
  #endif

  //apply a calibration (of sorts) to better linearize the response
  if (1) { 
    if (PRINT_CAL_TEXT) {
      if ((note_num_float > -10.0) && (note_num_float < 38.0)) {
        count++;
        if (count >= 20) {
          count = 0;
          Serial.print(F("Applying Cal: raw_ribbon_val = "));
          Serial.print(ribbon_value_float);
          Serial.print(F(", note_num_float = "));
          Serial.print(note_num_float);
          printDebug = true;
        }
      }
    }

    //search through calibration table and apply
    int iHigh = 1;
    while ((iHigh < (N_CAL-1)) && (note_num_float > cal_input_vals[iHigh])) {  iHigh++; }  
    int iLow = iHigh-1;
    note_num_float = (note_num_float - cal_input_vals[iLow])/(cal_input_vals[iHigh]-cal_input_vals[iLow])*(cal_output_vals[iHigh]-cal_output_vals[iLow])+cal_output_vals[iLow];

    if ((PRINT_CAL_TEXT) && (printDebug)) {
      if ((note_num_float > -10.0) & (note_num_float < ribbon_span_half_steps_float+2.0)) { 
        Serial.print(F(", final note_num_float = "));
        Serial.println(note_num_float);
      }
    }
  }

  return note_num_float;
}

#endif
