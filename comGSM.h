#ifndef comGSM_H
#define comGSM_H

#include "m590.h"

//debugging
#define DEBUG_ON
//#define DEBUG_AT
//#define DEBUG_BUFFER

//#define HW_SERIAL

#include <SoftwareSerial.h>
#include <Arduino.h>
#include <inttypes.h>

#define ctrlz 26 //Ascii character for ctr+z. End of a SMS.
#define cr    13 //Ascii character for carriage return.
#define lf    10 //Ascii character for line feed.
#define ctrlz 26 //Ascii character for ctr+z. End of a SMS.
#define cr    13 //Ascii character for carriage return.
#define lf    10 //Ascii character for line feed.
#define GSM_LIB_VERSION 308 // library version X.YY (e.g. 1.00)


#define PHONESIZE 13
#define MAXMSGLEN 160
#define BOOTTIME 25

#define PWR_OFF_ON_DELAY 3000L

//Timeouts
#define AT_TO      500
#define CPBR_TO     1000L
#define CMGR_TO     300
#define CMGD_TO     15000L
#define CMGDA_TO    10000L
#define CSQ_TO      30000L
#define CSCLK_TO    2000L
#define CIMI_TO     2000L
#define CLIP_TO     15000L
#define CPBS_TO     3000L
#define CFUN_TO    15000L
#define COPS_READ_TO 45000L
#define COPS_WRITE_TO 120000L
#define ATA_TO    10000L
#define ATH_TO    20000L
#define CPOWD_TO    10000L
#define CMEE_TO     10000L
#define IPR_TO      10000L
#define CMGF_TO     10000L
#define CNMI_TO     10000L
#define CREG_TO     30000L
#define CPMS_TO     10000L
#define CPIN_TO     5000L
#define CMGS1_TO    2000L
#define CMGS2_TO    20000L
#define UDPREPLY_TO   5000L

#ifdef HW_SERIAL
 #include "HWSerial.h"
#endif

// if defined - debug print is enabled with possibility to print out
// debug texts to the terminal program
//#define DEBUG_PRINT

// if defined - SMSs are not send(are finished by the character 0x1b
// which causes that SMS are not send)
// by this way it is possible to develop program without paying for the SMSs
//#define DEBUG_SMS_ENABLED

//#define DTMF_OUTPUT_ENABLE  71 // connect DTMF Output Enable not used
#define DTMF_DATA_VALID     14 // connect DTMF Data Valid to pin 14
#define DTMF_DATA0          72 // connect DTMF Data0 to pin 72
#define DTMF_DATA1          73 // connect DTMF Data1 to pin 73
#define DTMF_DATA2          74 // connect DTMF Data2 to pin 74
#define DTMF_DATA3          75 // connect DTMF Data3 to pin 75

// length for the internal communication buffer
#define COMM_BUF_LEN        128

// some constants for the IsRxFinished() method
#define RX_NOT_STARTED      0
#define RX_ALREADY_STARTED  1

// DTMF signal is NOT valid
//#define DTMF_NOT_VALID      0x10


// status bits definition
#define STATUS_NONE                 0
#define STATUS_INITIALIZED          1
#define STATUS_REGISTERED           2
#define STATUS_USER_BUTTON_ENABLE   4

// GPRS status
#define CHECK_AND_OPEN    0
#define CLOSE_AND_REOPEN  1

// Common string used
#define str_ok 		"OK"			//string to reduce stack usage
#define str_at		"AT"			//string to reduce stack usage

// SMS type
// use by method IsSMSPresent()

enum sms_type_enum
{
    SMS_UNREAD,
    SMS_READ,
    SMS_ALL,

    SMS_LAST_ITEM
};

enum rx_state_enum
{
    RX_NOT_FINISHED = 0,      // not finished yet
    RX_FINISHED,              // finished, some character was received
    RX_FINISHED_STR_RECV,     // finished and expected string received
    RX_FINISHED_STR_NOT_RECV, // finished, but expected string not received
    RX_TMOUT_ERR,             // finished, no character received
    // initial communication tmout occurred
    RX_LAST_ITEM
};


enum at_resp_enum
{
    AT_RESP_ERR_NO_RESP = -3,   // nothing received
    AT_RESP_ERR_DIF_RESP = -2,   // response_string is different from the response
    AT_RESP_OK = -1,             // response_string was included in the response

    AT_RESP_LAST_ITEM
};

enum cpas_enum
{
    CPAS_READY = 11,
    CPAS_UNKNOWN,
    CPAS_RINGING,
    CPAS_CALL_IN_PROGRESS,
    CPAS_ASLEEP,

    CPAS_LAST_ITEM
};

enum creg_enum
{
    CREG_NOT_REGISTERED_NOT_SEARCHING = 21,
    CREG_REGISTERED_HOME,
    CREG_REFUSED,
    CREG_SEARCHING,
    CREG_UNKNOWN,
    CREG_REGISTERED_ROAMING,
    CREG_NO_SIGNAL,

    CREG_LAST_ITEM
};

enum cfun_enum
{
    CFUN_OFF = 31,
    CFUN_ON,
    CFUN_AIRPLANE,
    CFUN_INVALID,

    CFUN_LAST_ITEM
};

enum pin_enum
{
    PIN_READY = 41,
    PIN_INPUT_PIN,
    PIN_INPUT_PUK,
    PIN_INPUT_PIN2,
    PIN_INPUT_PUK2,
    PIN_UNKNOWN,
    
    PIN_LAST_ITEM
};

enum call_ret_val_enum
{
    CALL_NONE = 51,
    CALL_INCOM_VOICE,
    CALL_ACTIVE_VOICE,
    CALL_INCOM_VOICE_AUTH,
    CALL_INCOM_VOICE_NOT_AUTH,
    CALL_INCOM_DATA_AUTH,
    CALL_INCOM_DATA_NOT_AUTH,
    CALL_ACTIVE_DATA,
    CALL_OTHERS,
    CALL_NO_RESPONSE,
    CALL_BUSY,
    CALL_NO_CARRIER,

    CALL_LAST_ITEM
};


enum getsms_ret_val_enum
{
    GETSMS_NO_SMS   = -10,
    GETSMS_UNREAD_SMS = 71,
    GETSMS_READ_SMS,
    GETSMS_OTHER_SMS,
    GETSMS_NOT_AUTH_SMS,
    GETSMS_AUTH_SMS,

    GETSMS_LAST_ITEM
};

enum param_set_enum
{
    INIT_POWER_ON   = 0,
    INIT_MODEM_STATUS,
    INIT_PIN,
    INIT_RF_ON,
    INIT_REPORT_NETWORK_REGISTRATION,
    INIT_CHECK_NETWORK_REGISTRATION,
    INIT_CHECK_NETWORK_SELECTION,
    INIT_CHECK_SIGNAL_QUALITY,
    PARAM_SET_8,
    PARAM_SET_9,
    PARAM_SET_10,
    
    PARAM_SET_LAST_ITEM
};


class GSM
{
public:
    enum GSM_st_e { ERROR, IDLE, READY, ATTACHED, TCPSERVERWAIT, TCPCONNECTEDSERVER, TCPCONNECTEDCLIENT };
    byte comm_buf[COMM_BUF_LEN+1];  // communication buffer +1 for 0x00 termination
    byte comm_buf2[COMM_BUF_LEN+1];
    char command[32];               // command buffer
    char phone_num[20];
    char at_sms_command[50];
    byte *p_comm_buf;               // pointer to the communication buffer
    uint16_t comm_buf_len;              // num. of characters in the buffer
    byte DST;
    char *psuttz;


private:
    int _status;
    byte comm_line_status;

    // global status - bits are used for representation of states
    byte module_status;

    // variables connected with communication buffer
    byte rx_state;                  // internal state of rx state machine

    uint16_t start_reception_tmout; // max tmout for starting reception
    uint16_t interchar_tmout;       // previous time in msec.
    unsigned long prev_time;        // previous time in msec.
    
    byte _bootPin = 255;
    byte _vccPin = 255;
    byte _resetPin = 255;
    long _baudRate = 9600;

    

protected:
#ifdef HW_SERIAL
    HWSerial _cell;
#endif
#ifndef HW_SERIAL
    SoftwareSerial _cell;
#endif
    int isIP(const char* cadena);

public:
    inline void setStatus(GSM_st_e status)
    {
        _status = status;
    }
    inline void setBootPin(byte bootPin)
    {
        _bootPin = bootPin;
    }
    inline void setVccPin(byte vccPin)
    {
        _vccPin = vccPin;
    }
    inline void setResetPin(byte resetPin)
    {
        _resetPin = resetPin;
    }
    inline void begin(long baudRate)
    {
        _baudRate = baudRate;
        _cell.begin(_baudRate);
    }
    inline void begin()
    {
        _cell.begin(_baudRate);
    }
    GSM();
    GSM(long baudRate);
    GSM(byte GSM_TXPIN, byte GSM_RXPIN);
    inline int getStatus()
    {
        return _status;
    };

    void RxInit(uint16_t start_comm_tmout, uint16_t max_interchar_tmout);
    char IsRxFinished(void);
    char IsStringReceived(char const *compare_string);
    char WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout);
    char WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                  char const *expected_resp_string);
    char SendATCmdWaitResp(char const *AT_cmd_string,
                           uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                           char const *response_string,
                           byte no_of_attempts);
    char SendATCmdWaitResp(const __FlashStringHelper *AT_cmd_string,
                           uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                           char const *response_string,
                           byte no_of_attempts);
    void SimpleWrite(char *comm);
    void SimpleWrite(char const *comm);
    void SimpleWrite(int comm);
    void SimpleWrite(const __FlashStringHelper *pgmstr);
    void SimpleWriteln(char *comm);
    void SimpleWriteln(char const *comm);
    void SimpleWriteln(const __FlashStringHelper *pgmstr);
    void SimpleWriteln(int comm);
    void SimpleRead();
    void WhileSimpleRead();
    virtual int read(char* result, int resultlength);
    virtual uint8_t read();
    virtual int available();
    void clearBuffer();
    void printBuffer();
    bool readToBuffer();
    char* readLineWithString(char const *compare_string);
    char* readLineWithString_P(char const *compare_string);
    char ComparePhoneNumber(byte position, char *phone_number);
    char GetPhoneNumber(byte position, char *phone_number);

    char AT();
    char getModemStatus();
    char getModemFunctions();
    char setModemFunctions(byte mode);
    char isNetworkAvailable();
    char Echo(bool state);
    long guessBaudRate(void);

    char getLocalTimestamp(bool mode);
    char clip(bool mode);

    char selectSIMPhoneBook();

    char getIMEI(char *imei);
    char getIMSI(char *imsi);
    char getCCID(char *ccid);
    char checkNetworkRegistration();
    uint8_t signalQuality();
    char PIN();
    char PIN(char *PIN);

    char setSMSTextMode();
    char setTEcharacterGSM();
    char InitSMSMemory(void);
    uint8_t getNetworkSelection();
    char modemInit(byte group);
    void toggleBoot(byte boot);
    void powerOff();
    void GSM::initSIM808(); 
  

};

#endif
