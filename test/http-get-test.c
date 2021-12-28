#include "base.h"

#define DEFAULT_HOST "thermostat"
#define DEFAULT_PORT 80
#define DEFAULT_URL "/tstat"

#define MODE_OFF  0
#define MODE_HEAT 1
#define MODE_COOL 2
#define MODE_AUTO 3

#define TRUE 1
#define FALSE 0

#define CTOF(TEMP) ((TEMP)*9.0/5.0+32.0)
#define ROUND5(N) (round( (N)*2.0 )/2.0)

int main( int argc, char** argv )
  {
  char json[BIGBUF];
  char post[BIGBUF];
  char response[BIGBUF];

  int err = 0;

  glob_argc = argc;
  glob_argv = argv;

  /*
  char* host = DEFAULT_HOST;
  int portNum = DEFAULT_PORT;
  char* relURL = DEFAULT_URL;
  */

  /* (void)JSONToHTTPPost( DEFAULT_URL, "{}", post, sizeof(post)-1 );A */
  snprintf( post, sizeof(post)-1,
            "GET %s HTTP/1.1\r\n\r\n", DEFAULT_URL );
  err = HTTPSendReceive( DEFAULT_HOST, DEFAULT_PORT,
                         post, response, sizeof(response)-1 );
  printf( "HTTP GET:\n" );
  printf( "%s\n", post );
  printf( "RESULT:\n" );
  printf( "%d\n", err );
  printf( "RESPONSE:\n" );
  printf( "%s\n", response );

  double temp = 16.0;

  char* ptr = NULL;
  int degF = (int) CTOF( temp );
  _TAG_VALUE* argsList = NULL;

  argsList = TagIntList( ptr, "tmode", MODE_HEAT, "t_heat", degF, NULL, 0 );
  (void)ListToJSON( argsList, json, sizeof(json)-1 );
  (void)JSONToHTTPPost( "/tstat", json, post, sizeof(post)-1 );
  printf("%s\n", post );

  err = HTTPSendReceive( DEFAULT_HOST, DEFAULT_PORT,
                         post, response, sizeof(response)-1 );
  printf( "HTTP GET:\n" );
  printf( "%s\n", post );
  printf( "RESULT:\n" );
  printf( "%d\n", err );
  printf( "RESPONSE:\n" );
  printf( "%s\n", response );

  FreeTagValue( argsList );
  argsList = NULL;

  /*
  argsList = TagIntList( ptr, "hold", FALSE, "override", 0, "t_heat", 0, NULL, 0 );
  (void)ListToJSON( argsList, json, sizeof(json)-1 );
  (void)JSONToHTTPPost( "/tstat", json, post, sizeof(post)-1 );
  printf("%s\n", post );
  */

  return 0;
  }
