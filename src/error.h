#ifndef _INCLUDE_ERROR
#define _INCLUDE_ERROR

extern int useMutexInErrorReporting;
extern FILE* logFileHandle;
extern char* parsingLocation;

void Error( char* fmt, ... );
void APIError( char* methodName, int errorCode, char* fmt, ... );
void Warning( char* fmt, ... );
void Notice( char* fmt, ... );

#endif
