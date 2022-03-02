#include "base.h"

int main( int argc, char** argv )
  {
  char json[BIGBUF];

  FILE* f = fopen( "sample.json", "r" );
  if( f==NULL )
    Error( "Cannot read sample.json" );

  char* ptr = json;
  char* endp = json + sizeof(json) - 1;

  int n=0;
  while( (n=fread( ptr, 1, endp-ptr-1, f ))>0 )
    {
    ptr += n;
    *ptr = 0;
    }
  fclose( f );

  printf( "Raw JSON: [%s]\n", json );

  _TAG_VALUE* parsed = ParseJSON( json );
  if( parsed==NULL ) Error( "Failed to parse JSON" );

  printf( "Parsed JSON:\n" );
  PrintTagValue( 0, parsed );

  FreeTagValue( parsed );
  parsed = NULL;

  printf("\n\n============================================================\n\n");

  f = fopen( "sample.2.json", "r" );
  if( f==NULL )
    Error( "Cannot read sample.json" );

  ptr = json;
  endp = json + sizeof(json) - 1;

  n=0;
  while( (n=fread( ptr, 1, endp-ptr-1, f ))>0 )
    {
    ptr += n;
    *ptr = 0;
    }
  fclose( f );

  printf( "Raw JSON: [%s]\n", json );

  parsed = ParseJSON( json );
  if( parsed==NULL ) Error( "Failed to parse JSON" );

  printf( "Parsed JSON:\n" );
  PrintTagValue( 0, parsed );

  return 0;
  }
