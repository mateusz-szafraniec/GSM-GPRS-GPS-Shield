#include <SoftwareSerial.h>
#include "comGSM.h"

//To change pins for Software Serial, use the two lines in GSM.cpp.

//GSM Shield for Arduino
//www.open-electronics.org
//this code is based on the example of Arduino Labs.

//Simple sketch to communicate with SIM900 through AT commands.
#define SIM808

GSM gsm(2, 3);

#define BOOT_PIN 7

int numdata;
char inSerial[40];
int i = 0;


void setup()
{
  pinMode(BOOT_PIN, OUTPUT);  
#ifdef M590
  digitalWrite(BOOT_PIN, LOW);
#endif
#ifdef SIM808
  digitalWrite(BOOT_PIN, HIGH);
#endif
  
  
  //Serial connection.
  Serial.begin(115200);
  Serial.println("GSM Shield testing.");
  //Start configuration of shield with baudrate.
  //For http uses is recommended to use 4800 or slower.
  gsm.begin(57600);
  gsm.setBootPin(BOOT_PIN);
  gsm.initSIM808();
};

void loop()
{
  //Read for new byte on serial hardware,
  //and write them on NewSoftSerial.
  serialhwread();
  //Read for new byte on NewSoftSerial.
  serialswread();
};

void serialhwread()
{
  i = 0;
  if (Serial.available() > 0) {
    while (Serial.available() > 0) {
      inSerial[i] = (Serial.read());
      delay(10);
      i++;
    }

    inSerial[i] = '\0';
    if (!strcmp(inSerial, "/END")) {
      Serial.println("_");
      inSerial[0] = 0x1a;
      inSerial[1] = '\0';
      gsm.SimpleWriteln(inSerial);
    } else if (!strcmp(inSerial, "L")) {
      digitalWrite(BOOT_PIN, LOW);
      Serial.println("LOW");
    } else if (!strcmp(inSerial, "H")) {
      digitalWrite(BOOT_PIN, HIGH);
      Serial.println("HIGH");
    }  else if (!strcmp(inSerial, "!")) {
      gsm.toggleBoot(BOOT_PIN);
    } else if (!strcmp(inSerial, "!?")) {
      Serial.print("boot pin state:");
      Serial.println(digitalRead(BOOT_PIN));
    } else
      //Send a saved AT command using serial port.
      if (!strcmp(inSerial, "TEST")) {
        Serial.println("SIGNAL QUALITY");
        gsm.SimpleWriteln("AT+CSQ");
      } else if (!strcmp(inSerial, "INIT")) {
        Serial.println("INIT!!");
        gsm.modemInit(INIT_POWER_ON);
      } else
    {
      //Serial.println(inSerial);
      gsm.SimpleWriteln(inSerial);
    }
    inSerial[0] = '\0';
  }
}

void serialswread()
{
  gsm.SimpleRead();
}