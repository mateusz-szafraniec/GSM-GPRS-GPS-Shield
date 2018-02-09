#include "sms.h"

#include "comGSM.h"

//SMSGSM::SMSGSM(){};

/**********************************************************
Method sends SMS

number_str:   pointer to the phone number string
message_str:  pointer to the SMS text string


return:
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0 - SMS was not sent
        1 - SMS was sent


an example of usage:
        GSM gsm;
        gsm.SendSMS("00XXXYYYYYYYYY", "SMS text");
**********************************************************/
char SMSGSM::SendSMS(char *number_str, char *message_str)
{
    if(strlen(message_str)>159)
        Serial.println(F("Don't send message longer than 160 characters"));
    char ret_val = -1;
    ret_val = gsm.isNetworkAvailable();
    if (ret_val!=AT_RESP_OK) return ret_val;
    ret_val = gsm.getModemStatus();
    if (ret_val!=CPAS_READY) return ret_val;
    byte i;
    char end[2];
    end[0]=0x1a;
    end[1]='\0';
    // try to send SMS 3 times in case there is some problem
    for (i = 0; i < 3; i++)
    {
        // send  AT+CMGS="number_str"
        strcpy_P(gsm.command,PSTR("AT+CMGS=\""));
        strcat(gsm.command,number_str);
        strcat(gsm.command,"\"");
        ret_val = gsm.SendATCmdWaitResp(gsm.command, 1000, 500, ">", 1);
#ifdef DEBUG_ON
        Serial.println("DEBUG:SMS TEST");
#endif
        // 1000 msec. for initial comm tmout
        // 50 msec. for inter character timeout
        if (AT_RESP_OK == ret_val)
        {
#ifdef DEBUG_ON
            Serial.println("DEBUG:>");
#endif
            // send SMS text
            strcat(message_str,end);
            ret_val = gsm.SendATCmdWaitResp(message_str, 20000, 5000, "+CMGS", 1);
            if (AT_RESP_OK==ret_val) return ret_val; // SMS was send correctly
        }
        else
        {
            // try again
            continue;
        }
    }
    return (ret_val);
}

/**********************************************************
Method sends SMS to the specified SIM phonebook position

sim_phonebook_position:   SIM phonebook position <1..20>
message_str:              pointer to the SMS text string


return:
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - specified position must be > 0

        OK ret val:
        -----------
        0 - SMS was not sent
        1 - SMS was sent


an example of usage:
        GSM gsm;
        gsm.SendSMS(1, "SMS text");
**********************************************************/
char SMSGSM::SendSMS(byte sim_phonebook_position, char *message_str)
{
    char ret_val = -1;
    char sim_phone_number[20];
    if (sim_phonebook_position == 0) return (-3);
    if (1 == gsm.GetPhoneNumber(sim_phonebook_position, sim_phone_number))
    {
        // there is a valid number at the spec. SIM position
        // => send SMS
        // -------------------------------------------------
        ret_val = SendSMS(sim_phone_number, message_str);
    }
    return (ret_val);
}

/**********************************************************
Method finds out if there is present at least one SMS with
specified status

Note:
if there is new SMS before IsSMSPresent() is executed
this SMS has a status UNREAD and then
after calling IsSMSPresent() method status of SMS
is automatically changed to READ

required_status:  SMS_UNREAD  - new SMS - not read yet
                  SMS_READ    - already read SMS
                  SMS_ALL     - all stored SMS

return:
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout

        OK ret val:
        -----------
        0 - there is no SMS with specified status
        1..20 - position where SMS is stored
                (suitable for the function GetSMS())


an example of use:
        GSM gsm;
        char position;
        char phone_number[20]; // array for the phone number string
        char sms_text[100];

        position = gsm.IsSMSPresent(SMS_UNREAD);
        if (position) {
          // read new SMS
          gsm.GetSMS(position, phone_num, sms_text, 100);
          // now we have phone number string in phone_num
          // and SMS text in sms_text
        }
**********************************************************/
char SMSGSM::IsSMSPresent(byte required_status)
{
    char ret_val = -1;
    char *p_char;
    switch (required_status)
    {
    case SMS_UNREAD:
        strcpy_P(gsm.command,PSTR("AT+CMGL=\"REC UNREAD\",1"));
        break;
    case SMS_READ:
        strcpy_P(gsm.command,PSTR("AT+CMGL=\"REC READ\",1"));
        break;
    case SMS_ALL:
        strcpy_P(gsm.command,PSTR("AT+CMGL=\"ALL\",1"));
        break;
    }
    ret_val = gsm.SendATCmdWaitResp(gsm.command,5000, 500, str_ok, 3);
    if (gsm.IsStringReceived("+CMGL:")) {
        // there is some SMS with status => get its position
        // response is:
        // +CMGL: <index>,<stat>,<oa/da>,,[,<tooa/toda>,<length>]
        // <CR><LF> <data> <CR><LF>OK<CR><LF>
        p_char = strchr((char *)gsm.comm_buf,':');
        if (p_char != NULL) return atoi(p_char+1);
    }
    if (AT_RESP_OK==ret_val) return GETSMS_NO_SMS;
    return ret_val;
}


/**********************************************************
Method reads SMS from specified memory(SIM) position

position:     SMS position <1..20>
phone_number: a pointer where the phone number string of received SMS will be placed
              so the space for the phone number string must be reserved - see example
SMS_text  :   a pointer where SMS text will be placed
max_SMS_len:  maximum length of SMS text excluding also string terminating 0x00 character

return:
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - specified position must be > 0

        OK ret val:
        -----------
        GETSMS_NO_SMS       - no SMS was not found at the specified position
        GETSMS_UNREAD_SMS   - new SMS was found at the specified position
        GETSMS_READ_SMS     - already read SMS was found at the specified position
        GETSMS_OTHER_SMS    - other type of SMS was found


an example of usage:
        GSM gsm;
        char position;
        char phone_num[20]; // array for the phone number string
        char sms_text[100]; // array for the SMS text string

        position = gsm.IsSMSPresent(SMS_UNREAD);
        if (position) {
          // there is new SMS => read it
          gsm.GetSMS(position, phone_num, sms_text, 100);
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }
**********************************************************/
char SMSGSM::GetSMS(byte position, char *phone_number,byte max_phone_len, char *SMS_text, byte max_SMS_len)
{
    char ret_val = -1;
    char *p_char;
    char *p_char1;
    byte len;
    char position_chr[3];
    if (position == 0) return (GETSMS_NO_SMS);
    phone_number[0] = 0;  // end of string for now
    //send "AT+CMGR=X" - where X = position
    strcpy_P(gsm.command,PSTR("AT+CMGR="));
    sprintf(position_chr, "%d", (int)position);
    strcat(gsm.command,position_chr);
    // 5000 msec. for initial comm tmout
    // 100 msec. for inter character tmout
    ret_val = gsm.SendATCmdWaitResp(gsm.command,5000, 500, str_ok, 3);
    if (ret_val!=AT_RESP_OK) return ret_val;
    if (!gsm.IsStringReceived("+CMGR")) return GETSMS_NO_SMS;
    if (gsm.IsStringReceived("\"REC UNREAD\""))
        {
            // get phone number of received SMS: parse phone number string
            // +XXXXXXXXXXXX
            // -------------------------------------------------------
            ret_val = GETSMS_UNREAD_SMS;
        }
        //response for already read SMS = old SMS:
        //<CR><LF>+CMGR: "REC READ","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
        //There is SMS text<CR><LF>
    else if(gsm.IsStringReceived(("\"REC READ\"")))
        {
            // get phone number of received SMS
            // --------------------------------
            ret_val = GETSMS_READ_SMS;
        }
    else
        {
            // other type like stored for sending..
            ret_val = GETSMS_OTHER_SMS;
        }

        // extract phone number string
        // ---------------------------
        p_char = strchr((char *)(gsm.comm_buf),',');
        p_char1 = p_char+2; // we are on the first phone number character
        p_char = strchr((char *)(p_char1),'"');
        if (p_char != NULL)
        {
            *p_char = 0; // end of string
            len = strlen(p_char1);
            if(len < max_phone_len)
            {
                strcpy(phone_number, (char *)(p_char1));
            }
            else
            {
                memcpy(phone_number,(char *)p_char1,(max_phone_len-1));
                phone_number[max_phone_len]=0;
            }
        }


        // get SMS text and copy this text to the SMS_text buffer
        // ------------------------------------------------------
        p_char = strchr(p_char+1, 0x0a);  // find <LF>
        if (p_char != NULL)
        {
            // next character after <LF> is the first SMS character
            p_char++; // now we are on the first SMS character

            // find <CR> as the end of SMS string
            p_char1 = strchr((char *)(p_char), 0x0d);
            if (p_char1 != NULL)
            {
                // finish the SMS text string
                // because string must be finished for right behaviour
                // of next strcpy() function
                *p_char1 = 0;
            }
            // in case there is not finish sequence <CR><LF> because the SMS is
            // too long (more then 130 characters) sms text is finished by the 0x00
            // directly in the gsm.WaitResp() routine

            // find out length of the SMS (excluding 0x00 termination character)
            len = strlen(p_char);

            if (len < max_SMS_len)
            {
                // buffer SMS_text has enough place for copying all SMS text
                // so copy whole SMS text
                // from the beginning of the text(=p_char position)
                // to the end of the string(= p_char1 position)
                strcpy(SMS_text, (char *)(p_char));
            }
            else
            {
                // buffer SMS_text doesn't have enough place for copying all SMS text
                // so cut SMS text to the (max_SMS_len-1)
                // (max_SMS_len-1) because we need 1 position for the 0x00 as finish
                // string character
                memcpy(SMS_text, (char *)(p_char), (max_SMS_len-1));
                SMS_text[max_SMS_len] = 0; // finish string
            }
        }
    return (ret_val);
}

/**********************************************************
Method reads SMS from specified memory(SIM) position and
makes authorization - it means SMS phone number is compared
with specified SIM phonebook position(s) and in case numbers
match GETSMS_AUTH_SMS is returned, otherwise GETSMS_NOT_AUTH_SMS
is returned

position:     SMS position to be read <1..20>
phone_number: a pointer where the tel. number string of received SMS will be placed
              so the space for the phone number string must be reserved - see example
SMS_text  :   a pointer where SMS text will be placed
max_SMS_len:  maximum length of SMS text excluding terminating 0x00 character

first_authorized_pos: initial SIM phonebook position where the authorization process
                      starts
last_authorized_pos:  last SIM phonebook position where the authorization process
                      finishes

                      Note(important):
                      ================
                      In case first_authorized_pos=0 and also last_authorized_pos=0
                      the received SMS phone number is NOT authorized at all, so every
                      SMS is considered as authorized (GETSMS_AUTH_SMS is returned)

return:
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        GETSMS_NO_SMS           - no SMS was found at the specified position
        GETSMS_NOT_AUTH_SMS     - NOT authorized SMS found at the specified position
        GETSMS_AUTH_SMS         - authorized SMS found at the specified position


an example of usage:
        GSM gsm;
        char phone_num[20]; // array for the phone number string
        char sms_text[100]; // array for the SMS text string

        // authorize SMS with SIM phonebook positions 1..3
        if (GETSMS_AUTH_SMS == gsm.GetAuthorizedSMS(1, phone_num, sms_text, 100, 1, 3)) {
          // new authorized SMS was detected at the SMS position 1
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }

        // don't authorize SMS with SIM phonebook at all
        if (GETSMS_AUTH_SMS == gsm.GetAuthorizedSMS(1, phone_num, sms_text, 100, 0, 0)) {
          // new SMS was detected at the SMS position 1
          // because authorization was not required
          // SMS is considered authorized
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG SMS phone number: ", 0);
            gsm.DebugPrint(phone_num, 0);
            gsm.DebugPrint("\r\n          SMS text: ", 0);
            gsm.DebugPrint(sms_text, 1);
          #endif
        }
**********************************************************/
char SMSGSM::GetAuthorizedSMS(byte position, char *phone_number,byte max_phone_len, char *SMS_text, byte max_SMS_len,
                              byte first_authorized_pos, byte last_authorized_pos)
{
    char ret_val = -1;
    byte i;

    ret_val = GetSMS(position, phone_number, max_phone_len, SMS_text, max_SMS_len);
    if (GETSMS_NO_SMS == ret_val) return ret_val;
    if ((GETSMS_UNREAD_SMS == ret_val) || (GETSMS_READ_SMS == ret_val) || (GETSMS_OTHER_SMS == ret_val))
    {
        // now SMS can has only READ attribute because we have already read
        // this SMS at least once by the previous function GetSMS()
        //
        // new READ SMS was detected on the specified SMS position =>
        // make authorization now
        // ---------------------------------------------------------
        if ((first_authorized_pos == 0) && (last_authorized_pos == 0))
        {
            // authorization is not required => it means authorization is OK
            // -------------------------------------------------------------
            ret_val = GETSMS_AUTH_SMS;
        }
        else
        {
            ret_val = GETSMS_NOT_AUTH_SMS;  // authorization not valid yet
            for (i = first_authorized_pos; i <= last_authorized_pos; i++)
            {
                if (gsm.ComparePhoneNumber(i, phone_number))
                {
                    // phone numbers are identical
                    // authorization is OK
                    // ---------------------------
                    ret_val = GETSMS_AUTH_SMS;
                    break;  // and finish authorization
                }
            }
        }
    }
    return (ret_val);
}

/**********************************************************
Method deletes SMS from the specified SMS position

position:     SMS position <1..20>

return:
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - SMS was not deleted
        1 - SMS was deleted
**********************************************************/
char SMSGSM::DeleteSMS(byte position)
{
    char ret_val = -1;

    if (position == 0) return (-3);
    char position_chr[3];
    //send "AT+CMGD=XY" - where XY = position
    strcpy_P(gsm.command,PSTR("AT+CMGD="));
    sprintf(position_chr, "%d", (int)position);
    strcat(gsm.command,position_chr);
    // 5000 msec. for initial comm tmout
    // 20 msec. for inter character timeout
    ret_val = gsm.SendATCmdWaitResp(gsm.command,5000, 100, str_ok, 3);
    return (ret_val);
}

byte SMSGSM::isSMSReceived()
{
    if (!gsm.IsStringReceived("+CMTI: \"SM\",")) return 0;
    char *p_char;
    p_char = strchr((char *)gsm.comm_buf,',');
    if (p_char != NULL) return atoi(p_char+1);
    return 0;
}

bool SMSGSM::isATcommand(byte position) {
    char ret_val = GetAuthorizedSMS(position, gsm.phone_num, 20, gsm.at_sms_command, 50,1,20);
    if (GETSMS_AUTH_SMS!=ret_val) return false;
    if (0==strncmp(gsm.at_sms_command,"AT",2)){
        return true;
    }
    return false;
}

void SMSGSM::execATcommand()
{
    gsm.SendATCmdWaitResp(gsm.at_sms_command,30000,5000,str_ok,1);
    strcpy(gsm.comm_buf2,gsm.comm_buf);
    this->SendSMS(gsm.phone_num,gsm.comm_buf2);
}
