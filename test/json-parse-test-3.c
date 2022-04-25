#include "base.h"

#define SAMPLE "{\"ison\":false,\"has_timer\":false,\"timer_started\":0,\"timer_duration\":0,\"timer_remaining\":0,\"overpower\":false,\"source\":\"input\"}"


int main( int argc, char** argv )
  {
  char json[BIGBUF];
  strncpy( json, SAMPLE, sizeof(json)-1 );

  _TAG_VALUE* parsed = ParseJSON( json );
  if( parsed==NULL ) Error( "Failed to parse JSON" );

  printf( "Parsed JSON:\n" );
  PrintTagValue( 0, parsed );

  FreeTagValue( parsed );
  parsed = NULL;

  return 0;
  }
