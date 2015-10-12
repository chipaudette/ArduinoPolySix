
//data type definitions and global macros
#include "dataTypes.h"

#define STAT1  10
#define STAT2  6

//void turnOnStatLight(int pin) {
//  digitalWrite(pin,LOW);
//}
//void turnOffStatLight(int pin) {
//  digitalWrite(pin,HIGH);
//}

byte byte1, byte2,byte3;
int noteNum, noteVel;
//const int noteTranspose = -27;
void serviceSerial(void) {
  
  //Are there any MIDI messages?
  if(Serial3.available() > 0)
  {
    //turnOnStatLight(STAT1);   //turn on the STAT1 light indicating that it's received some Serial comms
    
    //read the first byte
    byte1 = Serial3.read();
    
    if (byte1 == 0xF8) {
      //timing signal, skip
      //Serial.println("Timing Signal Received. 1.");
    } else {
      if (ECHOMIDI) Serial.write(byte1);
      
      if ((byte1 == 0x90) | (byte1 == 0x80) | (byte1 == 0xB0) | (byte1 == 0xD0)) {
        
        //wait and read 2nd byte in message
        byte2 = 0xF8;
        while (byte2 == 0xF8) { while (Serial3.available() == 0); byte2 = Serial3.read(); }
        noteNum = (int)byte2;
        if (ECHOMIDI) Serial.write(byte2);
        
        if (byte1 == 0xD0) {
          //aftertouch, no 3rd byte
          byte3 = 0;
        } else {
          //wait and read 3nd byte in message
          byte3 = 0xF8;
          while (byte3 == 0xF8) { while (Serial3.available() == 0); byte3 = Serial3.read(); }
          noteVel = (int)byte3;
          if (ECHOMIDI) Serial.write(byte3);
        }
      } 
      
      //shift notes to get the lowest note from the keybed to be note zero
      if((byte1==0x90) | (byte1==0x80)) noteNum += SHIFT_MIDI_NOTES;
      
      //check to see if it is a NOTE ON but with zero velocity
      if ((byte1==0x90) & (noteVel==0)) {
        if (DEBUG_TO_SERIAL) { Serial.print("Note On, but Zero Vel, so Note Off: noteNum = ");Serial.println(noteNum); }
        byte1=0x80; //make it a note off message
        noteVel = DEFAULT_ON_VEL;
      }
        
      //check message type
      switch (byte1) {
        case 0xF8:
          //timing message. ignore
          //if (DEBUG_TO_SERIAL) Serial.println("Timing Signal Received. 2.");
          break;
          
        case 0x90:
          //note on message
          if (DEBUG_TO_SERIAL) { 
            Serial.print("Note On Received: noteNum = ");
            Serial.print(noteNum); 
            Serial.print(", Vel = ");
            Serial.println(noteVel);
          }
          //turnOnStatLight(STAT2);//turn on STAT2 light indicating that a note is active
          trueKeybed.addKeyPress(noteNum,noteVel);
          break;
          
        case 0x80:
          //note off
          if (DEBUG_TO_SERIAL) Serial.println("Note Off Message Received");
          //turnOffStatLight(STAT2);//turn off light that had been indicating that a note was active
          trueKeybed.stopKeyPress(noteNum,noteVel);
          break;
        
        case 0xB0:
          //CC changes (mod wheel, footswitch, etc)
          if (DEBUG_TO_SERIAL) Serial.println("CC Message Received");
          interpretCCMessage(byte2,byte3);
          break;
          
        case 0xD0:
          //aftertouch
          aftertouch_val = (int)byte2;
          //Serial.print("AT: "); Serial.print(aftertouch_val,DEC);
          aftertouch_val = (int)aftertouch_lookup[aftertouch_val];  //do lookup table
          //Serial.print(", new: "); Serial.println(aftertouch_val,DEC);
          //Serial.println(aftertouch_val);
          
          //aftertouch_val = sqrt((int)byte2)*11; //sqrt(x)*sqrt(127)
          break;
          
        default: 
          //Serial.print(byte1,HEX);Serial.print(byte2,HEX);Serial.print(byte3,HEX);
          break;
      }
    }
  } else {
    //delay(1);
  }
}

#define N_CC_NUMBERS 128
void interpretCCMessage(const byte &controllerNumber, const byte &value_byte)
{
  static int value_int;
  static boolean firstTime=true;
  static boolean warningMessageIssued[N_CC_NUMBERS];
  
  if (firstTime) {
    for (int i=0;i<N_CC_NUMBERS;i++) {
      warningMessageIssued[i]=false;
    }
  }
  
  switch (controllerNumber) {
    case 0x01:
      // modulation wheel
      updateModWheel(value_byte);
      break;
    case 0x40:
      updateSustainPedal(value_byte);
      break;
    default:
      //this CC # is not yet handled
      if ((DEBUG_TO_SERIAL) & (!warningMessageIssued[(int)controllerNumber])) {
        Serial.print("serialMIDIProcessing: CC# 0x");
        Serial.print(controllerNumber,HEX);
        Serial.println(" not yet handled.");
        warningMessageIssued[(int)controllerNumber] = true;
      }
  }
  
  firstTime=false;
}
    
    
//interpret the Mod Wheel messages
void updateModWheel(const byte &value_byte) {
  //right now, let's alter the basic voice timing...as an experiment
  long newDuration_usec = 100; //minimum allowed voice duration...normal is 676usec
  
  //assume each increment in value_byte is worth 10 usec.  If value_byte spans 120, this will
  //yield a total range of zero to 1200 usec
  newDuration_usec += ((long)value_byte)*10L;
  
  if (value_byte > 120) {
    newDuration_usec = 250000;
  }
  
  //send he command to adjust the timing
  adjustVoiceTimerDuration(newDuration_usec);
}

void updateSustainPedal(const byte &value_byte) {
  if (value_byte > (byte)63) {
    //on
    updateHoldStateMIDI(ON);
  } else {
    //off
    updateHoldStateMIDI(OFF);
  }
}
  

