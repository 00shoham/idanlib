#ifndef _INCLUDE_DATE_UTILS
#define _INCLUDE_DATE_UTILS

int IsRecent( time_t when, int maxAgeSeconds );
char* DateStr( time_t t, char* buf, int bufLen );
char* DateNow( char* buf, int bufLen );
char* TimeStr( time_t t, char* buf, int bufLen, int showSeconds );
char* TimeNow( char* buf, int bufLen, int showSeconds );
char* DateTimeNow( char* buf, int bufLen, int showSeconds );
char* DateTimeStr( char* buf, int bufLen, int showSeconds, time_t t );
char* DateNow( char* buf, int bufLen );
int ValidTime( char* when );
int ValidDate( char* when );
long long TimeInMicroSeconds();
time_t ParseTimeString( char* str );

#endif
