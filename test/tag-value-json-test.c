#include "base.h"

#define SIMPLE_TEST "{\"temp\":70.00, \"tmode\":1, \"fmode\":0, \"override\":0, \"hold\":0, \"t_heat\":70.00}"

#define HARDER_TEST\
   "{\"temp\":70.00,"\
   "\"tmode\":1,"\
   "\"fmode\":0,"\
   "\"override\":0,"\
   "\"hold\":0,"\
   "\"t_heat\":70.00,"\
   "\"tstate\":0,"\
   "\"fstate\":0,"\
   "\"time\":{\"day\":0, \"hour\":10, \"minute\":44},"\
   "\"t_type_post\":0}"

#define API_TEST\
  "{\"0\":{\"time\":null,\"action\":\"set-temperature\",\"thermostat\":\"abc123\",\"target-temp\":26.0},\"1\":{\"time\":null,\"action\":\"set-temperature\",\"thermostat\":\"abc123\",\"target-temp\":23}}"

#define ARRAY_TEST_A\
  "[{\"time\":null,\"action\":\"set-temperature\",\"thermostat\":\"abc123\",\"target-temp\":26.0},{\"time\":null,\"action\":\"set-temperature\",\"thermostat\":\"abc123\",\"target-temp\":23}]"

#define ARRAY_TEST_B\
  "[ 1, 2, 3, 4 ]"

#define ARRAY_TEST_C\
  "{ \"one\" : [ 1, 2, 3, 4 ], \"two\" : \"string\" }"

#define ARRAY_TEST_D\
  "{ \"one\" : [ 1, 2, 3, { \"three\":[5,6,7] } ], \"two\" : \"string\" }"

#define ARRAY_TEST_E\
  "[{\"time\":null,\"action\":\"set-temperature\",\"device\":\"thermostat-1\",\"targetTemp\":26},{\"time\":null,\"action\":\"set-temperature\",\"device\":\"thermostat-1\",\"targetTemp\":23}]"

#define API_TEST_A\
  "{ \"result\":\"success\", \"code\": 0, \"LOCATIONS\" : [ \"Calgary home\", \"Canmore home\" ] }"

#define API_TEST_B "\
{ \"result\":\"success\", \"code\": 0, \"history\" :\
   [\
     { \"when\": \"2021-06-12 13:02\", \"temp\": 19.4, \"state\": 0 },\
     { \"when\": \"2021-06-12 13:44\", \"temp\": 19.7, \"state\": 0 }\
   ]\
}"

int main()
  {
  char buf[BUFLEN];
  _TAG_VALUE* tv = NULL;

  printf("\n\nInput: >>>%s<<<:\n", SIMPLE_TEST );
  tv = ParseJSON( SIMPLE_TEST );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );
  FreeTagValue( tv );

  printf("\n\nInput: >>>%s<<<:\n", HARDER_TEST );
  tv = ParseJSON( HARDER_TEST );
  printf("Parsed output - tv:\n");
  PrintTagValue( 2, tv );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );

  _TAG_VALUE* tv2 = NULL;
  tv2 = ParseJSON( HARDER_TEST );
  printf("Parsed output - tv2:\n");
  PrintTagValue( 2, tv2 );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );

  int c = CompareTagValueList( tv, tv2 );
  printf( "First comparison (should be same) is %d\n", c );

  tv2 = DeleteTagValue( tv2, "time" );
  printf("With 'time' removed:\n");
  PrintTagValue( 2, tv2 );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );

  int c1 = CompareTagValueList( tv2, tv );
  int c2 = CompareTagValueList( tv, tv2 );
  c = CompareTagValueListBidirectional( tv, tv2 );
  printf( "Second comparison (should be different) is %d | %d --> %d\n", c1, c2, c );

  printf("\n\nInput: >>>%s<<<:\n", API_TEST );
  tv = ParseJSON( API_TEST );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );
  FreeTagValue( tv );

  printf("\n\nInput: >>>%s<<<:\n", ARRAY_TEST_A );
  tv = ParseJSON( ARRAY_TEST_A );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );
  FreeTagValue( tv );

  printf("\n\nInput: >>>%s<<<:\n", ARRAY_TEST_B );
  tv = ParseJSON( ARRAY_TEST_B );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );
  FreeTagValue( tv );

  printf("\n\nInput: >>>%s<<<:\n", ARRAY_TEST_C );
  tv = ParseJSON( ARRAY_TEST_C );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );
  FreeTagValue( tv );

  printf("\n\nInput: >>>%s<<<:\n", ARRAY_TEST_D );
  tv = ParseJSON( ARRAY_TEST_D );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );
  FreeTagValue( tv );

  printf("\n\nInput: >>>%s<<<:\n", ARRAY_TEST_E );
  tv = ParseJSON( ARRAY_TEST_E );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );
  FreeTagValue( tv );

  printf("\n\nInput: >>>%s<<<:\n", API_TEST_A );
  tv = ParseJSON( API_TEST_A );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );
  FreeTagValue( tv );

  printf("\n\nInput: >>>%s<<<:\n", API_TEST_B );
  tv = ParseJSON( API_TEST_B );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );
  ListToJSON( tv, buf, sizeof(buf)-1 );
  printf("Back to JSON: '%s'\n", buf );
  FreeTagValue( tv );

  return 0;
  }

