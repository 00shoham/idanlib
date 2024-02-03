#include "base.h"

#define SAMPLE "{ \"list\": [ \"abc\", 123, \"def\", \"abc \\\"quoted\\\" def\", \"\\\"\", \"\\\"\\\"\", \"\" ] }"


int main( int argc, char** argv )
  {
  char json[BIGBUF];
  strncpy( json, SAMPLE, sizeof(json)-1 );

  printf( "JSON string: %s\n", json );

  _TAG_VALUE* parsed = ParseJSON( json );
  if( parsed==NULL ) Error( "Failed to parse JSON" );

  printf( "Parsed JSON:\n" );
  PrintTagValue( 0, parsed );

  FreeTagValue( parsed );
  parsed = NULL;

  return 0;
  }
