#include <sys/time.h>
#include <stddef.h>
#include <time.h>
#include "time.h"

/** Daley execution of the thread by hardware api */
void delayMicrosecondsHard (unsigned int howLong) {
    struct timeval tNow, tLong, tEnd ;

    gettimeofday (&tNow, NULL) ;
    tLong.tv_sec  = howLong / 1000000 ;
    tLong.tv_usec = howLong % 1000000 ;
    timeradd (&tNow, &tLong, &tEnd) ;

    while (timercmp (&tNow, &tEnd, <))
        gettimeofday (&tNow, NULL) ;
}

/** Daley execution of the thread by os api */
void delayMicroseconds (unsigned int howLong) {
    struct timespec sleeper ;
    unsigned int uSecs = howLong % 1000000 ;
    unsigned int wSecs = howLong / 1000000 ;

    if (howLong ==   0)
         return ;
    else if (howLong  < 100)
        delayMicrosecondsHard (howLong) ;
    else {
        sleeper.tv_sec  = wSecs ;
        sleeper.tv_nsec = (long)(uSecs * 1000L) ;
        nanosleep (&sleeper, NULL) ;
    }
}
