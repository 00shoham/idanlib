#include "base.h"

#define CMD "/bin/bash /usr/local/bin/irrigation-safe.sh Calgary"

int main()
  {
  char buf[BUFLEN];

  int err = ReadLineFromCommand( CMD, buf, sizeof(buf)-1, 1, 5 );
  if( err==0 && NOTEMPTY( buf ) )
    {
    char* word = StripEOL( buf );
    if( NOTEMPTY( word ) )
      Warning( "Ran [%s] and got [%s]", CMD, word );
    }
  else
    Warning( "Tried to run [%s] but got error %d:%d:%s", CMD, err, errno, strerror( errno ) );

  return 0;
  }
