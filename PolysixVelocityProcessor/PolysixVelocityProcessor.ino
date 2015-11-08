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
#define A_PIN  18    //for reading the Polysix Address Line "A"
#define B_PIN  19    //for reading the Polysix Address Line "B"
#define C_PIN  20    //for reading the Polysix Address Line "C
#define INH_PIN  17  //for reading the Polysix INH line  sent to IC24 Multiplexer
#define TEST1_PIN 2   //puts it into a test mode and ignores serial inputs
#define TEST2_PIN 3   //puts it into a test mode and ignores serial inputs
#define TEST3_PIN 4   //puts it into a test mode and ignores serial inputs
#define TEST4_PIN 5   //puts it into a test mode and ignores serial inputs
#define TEST5_PIN 6   //puts it into a test mode and ignores serial inputs
#define TEST6_PIN 7   //puts it into a test mode and ignores serial inputs
#define TEST7_PIN 8   //puts it into a test mode and ignores serial inputs
#define TEST8_PIN 9   //puts it into a test mode and ignores serial inputs

// define data variables
#define N_CHAN 8
byte values[N_CHAN];  //span 0-255
volatile int cur_chan = 0;
int chan_from_given_addr = 0;
int chan_from_serial_comms = N_CHAN;  //default to something big

void setup() {
  //start serial for debugging
  //Serial.begin(115200);
  HWSERIAL.begin(2*115200);

  // initialize SPI:
  pinMode (SPI_CS, OUTPUT);
  digitalWrite(SPI_CS, HIGH);
  SPI.begin();
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));

  // setup digital input pins and interupt on INH pin
  pinMode(TEST1_PIN, INPUT); digitalWrite(TEST1_PIN, HIGH); //sets the digital pin as input, and add pullup
  pinMode(TEST2_PIN, INPUT); digitalWrite(TEST2_PIN, HIGH); //sets the digital pin as input, and add pullup
  pinMode(TEST3_PIN, INPUT); digitalWrite(TEST3_PIN, HIGH); //sets the digital pin as input, and add pullup
  pinMode(TEST4_PIN, INPUT); digitalWrite(TEST4_PIN, HIGH); //sets the digital pin as input, and add pullup
  pinMode(TEST5_PIN, INPUT); digitalWrite(TEST5_PIN, HIGH); //sets the digital pin as input, and add pullup
  pinMode(TEST6_PIN, INPUT); digitalWrite(TEST6_PIN, HIGH); //sets the digital pin as input, and add pullup
  pinMode(TEST7_PIN, INPUT); digitalWrite(TEST7_PIN, HIGH); //sets the digital pin as input, and add pullup
  pinMode(TEST8_PIN, INPUT); digitalWrite(TEST8_PIN, HIGH); //sets the digital pin as input, and add pullup
  pinMode(A_PIN, INPUT); // sets the digital pin as input
  pinMode(B_PIN, INPUT); // sets the digital pin as input
  pinMode(C_PIN, INPUT); // sets the digital pin as input
  pinMode(INH_PIN, INPUT); // sets the digital pin as input
  attachInterrupt(INH_PIN, isrService, RISING); // interrrupt 1 is data ready
  delay(1000);

  // initialize values
  setAllChannels((byte)255);
  isrService();      //initialize the output by forcing a call to the isrService
}

#define HWSERIAL Serial1
int mode = 0;
void loop() {

  //testSpeed();  // use this to see how fast the pot servicing could possibly be
  //testSpeedISR();  // use this to see how fast the pot servicing could possibly be


  if (digitalRead(TEST1_PIN) == LOW) {
    //the user has pulled this pin LOW.  Go into test mode.
    setCurValuesToTestMode_oneChannel(1 - 1);
  } else if (digitalRead(TEST2_PIN) == LOW) {
    //the user has pulled this pin LOW.  Go into test mode.
    setCurValuesToTestMode_oneChannel(2 - 1);
  } else if (digitalRead(TEST3_PIN) == LOW) {
    //the user has pulled this pin LOW.  Go into test mode.
    setCurValuesToTestMode_oneChannel(3 - 1);
  } else if (digitalRead(TEST4_PIN) == LOW) {
    //the user has pulled this pin LOW.  Go into test mode.
    setCurValuesToTestMode_oneChannel(4 - 1);
  } else if (digitalRead(TEST5_PIN) == LOW) {
    //the user has pulled this pin LOW.  Go into test mode.
    setCurValuesToTestMode_oneChannel(5 - 1);
  } else if (digitalRead(TEST6_PIN) == LOW) {
    //the user has pulled this pin LOW.  Go into test mode.
    setCurValuesToTestMode_oneChannel(6 - 1);
  } else if (digitalRead(TEST7_PIN) == LOW) {
    //the user has pulled this pin LOW.  Go into test mode.
    setCurValuesToTestMode_descendingSteps();
  } else if (digitalRead(TEST8_PIN) == LOW) {
    //the user has pulled this pin LOW.  Go into test mode.
    setAllChannels((byte)255);
  } else {
    //we are not in test mode.  Normal behavior.

    //service Serial (both USB and hardware
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
}

void setCurValuesToTestMode_descendingSteps(void) {
  int n_active_voices = 4;
  for (int i = 0; i < N_CHAN; i++) {
    if (i < n_active_voices) {
      values[i] = (byte)((255 * (n_active_voices - 1 - i)) / (n_active_voices - 1));
    } else {
      values[i] = 0;
    }
  }
}

void setCurValuesToTestMode_oneChannel(int target_voice) {
  target_voice = constrain(target_voice, 0, N_CHAN - 1);
  for (int i = 0; i < N_CHAN; i++) {
    if (i == target_voice) {
      values[i] = (byte)255;
    } else {
      values[i] = 0;
    }
  }
}

void setAllChannels(byte new_val) {
  for (int i = 0; i < N_CHAN; i++) values[i] = new_val;
}


void interpretSerialByte(byte serialByte) {

  //original method...single byte
  //int chan = (serialByte & 0b00000111);  //low bits are channel
  //values[chan]  = (byte)(serialByte & 0b11111000);  //high bits are the value
  //Serial.print("Received: Chan ");
  //Serial.print(chan);
  //Serial.print(", Value ");
  //Serial.println(values[chan]);

  //high res method...two bytes
  //...first byte starts with 1 and has channel #
  //...second byte starts with 0 and has value (0-127)
  if (serialByte & 0b10000000) {
    //first bit is one...this is a channel byte
    chan_from_serial_comms = (serialByte & 0b00000111);
  } else {
    //first bit is zero...this is a data byte
    if (chan_from_serial_comms < N_CHAN) {
      values[chan_from_serial_comms] = serialByte << 1; //0-127 to 0-255
//      Serial.print("Received: Chan ");
//      Serial.print(chan_from_serial_comms);
//      Serial.print(", Value ");
//      Serial.println(values[chan_from_serial_comms]);
    }
  }

  
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
  delayMicroseconds(5);  //maybe this is necessary to let ABC settle.
  chan_from_given_addr = (digitalRead(A_PIN)) |
                         (digitalRead(B_PIN) << 1) |
                         (digitalRead(C_PIN) << 2);

  if (chan_from_given_addr >= N_CHAN) {
    chan_from_given_addr = N_CHAN - 1;
  }

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
    if (force_chan >= N_CHAN) force_chan = 0;
    cur_chan = force_chan;
    isrService();
  }
}


