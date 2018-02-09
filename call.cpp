#include "call.h"
#include "comGSM.h"

CallGSM::CallGSM(){};

bool CallGSM::isRinging()
{
    return gsm.IsStringReceived("RING");
}


/**********************************************************
Method checks status of call(incoming or active)
and makes authorization with specified SIM positions range

phone_number: a pointer where the tel. number string of current call will be placed
              so the space for the phone number string must be reserved - see example
first_authorized_pos: initial SIM phonebook position where the authorization process
                      starts
last_authorized_pos:  last SIM phonebook position where the authorization process
                      finishes

                      Note(important):
                      ================
                      In case first_authorized_pos=0 and also last_authorized_pos=0
                      the received incoming phone number is NOT authorized at all, so every
                      incoming is considered as authorized (CALL_INCOM_VOICE_NOT_AUTH is returned)

return:
      CALL_NONE                   - no call activity
      CALL_INCOM_VOICE_AUTH       - incoming voice - authorized
      CALL_INCOM_VOICE_NOT_AUTH   - incoming voice - not authorized
      CALL_ACTIVE_VOICE           - active voice
      CALL_INCOM_DATA_AUTH        - incoming data call - authorized
      CALL_INCOM_DATA_NOT_AUTH    - incoming data call - not authorized
      CALL_ACTIVE_DATA            - active data call
      CALL_NO_RESPONSE            - no response to the AT command
      CALL_COMM_LINE_BUSY         - comm line is not free
**********************************************************/
char CallGSM::CallStatusWithAuth(char *phone_number,
                                   byte first_authorized_pos, byte last_authorized_pos)
{
    char ret_val = CALL_NONE;
    byte search_phone_num = 0;
    byte i;
    byte status;
    char *p_char;
    char *p_char1;

    phone_number[0] = 0x00;  // no phonr number so far

    ret_val = gsm.SendATCmdWaitResp(F("AT+CLCC"), 5000, 1500, str_ok, 3);
    if (AT_RESP_OK!=ret_val) return ret_val;
    if(gsm.IsStringReceived("+CLCC: 1,1,4,0,0"))
    {
        // incoming VOICE call - not authorized so far
        // -------------------------------------------
        search_phone_num = 1;
        ret_val = CALL_INCOM_VOICE_NOT_AUTH;
    }
    else if(gsm.IsStringReceived("+CLCC: 1,1,4,1,0"))
    {
        // incoming DATA call - not authorized so far
        // ------------------------------------------
        search_phone_num = 1;
        ret_val = CALL_INCOM_DATA_NOT_AUTH;
    }
    else if(gsm.IsStringReceived("+CLCC: 1,0,0,0,0"))
    {
        // active VOICE call - GSM is caller
        // ----------------------------------
        search_phone_num = 1;
        ret_val = CALL_ACTIVE_VOICE;
    }
    else if(gsm.IsStringReceived("+CLCC: 1,1,0,0,0"))
    {
        // active VOICE call - GSM is listener
        // -----------------------------------
        search_phone_num = 1;
        ret_val = CALL_ACTIVE_VOICE;
    }
    else if(gsm.IsStringReceived("+CLCC: 1,1,0,1,0"))
    {
        // active DATA call - GSM is listener
        // ----------------------------------
        search_phone_num = 1;
        ret_val = CALL_ACTIVE_DATA;
    }
    else if(gsm.IsStringReceived("+CLCC:"))
    {
        // other string is not important for us - e.g. GSM module activate call
        // etc.
        // IMPORTANT - each +CLCC:xx response has also at the end
        // string <CR><LF>OK<CR><LF>
        ret_val = CALL_OTHERS;
    }
    else if(gsm.IsStringReceived("OK"))
    {
        // only "OK" => there is NO call activity
        // --------------------------------------
        ret_val = CALL_NONE;
    }


    // now we will search phone num string
    if (search_phone_num)
    {
        // extract phone number string
        // ---------------------------
        p_char = strchr((char *)(gsm.comm_buf),'"');
        p_char1 = p_char+1; // we are on the first phone number character
        p_char = strchr((char *)(p_char1),'"');
        if (p_char != NULL)
        {
            *p_char = 0; // end of string
            strcpy(phone_number, (char *)(p_char1));
            Serial.print(F("ATTESO: "));
            Serial.println(phone_number);
        }
        else
            //Serial.println(gsm.comm_buf);
            Serial.println(F("NULL"));

        if ( (ret_val == CALL_INCOM_VOICE_NOT_AUTH)
                || (ret_val == CALL_INCOM_DATA_NOT_AUTH))
        {

            if ((first_authorized_pos == 0) && (last_authorized_pos == 0))
            {
                // authorization is not required => it means authorization is OK
                // -------------------------------------------------------------
                if (ret_val == CALL_INCOM_VOICE_NOT_AUTH) ret_val = CALL_INCOM_VOICE_AUTH;
                else ret_val = CALL_INCOM_DATA_AUTH;
            }
            else
            {
                // make authorization
                // ------------------
                //gsm.SetCommLineStatus(CLS_FREE);
                for (i = first_authorized_pos; i <= last_authorized_pos; i++)
                {
                    if (gsm.ComparePhoneNumber(i, phone_number))
                    {
                        // phone numbers are identical
                        // authorization is OK
                        // ---------------------------
                        if (ret_val == CALL_INCOM_VOICE_NOT_AUTH) ret_val = CALL_INCOM_VOICE_AUTH;
                        else ret_val = CALL_INCOM_DATA_AUTH;
                        break;  // and finish authorization
                    }
                }
            }
        }
    }
    return (ret_val);
}

/**********************************************************
Method picks up an incoming call

return:
**********************************************************/
char CallGSM::PickUp(void)
{
    char ret_val = -1;
    ret_val =  gsm.SendATCmdWaitResp(F("ATA"), ATA_TO, 100, str_ok, 3);
    if (ret_val==AT_RESP_OK) return AT_RESP_OK;
    if (gsm.IsStringReceived("NO CARRIER")) return CALL_NO_CARRIER;
    if (gsm.IsStringReceived("ERROR")) return CALL_NO_CARRIER; //SIM808 zglasza ERROR przy braku aktywnego polaczenia
    return ret_val;

}

/**********************************************************
Method hangs up incoming or active call

return:
**********************************************************/
char CallGSM::HangUp(void)
{
    return  gsm.SendATCmdWaitResp(F("ATH"), ATH_TO, 100, str_ok, 3);
}


/**********************************************************
Method calls the specific number

number_string: pointer to the phone number string
               e.g. gsm.Call("+420123456789");

**********************************************************/
char CallGSM::Call(char *number_string)
{
    char ret_val=-1;
    ret_val = gsm.isNetworkAvailable();
    if (ret_val!=AT_RESP_OK) return ret_val;
    ret_val = gsm.getModemStatus();
    if (ret_val!=CPAS_READY) return ret_val;
    // ATDxxxxxx;<CR>
    strcpy_P(gsm.command,PSTR("ATD"));
    strcat(gsm.command,number_string);
    strcat(gsm.command,";");
    // 10 sec. for initial comm tmout
    // 50 msec. for inter character timeout
    ret_val = gsm.SendATCmdWaitResp(gsm.command, 15000, 500, str_ok, 1);
    if (AT_RESP_OK==ret_val) return ret_val; //Answered
    if (gsm.IsStringReceived("NO CARRIER")) return CALL_NO_CARRIER;
    if (gsm.IsStringReceived("BUSY")) return CALL_BUSY;
    if (AT_RESP_ERR_NO_RESP==ret_val) return CALL_NO_RESPONSE;
    return ret_val;
}


/**********************************************************
Method calls the number stored at the specified SIM position

sim_position: position in the SIM <1...>
              e.g. gsm.Call(1);
**********************************************************/
char CallGSM::Call(int sim_position)
{
    char ret_val=-1;
    ret_val = gsm.isNetworkAvailable();
    if (ret_val!=AT_RESP_OK) return ret_val;
    ret_val = gsm.getModemStatus();
    if (ret_val!=CPAS_READY) return ret_val;
    strcpy_P(gsm.command,PSTR("ATD>"));
    strcat(gsm.command,sim_position);
    strcat(gsm.command,";");
    // 10 sec. for initial comm tmout
    // 50 msec. for inter character timeout
    ret_val = gsm.SendATCmdWaitResp(gsm.command, 15000, 500, str_ok, 3);
    if (AT_RESP_OK==ret_val) return ret_val; //Answered
    if (gsm.IsStringReceived("NO CARRIER")) return CALL_NO_CARRIER;
    if (gsm.IsStringReceived("BUSY")) return CALL_BUSY;
    if (AT_RESP_ERR_NO_RESP==ret_val) return CALL_NO_RESPONSE;
    return ret_val;

}

/*
void CallGSM::SendDTMF(char *number_string, int time)
{
     //if (CLS_FREE != gsm.GetCommLineStatus()) return;
     //gsm.SetCommLineStatus(CLS_ATCMD);

     gsm.SimpleWrite(F("AT+VTD="));
     gsm.SimpleWriteln(time);
     gsm.WaitResp(1000, 100, "OK");

     gsm.SimpleWrite(F("AT+VTS=\""));
     gsm.SimpleWrite(number_string);
     gsm.SimpleWriteln(F("\""));

     gsm.WaitResp(5000, 100, "OK");
     //gsm.SetCommLineStatus(CLS_FREE);
}

void CallGSM::SetDTMF(int DTMF_status)
{
     if(DTMF_status==1)
          gsm.SendATCmdWaitResp("AT+DDET=1", 500, 50, "OK", 5);
     else
          gsm.SendATCmdWaitResp("AT+DDET=0", 500, 50, "OK", 5);
}


char CallGSM::DetDTMF()
{
     char *p_char;
     char *p_char1;
     char dtmf_char='-';
     gsm.WaitResp(1000, 500);
     {
          //Serial.print("BUF: ");
          //Serial.println((char *)gsm.comm_buf);
          //Serial.println("end");
          p_char = strstr((char *)(gsm.comm_buf),"+DTMF:");
          if (p_char != NULL) {
               p_char1 = p_char+6;  //we are on the first char of BCS
               dtmf_char = *p_char1;
          }
     }
     return dtmf_char;
}
*/