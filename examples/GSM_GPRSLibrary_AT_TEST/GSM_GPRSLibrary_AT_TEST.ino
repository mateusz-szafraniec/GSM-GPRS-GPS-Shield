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

#include <SoftwareSerial.h>
//#include "inetGSM.h"
#include "sms.h"
#include "call.h"
#define BAUD_RATE 57600


//InetGSM inet;
CallGSM call;
SMSGSM sms;
//GPSGSM gps;

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
char inSerial[32];
int i=0;
boolean started=false;
#define M590

GSM gsm(2, 3);

#define BOOT_PIN 7 

void setup()
{
     //Serial connection.
     Serial.begin(115200);
     Serial.println("GSM Shield testing.");
     #ifdef M590
  digitalWrite(BOOT_PIN, LOW);
#endif
#ifdef SIM808
  digitalWrite(BOOT_PIN, HIGH);
#endif
  pinMode(BOOT_PIN, OUTPUT);
     //Start configuration of shield with baudrate.
     //For http uses is raccomanded to use 4800 or slower.
     gsm.begin(BAUD_RATE);
     modemInit(PARAM_SET_0);
          //Serial.println("\nstatus=READY");
          //gsm.forceON();  //To ensure that SIM908 is not only in charge mode
          started=true;
     //} else Serial.println("\nstatus=IDLE");
     
     res=gsm.getIMEI(tmp);
     Serial.print(F("res:"));
     Serial.print((int)res);
     Serial.print(F(" IMEI:"));
     Serial.println(tmp);
     
     res=gsm.getIMSI(tmp);
     Serial.print(F("res:"));
     Serial.print((int)res);
     Serial.print(F(" IMSI:"));
     Serial.println(tmp);
     
     res=gsm.getCCID(tmp);
     Serial.print(F("res:"));
     Serial.print((int)res);
     Serial.print(F(" CCID:"));
     Serial.println(tmp);
     
     Serial.print(F("ModemStatus:"));
     Serial.println((int)gsm.getModemStatus());

     Serial.print(F("checkNetworkRegistration:"));
     Serial.println((int)gsm.checkNetworkRegistration());
    
     Serial.print(F("ModemFunctions:"));
     Serial.println((int)gsm.getModemFunctions());
     Serial.println(freeRam());
/*
     Serial.println("delay...");
     delay(5000);
     Serial.print(F("Reset:"));
     Serial.println((int)gsm.Reset());
     Serial.println("delay...");
     delay(10000);
     
     Serial.print(F("ModemStatus:"));
     
     Serial.println((int)gsm.getModemStatus());
     
     Serial.print("set ModemFunctions CFUN_AIRPLANE:");
     Serial.println((int)gsm.setModemFunctions(CFUN_AIRPLANE));
     Serial.println("delay...");
     delay(10000);
     
     Serial.print("ModemFunctions:");
     Serial.println((int)gsm.getModemFunctions());
     
     Serial.print("set ModemFunctions CFUN_ON:");
     Serial.println((int)gsm.setModemFunctions(CFUN_ON));
     Serial.println("delay...");
     delay(10000);
      
     Serial.print("checkNetworkRegistration:");
     Serial.println((int)gsm.checkNetworkRegistration());
*/
     Serial.print("signalQuality():");
     Serial.println((int)gsm.signalQuality());

/*
     gsm.printBuffer();
     Serial.println("Koniec");
     */
     //Serial.println((int)sms.SendSMS(1,"BlahBlah!"));
     Serial.println((int)sms.IsSMSPresent(SMS_ALL));
     char position = sms.IsSMSPresent(SMS_ALL);
     Serial.println((int)sms.GetSMS(1, phone_num, 20, sms_text, 200));
     Serial.println(phone_num);
     Serial.println(sms_text);
          
     Serial.println((int)sms.GetAuthorizedSMS(1, phone_num, 20, sms_text, 200,1,20));
     Serial.println(phone_num);
     Serial.println(sms_text);

     Serial.println((int)sms.GetAuthorizedSMS(3, phone_num, 20, sms_text, 200,1,20));
     Serial.println(phone_num);
     Serial.println(sms_text);

     Serial.println((int)sms.GetAuthorizedSMS(4, phone_num, 20, sms_text, 200,1,20));
     Serial.println(phone_num);
     Serial.println(sms_text);

     Serial.println((int)sms.DeleteSMS(3));
     
};

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
       if (smsposition>0) {
          Serial.print(F("SMS:"));
          Serial.println((int)smsposition);
          Serial.println((int)sms.GetAuthorizedSMS(smsposition, phone_num, 20, sms_text, 200,1,20));
          Serial.println(phone_num);
          Serial.println(sms_text);
          /*
          if (0==strncmp(sms_text,"AT",2)){
              Serial.println("AT!");
              gsm.SendATCmdWaitResp(sms_text,5000,500,str_ok,1);
              strcpy(sms_text,gsm.comm_buf);
              sms.SendSMS("+48xxxxxxx",sms_text);
            }*/
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

