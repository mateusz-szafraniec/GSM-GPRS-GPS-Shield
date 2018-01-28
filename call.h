#ifndef _CALL_H_
#define _CALL_H_

#include "comGSM.h"
class CallGSM
{
public:
    bool isRinging();
    // finds out the status of call
    char CallStatusWithAuth(char *phone_number,
                              byte first_authorized_pos, byte last_authorized_pos);
    // picks up an incoming call
    char PickUp(void);
    // hangs up an incomming call
    char HangUp(void);
    // calls the specific number
    char Call(char *number_string);
    // makes a call to the number stored at the specified SIM position
    char Call(int sim_position);
    
    CallGSM::CallGSM();

    /*
    void SendDTMF(char *number_string, int time);

    void SetDTMF(int DTMF_status);
    char DetDTMF();
    */

};
extern GSM gsm;
#endif

