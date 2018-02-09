/*
This is a Beta version.
last modified 18/08/2012.

This library is based on one developed by Arduino Labs
and it is modified to preserve the compability
with the Arduino's product.

The library is modified to use the GSM Shield,
developed by www.open-electronics.org
(http://www.open-electronics.org/arduino-gsm-shield/)
and based on SIM900 chip,
with the same commands of Arduino Shield,
based on QuectelM10 chip.
*/

#include "comGSM.h"

#ifndef HW_SERIAL
GSM::GSM(byte GSM_TXPIN, byte GSM_RXPIN):_cell(GSM_TXPIN,GSM_RXPIN),_status(IDLE){};
#endif

#ifdef HW_SERIAL
GSM::GSM(long baud_rate),_status(IDLE){
 this->begin(baud_rate);
};
#endif


/**********************************************************
Method waits for response

return:
      RX_TMOUT_ERR = 4,                // timeout, no response received
      RX_FINISHED_STR_NOT_RECV = 3,    // expected string not received
      RX_FINISHED_STR_RECV = 2,        // expected string received
**********************************************************/
byte GSM::WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                   char const *expected_resp_string)
{
    byte status;
    byte ret_val;

    RxInit(start_comm_tmout, max_interchar_tmout);
    // wait until response is not finished
    do
    {
        status = IsRxFinished();
    }
    while (status == RX_NOT_FINISHED);

    if (status == RX_FINISHED)
    {
        // something was received but what was received?
        // ---------------------------------------------

        if(IsStringReceived(expected_resp_string))
        {
            // expected string was received
            // ----------------------------
            ret_val = RX_FINISHED_STR_RECV;
        }
        else
        {
            ret_val = RX_FINISHED_STR_NOT_RECV;
        }
    }
    else
    {
        // nothing was received
        // --------------------
        ret_val = RX_TMOUT_ERR;
    }
    return (ret_val);
}


/**********************************************************
Method sends AT command and waits for response

return:
      AT_RESP_ERR_NO_RESP = -3,    // no response received
      AT_RESP_ERR_DIF_RESP = -2,   // response_string is different from the response
      AT_RESP_OK = -1,             // response_string was included in the response
**********************************************************/
char GSM::SendATCmdWaitResp(char const *AT_cmd_string,
                            uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                            char const *response_string,
                            byte no_of_attempts)
{
    byte status;
    char ret_val = AT_RESP_ERR_NO_RESP;
    byte i;

    for (i = 0; i < no_of_attempts; i++)
    {
        // delay 500 msec. before sending next repeated AT command
        // so if we have no_of_attempts=1 tmout will not occurred
        if (i > 0) delay(500);
#ifdef DEBUG_AT
        Serial.print(F("AT COMMAND:"));
        Serial.println(AT_cmd_string);
#endif
        _cell.println(AT_cmd_string);
        status = WaitResp(start_comm_tmout, max_interchar_tmout);
        if (status == RX_FINISHED)
        {
            // something was received but what was received?
            // ---------------------------------------------
            if(IsStringReceived(response_string))
            {
                ret_val = AT_RESP_OK;
#ifdef DEBUG_AT
                Serial.println(F("RESULT:OK"));
#endif
                break;  // response is OK => finish
            }
            else
            {
                ret_val = AT_RESP_ERR_DIF_RESP;
#ifdef DEBUG_AT
                Serial.println(F("RESULT:DIFF"));
#endif
            }

        }
        else
        {
            // nothing was received
            // --------------------
#ifdef DEBUG_AT
            Serial.println(F("RESULT:timeout"));
#endif
            ret_val = AT_RESP_ERR_NO_RESP;
        }

    }
    return (ret_val);
}




/**********************************************************
Method sends AT command (from FlashStringHelper) waits for response

return:
      AT_RESP_ERR_NO_RESP = -3,   // no response received
      AT_RESP_ERR_DIF_RESP = -2,   // response_string is different from the response
      AT_RESP_OK = -1,             // response_string was included in the response
**********************************************************/
char GSM::SendATCmdWaitResp(const __FlashStringHelper *AT_cmd_string,
                            uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                            char const *response_string,
                            byte no_of_attempts)
{
    byte status;
    char ret_val = AT_RESP_ERR_NO_RESP;
    byte i;

    for (i = 0; i < no_of_attempts; i++)
    {
        // delay 500 msec. before sending next repeated AT command
        // so if we have no_of_attempts=1 tmout will not occurred
        if (i > 0) delay(500);
#ifdef DEBUG_AT
        Serial.print(F("AT COMMAND:"));
        Serial.println(AT_cmd_string);
#endif
        _cell.println(AT_cmd_string);
        status = WaitResp(start_comm_tmout, max_interchar_tmout);
        if (status == RX_FINISHED)
        {
            // something was received but what was received?
            // ---------------------------------------------
            if(IsStringReceived(response_string))
            {
                ret_val = AT_RESP_OK;
#ifdef DEBUG_AT
                Serial.println(F("RESULT:OK"));
#endif
                break;  // response is OK => finish
            }
            else
            {
                ret_val = AT_RESP_ERR_DIF_RESP;
#ifdef DEBUG_AT
                Serial.println(F("RESULT:DIFF"));
#endif // DEBUG_AT
            }
        }
        else
        {
            // nothing was received
            // --------------------
#ifdef DEBUG_AT
            Serial.println(F("RESULT:timeout"));
#endif
            ret_val = AT_RESP_ERR_NO_RESP;
        }

    }
    return (ret_val);
}




/**********************************************************
Method waits for response

return:
    RX_FINISHED = 1               // finished, some character was received
    RX_TMOUT_ERR = 4              // finished, no character received
**********************************************************/
byte GSM::WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
    byte status;

    RxInit(start_comm_tmout, max_interchar_tmout);
    // wait until response is not finished
    do
    {
        status = IsRxFinished();
    }
    while (status == RX_NOT_FINISHED);
    return (status);
}


/**********************************************************
Method checks is Rx transfer is finished

return:
    RX_NOT_FINISHED = 0,          // not finished yet
    RX_FINISHED = 1               // finished, some character was received
    RX_TMOUT_ERR = 4              // finished, no character received
**********************************************************/
byte GSM::IsRxFinished(void)
{
    uint16_t num_of_bytes;
    byte ret_val = RX_NOT_FINISHED;  // default not finished

    // Rx state machine
    // ----------------

    if (rx_state == RX_NOT_STARTED)
    {
        // Reception is not started yet - check tmout
        if (!_cell.available())
        {
            // still no character received => check timeout
            if ((unsigned long)(millis() - prev_time) >= start_reception_tmout)
            {
                // timeout elapsed => GSM module didn't start with response
                // so communication is takes as finished
                comm_buf[comm_buf_len] = 0x00;
                ret_val = RX_TMOUT_ERR;
            }
        }
        else
        {
            // at least one character received => so init inter-character
            // counting process again and go to the next state
            prev_time = millis(); // init tmout for inter-character space
            rx_state = RX_ALREADY_STARTED;
        }
    }

    if (rx_state == RX_ALREADY_STARTED)
    {
        // Reception already started
        // check new received bytes
        // only in case we have place in the buffer
        num_of_bytes = _cell.available();
        // if there are some received bytes postpone the timeout
        if (num_of_bytes) prev_time = millis();

        // read all received bytes
        while (num_of_bytes)
        {
            num_of_bytes--;
            if (comm_buf_len < COMM_BUF_LEN)
            {
                // we have still place in the GSM internal comm. buffer =>
                // move available bytes from circular buffer
                // to the rx buffer
                *p_comm_buf = _cell.read();

                p_comm_buf++;
                comm_buf_len++;
                comm_buf[comm_buf_len] = 0x00;  // and finish currently received characters
                // so after each character we have
                // valid string finished by the 0x00
            }
            else
            {
                // comm buffer is full, other incoming characters
                // will be discarded
                // but despite of we have no place for other characters
                // we still must to wait until
                // inter-character tmout is reached

                // so just readout character from circular RS232 buffer
                // to find out when communication id finished(no more characters
                // are received in inter-char timeout)
                _cell.read();
            }
        }

        // finally check the inter-character timeout
        if ((unsigned long)(millis() - prev_time) >= interchar_tmout)
        {
            // timeout between received character was reached
            // reception is finished
            // ---------------------------------------------
            comm_buf[comm_buf_len] = 0x00;  // for sure finish string again
            // but it is not necessary
            ret_val = RX_FINISHED;
        }
    }


    return (ret_val);
}


/**********************************************************
Method checks BUFFER (received bytes) for string

compare_string - pointer to the string which should be find

return: 0 - string was NOT received
        1 - string was received
**********************************************************/
byte GSM::IsStringReceived(char const *compare_string)
{
    char *ch;
    byte ret_val = 0;

    if(comm_buf_len)
    {

#ifdef DEBUG_BUFFER
        Serial.print(F("Searched: "));
        Serial.println(compare_string);
        Serial.print(F("Buffer: "));
        Serial.println((char *)comm_buf);
#endif
        ch = strstr(comm_buf, compare_string);
        if (ch != NULL)
        {
            ret_val = 1;
        }
        else
        {

        }
    }
    else
    {
#ifdef DEBUG_BUFFER
        Serial.print(F("Searched: "));
        Serial.println(compare_string);
        Serial.println(F("Buffer: NO STRING RCVD"));
#endif
    }

    return (ret_val);
}

//OK
void GSM::RxInit(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
    rx_state = RX_NOT_STARTED;
    start_reception_tmout = start_comm_tmout;
    interchar_tmout = max_interchar_tmout;
    prev_time = millis();
    comm_buf[0] = 0x00; // end of string
    p_comm_buf = &comm_buf[0];
    comm_buf_len = 0;
    _cell.flush(); // erase rx circular buffer
}

char GSM::Echo(bool state)
{
    char retval=AT_RESP_ERR_NO_RESP;
    if (state==true)
    {
        retval=SendATCmdWaitResp(F("ATE1"), AT_TO, 100, str_ok, 5);
    }
    else
    {
        retval=SendATCmdWaitResp(F("ATE0"), AT_TO, 100, str_ok, 5);
    }
    return retval;
}

char GSM::InitSMSMemory(void) //TODO do przepisania - return values
{
    char ret_val = AT_RESP_ERR_NO_RESP;
    ret_val = 0; // not initialized yet
    //Disable messages about new SMS from the GSM module
    //SendATCmdWaitResp(F("AT+CNMI=2,0"), CNMI_TO, 50, str_ok, 2);

    //Message about new SMS from the GSM module
    SendATCmdWaitResp(F("AT+CNMI=2,1"), CNMI_TO, 50, str_ok, 5);

    // send AT command to init memory for SMS in the SIM card
    // response:
    // +CPMS: <usedr>,<totalr>,<usedw>,<totalw>,<useds>,<totals>
    if (AT_RESP_OK == SendATCmdWaitResp(F("AT+CPMS=\"SM\",\"SM\",\"SM\""), CNMI_TO, 1000, "+CPMS:", 10))
    {
        ret_val = 1;
    }
    else ret_val = 0;
    return (ret_val);
}

//TODO Do sprawdzenia kiedys
int GSM::isIP(const char* cadena)
{
    int i;
    for (i=0; i<strlen(cadena); i++)
        if (!(cadena[i]=='.' || ( cadena[i]>=48 && cadena[i] <=57)))
            return 0;
    return 1;
}

//OK
long GSM::guessBaudRate(void)
{
    boolean turnedON=false;
    long baud_rate=AT_RESP_ERR_NO_RESP;
#ifdef DEBUG_ON
    Serial.println(F("DB:AUTO BAUD RATE"));
#endif
    for (int i=0; i<8; i++)
    {
        switch (i)
        {
        case 0:
            baud_rate=1200;
            _cell.begin(baud_rate);
            break;

        case 1:
            baud_rate=2400;
            _cell.begin(baud_rate);
            break;

        case 2:
            baud_rate=4800;
            _cell.begin(baud_rate);
            break;

        case 3:
            baud_rate=9600;
            _cell.begin(baud_rate);
            break;

        case 4:
            baud_rate=19200;
            _cell.begin(baud_rate);
            break;

        case 5:
            baud_rate=38400;
            _cell.begin(baud_rate);
            break;

        case 6:
            baud_rate=57600;
            _cell.begin(baud_rate);
            break;

        case 7:
            baud_rate=115200;
            _cell.begin(baud_rate);
            break;

            // if nothing else matches, do the default
            // default is optional
        }

        delay(100);

        if (AT_RESP_OK == SendATCmdWaitResp(str_at, AT_TO, 100, str_ok, 5))
        {
#ifdef DEBUG_ON
            Serial.print(F("DB:FOUND BR:"));
            Serial.println((long)baud_rate);
#endif
            //_cell.print("AT+IPR=");
            //_cell.print(baud_rate);
            //_cell.print("\r"); // send <CR>
            //delay(500);
            _cell.begin(baud_rate);
            delay(100);
            if (AT_RESP_OK == SendATCmdWaitResp(str_at, AT_TO, 100, str_ok, 5))
            {
#ifdef DEBUG_ON
                Serial.println(F("DB:OK BR"));

#endif
                turnedON=true;
            }

            break;
        }
    }
    // pointer is initialized to the first item of comm. buffer
    p_comm_buf = &comm_buf[0];
    if (turnedON) return baud_rate;
    return (AT_RESP_ERR_NO_RESP);
}

char GSM::getIMEI(char *imei)
{
    char resp = AT_RESP_ERR_NO_RESP;
    resp = SendATCmdWaitResp(F("AT+GSN"), AT_TO, 100, str_ok, 5);
    if (resp!=AT_RESP_OK) return resp;
    memcpy(imei,comm_buf+2,15);
    imei[15]='\0';
    return AT_RESP_OK;
}

char GSM::getIMSI(char *imsi)
{
    char resp = AT_RESP_ERR_NO_RESP;
    resp = SendATCmdWaitResp(F("AT+CIMI"), CIMI_TO, 100, str_ok, 5);
    if (resp!=AT_RESP_OK) return resp;
    memcpy(imsi,comm_buf+2,15);
    imsi[15]='\0';
    return AT_RESP_OK;
}

char GSM::getCCID(char *ccid)
{
    char resp = AT_RESP_ERR_NO_RESP;
    resp = SendATCmdWaitResp(F("AT+CCID"), CIMI_TO, 100, str_ok, 5);
    if (resp!=AT_RESP_OK) return resp;
    memcpy(ccid,comm_buf+2,20);
    ccid[20]='\0';
    return AT_RESP_OK;
}

char GSM::checkNetworkRegistration()
/*
 RETURN:
  -3,-2     Error sending AT command
  0:not registered, the terminal is not searching new operators
  1:has been registered local network
  2:the registration is refused
  3:not registered, the terminal is searching base stations
  4:unknown code
  5:has been registered, at roaming status
  */
{
    char result = AT_RESP_ERR_NO_RESP;
    result = SendATCmdWaitResp(F("AT+CREG?"), CREG_TO, 100, str_ok, 5);
    if (result!=AT_RESP_OK) return result;
    if (strstr_P(comm_buf, PSTR(",0")) != NULL) return CREG_NOT_REGISTERED_NOT_SEARCHING;
    if (strstr_P(comm_buf, PSTR(",1")) != NULL) return CREG_REGISTERED_HOME;
    if (strstr_P(comm_buf, PSTR(",2")) != NULL) return CREG_REFUSED;
    if (strstr_P(comm_buf, PSTR(",3")) != NULL) return CREG_SEARCHING;
    if (strstr_P(comm_buf, PSTR(",4")) != NULL) return CREG_UNKNOWN;
    if (strstr_P(comm_buf, PSTR(",5")) != NULL) return CREG_REGISTERED_ROAMING;
    return CREG_UNKNOWN;
}

/*************************************************************
  Procedure to check the GSM signal quality. The higher the
  number the better the signal quality.
  RETURN:
    -3,-2     Error sending AT command
    0         No signal
    1 - 99    RSSI signal strength

**************************************************************/
uint8_t GSM::signalQuality()
{   
    char result = AT_RESP_ERR_NO_RESP;
    result = SendATCmdWaitResp(F("AT+CSQ"), CSQ_TO, 100, str_ok, 5);
    if (result != AT_RESP_OK) return result;
    char *str = NULL;
    char *ptr = NULL;
    ptr = strtok_r(comm_buf, " ", &str);
    ptr = strtok_r(NULL, ",", &str);
    return (atoi(ptr));
}

//TODO TIMEOUT oraz zwracane wartosci
uint8_t GSM::getNetworkSelection()
{
    char result = AT_RESP_ERR_NO_RESP;
    result = SendATCmdWaitResp(F("AT+COPS?"), CSQ_TO, 100, str_ok, 5);
    if (result != AT_RESP_OK) return result;
    char *str = NULL;
    char *ptr = NULL;
    ptr = strtok_r(comm_buf, " ", &str);
    ptr = strtok_r(NULL, ",", &str);
    return (atoi(ptr));
}

//OK
void GSM::SimpleWrite(char *comm)
{
    _cell.print(comm);
}

//OK
void GSM::SimpleWrite(const char *comm)
{
    _cell.print(comm);
}

//OK
void GSM::SimpleWrite(int comm)
{
    _cell.print(comm);
}

//OK
void GSM::SimpleWrite(const __FlashStringHelper *pgmstr)
{
    _cell.print(pgmstr);
}

//OK
void GSM::SimpleWriteln(char *comm)
{
    _cell.println(comm);
}

//OK
void GSM::SimpleWriteln(const __FlashStringHelper *pgmstr)
{
    _cell.println(pgmstr);
}

//OK
void GSM::SimpleWriteln(char const *comm)
{
    _cell.println(comm);
}

//OK
void GSM::SimpleWriteln(int comm)
{
    _cell.println(comm);
}

//OK
int GSM::available()
{
    return _cell.available();
}

//OK
uint8_t GSM::read()
{
    return _cell.read();
}

//OK
void GSM::SimpleRead()
{
    char datain;
    if(_cell.available()>0)
    {
        datain=_cell.read();
        if(datain>0)
        {
            Serial.print(datain);
        }
    }
}

//OK
void GSM::WhileSimpleRead()
{
    char datain;
    while(_cell.available()>0)
    {
        datain=_cell.read();
        if(datain>0)
        {
            Serial.print(datain);
        }
    }
}

//OK
/**
 * SIMCOM808::read(char* buffer, int buffersize)
 *
 * Waits for data to be readable from the gsm module, reads data until
 * no more is available or the buffer has been filled
 *
 * returns number of bytes read
 *
 */
int GSM::read(char* result, int resultlength)
{
    char temp;
    int i=0;

#ifdef DEBUG_ON
    Serial.print(F("Starting read..\nWaiting for Data.."));
#endif
    // Wait until we start receiving data
    while(available()<1)
    {
        delay(100);
#ifdef DEBUG_ON
        Serial.print(F("."));
#endif
    }

    while(available()>0 && i<(resultlength-1))
    {
        temp=_cell.read();
        if(temp>0)
        {
#ifdef DEBUG_ON
            Serial.print(temp);
#endif
            result[i]=temp;
            i++;
        }
        delay(1);
    }

    // Terminate the string
    result[resultlength-1]='\0';

#ifdef DEBUG_ON
    Serial.println(F("\nDone.."));
#endif
    return i;
}

char GSM::AT()
{
#ifdef DEBUG_ON
    Serial.println(F("DB:AT"));
#endif
    char ret_val = AT_RESP_ERR_NO_RESP;
    ret_val = SendATCmdWaitResp(F("AT"), AT_TO, 100, str_ok, 3);
    if (AT_RESP_OK!=ret_val)
    {
        setStatus(IDLE);
        return ret_val;
    }
    setStatus(READY);
    return AT_RESP_OK;
}

char GSM::getLocalTimestamp(bool mode)
{
#ifdef DEBUG_ON
    Serial.println(F("DB:getLocalTimestamp"));
#endif
    char result=AT_RESP_ERR_NO_RESP;
    if (mode) result=SendATCmdWaitResp(F("AT+CLTS=1"), AT_TO, 100, str_ok, 5);
    else
        result=SendATCmdWaitResp(F("AT+CLTS=0"), AT_TO, 100, str_ok, 5);
    return result;
}

//OK
void GSM::clearBuffer()
{
    comm_buf[0] = 0x00; // end of string
    p_comm_buf = &comm_buf[0];
    comm_buf_len = 0;
    _cell.flush(); // erase rx circular buffer
}

//OK
void GSM::printBuffer()
{
    Serial.println((char *)comm_buf);
}

char GSM::clip(bool mode)
{
#ifdef DEBUG_ON
    Serial.println(F("DB:clip"));
#endif
    char result=AT_RESP_ERR_NO_RESP;
    if (mode) result=SendATCmdWaitResp(F("AT+CLIP=1"), CLIP_TO, 100, str_ok, 5);
    else
        result=SendATCmdWaitResp(F("AT+CLIP=0"), CLIP_TO, 100, str_ok, 5);
    return result;
}

char GSM::selectSIMPhoneBook()
{
#ifdef DEBUG_ON
    Serial.println(F("DB:selectSIMPhoneBook"));
#endif
    return SendATCmdWaitResp(F("AT+CPBS=\"SM\""), CPBS_TO, 100, str_ok, 5);
}

char GSM::isNetworkAvailable()
{
#ifdef DEBUG_ON
    Serial.println(F("DB:isNetworkAvailable"));
#endif
    char result=AT_RESP_ERR_NO_RESP;
    result=this->AT();
    if (result!=AT_RESP_OK) return result;
    result=this->getModemFunctions();
    if (CFUN_ON!=result) return result;
    boolean ok = false;
    byte attempt=0;
    while (!ok)
    {
        result = this->checkNetworkRegistration();
        if ((result != CREG_REGISTERED_HOME) && (result != CREG_REGISTERED_ROAMING))
        {
#ifdef DEBUG_ON
            Serial.print(F("."));
#endif
            delay(5000);
            attempt++;
            if (attempt>6)
            {
#ifdef DEBUG_ON
                Serial.println(F(" failed"));
#endif
                return result;
            }
        }
        else
        {
            ok = true;
        }
    }
#ifdef DEBUG_ON
    Serial.println(F("NETWORK OK"));
    Serial.println(F("Checking signal quality"));
#endif
    ok = false;
    attempt=0;
    while (!ok)
    {
        result = this->signalQuality();
        if ((result == 0) || (result == 99))
        {
#ifdef DEBUG_ON
            Serial.print(F("."));
#endif
            delay(5000);
            attempt++;
            if (attempt>10)
            {
#ifdef DEBUG_ON
                Serial.println(F(" failed"));
#endif
                return CREG_NO_SIGNAL;
            }
        }
        else
        {
            ok = true;
        }
    }
#ifdef DEBUG_ON
    Serial.println(F("SIGNAL OK"));
#endif
    return AT_RESP_OK;
}

char GSM::getModemStatus()
/*
  0:ready(the module can implement AT commands)
  2:unknow(unknown status)
  3:ringing(the module can implement AT command,it will be ringing status when
  there is an incoming call)
  4:call in progress(the module can implement AT command, in call connecting or
    caller ringing status.)
  5:asleep(Module is in sleep mode, not ready)
  */
{
    char *str = NULL;
    char *ptr = NULL;
    char resp = AT_RESP_ERR_NO_RESP;
    byte cpas = 0;
    resp = SendATCmdWaitResp(F("AT+CPAS"), AT_TO, 100, str_ok, 5);
    if (resp!=AT_RESP_OK) return resp;
    ptr = strtok_r(comm_buf, "\n", &str);
    ptr = strtok_r(NULL, "+CPAS: ", &str);
    ptr = strtok_r(ptr, "\n", &str);
    cpas = atoi(ptr);
    if (cpas==0) return CPAS_READY;
    if (cpas==2) return CPAS_UNKNOWN;
    if (cpas==3) return CPAS_RINGING;
    if (cpas==4) return CPAS_CALL_IN_PROGRESS;
    if (cpas==5) return CPAS_ASLEEP;
    return CPAS_UNKNOWN;
}

char GSM::getModemFunctions()
/*
  1: MS is switched on
  2: invalid mode
  17: airplane mode
  */
{
    char resp = AT_RESP_ERR_NO_RESP;
    resp = SendATCmdWaitResp(F("AT+CFUN?"), CFUN_TO, 100, str_ok, 5);
    if (resp!=AT_RESP_OK) return resp;
    if (this->IsStringReceived("+CFUN: 17")) return CFUN_AIRPLANE;
    if (this->IsStringReceived("+CFUN: 1")) return CFUN_ON;
    if (this->IsStringReceived("+CFUN: 2")) return CFUN_INVALID;
    if (this->IsStringReceived("+CFUN: 4")) return CFUN_AIRPLANE;
    if (this->IsStringReceived("+CFUN: 0")) return CFUN_OFF;
    return CFUN_INVALID;
}

char GSM::setModemFunctions(byte mode)
{
    char resp = AT_RESP_ERR_NO_RESP;
    switch (mode)
    {
    case CFUN_OFF:
        resp = SendATCmdWaitResp(F("AT+CFUN=0"), CFUN_TO, 5000, str_ok, 5);
        break;
    case CFUN_ON:
        resp = SendATCmdWaitResp(F("AT+CFUN=1"), CFUN_TO, 5000, str_ok, 5);
        break;
    case CFUN_AIRPLANE:
        resp = SendATCmdWaitResp(F("AT+CFUN=4"), CFUN_TO, 5000, str_ok, 5);
        break;
    case 17:
        resp = SendATCmdWaitResp(F("AT+CFUN=17"), CFUN_TO, 5000, str_ok, 5);
        break;
    }
    return resp;
}

char GSM::PIN()
/*
  -READY:no need to input any passwords
  -SIM PIN:need to input PIN code
  -SIM PUK:need to input PUK code
  -SIM PIN2:need to input PIN2 code
  -SIM PUK2:need to input PUK2 code
*/
{
    char resp = AT_RESP_ERR_NO_RESP;
    resp = SendATCmdWaitResp(F("AT+CPIN?"), CPIN_TO, 100, str_ok, 5);
    if (resp!=AT_RESP_OK) return resp;
    if (this->IsStringReceived("READY")) return PIN_READY;
    if (this->IsStringReceived("SIM PIN2")) return PIN_INPUT_PIN2;
    if (this->IsStringReceived("SIM PUK2")) return PIN_INPUT_PUK2;
    if (this->IsStringReceived("SIM PIN")) return PIN_INPUT_PIN;
    if (this->IsStringReceived("SIM PUK")) return PIN_INPUT_PUK;
    return PIN_UNKNOWN;
}

char GSM::PIN(const char* PIN)
{
    strcpy_P(command, PSTR("AT+CPIN=\""));
    strcat(command,PIN);
    strcat(command,"\"");
    return SendATCmdWaitResp(command, CPIN_TO, 100, str_ok, 5);
}

char GSM::setSMSTextMode()
{
    return SendATCmdWaitResp(F("AT+CMGF=1"), AT_TO, 50, str_ok, 5);
}

char GSM::setTEcharacterGSM()
{
    return SendATCmdWaitResp(F("AT+CSCS=\"GSM\""), AT_TO, 50, str_ok, 5);
}

//OK
/**********************************************************
Function compares specified phone number string
with phone number stored at the specified SIM position
position:       SMS position <1..20>
phone_number:   phone number string which should be compare
return:
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0
        OK ret val:
        -----------
        0 - phone numbers are different
        1 - phone numbers are the same
an example of usage:
        if (1 == gsm.ComparePhoneNumber(1, "123456789")) {
          // the phone num. "123456789" is stored on the SIM pos. #1
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are the same", 1);
          #endif
        }
        else {
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are different", 1);
          #endif
        }
**********************************************************/
char GSM::ComparePhoneNumber(byte position, char *phone_number)
{
    char ret_val = AT_RESP_ERR_NO_RESP;
    char sim_phone_number[20];


    ret_val = 0; // numbers are not the same so far
    if (position == 0) return (-3);
    if (1 == GetPhoneNumber(position, sim_phone_number))
    {
        // there is a valid number at the spec. SIM position
        // -------------------------------------------------
        if (0 == strcmp(phone_number, sim_phone_number))
        {
            // phone numbers are the same
            // --------------------------
            ret_val = 1;
        }
    }
    return (ret_val);
}

/**********************************************************
Method reads phone number string from specified SIM position
position:     SMS position <1..20>
return:
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0
        phone_number is empty string
        OK ret val:
        -----------
        0 - there is no phone number on the position
        1 - phone number was found
        phone_number is filled by the phone number string finished by 0x00
                     so it is necessary to define string with at least
                     15 bytes(including also 0x00 termination character)
an example of usage:
        GSM gsm;
        char phone_num[20]; // array for the phone number string
        if (1 == gsm.GetPhoneNumber(1, phone_num)) {
          // valid phone number on SIM pos. #1
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone number: ", 0);
            gsm.DebugPrint(phone_num, 1);
          #endif
        }
        else {
          // there is not valid phone number on the SIM pos.#1
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG there is no phone number", 1);
          #endif
        }
**********************************************************/

//TODO Do przepisania metoda wysylania
char GSM::GetPhoneNumber(byte position, char *phone_number)
{
    char ret_val = AT_RESP_ERR_NO_RESP;

    char *p_char;
    char *p_char1;


    if (position == 0) return (-3);
    ret_val = 0; // not found yet
    phone_number[0] = 0; // phone number not found yet => empty string

    //send "AT+CPBR=XY" - where XY = position
    _cell.print(F("AT+CPBR="));
    _cell.print((int)position);
    _cell.print("\r");

    // 5000 msec. for initial comm tmout
    // 50 msec. for inter character timeout
    switch (WaitResp(5000, 50, "+CPBR"))
    {
    case RX_TMOUT_ERR:
        // response was not received in specific time
        ret_val = -2;
        break;

    case RX_FINISHED_STR_RECV:
        comm_buf2[0] = 0x00;
        memcpy(comm_buf2,comm_buf,comm_buf_len);
        // response in case valid phone number stored:
        // <CR><LF>+CPBR: <index>,<number>,<type>,<text><CR><LF>
        // <CR><LF>OK<CR><LF>

        // response in case there is not phone number:
        // <CR><LF>OK<CR><LF>
        p_char = strstr((char *)(comm_buf2),",\"");
        if (p_char != NULL)
        {
            p_char++;
            p_char++;       // we are on the first phone number character
            // find out '"' as finish character of phone number string
            p_char1 = strchr((char *)(p_char),'"');
            if (p_char1 != NULL)
            {
                *p_char1 = 0; // end of string
            }
            // extract phone number string
            strcpy(phone_number, (char *)(p_char));
            // output value = we have found out phone number string
            ret_val = 1;
        }
        break;

    case RX_FINISHED_STR_NOT_RECV:
        // only OK or ERROR => no phone number
        ret_val = 0;
        break;
    }
    return (ret_val);
}

//OK
bool GSM::readToBuffer()
{
    if (_cell.available()==0) return false;
#ifdef DEBUG_ON
    Serial.println(F("Something in buffer, waiting for EOT"));
#endif // DEBUG_ON
    WaitResp(2000,500);
    return true;

}

//OK
char* GSM::readLineWithString(char const *compare_string)
{
    char *str = NULL;
    char *ptr = NULL;
    char *ch = NULL;
    comm_buf2[0] = 0x00;
    memcpy(comm_buf2,comm_buf,comm_buf_len);
    bool znalazl=false;
    ptr = strtok_r(comm_buf2, "\n", &str);
    ch = strstr((char *)ptr, compare_string);
    if (ch != NULL) znalazl=true;
    if (!znalazl)
    {
        while ((!znalazl) && (str!=NULL))
        {
            ptr = strtok_r(NULL, "\n", &str);
            ch = strstr((char *)ptr, compare_string);
            if (ch != NULL) znalazl=true;
        }
    }
    return ptr;
}

//OK
char* GSM::readLineWithString_P(char const PROGMEM *compare_string)
{
    char *str = NULL;
    char *ptr = NULL;
    char *ch = NULL;
    comm_buf2[0] = 0x00;
    memcpy(comm_buf2,comm_buf,comm_buf_len);
    bool znalazl=false;
    ptr = strtok_r(comm_buf2, "\n", &str);
    ch = strstr_P((char *)ptr, compare_string);
    if (ch != NULL) znalazl=true;
    if (!znalazl)
    {
        while ((!znalazl) && (str!=NULL))
        {
            ptr = strtok_r(NULL, "\n", &str);
            ch = strstr_P((char *)ptr, compare_string);
            if (ch != NULL) znalazl=true;
        }
    }
    return ptr;
}

char GSM::modemInit(byte group)
{
  char result;
  switch (group)
  {
  case INIT_POWER_ON:
    result = this->AT();
    if (result != AT_RESP_OK) 
    {
      #ifdef DEBUG_ON
        Serial.println(F("modem not ready"));
      #endif
      this->toggleBoot(_bootPin);
      result = this->WaitResp(5000,500,STARTUP_MESSAGE); //TODO module-specific variable
      #ifdef DEBUG_ON
        Serial.print(F("Startup: "));
        Serial.println((int)result);
      #endif
      if ((result != RX_FINISHED_STR_RECV) && (this->AT() != AT_RESP_OK)) 
        {
          #ifdef DEBUG_ON
            Serial.println(F("modem still not ready"));
          #endif
          return AT_RESP_ERR_NO_RESP;
          break;
        }
    }
    #ifdef DEBUG_ON
      Serial.println(F("AT ok"));
    #endif
    return AT_RESP_OK;
    break; 
  
  case INIT_MODEM_STATUS:
    result = this->getModemStatus();
    if ((result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_DIF_RESP)) 
    {
      #ifdef DEBUG_ON
        Serial.println(F("no response"));
      #endif
      return AT_RESP_ERR_NO_RESP;
      break;
    }
    if ((result == CPAS_UNKNOWN) || (result == CPAS_ASLEEP)) 
    {
      #ifdef DEBUG_ON
        Serial.print(F("modem not ready. result: "));
        Serial.println((int)result);
      #endif
      byte k=0;
      while (((result == CPAS_UNKNOWN) || (result == CPAS_ASLEEP)) && (k<20)) 
        {
          delay(500);
          result = this->getModemStatus();
          k++;
          #ifdef DEBUG_ON
            Serial.print(F("."));
          #endif
        }
        if ((result == CPAS_UNKNOWN) || (result == CPAS_ASLEEP) || (result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_DIF_RESP)) 
        {
          #ifdef DEBUG_ON
            Serial.println(F("no response"));
          #endif
          return result;
          break;
        }
    }
    
    #ifdef DEBUG_ON
      Serial.println(F(""));
      Serial.print(F("modem status result: "));
      Serial.println((int)result);
    #endif
    return AT_RESP_OK;
    break; 
  
  case INIT_PIN:
    result = this->PIN();
    if ((result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_DIF_RESP)) 
    {
      return AT_RESP_ERR_NO_RESP;
      break;
    }
    #ifdef DEBUG_ON
      Serial.print(F("PIN status result: "));
      Serial.println((int)result);
    #endif
    if (result != PIN_READY) 
    {
      #ifdef DEBUG_ON
        Serial.println(F("PIN LOCK"));
      #endif 
      return result;
      break;
    }
    return AT_RESP_OK;
    break; 
   
  case INIT_RF_ON:
    result = this->getModemFunctions();
    if ((result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_DIF_RESP)) 
    {
      return AT_RESP_ERR_NO_RESP;
      break;
    }
    #ifdef DEBUG_ON
      Serial.print(F("CFUN result: "));
      Serial.println((int)result);
    #endif
    if (result != CFUN_ON) 
    {
      #ifdef DEBUG_ON
        Serial.println(F("RF was off"));
      #endif
      result = this->setModemFunctions(CFUN_ON);
      #ifdef DEBUG_ON
        Serial.print(F("set CFUN result: "));
        Serial.println((int)result);
      #endif
      if ((result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_DIF_RESP)) 
      {
        return AT_RESP_ERR_NO_RESP;
        break;
      }
      result = this->getModemFunctions();
      if (result != CFUN_ON) 
        {
          return AT_RESP_ERR_NO_RESP;
          break;
        }
    }
    return AT_RESP_OK;
    break; 
  
  case INIT_REPORT_NETWORK_REGISTRATION:
    result = this->SendATCmdWaitResp(F("AT+CREG=2"), CREG_TO, 100, str_ok, 1);
    if ((result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_DIF_RESP)) 
    {
      return AT_RESP_ERR_NO_RESP;
      break;
    }
    #ifdef DEBUG_ON
      Serial.print(F("set CREG result: "));
      Serial.println((int)result);
    #endif
    return AT_RESP_OK;
    break; 
  
  case INIT_CHECK_NETWORK_REGISTRATION:
    result = this->checkNetworkRegistration();
    if ((result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_DIF_RESP)) 
    {
      return AT_RESP_ERR_NO_RESP;
      break;
    }
    #ifdef DEBUG_ON
      Serial.print(F("creg result: "));
      Serial.println((int)result);
    #endif
    if ((result==CREG_NOT_REGISTERED_NOT_SEARCHING) || (result==CREG_REFUSED) || (result==CREG_UNKNOWN) || (result==CREG_NO_SIGNAL)) {
    #ifdef DEBUG_ON
        Serial.println(F("Not registered, not searching"));
        Serial.print(F("searching"));
    #endif
        #ifdef DEBUG_ON
           Serial.println(F("Setting AT+COPS=0,0"));
        #endif
        result = this->SendATCmdWaitResp(F("AT+COPS=0,0"), COPS_WRITE_TO, 100, str_ok, 2);
        if (result!=AT_RESP_OK) 
        {
           #ifdef DEBUG_ON
               Serial.println(F("not registered"));
               Serial.println(F("Init modem again"));
           #endif
           return AT_RESP_ERR_NO_RESP;
           break;
        }
        this->modemInit(INIT_CHECK_NETWORK_REGISTRATION);
        break;
    } else 
       if (result==CREG_SEARCHING) 
       {
          #ifdef DEBUG_ON
            Serial.print(F("Waiting for network"));
          #endif
          byte k=0;
          while ((result==CREG_SEARCHING) && (k<240))
          {
            delay(500);
            #ifdef DEBUG_ON
              Serial.print(F("."));
            #endif
            result = this->checkNetworkRegistration();
            k++;
          }
          if (k=240) 
          {
           return CREG_SEARCHING;
           break;
          } 
          Serial.println(F(""));
        }
    return AT_RESP_OK;
    break;   
  
  case INIT_CHECK_NETWORK_SELECTION:
    result = this->getNetworkSelection();
    if ((result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_DIF_RESP)) 
    {
      return AT_RESP_ERR_NO_RESP;
      break;
    }
    #ifdef DEBUG_ON
      Serial.print(F("COPS: "));
      Serial.println((int)result);
    #endif
    if (result!=0) 
    {
      result = this->SendATCmdWaitResp(F("AT+COPS=0,0"), COPS_WRITE_TO, 100, str_ok, 2);
      #ifdef DEBUG_ON
        Serial.print(F("set COPS result: "));
        Serial.println((int)result);
      #endif
      if (result!=AT_RESP_OK) 
      {
         #ifdef DEBUG_ON
           Serial.println(F("not registered"));
           Serial.println(F("Init modem again"));
         #endif
         return AT_RESP_ERR_NO_RESP;
         break;
      }
      this->modemInit(INIT_CHECK_NETWORK_REGISTRATION);
      break;
    }
    return AT_RESP_OK;
    break; 
  
  case INIT_CHECK_SIGNAL_QUALITY:
    result = this->signalQuality();
    if ((result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_DIF_RESP)) 
    {
      return AT_RESP_ERR_NO_RESP;
      break;
    }
    #ifdef DEBUG_ON
      Serial.print(F("CSQ: "));
      Serial.println((int)result);
    #endif
    if ((result==99) || (result==0)) 
    {
        byte k=0;
        while (((result==0) || (result==99)) && (k<120)) 
        {
            result = this->signalQuality();
            if ((result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_DIF_RESP)) 
            {
              return AT_RESP_ERR_NO_RESP;
              break;
            }
            #ifdef DEBUG_ON
              Serial.print(F("."));
            #endif
            delay(500);
            k++;
        }
        #ifdef DEBUG_ON
          Serial.print(F("CSQ: "));
          Serial.println((int)result);
        #endif
        if (k=120) 
        {
            return AT_RESP_ERR_NO_RESP;
            break;
        }
    }
    return AT_RESP_OK;
    break; 

  case PARAM_SET_9:  //TODO
    result = this->selectSIMPhoneBook();
    #ifdef DEBUG_ON
      Serial.print(F("."));
    #endif
    result = this->setTEcharacterGSM();
    #ifdef DEBUG_ON
      Serial.print(F("."));
    #endif
    result = this->setSMSTextMode();
    #ifdef DEBUG_ON
      Serial.print(F("."));
    #endif
    result = this->InitSMSMemory();
    #ifdef DEBUG_ON
      Serial.print(F("."));
    #endif
    result = this->clip(true);
    #ifdef DEBUG_ON
      Serial.print(F("."));
    #endif
  
    result = this->isNetworkAvailable();
    #ifdef DEBUG_ON
    Serial.println(F("."));
    Serial.print(F("Init: "));
    #endif
    if (result==AT_RESP_OK) {
      #ifdef DEBUG_ON
        Serial.println(F("DONE!"));
      #endif
    }
    else {
    #ifdef DEBUG_ON
      Serial.println(F("FAILED!!!"));
    #endif
    }
    return result;
  }
}

void GSM::initSIM808() 
{
  char result;
  result = this->modemInit(INIT_POWER_ON);
  
  if (result != AT_RESP_OK)
  { 
    this-> initSIM808();
  }
  
  result = this->Echo(false);
  
  if (result != AT_RESP_OK)
  { 
    this-> initSIM808();
  }
  
  // Mobile Equipment Error Code
  #ifdef DEBUG_AT
    SendATCmdWaitResp(F("AT+CMEE=2"), AT_TO, 50, str_ok, 5);
  #endif
  #ifndef DEBUG_AT
    SendATCmdWaitResp(F("AT+CMEE=0"), AT_TO, 50, str_ok, 5);
  #endif
  
  
  result = this->modemInit(INIT_PIN);
  
  if (result != AT_RESP_OK)
  { 
    if ((result == AT_RESP_ERR_NO_RESP) || (result == AT_RESP_ERR_NO_RESP)) 
    {
      this-> initSIM808();
    }
    if (result == PIN_INPUT_PIN) 
    {
      result = this->PIN("1111");
    }
  }
  
  result = this->modemInit(INIT_RF_ON);
  
  if (result != AT_RESP_OK)
  { 
    this-> initSIM808();
  }
      
  result = this->modemInit(INIT_MODEM_STATUS);
  
  if (result != AT_RESP_OK)
  { 
    this-> initSIM808();
  }
   
  this->modemInit(INIT_REPORT_NETWORK_REGISTRATION);
  
  if (result != AT_RESP_OK)
  { 
    this-> initSIM808();
  }
  
  this->modemInit(INIT_CHECK_NETWORK_REGISTRATION);
  
  if (result != AT_RESP_OK)
  { 
    this-> initSIM808();
  }
  
  this->modemInit(INIT_CHECK_NETWORK_SELECTION);
  
  if (result != AT_RESP_OK)
  { 
    this-> initSIM808();
  }
  
  this->modemInit(INIT_CHECK_SIGNAL_QUALITY);
    
  if (result != AT_RESP_OK)
  { 
    this-> initSIM808();
  }
  
  this->modemInit(PARAM_SET_9);
  
}

void GSM::toggleBoot(byte boot)
{ 
  if (boot!=255) {
    pinMode(boot, OUTPUT);
    digitalWrite(boot, !(digitalRead(boot)));
    delay(1000);
    digitalWrite(boot, !(digitalRead(boot)));
  }
}

void GSM::powerOff() {
  Serial.println(F("TODO!")); //TODO
}
