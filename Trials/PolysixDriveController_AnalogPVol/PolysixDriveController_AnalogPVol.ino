//Created: Chip Audette (Synthhacker.blogspot.com), 2015
//License: The MIT License
//Arduino IDE: arduino.cc  v1.6.5

#include "SPI.h"

//define digipot control chip select pin
const int SS_pin = 10;

//define parameters for reading the analog "P-Vol" signal
const int IN_PIN = A0;  //which pin is connected to "P-Vol" signal in the Polysix
const int max_val = (int)(0.48 / 5.0 * 1023.0);  //max analogRead value expected
#define N_VAL (11)
int thresh_val[N_VAL] = {2, 4, 10, 22, 37, 47, 60, 70, 81, 95, 9999};  //here are the decision boundaries in analog counts (0-1023)

//define the parameters relating to the desired digipot levels
const float wiper_ohm = 220.0;       //resistance of wiper on digipot
const float pot_full_ohm = 20000.0;  //max resistance of digipot
const float fixed_r_ohm = 10000.0;   //R155 in Polysix
const float pot_ohm[] = {0.0, 300.0, 600.0, 1200.0, 3100.0, 20000.0, 99999.0, 99999.0, 99999.0, 99999.0, 99999.0}; //here are the desired resistance values
byte out_val[N_VAL]; //this will hold the actual digipot values corresponding to the desired resistances
byte potValue;

//define functions
void writeAD5260(int SS_pin, byte value) {
    //send value via SPI
    digitalWrite(SS_pin,LOW);// take the SS pin low to select the chip
    SPI.transfer(value);//  send value via SPI:
    digitalWrite(SS_pin,HIGH);// take the SS pin high to de-select the chip
}

void setup() {
  //setup Serial for debugging...it's only used for debugging
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
  
  //setup analog pin to read "P-Vol" from the polysix
  pinMode(IN_PIN,INPUT);

  //compute the digipot byte values to achieve desired resistance
  delay(2000); //delay to make sure 
  for (int i=0; i < N_VAL ; i++) {
    out_val[i] = max(0,(byte)(min(255.0,max(0.0,(pot_ohm[i]-wiper_ohm) / pot_full_ohm * 255.0 + 0.5))));
    Serial.print(F("Thresh "));Serial.print(i);
    Serial.print(": "); Serial.print(thresh_val[i]);
    Serial.print(": Out pot val = "); Serial.print(out_val[i]);
    Serial.println();
  }

}

//initialize some variables to use in the main loop
int ind=255;
int prev_ind = 255;
int in_val = max_val;
int val = max_val;
float smooth_val = max_val;
float smooth_fac = 0.75;  //between zero and one.  Closer to one is more smoothing
void loop() {
  delay(50); //slow down the loop a little bit

  //read analog value of the "P-Vol" signal from the Polysix 
  in_val = analogRead(IN_PIN);
  val = min(in_val,max_val);
  if (0) {  //do smoothing of input values?
    smooth_val = val; //no smoothing
  } else {
    smooth_val = smooth_fac*smooth_val + (1.0-smooth_fac)*(float)val;  //smoothing
  }
  smooth_val = min(smooth_val,(float)max_val);

  //find which of the 11 target digipot steps this analog input value corresponds to 
  ind = 0;
  while ((ind < N_VAL) & ((int)smooth_val > thresh_val[ind])) ind++;
  ind = min(ind,N_VAL-1);

  //has the analog input value changed?
  if (ind != prev_ind) {
     prev_ind = ind;
      
    //set new pot value output values
    potValue=out_val[ind];

    //write a bunch of text for debugging
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

    //send the desired value to the digipot
    writeAD5260(SS_pin,potValue);
  }
}


