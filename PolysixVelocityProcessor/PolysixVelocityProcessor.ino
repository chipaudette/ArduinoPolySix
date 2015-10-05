/*
  Korg Polysix Velocity Controller

  Chip Audette, SynthHacker
  October 2015

  This code is written assuming the use of a Teensy 3.1.
  It might work for an Arduino UNO, as well, though the Uno might
  not be fast enough.

  Controls the multiplexed envelope CV using a digital potentiometer.
  The setting for the digital pot is received from single-byte messages
  received by the Teensy over its TTL serial (Serial1) or over USB serial.

  A new digital pot setting is given every time the INH is pulled high.
  To make it faster, it assumes what the current channel is based on 
  the previously addressed channel from the ABC address lines.
  
*/


// include the SPI library:
#include <SPI.h>

// define which serial pins are used
#define HWSERIAL Serial1    //for Teensy 3.1, Serial1 Rx = Pin 0, Tx = Pin1

// define the TEENSY pins
#define SPI_CS 10   //SPI chip select pin for the digital pot
#define A_PIN  5    //for reading the Polysix Address Line "A"
#define B_PIN  6    //for reading the Polysix Address Line "B"
#define C_PIN  7    //for reading the Polysix Address Line "C
#define INH_PIN  8  //for reading the Polysix INH line  sent to IC24 Multiplexer

// define data variables
#define N_CHAN 8
byte values[N_CHAN];  //span 0-255
int cur_chan = 0;
int chan_from_given_addr = 0;

void setup() {
  //start serial for debugging
  Serial.begin(115200);
  HWSERIAL.begin(115200);

  // initialize SPI:
  pinMode (SPI_CS, OUTPUT);
  pinMode (SPI_CS, HIGH);
  SPI.begin();
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  // setup digital input pins and interupt on INH pin
  pinMode(A_PIN, INPUT); // sets the digital pin as output
  pinMode(B_PIN, INPUT); // sets the digital pin as output
  pinMode(C_PIN, INPUT); // sets the digital pin as output
  pinMode(INH_PIN, INPUT); // sets the digital pin as output
  attachInterrupt(INH_PIN, isrService, RISING); // interrrupt 1 is data ready
  //delay(1000);

  // initialize values
  for (int i=0; i<6; i++) values[i] = 255;  //purposely leave last two voices uninitialized
  isrService();      //initialize the output by forcing a call to the isrService
}

#define HWSERIAL Serial1

void loop() {

  //testSpeed();  // use this to see how fast the pot servicing could possibly be
  testSpeedISR();  // use this to see how fast the pot servicing could possibly be

  //service Serial
  int serialValue;
  if (Serial.available() > 0) {
    serialValue = Serial.read();
    interpretSerialByte((byte)serialValue);
  }
  if (HWSERIAL.available() > 0) {
    serialValue = HWSERIAL.read();
    interpretSerialByte((byte)serialValue);
  }
}

void interpretSerialByte(byte serialByte) {
  int chan = (serialByte & 0b00000111);  //low bits are channel
  values[chan]  = (byte)(serialByte & 0b11111000);  //high bits are the value 
  //Serial.print("Received: Chan ");
  //Serial.print(chan);
  //Serial.print(", Value ");
  //Serial.println(values[chan]);
}

// When INH goes high, output a new value
void isrService()
{
  //cli();
//  Serial.print("Service: chan ");
//  Serial.print(cur_chan);
//  Serial.print(", value ");
//  Serial.print(values[cur_chan]);
  
  //write value for this channel
  digitalWrite(SPI_CS, LOW); // take the SS pin low to select the chip
  SPI.transfer(values[cur_chan]);//  send value via SPI: 
  digitalWrite(SPI_CS, HIGH); // take the SS pin high to de-select the chip

  //get channel from address lines (hopefully, they've been set by now)
  //delayMicroseconds(3);  //maybe this is necessary to let ABC settle.
  chan_from_given_addr = (digitalRead(A_PIN) << 2) |
                         (digitalRead(B_PIN) << 1) |
                         (digitalRead(C_PIN));

//  Serial.print(", ABC Chan ");
//  Serial.print(chan_from_given_addr);
//  Serial.println();

  //prepare for next time through
  cur_chan = chan_from_given_addr + 1;
  if (cur_chan >= N_CHAN) cur_chan = 0;

  //sei();
}


void testSpeed(void) {

  while (1) {  //loop forever

    // change the resistance from min to max:
    for (int I_chan = 0; I_chan < N_CHAN; I_chan++) {
      //Serial.print("Commanding value: ");
      //Serial.print(values[I_chan]);
      //Serial.print(", Should Indicate ");
      //Serial.print(((float)level) / 255.0 * 5.0);
      //Serial.println(" V");

      //send value via SPI
      digitalWrite(SPI_CS, LOW); // take the SS pin low to select the chip
      SPI.transfer(values[I_chan]);//  send value via SPI:
      digitalWrite(SPI_CS, HIGH); // take the SS pin high to de-select the chip

      //wait to allow measurement
      //delayMicroseconds(5);
    }
  }
}

void testSpeedISR(void) {
  int force_chan = 0;
  detachInterrupt(INH_PIN);
  while (1) {
    force_chan++; 
    if (force_chan >= N_CHAN) force_chan=0;
    cur_chan = force_chan;
    isrService();
  }
}


