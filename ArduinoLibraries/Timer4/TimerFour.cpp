/*
 *  Interrupt and PWM utilities for 16 bit Timer4 on ATmega640/1280/2560
 *  Original code by Jesse Tane for http://labs.ideo.com August 2008
 *  Modified March 2009 by Jérôme Despatis and Jesse Tane for ATmega328 support
 *  Modified June 2009 by Michael Polli and Jesse Tane to fix a bug in setPeriod() which caused the timer to stop
 *  Modified Oct 2009 by Dan Clemens to work with timer3 of the ATMega1280 or Arduino Mega
 *  Modified Aug 2011 by RobotFreak to work with timer4 of the ATMega640/1280/2560 or Arduino Mega/ADK
 *
 *  This is free software. You can redistribute it and/or modify it under
 *  the terms of Creative Commons Attribution 3.0 United States License. 
 *  To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/us/ 
 *  or send a letter to Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
 *
 */

#include "TimerFour.h"

TimerFour Timer4;              // preinstatiate

ISR(TIMER4_OVF_vect)          // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  Timer4.isrCallback();
}

void TimerFour::initialize(long microseconds)
{
  TCCR4A = 0;                 // clear control register A 
  TCCR4B = _BV(WGM43);        // set mode as phase and frequency correct pwm, stop the timer
  setPeriod(microseconds);
}

void TimerFour::setPeriod(long microseconds)
{
  long cycles = (F_CPU * microseconds) / 2000000;                                // the counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2
  if(cycles < RESOLUTION)              clockSelectBits = _BV(CS40);              // no prescale, full xtal
  else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS41);              // prescale by /8
  else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS41) | _BV(CS40);  // prescale by /64
  else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS42);              // prescale by /256
  else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS42) | _BV(CS40);  // prescale by /1024
  else        cycles = RESOLUTION - 1, clockSelectBits = _BV(CS42) | _BV(CS40);  // request was out of bounds, set as maximum
  ICR4 = pwmPeriod = cycles;                                                     // ICR1 is TOP in p & f correct pwm mode
  TCCR4B &= ~(_BV(CS40) | _BV(CS41) | _BV(CS42));
  TCCR4B |= clockSelectBits;                                                     // reset clock select register
}

void TimerFour::setPwmDuty(char pin, int duty)
{
  unsigned long dutyCycle = pwmPeriod;
  dutyCycle *= duty;
  dutyCycle >>= 10;
  if(pin == 6) OCR4A = dutyCycle;
  if(pin == 7) OCR4B = dutyCycle;
  if(pin == 8) OCR4C = dutyCycle;
}

void TimerFour::pwm(char pin, int duty, long microseconds)  // expects duty cycle to be 10 bit (1024)
{
  if(microseconds > 0) setPeriod(microseconds);
  
	// sets data direction register for pwm output pin
	// activates the output pin
  if(pin == 6) { DDRE |= _BV(PORTH3); TCCR4A |= _BV(COM4A1); }
  if(pin == 7) { DDRE |= _BV(PORTH4); TCCR4A |= _BV(COM4B1); }
  if(pin == 8) { DDRE |= _BV(PORTH5); TCCR4A |= _BV(COM4C1); }
  setPwmDuty(pin, duty);
  start();
}

void TimerFour::disablePwm(char pin)
{
  if(pin == 6) TCCR4A &= ~_BV(COM4A1);   // clear the bit that enables pwm on PE3
  if(pin == 7) TCCR4A &= ~_BV(COM4B1);   // clear the bit that enables pwm on PE4
  if(pin == 8) TCCR4A &= ~_BV(COM4C1);   // clear the bit that enables pwm on PE5
}

void TimerFour::attachInterrupt(void (*isr)(), long microseconds)
{
  if(microseconds > 0) setPeriod(microseconds);
  isrCallback = isr;                                       // register the user's callback with the real ISR
  TIMSK4 = _BV(TOIE4);                                     // sets the timer overflow interrupt enable bit
  sei();                                                   // ensures that interrupts are globally enabled
  start();
}

void TimerFour::detachInterrupt()
{
  TIMSK4 &= ~_BV(TOIE4);                                   // clears the timer overflow interrupt enable bit 
}

void TimerFour::start()
{
  TCCR4B |= clockSelectBits;
}

void TimerFour::stop()
{
  TCCR4B &= ~(_BV(CS40) | _BV(CS41) | _BV(CS42));          // clears all clock selects bits
}

void TimerFour::restart()
{
  TCNT4 = 0;
}
