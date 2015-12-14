
#include "MCP42XXX.h"
#include "SPI.h"

//define digipot control
const int SS_pin = 4;
const int chan_per_device = 2;
MCP42XXX digiPots = MCP42XXX(SS_pin,true,chan_per_device);
const int n_Devices = 1;   //could be 1 (single device) or 2 (daisy chained
const int n_PotValues = 2;  //should be n_Devices*chan_per_device

//define analog read
const int IN_PIN = A0;
const int max_val = (int)(0.5 / 5.0 * 1024.0);
#define N_VAL (11)
int thresh_val[N_VAL];
const byte out_val[] = {0, 11, 21, 42, 85, 255, 255, 255, 255, 255, 255};


byte potValues[n_PotValues];

void setup() {
  Serial.begin(115200);
  delay(2000);
  //Serial.println(F("MCP2XXX_digitalPotentiometer: starting..."));

  //setup analog pin
  pinMode(IN_PIN,INPUT);

  //compute threshold values for analog comparison
  delay(2000);
  float step_counts = ((float)max_val) /((float)(N_VAL - 1));
  float win_counts = step_counts / 2.0;
  for (int i=0; i < N_VAL ; i++) {
    thresh_val[i] = (int)(((float)(i+1))*step_counts - win_counts);
    if (i==0) thresh_val[i]=thresh_val[i]*2/3;
    Serial.print(F("Thresh "));Serial.print(i);
    Serial.print(": "); Serial.println(thresh_val[i]);
  }

  //initialize digipot
  potValues[0]=255;
  potValues[1]=255; //same value for both channels
  Serial.print(F("Changing Values:")); 
  Serial.print(" ");  Serial.print(potValues[0]); 
  Serial.print(" ");  Serial.print(potValues[1]); 
  Serial.println();
  digiPots.setValues(potValues,n_PotValues);
}

int ind=255;
int prev_ind = 255;
int val = max_val;
int prev_val;
void loop() {
  delay(100);

  //read analog value
  prev_val = val;
  int val = analogRead(IN_PIN);
  //val = (val + prev_val)/2;  //smoothing
  val = min(val,max_val);

  //find matching step
  prev_ind = ind;
  ind = 0;
  while ((ind < N_VAL) & (val > thresh_val[ind])) ind++;
  //ind = (ind + prev_ind)/2;  //smoothing
  ind = min(ind,N_VAL-1);

  //has the value changed?
  if (ind != prev_ind) {
    
    //prepare output values
    potValues[0]=out_val[ind];
    potValues[1]=out_val[ind]; //same value for both channels
  
    Serial.print(F("Changing Values: in_val = ")); Serial.print(val); 
    Serial.print(", out_ind = ");Serial.print(ind);
    Serial.print(", out_vals =");
    Serial.print(" ");  Serial.print(potValues[0]); 
    Serial.print(" ");  Serial.print(potValues[1]); 

    Serial.println();

    //write the values to the digipot
    digiPots.setValues(potValues,n_PotValues);
  }
}
