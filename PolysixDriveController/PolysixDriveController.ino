/*
  PolysixDriveController

  This is a custom PCB with an AVR ATMEL 328P (ie, Arduino Uno) and an
  AD5262 2-channel digital potentiometer.  For more info on the PCB,
  see http://synthhacker.blogspot.com/2016/01/pcb-for-polysix-ota-overdrive.html

  The "input" to this board are the four digital bins (D2-D5) that listen
  to logic signals in the Korg Polysix.  This 4-bit logic signal defines
  the setting for the programmable attenuator.  I adjust the digipot on
  this PCB in response to the 4-bit value.
  
  The two-channel AD5262 digipot is SPI-compatible.  To command it, you send
  one bit specifying which of the two potentiometers to use followed by one
  byte specifyin the resistance value (0 - 255).  
  
  The Arduino-relevant part of the digipot control circuit:
  * CS - to Arduino digital pin 10  (SS pin)
  * SDI - to Arduino digital pin 11 (MOSI pin)
  * CLK - to Arduino digital pin 13 (SCK pin)

  This code also slows down the Arduino to 1MHz to save power.
  
  Created 02 JAN 2016
  by Chip Audette

*/


// include the SPI library:
#include <SPI.h>

//Clockspeed routines from: http://forum.arduino.cc/index.php?topic=223771.0
//define constants for changing the clock speed
#define CLOCK_PRESCALER_1   (0x0)  //  16MHz   (FYI: 15mA   on an Arduino Pro Mini @5V Vcc w. LEDs removed)
#define CLOCK_PRESCALER_2   (0x1)  //   8MHz   (FYI: 10mA   on an Arduino Pro Mini @5V Vcc w. LEDs removed)
#define CLOCK_PRESCALER_4   (0x2)  //   4MHz   (FYI:  8mA   on an Arduino Pro Mini @5V Vcc w. LEDs removed)
#define CLOCK_PRESCALER_8   (0x3)  //   2MHz   (FYI:  6.5mA on an Arduino Pro Mini @5V Vcc w. LEDs removed)
#define CLOCK_PRESCALER_16  (0x4)  //   1MHz   (FYI:  5.5mA on an Arduino Pro Mini @5V Vcc w. LEDs removed)
#define CLOCK_PRESCALER_32  (0x5)  // 500KHz   (FYI:  5mA   on an Arduino Pro Mini @5V Vcc w. LEDs removed)
#define CLOCK_PRESCALER_64  (0x6)  // 250KHz   (FYI:  4.8mA on an Arduino Pro Mini @5V Vcc w. LEDs removed)
#define CLOCK_PRESCALER_128 (0x7)  // 125KHz   (FYI:  4.8mA on an Arduino Pro Mini @5V Vcc w. LEDs removed)
#define CLOCK_PRESCALER_256 (0x8)  // 62.5KHz  (FYI:  4.6mA on an Arduino Pro Mini @5V Vcc w. LEDs removed)

// Choose which default Prescaler option that will be used in this sketch
#define    CLOCK_PRESCALE_DEFAULT   CLOCK_PRESCALER_16


// define program constants
const int slaveSelectPin = 10; //set pin 10 as the slave select for the digital pot:
#define N_OUT 16
//attenuator setting  -10,  -8,  -6,  -4,  -2,  0,  +2,  +4,  +6,  +8,  +10, xx, xx, xx, xx, xx
byte all_out_vals[] = {255, 125,  64,  32,  16,  0,   0,   0,   0,   0,    0,  0,  0,  0,  0,  0};

// begin code
void setup() {
  //change clockspeed
  setPrescale();   // reduce MPU power usage at run-time by slowing down the Main Clock source  
    
  //start serial for debugging
  //Serial.begin(115200);

  //set the digital input pins
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  
  // set the slaveSelectPin as an output:
  pinMode (slaveSelectPin, OUTPUT);
  pinMode (slaveSelectPin, HIGH);
  //pinMode(ledPin,OUTPUT);
  //digitalWrite(ledPin,LED_ON);
  
  // initialize SPI:
  SPI.begin(); 
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
}


void loop() {

  // read the 4-bit value for the Polysix's attenuator setting
  int input_val = (digitalRead(2) << 3) || 
                  (digitalRead(3) << 2) || 
                  (digitalRead(4) << 1) || 
                  (digitalRead(5) << 0);
  
  // choose the output value
  byte out_val = 0;
  if (input_val < N_OUT) out_val = all_out_vals[input_val];
  
  //send value to first channel via SPI
  digitalWrite(slaveSelectPin,LOW);// take the SS pin low to select the chip
  SPI.transfer(0b00000000);  //first channel
  SPI.transfer(out_val);//  send value via SPI:
  digitalWrite(slaveSelectPin,HIGH);// take the SS pin high to de-select the chip
  
  if (1) {
    //send value to first channel via SPI
    delayMicroseconds(20);  //I don't know how much delay is needed (if any).
    digitalWrite(slaveSelectPin,LOW);// take the SS pin low to select the chip
    SPI.transfer(0b00000001);  //second channel
    SPI.transfer(out_val);//  send value via SPI:
    digitalWrite(slaveSelectPin,HIGH);// take the SS pin high to de-select the chip
  }
  
  //wait a bit to slow things down (note that this wait time is multiplied by the pre-scaler!)
  delay(20);  //comment out this line to go as fast as possible
}


/***********************************************************************
****
****   at run-time - MPU speed modifications
****
***********************************************************************/

void setPrescale() {
  
  /* 
   * Setting the Prescale is a timed event, 
   * meaning that two MCU instructions must be executed  
   * within a few clockcycles. 
   */
  
  //To ensure timed events, first turn off interrupts
  cli();                   // Disable interrupts
  CLKPR = _BV(CLKPCE);     //  Enable change. Write the CLKPCE bit to one and all the other to zero. Within 4 clock cycles, set CLKPR again
  CLKPR = CLOCK_PRESCALE_DEFAULT; // Change clock division. Write the CLKPS0..3 bits while writing the CLKPE bit to zero
  sei();                   // Enable interrupts
  
  
  // To get the fastest (and still reliable) ADC (Analog to Digital Converter)
  // operations, when changing the prescale register,
  // you also need to set the ADC_Clk_prescale in the ADCSRA register
  
                                                     // Preferred: 50KHz < ADC_Clk < 200KHz 
#if  CLOCK_PRESCALE_DEFAULT == CLOCK_PRESCALER_1
#define ADC_SPEED 7                                  //ADC_Clk = F_CPU_Pre / 128 => 125KHz
#elif CLOCK_PRESCALE_DEFAULT == CLOCK_PRESCALER_2
#define ADC_SPEED 6                                  //ADC_Clk = F_CPU_Pre /  64 => 125KHz
#elif CLOCK_PRESCALE_DEFAULT == CLOCK_PRESCALER_4
#define ADC_SPEED 5                                  //ADC_Clk = F_CPU_Pre /  32 => 125KHz
#elif CLOCK_PRESCALE_DEFAULT == CLOCK_PRESCALER_8
#define ADC_SPEED 4                                  //ADC_Clk = F_CPU_Pre /  16 => 125KHz
#elif CLOCK_PRESCALE_DEFAULT == CLOCK_PRESCALER_16
#define ADC_SPEED 3                                  //ADC_Clk = F_CPU_Pre /   8 => 125KHz
#elif CLOCK_PRESCALE_DEFAULT == CLOCK_PRESCALER_32
#define ADC_SPEED 2                                  //ADC_Clk = F_CPU_Pre /   4 => 125KHz
#elif CLOCK_PRESCALE_DEFAULT == CLOCK_PRESCALER_64
#define ADC_SPEED 1                                  //ADC_Clk = F_CPU_Pre /   2 => 125KHz
#elif CLOCK_PRESCALE_DEFAULT == CLOCK_PRESCALER_128
#define ADC_SPEED 0                                  //ADC_Clk = F_CPU_Pre /   1 => 125KHz
#elif CLOCK_PRESCALE_DEFAULT == CLOCK_PRESCALER_256
#define ADC_SPEED 0                                  //ADC_Clk = F_CPU_Pre /   1 => 62.5KHz
#endif

  ADCSRA = ( 0x80 | ADC_SPEED);  // Activate ADC and set ADC_Clk 
}


