#include "base.h"

int main( int argc, char** argv )
  {
  char json[BUFLEN];

  FILE* f = popen( "tail -n 1 canmore-main.json", "r" );
  if( f==NULL ) Error( "Cannot run tail cmd" );
  if( fgets( json, sizeof(json)-1, f )!=json ) Error( "Cannot read from tail" );
  fclose( f );

  char* ptr = NULL;
  char* thermostatID = strtok_r( json, " ", &ptr );
  char* hexTime = strtok_r( NULL, " ", &ptr );

  printf( "Thermostat: %s\n", thermostatID );

  char friendlyTime[BUFLEN];
  long t;
  if( sscanf( hexTime, "%lx", &t )!=1 ) Error( "Invalid time string [%s]", hexTime );
  (void)DateTimeStr( friendlyTime, sizeof( friendlyTime )-1, 0, (time_t)t );
  printf( "Time: %s\n", friendlyTime );

  _TAG_VALUE* parsed = ParseJSON( ptr );
  if( parsed==NULL ) Error( "Failed to parse JSON" );

  printf("Parsed JSON:\n");
  PrintTagValue( 0, parsed);

  char* tstate = GetTagValueSafe( parsed, "tstate", "[01]" );
  if( tstate!=NULL )
    {
    if( strcmp( tstate, "0" )==0 ) printf( "Heating off\n" );
    else printf( "Furnace running\n" );
    }

  if( NOTEMPTY( tstate ) && strcmp( tstate, "1" )==0 )
    {
    printf( "Fan running\n" );
    }
  else
    {
    char* fstate = GetTagValueSafe( parsed, "fstate", RE_BOOL );
    if( fstate!=NULL )
      {
      if( strcmp( fstate, "0" )==0 ) printf( "Fan off\n" );
      else printf( "Fan running\n" );
      }
    }

  char* targetTempStr = GetTagValueSafe( parsed, "t_heat", RE_FLOAT );
  if( targetTempStr!=NULL )
    {
    double targetTempF = atof( targetTempStr );
    double targetTempC = FTOC( targetTempF );
    targetTempC = ROUND5( targetTempC );
    printf( "Target temperature: %.1lf degC\n", targetTempC );
    }

  char* currentTempStr = GetTagValueSafe( parsed, "temp", RE_FLOAT );
  if( currentTempStr!=NULL )
    {
    double currentTempF = atof( currentTempStr );
    double currentTempC = FTOC( currentTempF );
    currentTempC = ROUND5( currentTempC );
    printf( "current temperature: %.1lf degC\n", currentTempC );
    }

  /*
  "t_type_post": "0"
  "time": 0
    "minute": "20"
    "hour": "21"
    "day": "3"
  "fstate": "0"
  "tstate": "1"
  "t_heat": "70.00"
  "hold": "0"
  "override": "0"
  "fmode": "0"
  "tmode": "1"
  "temp": "64.00"
  */

  return 0;
  }
