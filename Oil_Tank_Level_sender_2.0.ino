/******************************************************
 * Oil Tank Level Sensor
 * JeeNode version Septamber 24, 2016
 ******************************************************
 */


/// @dir radioBlip2
/// Send out a radio packet every minute, consuming as little power as possible.
// 2012-05-09 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <JeeLib.h>
#include <avr/sleep.h>
#include "DHT.h"

#define BLIP_NODE 6     // wireless node ID to use for sending blips
#define BLIP_GRP  212   // wireless net group to use for sending blips
#define SEND_MODE 2     // set to 3 if fuses are e=06/h=DE/l=CE, else set to 2

struct {
  byte oil_lvl;  // oil tank level
} payload;

volatile bool adcDone;

// for low-noise/-power ADC readouts, we'll use ADC completion interrupts
ISR(ADC_vect) { adcDone = true; }

// this must be defined since we're using the watchdog for low-power waiting
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

static byte vccRead (byte count =4) {
  set_sleep_mode(SLEEP_MODE_ADC);
  ADMUX = bit(REFS0) | 14; // use VCC as AREF and internal bandgap as input
  bitSet(ADCSRA, ADIE);
  while (count-- > 0) {
    adcDone = false;
    while (!adcDone)
      sleep_mode();
  }
  bitClear(ADCSRA, ADIE);  
  // convert ADC readings to fit in one byte, i.e. 20 mV steps:
  //  1.0V = 0, 1.8V = 40, 3.3V = 115, 5.0V = 200, 6.0V = 250
  return (55U * 1024U) / (ADC + 1) - 50;
}

void setup() {
  Serial.begin(57600); 
  Serial.println("test!");

  pinMode(3, INPUT);  // Reed Switch 0
  pinMode(4, INPUT);  // Reed Switch 1
  pinMode(5, INPUT);  // Reed Switch 2
  pinMode(6, INPUT);  // Reed Switch 3
  pinMode(7, INPUT);  // Reed Switch 4
  pinMode(8, INPUT);  // Reed Switch 5
  pinMode(9, INPUT);  // Reed Switch 6
  pinMode(A0, INPUT);  // Reed Switch 7
  pinMode(A1, OUTPUT);  // Warning LED

  cli();
  CLKPR = bit(CLKPCE);
#if defined(__AVR_ATtiny84__)
  CLKPR = 0; // div 1, i.e. speed up to 8 MHz
#else
  //CLKPR = 1; // div 2, i.e. slow down to 8 MHz
#endif
  sei();
  rf12_initialize(BLIP_NODE, RF12_433MHZ, BLIP_GRP);
  // see http://tools.jeelabs.org/rfm12b
  rf12_control(0xC040); // set low-battery level to 2.2V i.s.o. 3.1V
  rf12_sleep(RF12_SLEEP);
}

static byte sendPayload () {

  rf12_sleep(RF12_WAKEUP);
  while (!rf12_canSend())
    rf12_recvDone();
  rf12_sendStart(0, &payload, sizeof payload);
  rf12_sendWait(SEND_MODE);
  rf12_sleep(RF12_SLEEP);  
}

int sensorpin=0;       // What sensor pin we are checking


void loop() {
  // OIL LEVEL SECTION ******************************

  // Poll each reed switch from the top;
  // if no switch is HIGH, then use the previous setting.

  digitalWrite(A1, LOW);    // reset warning LED

  boolean rs7 = digitalRead(A0);
  boolean rs6 = digitalRead(9);
  boolean rs5 = digitalRead(8);
  boolean rs4 = digitalRead(7);
  boolean rs3 = digitalRead(6);
  boolean rs2 = digitalRead(5);
  boolean rs1 = digitalRead(4);
  boolean rs0 = digitalRead(3);
  
  if (rs7 == HIGH) { 
    sensorpin = 7;
  } else if (rs6 == HIGH) {
    sensorpin = 6;
  } else if (rs5 == HIGH) {
    sensorpin = 5;
  } else if (rs4 == HIGH) {
    sensorpin = 4;
  } else if (rs3 == HIGH) {
    sensorpin = 3;
  } else if (rs2 == HIGH) {
    sensorpin = 2;
  } else if (rs1 == HIGH) {
    sensorpin = 1;
  } else if (rs0 == HIGH) {
    sensorpin = 0;
    digitalWrite(A1, HIGH);    // turn on warning LED 
    }
 
     
  payload.oil_lvl = sensorpin;
  
  sendPayload();
  Sleepy::loseSomeTime(60000); //wake up and report in every 2 minutes
  
}
