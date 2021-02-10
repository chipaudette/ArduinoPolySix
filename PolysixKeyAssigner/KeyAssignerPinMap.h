
//include "dataTypes.h"

#ifndef _KeyAssignerPinMap_h
#define _KeyAssignerPinMap_h

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
#define SW_TAPEENABLE_PIN 52  //added Jan 31, 2021 for switching to tuning mode
#define PORTAMENTO_PEDAL_PIN 2
#define SUSTAIN_PEDAL_PIN 18
#define PS_ACKI_PIN  (19)      //this is attached to an interrupt #4 (see "PS_ACKI_INT" in PolysixKeyAssigner.ino)  http://arduino.cc/en/Reference/AttachInterrupt 
//define LFO_OUT_PIN (7) //pins 6, 7, 8 are controlled by Timer4 on the Mega 2560
#define RIBBON_PIN A7
#define RIBBON_REF_PIN A6  //set this to input_pullup.  It's connected to AREF and its connected to gnd via 20K

//Define some constants
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


#endif
