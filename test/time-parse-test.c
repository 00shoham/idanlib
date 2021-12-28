#include "base.h"

void Test( char* timeStr )
  {
  time_t tNow = time(NULL);
  printf( "Time = [%s]\n", timeStr );
  time_t t = ParseTimeString( timeStr );
  if( t==-1 )
    printf("Error!  Bad time\n");
  else
    {
    printf( "Seconds = %d from now\n", (int)(t - tNow) );
    char buf[100];
    (void)DateTimeStr( buf, sizeof(buf)-1, 1, t );
    printf( "Full description = %s\n", buf );
    }
  }

int main()
  {
  Test( "+20m" );
  Test( "+2h15m" );
  Test( "2021-07-02 19:00:00" );
  Test( "hello" );
  Test( "2199-07-02 19:00:00" );
  Test( "2022-07-02 19:" );
  }
