#include "utils.h"

void mysyslog( int level, char* prefix, char* msg )
  {
  if( inDaemon || inCGI )
    {
    if( prefix!=NULL && strcmp(prefix,"NOTICE")==0 ) level = LOG_NOTICE;
    else if( prefix!=NULL && strcmp(prefix,"WARNING")==0 ) level = LOG_WARNING;
    else if( prefix!=NULL && strcmp(prefix,"ERROR")==0 ) level = LOG_ERR;
    syslog( level, "%s", msg );
    }
  else
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
    fputs( longtime, stdout );
    fputs( msg, stdout );
    fputs( "\n", stdout );
    fflush( stdout );
    }
  }

void Error( char* fmt, ... )
  {
  va_list arglist;
  char buf[BIGBUF];

  va_start( arglist, fmt );
  vsnprintf( buf, sizeof(buf), fmt, arglist );
  va_end( arglist );

  if( inCGI>1 )
    fputs( "Content-Type: text/html\r\n\r\n", stdout );

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

void APIError( char* methodName, int errorCode, char* fmt, ... )
  {
  va_list arglist;
  char errmsg[BUFLEN];
  char* ptr = errmsg;
  char* end = errmsg+sizeof(errmsg)-1;
  /*
  snprintf( ptr, end-ptr, "Error in API call.  " );
  ptr += strlen(ptr );
  */

  va_start( arglist, fmt );
  vsnprintf( ptr, end-ptr, fmt, arglist );
  va_end( arglist );

  mysyslog( LOG_USER|LOG_ERR, "APIERROR", errmsg );

  char responseBuf[BUFLEN];
  _TAG_VALUE* response = NULL;
  response = NewTagValueInt( "code", errorCode, response, 1 );
  response = NewTagValue( "result", errmsg, response, 1 );
  ListToJSON( response, responseBuf, sizeof(responseBuf)-1 );

  fputs( responseBuf, stdout );
  fputs( "\r\n", stdout );
  fflush( stdout );

  exit(1);
  }

void Warning( char* fmt, ... )
  {
  va_list arglist;
  char buf[BIGBUF];

  va_start( arglist, fmt );
  vsnprintf( buf, sizeof(buf), fmt, arglist );
  va_end( arglist );

  mysyslog( LOG_USER|LOG_WARNING, "WARNING", buf );
  return;
  }

void Notice( char* fmt, ... )
  {
  va_list arglist;
  char buf[BIGBUF];

  va_start( arglist, fmt );
  vsnprintf( buf, sizeof(buf), fmt, arglist );
  va_end( arglist );

  mysyslog( LOG_USER|LOG_NOTICE, "NOTICE", buf );
  return;
  }
