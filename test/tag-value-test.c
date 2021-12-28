#include "base.h"

int main()
  {
  _TAG_VALUE* list = NULL;

  printf( "Basics:\n" );
  list = NewTagValue( "string", "string-value", list, 0 );
  list = NewTagValueInt( "int", 123, list, 0 );
  list = NewTagValueDouble( "float", 456.789, list, 0 );
  PrintTagValue( 2, list );
  printf("\n");

  printf( "Duplicates:\n" );
  list = NewTagValue( "string", "string-value-2", list, 1 );
  list = NewTagValueInt( "int", 1234, list, 1 );
  list = NewTagValueDouble( "float", 3456.789, list, 1 );
  PrintTagValue( 2, list );
  printf("\n");

  printf( "List:\n" );
  _TAG_VALUE* subList = NULL;
  subList = NewTagValue( "string", "string-value", subList, 0 );
  subList = NewTagValueInt( "int", 123, subList, 0 );
  subList = NewTagValueDouble( "float", 456.789, subList, 0 );
  list = NewTagValueList( "list", subList, list, 0 );
  PrintTagValue( 2, list );
  printf("\n");

  printf( "Guess Types:\n" );
  list = NewTagValueGuessType( "guess-int", "15", list, 0 );
  list = NewTagValueGuessType( "guess-float", "99.11", list, 0 );
  list = NewTagValueGuessType( "guess-string", "Hello, world!", list, 0 );
  list = NewTagValueNull( "null item", list, 0 );
  PrintTagValue( 2, list );
  printf("\n");

  printf( "Compare list to self: %d\n", CompareTagValueList( list, list ) );
  printf( "Compare list to sub: %d\n", CompareTagValueList( list, subList ) );

  printf( "Copy:\n" );
  _TAG_VALUE* copy = CopyTagValueList( list );
  PrintTagValue( 2, copy );
  printf("\n");

  printf( "Compare list to copy: %d\n", CompareTagValueList( list, copy ) );

  printf( "Deleted guess-int, guess-float and list:\n" );
  list = DeleteTagValue( list, "guess-int" );
  list = DeleteTagValue( list, "guess-float" );
  list = DeleteTagValue( list, "list" );
  PrintTagValue( 2, list );
  printf("\n");

  printf( "Compare list to copy: %d\n", CompareTagValueList( list, copy ) );
  printf( "BiDiComp list to copy: %d\n", CompareTagValueListBidirectional( list, copy ) );
  printf( "Compare copy to list: %d\n", CompareTagValueList( copy, list ) );

  FreeTagValue( list );
  FreeTagValue( copy );

  char strA[200];
  char strB[200];

  _TAG_VALUE* toDelete = NewTagValueNull( "time", NULL, 0 );
  strcpy( strA, "{\"temp\":70.00,\"tmode\":1,\"fmode\":0,\"override\":1,\"hold\":0,\"t_heat\":68.00,\"tstate\":0,\"fstate\":0,\"time\":{\"day\":5,\"hour\":16,\"minute\":3},\"t_type_post\":0}" );
  strcpy( strB, "{\"temp\":70.00,\"tmode\":1,\"fmode\":0,\"override\":1,\"hold\":0,\"t_heat\":68.00,\"tstate\":0,\"fstate\":0,\"time\":{\"day\":5,\"hour\":16,\"minute\":4},\"t_type_post\":0}" );

  int c1 = CompareJSON( strA, strB, NULL );
  printf( "Compare JSON - with time intact = %d\n", c1 );

  int c2 = CompareJSON( strA, strB, toDelete );
  printf( "Compare JSON - with time removed = %d\n", c2 );

  FreeTagValue( toDelete );

  return 0;
  }
