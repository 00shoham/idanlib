#include "base.h"

#define SAMPLE "{\"FILENAME\":\"config\",\"LINENUMBER\":2,\"TEXT\":\"#include \\\"holidays2.ini\\\"\"}"


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
