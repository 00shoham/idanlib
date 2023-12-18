#include "utils.h"

/* this parser only works for simple JSON - without arrays. */

#define OPENBR '{'
#define CLOSEBR '}'
#define OPENSQ '['
#define CLOSESQ ']'
#define QUOTE '"'
#define COMMA ','
#define COLON ':'
#define BS '\\'

/* {"temp":70.00,
    "tmode":1,
    "fmode":0,
    "override":0,
    "hold":0,
    "t_heat":70.00,
    "tstate":0,
    "fstate":0,
    "time":{"day":0, "hour":10, "minute":44},
    "t_type_post":0} */

enum parser_state
  {
  PS_PRE_OPENBR,
  PS_PRE_PAIR,
  PS_IN_PAIR_RAW_TAG,
  PS_IN_PAIR_QUOTED_TAG,
  PS_IN_PAIR_PRE_COLON,
  PS_IN_PAIR_PRE_VALUE,
  PS_IN_PAIR_RAW_VALUE,
  PS_IN_PAIR_QUOTED_VALUE,
  PS_IN_PAIR_PRE_COMMA,
  PS_PRE_LIST,
  PS_IN_LIST_QUOTED_VALUE,
  PS_IN_LIST_RAW_VALUE,
  PS_IN_LIST_PRE_COMMA
  };

int IsWhiteSpace( int c )
  {
  if( c==' ' || c=='\t' || c=='\r' || c=='\n' ) return 1;
  return 0;
  }

char* ParseStateName( enum parser_state state )
  {
  switch( state )
    {
    case PS_PRE_OPENBR: return "PS_PRE_OPENBR";
    case PS_PRE_PAIR: return "PS_PRE_PAIR";
    case PS_IN_PAIR_RAW_TAG: return "PS_IN_PAIR_RAW_TAG";
    case PS_IN_PAIR_QUOTED_TAG: return "PS_IN_PAIR_QUOTED_TAG";
    case PS_IN_PAIR_PRE_COLON: return "PS_IN_PAIR_PRE_COLON";
    case PS_IN_PAIR_PRE_VALUE: return "PS_IN_PAIR_PRE_VALUE";
    case PS_IN_PAIR_RAW_VALUE: return "PS_IN_PAIR_RAW_VALUE";
    case PS_IN_PAIR_QUOTED_VALUE: return "PS_IN_PAIR_QUOTED_VALUE";
    case PS_IN_PAIR_PRE_COMMA: return "PS_IN_PAIR_PRE_COMMA";
    case PS_PRE_LIST: return "PS_PRE_LIST";
    case PS_IN_LIST_QUOTED_VALUE: return "PS_IN_LIST_QUOTED_VALUE";
    case PS_IN_LIST_RAW_VALUE: return "PS_IN_LIST_RAW_VALUE";
    case PS_IN_LIST_PRE_COMMA: return "PS_IN_LIST_PRE_COMMA";
    }
  return "UNKNOWN";
  }

char* GetBracketedString( char* string )
  {
  char* src = string;
  int brDepth = 0;
  int sqDepth = 0;
  int inQuote = 0;
  int bsState = 0;
  int c = -1;
  for( ; *src!=0; ++src )
    {
    c = *src;
    if( inQuote )
      {
      if( c==BS )
        {
        bsState = bsState ? 0 : 1;
        }
      else if( c==QUOTE && bsState==0 )
        {
        inQuote = 0;
        }
      }
    else
      {
      if( c==OPENBR )
        {
        ++brDepth;
        }
      else if( c==CLOSEBR )
        {
        --brDepth;
        if( brDepth==0 && sqDepth==0 )
          {
          break;
          }
        }
      else if( c==OPENSQ )
        {
        ++sqDepth;
        }
      else if( c==CLOSESQ )
        {
        --sqDepth;
        if( brDepth==0 && sqDepth==0 )
          {
          break;
          }
        }
      else if( c==QUOTE )
        {
        inQuote = 1;
        bsState = 0;
        }
      }
    }

  if( inQuote==0 && bsState==0 && ( c==CLOSEBR || c==CLOSESQ ) )
    { /* got a good string */
    int len = src - string + 1;
    char* buf = (char*)malloc( len+1 );
    memcpy( buf, string, len );
    buf[len] = 0;
    return buf;
    }

  Warning( "Failed to find valid bracketed string in [%s]", string );
  return NULL;
  }

void TrimIntoSmallBuf( char* smallbuf, size_t nBytes, char* src )
  {
  memset( smallbuf, 0, nBytes );
  for( int i=0; i<nBytes; ++i )
    {
    int c = *src;
    if( c==0 )
      break;
    *(smallbuf++) = c;
    ++src;
    }
  }

void ParseWarning( enum parser_state state, int c, int cNum, char* ptr )
  {
  char smallbuf[10];

  TrimIntoSmallBuf( smallbuf, sizeof(smallbuf), ptr );

  Warning( "Problem parsing JSON: char '%c' at position %d (state %s) [%s...]",
           c, cNum, ParseStateName( state ), smallbuf );
  }

_TAG_VALUE* ParseJSON( const char* string )
  {
  enum parser_state state = PS_PRE_OPENBR;
  _TAG_VALUE* list = NULL;
  _TAG_VALUE* last = NULL;
  char* tag = NULL;
  char* value = NULL;

  /* printf( "ParseJSON( %s )\n", NULLPROTECT( string ) ); */

  if( EMPTY( string ) )
    {
    /* maybe that's sometimes okay */
    /* Warning( "Cannot parse empty JSON" ); */
    return NULL;
    }

  char* workingBuffer = strdup( string );

  int cNum = 0;
  int backSlashStatus = 0;
  for( char* ptr=workingBuffer; *ptr!=0; ++ptr )
    {
    int c = *ptr;
    /* printf("%03d - %s - %c\n", cNum, ParseStateName(state), c); */
    switch( state )
      {
      case PS_PRE_OPENBR:
        if( IsWhiteSpace( c ) )
          {}
        else if( c==OPENBR )
          {
          state = PS_PRE_PAIR;
          tag = NULL;
          value = NULL;
          }
        else if( c==OPENSQ )
          {
          state = PS_PRE_LIST;
          tag = NULL;
          value = NULL;
          }
        else
          {
          ParseWarning( state, c, cNum, ptr );
          free( workingBuffer );
          FreeTagValue( list );
          return NULL;
          }
        break;

      case PS_PRE_PAIR:
        if( IsWhiteSpace( c ) )
          {}
        else if( c==QUOTE )
          {
          tag = ptr+1;
          backSlashStatus = 0;
          state = PS_IN_PAIR_QUOTED_TAG;
          }
        else
          {
          tag = ptr;
          state = PS_IN_PAIR_RAW_TAG;
          }
        break;

      case PS_PRE_LIST:
        if( IsWhiteSpace( c ) )
          {}
        else if( c==QUOTE )
          {
          value = ptr+1;
          backSlashStatus = 0;
          state = PS_IN_LIST_QUOTED_VALUE;
          }
        else if( c==OPENBR )
          {
          char* inBR = GetBracketedString( ptr );
          if( inBR==NULL )
            {
            free( workingBuffer );
            FreeTagValue( list );
            return NULL;
            }
          last = NewTagValueList( NULL, ParseJSON( inBR ), NULL, 0 );
          list = AppendTagValue( list, last );
          ptr += strlen( inBR ) - 1;
          free( inBR );
          inBR = NULL;
          value = NULL;
          backSlashStatus = 0;
          state = PS_IN_LIST_PRE_COMMA;
          }
        else if( c==COMMA )
          { /* perhaps last item was a {...} expression? */
          }
        else
          {
          value = ptr;
          state = PS_IN_LIST_RAW_VALUE;
          }
        break;

      case PS_IN_LIST_QUOTED_VALUE:
        if( c==BS )
          {
          backSlashStatus = backSlashStatus ? 0 : 1;
          }
        else if( c==QUOTE && ! backSlashStatus )
          {
          *ptr = 0;
          last = NewTagValueGuessType( NULL, value, NULL, 0 );
          list = AppendTagValue( list, last );
          state = PS_IN_LIST_PRE_COMMA;
          }
        break;

      case PS_IN_LIST_RAW_VALUE:
        if( IsWhiteSpace( c ) )
          {
          *ptr = 0;
          if( NOTEMPTY( value ) && strcasecmp( value, "NULL" )==0 )
            {
            last = NewTagValueNull( NULL, NULL, 0 );
            list = AppendTagValue( list, last );
            }
          else
            {
            last = NewTagValueGuessType( NULL, value, NULL, 0 );
            list = AppendTagValue( list, last );
            }
          state = PS_IN_LIST_PRE_COMMA;
          }
        else if( c==COMMA )
          {
          *ptr = 0;
          last = NewTagValueGuessType( NULL, value, NULL, 0 );
          list = AppendTagValue( list, last );
          state = PS_PRE_LIST;
          }
        else if( c==CLOSESQ )
          {
          *ptr = 0;
          last = NewTagValueGuessType( NULL, value, NULL, 0 );
          list = AppendTagValue( list, last );
          /* QQQ if we have a tag, make the list its value. */
          state = PS_IN_PAIR_PRE_COMMA;
          }
        break;

      case PS_IN_LIST_PRE_COMMA:
        if( IsWhiteSpace( c ) ) { }
        else if( c==COMMA ) { state = PS_PRE_LIST; }
        else if( c==CLOSESQ ) { state = PS_IN_PAIR_PRE_COMMA; }
        else
          {
          ParseWarning( state, c, cNum, ptr );
          free( workingBuffer );
          FreeTagValue( list );
          return NULL;
          }
        break;

      case PS_IN_PAIR_RAW_TAG:
        if( IsWhiteSpace( c ) )
          {
          *ptr = 0;
          state = PS_IN_PAIR_PRE_COLON;
          }
        else if( c==COLON )
          {
          *ptr = 0;
          state = PS_IN_PAIR_PRE_VALUE;
          }
        break;

      case PS_IN_PAIR_QUOTED_TAG:
        if( c==BS )
          {
          backSlashStatus = backSlashStatus ? 0 : 1;
          }
        else if( c==QUOTE && ! backSlashStatus )
          {
          *ptr = 0;
          state = PS_IN_PAIR_PRE_COLON;
          }
        break;

      case PS_IN_PAIR_PRE_COLON:
        if( IsWhiteSpace( c ) )
          { }
        else if( c==COLON )
          {
          state = PS_IN_PAIR_PRE_VALUE;
          }
        else
          {
          ParseWarning( state, c, cNum, ptr );
          free( workingBuffer );
          FreeTagValue( list );
          return NULL;
          }
        break;

      case PS_IN_PAIR_PRE_VALUE:
        if( IsWhiteSpace( c ) )
          {}
        else if( c==QUOTE )
          {
          value = ptr+1;
          backSlashStatus = 0;
          state = PS_IN_PAIR_QUOTED_VALUE;
          }
        else if( c==OPENBR )
          {
          char* inBR = GetBracketedString( ptr );
          if( inBR==NULL )
            {
            free( workingBuffer );
            FreeTagValue( list );
            return NULL;
            }
          last = NewTagValueList( tag, ParseJSON( inBR ), NULL, 0 );
          list = AppendTagValue( list, last );
          ptr += strlen( inBR ) - 1;
          free( inBR );
          inBR = NULL;
          state = PS_IN_PAIR_PRE_COMMA;
          }
        else if( c==OPENSQ )
          {
          char* inBR = GetBracketedString( ptr );
          if( inBR==NULL )
            {
            free( workingBuffer );
            FreeTagValue( list );
            return NULL;
            }
          last = NewTagValueList( tag, ParseJSON( inBR ), NULL, 0 );
          list = AppendTagValue( list, last );
          ptr += strlen( inBR ) - 1;
          free( inBR );
          inBR = NULL;
          state = PS_IN_PAIR_PRE_COMMA;
          }
        else
          {
          value = ptr;
          state = PS_IN_PAIR_RAW_VALUE;
          }
        break;

      case PS_IN_PAIR_RAW_VALUE:
        if( IsWhiteSpace( c ) )
          {
          *ptr = 0;
          state = PS_IN_PAIR_PRE_COMMA;
          if( NOTEMPTY( value ) && strcasecmp( value, "NULL" )==0 )
            {
            last = NewTagValueNull( NULL, NULL, 0 );
            list = AppendTagValue( list, last );
            }
          else
            {
            last = NewTagValueGuessType( tag, value, NULL, 0 );
            list = AppendTagValue( list, last );
            }
          }
        else if( c==COMMA )
          {
          *ptr = 0;
          state = PS_PRE_PAIR;
          last = NewTagValueGuessType( tag, value, NULL, 0 );
          list = AppendTagValue( list, last );
          }
        else if( c==CLOSEBR )
          {
          *ptr = 0;
          last = NewTagValueGuessType( tag, value, NULL, 0 );
          list = AppendTagValue( list, last );
          state = PS_PRE_OPENBR;
          }
        break;

      case PS_IN_PAIR_QUOTED_VALUE:
        if( c==BS )
          {
          backSlashStatus = backSlashStatus ? 0 : 1;
          }
        else if( c==QUOTE && ! backSlashStatus )
          {
          *ptr = 0;
          last = NewTagValue( tag, value, NULL, 0 );
          list = AppendTagValue( list, last );
          state = PS_IN_PAIR_PRE_COMMA;
          }
        break;

      case PS_IN_PAIR_PRE_COMMA:
        if( IsWhiteSpace( c ) ) { }
        else if( c==COMMA ) { state = PS_PRE_PAIR; }
        else if( c==CLOSEBR ) { state = PS_PRE_OPENBR; }
        else
          {
          ParseWarning( state, c, cNum, ptr );
          free( workingBuffer );
          FreeTagValue( list );
          return NULL;
          }
        break;
      }
    ++ cNum;
    }

  free( workingBuffer );
  return list;
  }

/* 0 == same */
int CompareJSON( const char* jsonA, const char* jsonB,
                 _TAG_VALUE* tagsToDelete )
  {
  _TAG_VALUE* tvA = ParseJSON( jsonA );
  _TAG_VALUE* tvB = ParseJSON( jsonB );

  if( tvA==NULL && tvB==NULL )
    return 0;
  if( tvA!=NULL && tvB==NULL )
    {
    FreeTagValue( tvA );
    return -1;
    }
  if( tvA==NULL && tvB!=NULL )
    {
    FreeTagValue( tvB );
    return -2;
    }

  for( _TAG_VALUE* t=tagsToDelete; t!=NULL; t=t->next )
    {
    if( NOTEMPTY( t->tag ) )
      {
      tvA = DeleteTagValue( tvA, t->tag );
      tvB = DeleteTagValue( tvB, t->tag );
      }
    }

  int c = CompareTagValueListBidirectional( tvA, tvB );
  FreeTagValue( tvA );
  FreeTagValue( tvB );

  return c;
  }

int NearlyInteger( double d, int* iValue )
  {
  if( d==0.0 )
    {
    *iValue = 0;
    return 1;
    }

  int isNegative = d<0 ? 1 : 0;
  double positiveD = fabs(d);
  double nearestInt = floor( positiveD + 0.499999999 );
  double gapD = fabs( nearestInt - positiveD );
  
  if( gapD < 0.000000001 )
    {
    if( positiveD < nearestInt )
      positiveD += 0.000000002;
    d = floor( nearestInt );
    if( isNegative )
      {
      *iValue = -1 * (int)d;
      }
    else
      {
      *iValue = (int)d;
      }
    return 1;
    }

  return 0;
  }

int ListToJSON( _TAG_VALUE* list, char* buf, int bufLen )
  {
  if( buf==NULL || bufLen<10 )
    {
    return -1;
    }

  char* ptr = buf;
  char* end = buf+bufLen-1;
  int inArray = 0;
  for( _TAG_VALUE* tv=list; tv!=NULL; tv=tv->next )
    {
    if( EMPTY( tv->tag ) )
      {
      inArray = 1;
      break;
      }
    }

  if( inArray )
    strcpy( ptr, "[ " );
  else
    strcpy( ptr, "{ " );

  ptr += strlen( ptr );

  int itemNum = 0;
  for( ; list!=NULL; list=list->next )
    {
    if( itemNum )
      {
      strncpy( ptr, ", ", end-ptr );
      ptr += strlen( ptr );
      }

    if( NOTEMPTY( list->tag ) )
      {
      snprintf( ptr, end-ptr, "\"%s\": ", list->tag );
      ptr += strlen( ptr );
      }

    if( list->subHeaders==NULL )
      { /* scalar value or empty list */
      switch( list->type )
        {
        case VT_INVALID:
          Warning( "Composing tags into JSON - [%s] has invalid type", NULLPROTECT( list->tag ) );
          break;

        case VT_STR:
          if( NOTEMPTY( list->value ) )
            {
            snprintf( ptr, end-ptr, "\"%s\"", list->value );
            ptr += strlen( ptr );
            }
          else
            {
            snprintf( ptr, end-ptr, "\"\"" );
            ptr += strlen( ptr );
            }
          break;

        case VT_INT:
          snprintf( ptr, end-ptr, "%d", list->iValue );
          ptr += strlen( ptr );
          break;

        case VT_DOUBLE:
          {
          int iValue;
          if( NearlyInteger( list->dValue, &iValue ) )
            {
            snprintf( ptr, end-ptr, "%d", iValue );
            ptr += strlen( ptr );
            }
          else
            {
            snprintf( ptr, end-ptr, "%lf", list->dValue );
            ptr += strlen( ptr );
            }
          }
          break;

        case VT_LIST:
          snprintf( ptr, end-ptr, "[ ]" );
          ptr += strlen( ptr );
          break;

        case VT_NULL:
          snprintf( ptr, end-ptr, "null" );
          ptr += strlen( ptr );
          break;
        }
      }
    else
      { /* vector value in subheaders */
      strncpy( ptr, "[", end-ptr );
      int err = ListToJSON( list->subHeaders, ptr, end-ptr );
      ptr += strlen( ptr );
      strncpy( ptr, "]", end-ptr );
      if( err )
        return err;
      }

    ++itemNum;
    }

  if( inArray )
    strncpy( ptr, " ]", end-ptr );
  else
    strncpy( ptr, " }", end-ptr );

  ptr += strlen( ptr );

  return 0;
  }

int NestedListToJSON( const char* arrayName, _TAG_VALUE* list, char* buf, int bufLen )
  {
  if( buf==NULL || bufLen<10 )
    {
    return -1;
    }

  char* ptr = buf;
  char* end = buf+bufLen-1;

  snprintf( ptr, end-ptr, "\{\"%s\": [", NULLPROTECT(arrayName) );
  ptr += strlen( ptr );

  int itemNum = 0;
  for( ; list!=NULL; list=list->next )
    {
    if( itemNum )
      {
      strncpy( ptr, ", ", end-ptr );
      ptr += strlen( ptr );
      }

    (void)ListToJSON( list->subHeaders, ptr, end-ptr-1 );
    ptr += strlen( ptr );
    ++itemNum;
    }

  strcpy( ptr, " ]}" );
  ptr += strlen( ptr );

  return 0;
  }

int JSONToHTTPPost( char* relURL, char* json, char* buf, int bufLen )
  {
  if( buf==NULL || bufLen<10 )
    {
    Warning( "JSONToHTTPPost - NULL or short output buffer (aborting)" );
    return -1;
    }

  if( EMPTY( json ) )
    {
    Warning( "JSONToHTTPPost - NULL or empty JSON (continuing with empty string)" );
    buf[0] = 0;
    }

  char* ptr = buf;
  snprintf( ptr, bufLen, "POST %s HTTP/1.1\r\n", relURL );
  bufLen -= strlen( ptr );
  ptr += strlen( ptr );

  int l = 0;
  if( NOTEMPTY( json ) )
    l = strlen( json );

  snprintf( ptr, bufLen, "Content-Length: %d\r\n", l );
  bufLen -= strlen( ptr );
  ptr += strlen( ptr );

  snprintf( ptr, bufLen, "Content-Type: application/json\r\n\r\n" );
  bufLen -= strlen( ptr );
  ptr += strlen( ptr );

  snprintf( ptr, bufLen, "%s\r\n", NULLPROTECT(json) );

  return 0;
  }

#ifdef TEST_PARSER

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

int main()
  {
  _TAG_VALUE* tv = NULL;

  printf("Input: [%s]:\n", SIMPLE_TEST );
  tv = ParseJSON( SIMPLE_TEST );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );

  printf("Input: [%s]:\n", HARDER_TEST );
  tv = ParseJSON( HARDER_TEST );
  printf("Parsed output:\n");
  PrintTagValue( 2, tv );

  return 0;
  }

#endif
