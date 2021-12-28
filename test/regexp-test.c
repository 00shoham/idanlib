#include "base.h"

int main()
  {
  char* re = RE_POSINT;
  char* str = "15";
  int m = 0;

  m = StringMatchesRegex( re, str );
  printf( "StringMatchesRegex( %s, %s ) ==> %d\n", re, str, m );

  str = "%DEGF%";
  m = StringMatchesRegex( re, str );
  printf( "StringMatchesRegex( %s, %s ) ==> %d\n", re, str, m );

  str = "15.3";
  m = StringMatchesRegex( re, str );
  printf( "StringMatchesRegex( %s, %s ) ==> %d\n", re, str, m );

  str = "0";
  m = StringMatchesRegex( re, str );
  printf( "StringMatchesRegex( %s, %s ) ==> %d\n", re, str, m );

  str = "023";
  m = StringMatchesRegex( re, str );
  printf( "StringMatchesRegex( %s, %s ) ==> %d\n", re, str, m );

  str = "true";
  m = StringMatchesRegex( "(true|false)", str );
  printf( "StringMatchesRegex( %s, %s ) ==> %d\n", re, str, m );

  str = "false";
  m = StringMatchesRegex( "(true|false)", str );
  printf( "StringMatchesRegex( %s, %s ) ==> %d\n", re, str, m );

  str = "hello";
  m = StringMatchesRegex( "(true|false)", str );
  printf( "StringMatchesRegex( %s, %s ) ==> %d\n", re, str, m );

  return 0;
  }
