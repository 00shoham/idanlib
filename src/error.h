#ifndef _INCLUDE_ERROR
#define _INCLUDE_ERROR

extern int useMutexInErrorReporting;
extern FILE* logFileHandle;
extern char* parsingLocation;

void Error( const char* fmt, ... );
void APIError( const char* methodName, int errorCode, const char* fmt, ... );
void Warning( const char* fmt, ... );
void Notice( const char* fmt, ... );

#endif
