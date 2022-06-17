#ifndef _INCLUDE_DATE_UTILS
#define _INCLUDE_DATE_UTILS

#define DAY_IN_SECONDS (24 * 60 * 60)

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
int YearNow();
int DayOfWeek( time_t t );

typedef struct _mmdd
  {
  int year;
  int month;
  int day;
  } _MMDD;

int IsValidMMDD( char* value, _MMDD* date );
int EmptyMMDD( _MMDD* date );
time_t MMDDToTime( _MMDD* date );
int TimeToMMDD( time_t t, _MMDD* date );
int NumberOfDays( _MMDD* first, _MMDD* last );
int MonthFromTime( time_t t );
int YearFromTime( time_t t );
int NumberOfMonths( _MMDD* first, _MMDD* last );

#endif
