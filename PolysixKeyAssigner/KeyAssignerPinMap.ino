
//include "dataTypes.h"

#define DEBUG_THIS_FILE false

//Setup the PolySix data bus lines on PortA...but sure to edit setupDigitalPins() !!!
//Port A includes pins 22 (Bit 0) through 29 (Bit 7)
#define PS_DB_PORT PORTA

//Setup the PolySix Psense lines on PINB...but sure to edit setupDigitalPins() !!!
#define PS_PSENSE_PORT PINC
#define PS_PSENSE_OUTPORT PORTC

//Define the rest of the pins individually
#define PS_A_PIN  38
#define PS_B_PIN  39
#define PS_C_PIN  40
#define PS_LATCH1_PIN  41
#define PS_LATCH2_PIN  42
#define PS_INH_PIN  43
#define PS_MC_PIN  44
#define PS_ACKR_PIN  45
#define PS_4FT_PIN  46
//define PS_8FT_PIN  16
//define PS_RESET_PIN 17
#define PS_8FT_PIN  3    //moved Oct 2015 to free-up Serial2
#define PS_RESET_PIN 4   //moved Oct 2015 to free-up Serial2

#define SW_FROMTAPE_PIN 53

#define BUT_ARP_FULL_BIT 7
#define BUT_ARP_2OCT_BIT 6
#define BUT_ARP_1OCT_BIT 5
#define BUT_ARP_UP_BIT 4
#define BUT_ARP_DOWN_BIT 2  //the schem does show crossed wires
#define BUT_ARP_UPDOWN_BIT 3 //the schem does show crossed wires
#define BUT_LATCH_BIT 1
#define BUT_ARP_BIT 7
#define BUT_POLY_BIT 4
#define BUT_UNISON_BIT 2
#define BUT_CHORD_BIT 1
#define BUT_HOLD_BIT 0

#define LED_ARP_BIT 6
#define LED_POLY_BIT 4
#define LED_UNISON_BIT 2
#define LED_CHORD_BIT 1
#define LED_HOLD_BIT 0

#define PORTAMENTO_PEDAL_PIN 2
#define SUSTAIN_PEDAL_PIN 18

#ifndef _SETUP_PS_HARDWARE_SERVICES
#define _SETUP_PS_HARDWARE_SERVICES

#define SET_INHIBIT (digitalWrite(PS_INH_PIN,HIGH))  //active HIGH
#define CLEAR_INHIBIT (digitalWrite(PS_INH_PIN,LOW))  //default is LOW
#define SET_MC (digitalWrite(PS_MC_PIN,LOW)) //active is low...default is high
#define CLEAR_MC (digitalWrite(PS_MC_PIN,HIGH)) //high is the default state
#define WRITE_DB_BUS(x) (PS_DB_PORT = (byte)x)

#define C7_NOTE_NUM (84)  //If C0 is zero, then C7 should be 84

#define SET_LATCH1 (digitalWrite(PS_LATCH1_PIN,HIGH))
#define CLEAR_LATCH1 (digitalWrite(PS_LATCH1_PIN,LOW))
#define SET_LATCH2 (digitalWrite(PS_LATCH2_PIN,HIGH))
#define CLEAR_LATCH2 (digitalWrite(PS_LATCH2_PIN,LOW))
#define SET_DBBUS_HIGH (PS_DB_PORT = 0b11111111)

#define SET_A_HIGH  (digitalWrite(PS_A_PIN,HIGH));
#define SET_A_LOW  (digitalWrite(PS_A_PIN,LOW));
#define SET_B_HIGH (digitalWrite(PS_B_PIN,HIGH))
#define SET_B_LOW (digitalWrite(PS_B_PIN,LOW))

void setupDigitalPins(void) {
  Serial.println("setupDigitalPins: OK!");
  //setup the DB port as outputs
  DDRA = 0b11111111;  //set all of portA as outputs

  //setup the Psense port as inputs
  DDRC = 0b00000000;  //set all of portC as inputs
  //  for (int i=30;i<38;i++){
  //    pinMode(i,INPUT);
  //    digitalWrite(i,HIGH);
  //  }
  PORTC = 0b11111111;  //use the pullups

  //setup the individual pins
  pinMode(PS_A_PIN,OUTPUT);
  pinMode(PS_B_PIN,OUTPUT);
  pinMode(PS_C_PIN,OUTPUT);
  pinMode(PS_LATCH1_PIN,OUTPUT);
  pinMode(PS_LATCH2_PIN,OUTPUT);
  pinMode(PS_INH_PIN,OUTPUT);
  pinMode(PS_MC_PIN,OUTPUT);
  pinMode(PS_INH_PIN,OUTPUT);

  pinMode(PS_ACKR_PIN,OUTPUT); //or is it an input?

  pinMode(PS_4FT_PIN,INPUT);
  pinMode(PS_8FT_PIN,INPUT);
  pinMode(PS_RESET_PIN,INPUT);
  pinMode(PS_ACKI_PIN,INPUT);

  pinMode(PORTAMENTO_PEDAL_PIN,INPUT);
  digitalWrite(PORTAMENTO_PEDAL_PIN,HIGH);
  pinMode(SUSTAIN_PEDAL_PIN,INPUT);
  digitalWrite(SUSTAIN_PEDAL_PIN,HIGH);
  pinMode(SW_FROMTAPE_PIN,INPUT);
  digitalWrite(SW_FROMTAPE_PIN,HIGH);

}

//set the ABC bus to tell the rest of the synth what voice we're on.
void setABCbus(const int &voiceInd)
{
  int A=LOW,B=LOW,C=LOW;

  switch (voiceInd) {
  case 1:
    A=LOW;
    B=LOW;
    C=LOW;
    break;
  case 2:
    A=HIGH;
    B=LOW;
    C=LOW;
    break;
  case 3:
    A=LOW;
    B=HIGH;
    C=LOW;
    break;
  case 4:
    A=HIGH;
    B=HIGH;
    C=LOW;
    break;  
  case 5:
    A=LOW;
    B=LOW;
    C=HIGH;
    break; 
  case 6:
    A=HIGH;
    B=LOW;
    C=HIGH;
    break;  
  case 7:
    A=LOW;
    B=HIGH;
    C=HIGH;
    break;      
  case 8:
    A=HIGH;
    B=HIGH;
    C=HIGH;
    break; 
  default:
    A=HIGH;
    B=HIGH;
    C=HIGH;
    break; //for intervoice period...and assume true for anything else, too
  }
  digitalWrite(PS_A_PIN,A);
  digitalWrite(PS_B_PIN,B);
  digitalWrite(PS_C_PIN,C);
} 

//read the user buttons/switches that are attached to the Key Assigner
void readButtons(void)
{
  //first, read the switches with B set high and A LOW
  SET_B_HIGH;
  SET_A_LOW;
  delayMicroseconds(10);
  byte Psense_AlowBhigh = PS_PSENSE_PORT;//button press makes it LOW, so flip the bits to make it HIGH

  //next,read with B set low
  SET_B_LOW;
  SET_A_HIGH;
  delayMicroseconds(10);
  byte Psense_AhighBlow = PS_PSENSE_PORT; //button press makes it LOW, so flip the bits to make it HIGH

  //interpret the readings
  interpretBytesForSwitch(Psense_AlowBhigh,Psense_AhighBlow);

  //read and interpret the octave switch
  assignerState.octave = OCTAVE_16FT;
  if (digitalRead(PS_8FT_PIN) == LOW) assignerState.octave = OCTAVE_8FT;
  if (digitalRead(PS_4FT_PIN) == LOW) assignerState.octave = OCTAVE_4FT;

  //read and interpret foot switches
  assignerButtonState.sustainPedal.updateState(!digitalRead(SUSTAIN_PEDAL_PIN));
  updateFootswitchHoldState(assignerButtonState.sustainPedal.state,assignerButtonState.sustainPedal.didStateChange());
  assignerButtonState.portamentoPedal.updateState(!digitalRead(PORTAMENTO_PEDAL_PIN));
  assignerButtonState.fromTapeToggle.updateState(!digitalRead(SW_FROMTAPE_PIN));
  
}

//Interpret the bytes into actual flags to assign to our internal variables.
//This routine does not actually cause any of effects that the switches are 
//intended to cause.  That must be done in another routine.
#define BUTSTATE_FROM_BIT(x,y) (!(bitRead(x,y)))  //a LOW means that the button has been pressed
void interpretBytesForSwitch(const byte &Psense_AlowBhigh,const byte &Psense_AhighBlow)
{ 
  static boolean firstTime=true;

  //update switch states
  int val=0;
  if (bitRead(Psense_AhighBlow,BUT_ARP_FULL_BIT)==LOW) val = ARP_RANGE_FULL;
  if (bitRead(Psense_AhighBlow,BUT_ARP_2OCT_BIT)==LOW) val = ARP_RANGE_2OCT;
  if (bitRead(Psense_AhighBlow,BUT_ARP_1OCT_BIT)==LOW) val = ARP_RANGE_1OCT;
  assignerButtonState.arp_range.updateState(val);
  val = 0;
  if (bitRead(Psense_AhighBlow,BUT_ARP_UP_BIT)==LOW) val = ARP_DIR_UP;
  if (bitRead(Psense_AhighBlow,BUT_ARP_DOWN_BIT)==LOW) val = ARP_DIR_DOWN;
  if (bitRead(Psense_AhighBlow,BUT_ARP_UPDOWN_BIT)==LOW) val = ARP_DIR_UPDOWN;
  assignerButtonState.arp_dir.updateState(val);
  assignerButtonState.arp_latch.updateState(BUTSTATE_FROM_BIT(Psense_AhighBlow,BUT_LATCH_BIT));

  //update based on the button presses...a button is LOW when pressed
  assignerButtonState.arp_on.updateState(BUTSTATE_FROM_BIT(Psense_AlowBhigh,BUT_ARP_BIT));
  assignerButtonState.poly.updateState(BUTSTATE_FROM_BIT(Psense_AlowBhigh,BUT_POLY_BIT));
  assignerButtonState.unison.updateState(BUTSTATE_FROM_BIT(Psense_AlowBhigh,BUT_UNISON_BIT));
  assignerButtonState.chord_mem.updateState(BUTSTATE_FROM_BIT(Psense_AlowBhigh,BUT_CHORD_BIT));
  assignerButtonState.hold.updateState(BUTSTATE_FROM_BIT(Psense_AlowBhigh,BUT_HOLD_BIT));

  firstTime = false;
}


//set the Gate pins depending upon which voices should be sounding right now
void setGatePins()
{
  byte gate_out = 0b00000000; //set all to LOW
  for (int i=0;i<6;i++) {
    if (allVoiceData[i].noteGate==HIGH) { //should we trigger the gate?
      if ((allVoiceData[i].forceRetrigger == false) | (assignerState.legato == true)) { //retrigger==true would force the insertion of a low gate
        //no re-triggering LOW-pulse necessary, just set the gate HIGH!
        bitSet(gate_out,i);  // set this voice's gate as HIGH
        bitSet(gate_out,7);  // and set the highest bit if any voice is active
      } 
      else {
        //we're not allowed to trigger the gate right now.  Leave it LOW and let the forceTrigger flag get cleared
      }
    }
    allVoiceData[i].forceRetrigger = false;  //clear this flag
  }

  //push the value out onto the DB Bus
  WRITE_DB_BUS(gate_out); 
  //delayMicroseconds(5); //added for DEBUG
}

void set_ACKR(const int &ACKR) {
  if (ACKR==true) {
    digitalWrite(PS_ACKR_PIN,HIGH);
  } 
  else {
    digitalWrite(PS_ACKR_PIN,LOW);
  }
}

//set the LED pins depending upon which buttons are active right now
#define SHOW_ASSIGNER_STATE (1)
#define SHOW_ARPGATE_LENGTH (2)
void setLEDPins()
{
  static boolean firstTime=true;
  static int PWM_counter=0;
#define LED_PWM_PERIOD 3
  byte LED_out = 0b11111111;  //set all to HIGH, this makes the LEDs off
  int whatToShow = SHOW_ASSIGNER_STATE;

  //increment the PWM counter.  Reset if necessary
  PWM_counter++;
  if (PWM_counter > LED_PWM_PERIOD) PWM_counter = 1;  //reset the counter

  //set ARP LED based on keypanel mode
  if (assignerState.keypanel_mode == KEYPANEL_MODE_ARP) {
    if (assignerState.arp > OFF) bitClear(LED_out,LED_ARP_BIT); //clear the bit to turn on the LED
    if (assignerButtonState.arp_on.state == ON) whatToShow = SHOW_ARPGATE_LENGTH;
  } 
  else {
    if (assignerState.detune > OFF) bitClear(LED_out,LED_ARP_BIT); //clear the bit to turn on the LED
  }
  
  //set the other LEDs based what we want to show
  switch (whatToShow) {
    case SHOW_ASSIGNER_STATE:
      if (assignerState.poly > OFF) bitClear(LED_out,LED_POLY_BIT);//clear the bit to turn on the LED
      if (assignerState.unison > OFF) bitClear(LED_out,LED_UNISON_BIT);//clear the bit to turn on the LED
      if (assignerState.chord_mem > OFF) bitClear(LED_out,LED_CHORD_BIT);//clear the bit to turn on the LED
      if (assignerState.hold > OFF) bitClear(LED_out,LED_HOLD_BIT);//clear the bit to turn on the LED
  
      //adjust the LED brightness for the legato state
      if (assignerState.legato == ON) {
        //PWM the state LED so that it dims a bit
        if (PWM_counter < LED_PWM_PERIOD) {
          //briefly turn off the LED for each of the two possible states that might legato
          bitSet(LED_out,LED_UNISON_BIT);  //a HIGH bit is OFF
          bitSet(LED_out,LED_CHORD_BIT); //a HIGH bit is Off
          bitSet(LED_out,LED_POLY_BIT); //a HIGH bit is Off
        }
      }
      break;
    case SHOW_ARPGATE_LENGTH:
      int val = arpManager.getArpGate_x7bits();
      int LED_BIT = LED_POLY_BIT;
      if (val < 32) {
        LED_BIT = LED_HOLD_BIT;
      } else if (val < 64) {
         LED_BIT = LED_CHORD_BIT;
      } else if (val < 96) {
         LED_BIT = LED_UNISON_BIT;
      } else {
        LED_BIT = LED_POLY_BIT;
      }
      bitClear(LED_out,LED_BIT);  //turn on the light
      if (PWM_counter < LED_PWM_PERIOD) {
        bitSet(LED_out,LED_BIT); //turn it back off
      }
      break;
  }



  //push the value out onto the DB Bus
  WRITE_DB_BUS(~LED_out);  //added bitwise NOT 2013-03-03

  firstTime=false;
}


#endif


