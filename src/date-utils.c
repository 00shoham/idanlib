#include "utils.h"

int IsRecent( time_t when, int maxAge )
  {
  int nSeconds = (int)(time(NULL) - when);
  nSeconds = abs( nSeconds );
  if( nSeconds < maxAge )
    return 1;
  return 0;
  }

char* DateStr( time_t t, char* buf, int bufLen )
  {
  if( buf==NULL || bufLen<11 )
    Error( "DateStr called with too-small buffer" );
  struct tm *tmp = localtime( &t );
  snprintf( buf, bufLen, "%04d-%02d-%02d",
            tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday );
  return buf;
  }

char* DateNow( char* buf, int bufLen )
  {
  time_t now = time(NULL);
  return DateStr( now, buf, bufLen );
  }

char* TimeStr( time_t t, char* buf, int bufLen, int showSeconds )
  {
  if( buf==NULL || bufLen<9 )
    Error( "TimeStr called with too-small buffer" );
  struct tm *tmp = localtime( &t );
  if( showSeconds )
    {
    snprintf( buf, bufLen, "%02d:%02d:%02d",
              tmp->tm_hour, tmp->tm_min, tmp->tm_sec );
    }
  else
    {
    snprintf( buf, bufLen, "%02d:%02d",
              tmp->tm_hour, tmp->tm_min );
    }
  return buf;
  }

char* TimeNow( char* buf, int bufLen, int showSeconds )
  {
  time_t now = time(NULL);
  return TimeStr( now, buf, bufLen, showSeconds );
  }

char* DateTimeNow( char* buf, int bufLen, int showSeconds )
  {
  if( buf==NULL || bufLen<21 )
    Error( "DateTimeNow called with too-small buffer" );
  (void)DateNow( buf, 11 );
  buf[10] = ' ';
  (void)TimeNow( buf+11, 9, showSeconds );
  return buf;
  }

char* DateTimeStr( char* buf, int bufLen, int showSeconds, time_t t )
  {
  if( buf==NULL || bufLen<21 )
    Error( "DateTimeNow called with too-small buffer" );
  (void)DateStr( t, buf, 11 );
  buf[10] = ' ';
  (void)TimeStr( t, buf+11, 9, showSeconds );
  return buf;
  }

int ValidDate( char* when )
  {
  int cy, m, d;
  if( sscanf( when, "%d-%d-%d", &cy, &m, &d )!=3 )
    return -1;
  if( cy<1900 || cy>2040 )
    return -2;
  if( m<1 || m>12 )
    return -3;
  if( d<1 || d>31 )
    return -4;
  return 0;
  }

int ValidTime( char* when )
  {
  int h=0, m=0, s=0;
  if( sscanf( when, "%d:%d:%d", &h, &m, &s )!=3
      && sscanf( when, "%d:%d", &h, &m )!=2 )
    return -1;
  if( h<0 || h>23 )
    return -2;
  if( m<0 || m>59 )
    return -3;
  if( s<0 || s>59 )
    return -4;
  return 0;
  }

long long TimeInMicroSeconds()
  {
  struct timeval te; 
  gettimeofday(&te, NULL); // get current time
  long long us = te.tv_sec*1000000LL + te.tv_usec;
  return us;
  }

/*
  Time strings converted to time_t.
  Either relative to "now" or absolute time/date.
  Formats as follows:
  +20m
  +2h15m
  2021-07-02 19:00:00
*/
time_t ParseTimeString( char* str )
  {
  if( EMPTY( str ) )
    return 0;

  time_t tNow = time(NULL);

  int Y=0;
  int M=0;
  int D=0;
  int h=0;
  int m=0;
  int s=0;

  if( str[0]=='+' )
    {
    if( StringMatchesRegex( "[0-9]+h[0-9]+m", str+1 )==0
        && sscanf( str+1, "%dh%dm", &h, &m )==2 )
      return tNow + h*60*60 + m*60;
    else if( StringMatchesRegex( "[0-9]+h", str+1 )==0
        && sscanf( str+1, "%dh", &h )==1 )
      return tNow + h*60*60;
    else if( StringMatchesRegex( "[0-9]+m", str+1 )==0
        && sscanf( str+1, "%dm", &m )==1 )
      return tNow + m*60;
    else if( StringMatchesRegex( "[0-9]+s", str+1 )==0
        && sscanf( str+1, "%ds", &s )==1 )
      return tNow + s;
    else if( StringMatchesRegex( "[0-9]+m[0-9]+s", str+1 )==0
        && sscanf( str+1, "%dm%ds", &m, &s )==2 )
      return tNow + m*60 + s;
    else if( StringMatchesRegex( "[0-9]+h[0-9]+m[0-9]+s", str+1 )==0
        && sscanf( str+1, "%dh%dm%ds", &h, &m, &s )==3 )
      return tNow + h*60*60 + m*60 + s;
    else
      return -1;
    }

  char* sp = strchr( str, ' ' );
  if( sp==NULL )
    return -1;

  if( ValidDate( str )==0
      && ValidTime( sp+1 )==0
      && sscanf( str, "%d-%d-%d", &Y, &M, &D )==3
      && ( sscanf( sp+1, "%d:%d:%d", &h, &m, &s )==3
           || sscanf( sp+1, "%d:%d", &h, &m )==2 ) )
    {
    struct tm *tmPtr = localtime( &tNow );
    struct tm tStr;
    memset( &tStr, 0, sizeof( tStr ) );
    tStr.tm_year = Y - 1900;
    tStr.tm_mon = M - 1;
    tStr.tm_mday = D;
    tStr.tm_hour = h;
    tStr.tm_min = m;
    tStr.tm_sec = s;
    tStr.tm_isdst = tmPtr->tm_isdst;
    return mktime( &tStr );
    }

  return -1;
  }

int YearNow()
  {
  time_t tNow = time(NULL);
  struct tm* tmPtr = localtime( &tNow );
  return tmPtr->tm_year + 1900;
  }

int IsValidMMDD( char* value, _MMDD* date )
  {
  int y = -1;
  int m = -1;
  int d = -1;
  if( EMPTY( value ) )
    return -1;

  if( strcasecmp( value, "today" )==0 )
    {
    time_t tNow = time(NULL);
    struct tm* tmPtr = localtime( &tNow );
    y = tmPtr->tm_year + 1900;
    m = tmPtr->tm_mon + 1;
    d = tmPtr->tm_mday;
    goto SET_DATE;
    }

  int isMMDD = 0;
  if( isdigit( value[0] )
      && isdigit( value[1] )
      && ( value[2] == '-' || value[2] == ':' )
      && isdigit( value[3] )
      && isdigit( value[4] )
      && value[5]==0 )
    isMMDD = 1;

  int isCCYYMMDD = 0;
  if( isdigit( value[0] )
      && isdigit( value[1] )
      && isdigit( value[2] )
      && isdigit( value[3] )
      && ( value[4] == '-' || value[4] == ':' )
      && isdigit( value[5] )
      && isdigit( value[6] )
      && ( value[7] == '-' || value[7] == ':' )
      && isdigit( value[8] )
      && isdigit( value[9] )
      && value[10]==0 )
    isCCYYMMDD = 1;

  if( isMMDD==0 && isCCYYMMDD==0 )
    return -2;

  if( isMMDD )
    {
    y = 0;
    m = 10 * (value[0] - '0') + (value[1] - '0');
    d = 10 * (value[3] - '0') + (value[4] - '0');
    }
  else
    {
    y =  1000 * (value[0]-'0')
      +   100 * (value[1]-'0')
      +    10 * (value[2]-'0')
      +         (value[3]-'0');
    m = 10 * (value[5]-'0') + value[6]-'0';
    d = 10 * (value[8]-'0') + value[9]-'0';
    }

  if( y!=0 )
    {
    if( y<2000 || y>2037 )
      return -3;
    }
  if( m<1 || m>12 )
    return -4;
  if( d<1 || d>31 )
    return -5;

  SET_DATE:
  if( date!=NULL )
    {
    date->year = y;
    date->month = m;
    date->day = d;
    }

  return 0;
  }

int EmptyMMDD( _MMDD* date )
  {
  if( date==NULL )
    return 0;  /* no date is an empty date */
  if( date->year!=0
      || date->month!=0
      || date->day!=0 )
    return -1;
  return 0;
  }

time_t MMDDToTime( _MMDD* date )
  {
  if( date==NULL )
    return time(NULL); /* why not? */

  time_t tNow = time(NULL);
  int Y=date->year;
  int M=date->month;
  int D=date->day;

  struct tm *tmPtr = localtime( &tNow );
  struct tm tStr;
  memset( &tStr, 0, sizeof( tStr ) );
  tStr.tm_year = Y - 1900;
  tStr.tm_mon = M - 1;
  tStr.tm_mday = D;
  tStr.tm_hour = 2;
  tStr.tm_min = 0;
  tStr.tm_sec = 0;
  tStr.tm_isdst = tmPtr->tm_isdst;
  return mktime( &tStr );
  }

int TimeToMMDD( time_t t, _MMDD* date )
  {
  if( date==NULL )
    return -1;

  struct tm *tmPtr = localtime( &t );
  date->year = tmPtr->tm_year + 1900;
  date->month = tmPtr->tm_mon + 1;
  date->day = tmPtr->tm_mday;

  return 0;
  }

int DayOfWeek( time_t t )
  {
  struct tm *tmPtr = localtime( &t );
  return tmPtr->tm_wday;
  }
