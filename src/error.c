#include "utils.h"

/* globs mentioned in error.h */
pthread_mutex_t errLock = PTHREAD_MUTEX_INITIALIZER;
int useMutexInErrorReporting = 0;
FILE* logFileHandle = NULL;
char* parsingLocation = NULL;

#define LOCK_MUTEX\
  if( useMutexInErrorReporting )\
    {\
    pthread_mutex_lock( &errLock );\
    }

#define UNLOCK_MUTEX\
  if( useMutexInErrorReporting )\
    {\
    pthread_mutex_unlock( &errLock );\
    }

void mysyslog( int level, const char* prefix, const char* msg )
  {
  time_t tnow = time(NULL);
  struct tm *tmp = localtime( &tnow );
  char longtime[BUFLEN];
  snprintf( longtime, sizeof(longtime)-1,
            "%04d-%02d-%02d %02d:%02d:%02d %s ",
            tmp->tm_year + 1900,
            tmp->tm_mon + 1,
            tmp->tm_mday,
            tmp->tm_hour,
            tmp->tm_min,
            tmp->tm_sec,
            prefix );

  if( inDaemon || inCGI )
    {
    if( prefix!=NULL && strcmp(prefix,"NOTICE")==0 ) level = LOG_NOTICE;
    else if( prefix!=NULL && strcmp(prefix,"WARNING")==0 ) level = LOG_WARNING;
    else if( prefix!=NULL && strcmp(prefix,"ERROR")==0 ) level = LOG_ERR;

    if( logFileHandle!=NULL )
      {
      LOCK_MUTEX
      fputs( longtime, logFileHandle );
      if( NOTEMPTY( parsingLocation ) )
        fputs( parsingLocation, logFileHandle );
      fputs( msg, logFileHandle );
      fputs( "\n", logFileHandle );
      fflush( logFileHandle );
      UNLOCK_MUTEX
      }
    else
      {
      if( NOTEMPTY( parsingLocation ) )
        {
        char buf[BUFLEN];
        snprintf(buf,sizeof(buf)-1,"%s %s", parsingLocation, msg );
        syslog( level, "%s", msg );
        }
      else
        {
        syslog( level, "%s", msg );
        }
      }
    }
  else
    {
    LOCK_MUTEX
    FILE* where = stdout;
    if( logFileHandle!=NULL )
      where = logFileHandle;
    fputs( longtime, where );
    if( NOTEMPTY( parsingLocation ) )
      fputs( parsingLocation, where );
    fputs( msg, where );
    fputs( "\n", where );
    fflush( where );
    UNLOCK_MUTEX
    }
  }

void Error( const char* fmt, ... )
  {
  va_list arglist;
  char buf[BIGBUF];

  va_start( arglist, fmt );
  vsnprintf( buf, sizeof(buf)-1, fmt, arglist );
  va_end( arglist );

  if( inCGI>1 && printedContentType==0 )
    {
    fputs( "Content-Type: text/html\r\n\r\n", stdout );
    printedContentType = 1;
    }

  if( inCGI )
    {
    fputs( "<p>ERROR: ", stdout );
    fputs( buf, stdout );
    fputs( "</p>\n", stdout );
    fflush(stdout);
    }

  mysyslog( LOG_USER|LOG_ERR, "ERROR", buf );
  exit(EXIT_FAILURE);
  }

void APIError( const char* methodName, int errorCode, const char* fmt, ... )
  {
  va_list arglist;
  char errmsg[BUFLEN];
  char* ptr = errmsg;
  char* end = errmsg+sizeof(errmsg)-1;

  va_start( arglist, fmt );
  vsnprintf( ptr, end-ptr, fmt, arglist );
  va_end( arglist );

  mysyslog( LOG_USER|LOG_ERR, "APIERROR", errmsg );

  char responseBuf[BUFLEN];
  _TAG_VALUE* response = NULL;
  response = NewTagValueInt( "code", errorCode, response, 1 );
  response = NewTagValue( "result", errmsg, response, 1 );
  ListToJSON( response, responseBuf, sizeof(responseBuf)-1 );

  if( inCGI>1 && printedContentType==0 )
    {
    fputs( "Content-Type: application/json\r\n\r\n", stdout );
    printedContentType = 1;
    }

  fputs( responseBuf, stdout );
  fputs( "\r\n", stdout );
  fflush( stdout );

  exit(1);
  }

void Warning( const char* fmt, ... )
  {
  va_list arglist;
  char buf[BIGBUF];

  va_start( arglist, fmt );
  vsnprintf( buf, sizeof(buf)-1, fmt, arglist );
  va_end( arglist );

  mysyslog( LOG_USER|LOG_WARNING, "WARNING", buf );
  return;
  }

void Notice( const char* fmt, ... )
  {
  va_list arglist;
  char buf[BIGBUF];

  va_start( arglist, fmt );
  vsnprintf( buf, sizeof(buf)-1, fmt, arglist );
  va_end( arglist );

  mysyslog( LOG_USER|LOG_NOTICE, "NOTICE", buf );
  return;
  }

