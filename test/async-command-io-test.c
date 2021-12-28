#include "base.h"

#undef DEBUG

#define LEN 500
#define MAX_DELAY 10

#define WEATHER_READ_TIMEOUT 2.0 /* seconds */
#define WEATHER_READ_MAX_TIMEOUT 10.0 /* seconds */
#define FAN_ON_THRESHOLD 4  /* degrees C warmer measured than target */
#define FAN_OFF_THRESHOLD 1 /* degrees C warmer measured than target */
#define COOL_AIR_DELTA   5  /* degrees C outside cooler than inside */

#define TESTCMD "/bin/bash /usr/local/bin/get-todays-forecast.sh Calgary"
int main()
  {
  char **bufs = NULL;
  bufs = SafeCalloc( 2, sizeof(char*), "multi-line-buffer 0" );
  bufs[0] = SafeCalloc( BUFLEN, sizeof(char), "multi-line-buffer 1" );
  bufs[1] = SafeCalloc( BUFLEN, sizeof(char), "multi-line-buffer 2" );
  Notice( "Running [%s]", TESTCMD );
  int err = ReadLinesFromCommand( TESTCMD, bufs, 2, BUFLEN, WEATHER_READ_TIMEOUT, WEATHER_READ_MAX_TIMEOUT );

  double highTemp = INVALID_TEMPERATURE;
  double lowTemp = INVALID_TEMPERATURE;
  if( err==0 && bufs[0][0]!=0 && bufs[1][0]!=0
      && sscanf( bufs[0], "High:%lf", &highTemp )==1
      && sscanf( bufs[1], "Low:%lf", &lowTemp )==1
      && SANE_TEMPERATURE( highTemp )
      && SANE_TEMPERATURE( lowTemp ) )
    {
    Notice( "Forecast is %.1lf low -- %.1lf high", lowTemp, highTemp );
    }
  else
    {
    Warning( "Failed to read weather forecast -- %d - [%s]/[%s]", err, NULLPROTECT( bufs[0] ), NULLPROTECT( bufs[1] ) );
    }

  FREE( bufs[1] );
  FREE( bufs[0] );
  FREE( bufs );

  return 0;
  }
