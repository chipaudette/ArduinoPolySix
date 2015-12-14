
#include "SPI.h"

//define digipot control
const int SS_pin = 10;


//define analog read
const int IN_PIN = A0;
const int max_val = (int)(0.48 / 5.0 * 1023.0);
#define N_VAL (11)
int thresh_val[N_VAL] = {2, 4, 10, 22, 37, 47, 60, 70, 81, 95, 9999};
const float wiper_ohm = 220.0;
const float pot_full_ohm = 20000.0;
const float fixed_r_ohm = 10000.0;
const float pot_ohm[] = {0.0, 300.0, 600.0, 1200.0, 3100.0, 20000.0, 99999.0, 99999.0, 99999.0, 99999.0, 99999.0}; 
byte out_val[N_VAL];
byte potValue;

//define functions
void writeAD5260(int SS_pin, byte value) {
    //send value via SPI
    digitalWrite(SS_pin,LOW);// take the SS pin low to select the chip
    SPI.transfer(value);//  send value via SPI:
    digitalWrite(SS_pin,HIGH);// take the SS pin high to de-select the chip
}

void setup() {
  //setup Serial
  Serial.begin(115200);
  delay(2000);

  //set the slaveSelectPin as an output:
  pinMode (SS_pin, OUTPUT);
  pinMode (SS_pin, HIGH);

  //initialize SPI:
  SPI.begin(); 
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  //delay to let everything settle
  delay(500);
  
  //initialize digipot
  potValue=255;
  Serial.print(F("Changing Values: ")); 
  Serial.println(potValue); 
  writeAD5260(SS_pin,potValue);
  
  //setup analog pin
  pinMode(IN_PIN,INPUT);

  //compute threshold values for analog comparison
  delay(2000);
  float step_counts = ((float)max_val) /((float)(N_VAL - 1));
  float win_counts = step_counts / 2.0;
  for (int i=0; i < N_VAL ; i++) {
    //thresh_val[i] = (int)(((float)(i+1))*step_counts - win_counts);
    //if (i==0) thresh_val[i]=thresh_val[i]*2/3;
    out_val[i] = max(0,(byte)(min(255.0,max(0.0,(pot_ohm[i]-wiper_ohm) / pot_full_ohm * 255.0 + 0.5))));
    Serial.print(F("Thresh "));Serial.print(i);
    Serial.print(": "); Serial.print(thresh_val[i]);
    Serial.print(": Out pot val = "); Serial.print(out_val[i]);
    Serial.println();
  }

}

int ind=255;
int prev_ind = 255;
int in_val = max_val;
int val = max_val;
float smooth_val = max_val;
float smooth_fac = 0.75;  //closer to one is more smoothing
//int prev_val;
void loop() {
  delay(50);

  //read analog value
  in_val = analogRead(IN_PIN);
  val = min(in_val,max_val);
  if (0) {
    smooth_val = val;
  } else {
    smooth_val = smooth_fac*smooth_val + (1.0-smooth_fac)*(float)val;  //smoothing
  }
  smooth_val = min(smooth_val,(float)max_val);

  //find matching step
  ind = 0;
  while ((ind < N_VAL) & ((int)smooth_val > thresh_val[ind])) ind++;
  //ind = (ind + prev_ind)/2;  //smoothing
  ind = min(ind,N_VAL-1);

  //has the value changed?
  if (ind != prev_ind) {
     prev_ind = ind;
      
    //set new pot value output values
    potValue=out_val[ind];
    Serial.print(F("Changing Values: ")); 
    Serial.print("In Val = "); Serial.print(in_val);
    Serial.print(", Sm Val = "); Serial.print(smooth_val);
    Serial.print(", In Volt = ");
    Serial.print(((float)in_val)/((float)1024)*5.0);
    Serial.print(", Out Ind = ");
    Serial.print(ind);
    Serial.print(", Out Val = ");
    Serial.print(potValue); 
    Serial.print(", Out Ohm = ");
    Serial.print(((float)potValue)/255.0*pot_full_ohm+wiper_ohm);
    Serial.println();
    writeAD5260(SS_pin,potValue);
  }
}


