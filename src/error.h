#ifndef _INCLUDE_ERROR
#define _INCLUDE_ERROR

void Error( char* fmt, ... );
void APIError( char* methodName, int errorCode, char* fmt, ... );
void Warning( char* fmt, ... );
void Notice( char* fmt, ... );

#endif

