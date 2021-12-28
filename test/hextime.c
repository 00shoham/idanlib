#include "base.h"

int main( int argc, char** argv)
  {
  char buf[BUFLEN];

  time_t t = time(NULL);
  (void)DateTimeStr( buf, sizeof(buf)-1, 1, t );
  printf("NOW: %lx (%s)\n", (long)t, buf );

  for( int i=1; i<argc; ++i )
    {
    if( strcmp( argv[i], "-hex" )==0 && i+1<argc )
      {
      ++i;
      long x = 0;
      if( sscanf( argv[i], "%lx", &x )==1 )
        {
        (void)DateTimeStr( buf, sizeof(buf)-1, 1, x );
        printf( "%lx ==> %s\n", x, buf );
        if( x>(long)t )
          {
          printf( "%d seconds in the future\n", (int)(x-(long)t) );
          }
        else
          {
          printf( "%d seconds ago\n", (int)((long)t-x) );
          }
        }
      else
        {
        Error( "String [%s] does not parse as hex number", argv[i] );
        }
      }
    else if( strcmp( argv[i], "-h" )==0 )
      {
      printf("USAGE: %s [-hex NNNNNNNN]\n", argv[0] );
      exit( 0 );
      }
    }

  return 0;
  }
