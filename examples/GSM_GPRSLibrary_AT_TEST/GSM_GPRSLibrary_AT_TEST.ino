/*
 * NOTE:If you use the new SIM808 please modify Gps.h definition,
 *  //#define  GPS_GNSS    //If you use the chip is the new SIM808 of the open definition
 *   and uncomment  of lines from 68 to 73 and comment of lines from 78 to 88.
 *If If you use the  old SIM808 or SIM908 please modify Gps.h definition,
 *  //#define  GPS_OLD   //If you use the chip is theold SIM808 or SIM908 of the open definition
 *   and uncomment of lines from 78 to 88 and comment  of lines from 68 to 73
 */     

//To change pins for Software Serial, use the two lines in GSM.cpp.
///If you use Maga to remember,uncomment Frist lines in HWSerial.h "#define MEGA"

//GSM Shield for Arduino
//www.open-electronics.org
//this code is based on the example of Arduino Labs.

//Simple sketch to start a connection as client.


//Simple sketch to communicate with SIM900 through AT commands.
#define SIM808
#include <SoftwareSerial.h>
#include "comGSM.h"
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


#include "sms.h"
#include "call.h"
CallGSM call;
SMSGSM sms;

char lon[15];
char lat[15];
char alt[15];
char time[20];
char vel[15];
char msg1[5];
char msg2[5];
char tmp[32];
char res;

     char phone_num[20]; // array for the phone number string
     char sms_text[200]; // array for the SMS text string

int stat;
boolean started=false;

void loop()
{     
     serialhwread();
     serialswread();
};

void serialhwread()
{
     i=0;
     if (Serial.available() > 0) {
          while (Serial.available() > 0) {
               inSerial[i]=(Serial.read());
               delay(10);
               i++;
          }

          inSerial[i]='\0';
          if(!strcmp(inSerial,"/END")) {
               Serial.println("_");
               inSerial[0]=0x1a;
               inSerial[1]='\0';
               gsm.SimpleWriteln(inSerial);
          }
          //Send a saved AT command using serial port.
          if(!strcmp(inSerial,"TEST\r\n")) {
              Serial.println((int)gsm.isNetworkAvailable());
          }
          if(!strcmp(inSerial,"!B\r\n")) {
              Serial.println(F("Printing buffer:"));
              gsm.printBuffer();
          }
          //Read last message saved.
          if(!strcmp(inSerial,"MSG")) {
          } else {
               Serial.println(inSerial);
               gsm.SimpleWriteln(inSerial);
          }
          inSerial[0]='\0';
     }
}

void serialswread()
{
     if (gsm.readToBuffer()) {
       if (gsm.IsStringReceived_P(PSTR("RING"))) Serial.println(F("Ring ring"));
       if (call.isRinging()) {
        Serial.println(F("Dzyn dzyn"));
        call.CallStatusWithAuth(phone_num,1,10);
       }
       byte smsposition = sms.isSMSReceived();
       if (smsposition>0) 
       {
          Serial.print(F("SMS:"));
          Serial.println((int)smsposition);
          Serial.println((int)sms.GetAuthorizedSMS(smsposition, phone_num, 20, sms_text, 200,1,20));
          Serial.println(phone_num);
          Serial.println(sms_text);
          if (0==strncmp(sms_text,"AT",2))
          {
              Serial.println("AT!");
              gsm.SendATCmdWaitResp(sms_text,5000,500,str_ok,1);
              strcpy(sms_text,gsm.comm_buf);
              sms.SendSMS("+48xxxxxxx",sms_text);
          }
          if (sms.isATcommand(smsposition)) sms.execATcommand();
       }
        if (gsm.IsStringReceived_P(PSTR("+CLIP: "))) {
        Serial.println(F("Clip"));
        Serial.println(gsm.readLineWithString_P(PSTR("+CLIP: ")));
       }
       if (gsm.IsStringReceived_P(PSTR("+CREG: "))) Serial.println(F("Creg"));
       if (gsm.IsStringReceived_P(PSTR("*PSUTTZ: "))) Serial.println(F("time"));
       if (gsm.IsStringReceived_P(PSTR("+CTZV: "))) Serial.println(F("costam"));
       if (gsm.IsStringReceived_P(PSTR("+CMTI: "))) Serial.println(F("SMS"));
       if (gsm.IsStringReceived_P(PSTR("+COPS: "))) Serial.println(F("COPS"));
       if (gsm.IsStringReceived_P(PSTR("NO CARRIER"))) Serial.println(F("rozlaczono"));
       
       gsm.printBuffer();
       gsm.clearBuffer();
     }
}

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
