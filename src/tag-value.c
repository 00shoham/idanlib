#include "utils.h"

enum valueType GuessType( char* str )
  {
  if( str==NULL )
    return VT_INVALID;

  if( *str==0 )
    return VT_STR;

  if( ( (*str)=='-' && AllDigits( str+1 )==0 )
      || AllDigits( str )==0 )
    return VT_INT;

  if( ( (*str)=='-' && AllDigitsSingleDot( str+1 )==0 )
      || AllDigitsSingleDot( str )==0 )
    return VT_DOUBLE;

  return VT_STR;
  }

char* TypeName( enum valueType t )
  {
  switch( t )
    {
    case VT_STR: return "STR";
    case VT_INT: return "INT";
    case VT_DOUBLE: return "DOUBLE";
    default: return "UNDEFINED";
    }
  }

/* NewTagValue()
 * Allocates and initializes a _TAG_VALUE data structure.
 * This is a linked list of tag/value pairs with an option to have a 'child' linked
 * list as well (tag/value/sub-list).
 * Memory management: Don't forget to clean up with FreeTagValue when done.
 */
_TAG_VALUE* NewTagValue( char* tag, const char* value, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( ( NOTEMPTY( t->tag ) && strcmp( t->tag,tag )==0 ) /* same tag */
          || ( EMPTY( t->tag ) && EMPTY( tag ) && EMPTY( value ) && EMPTY( t->value ) ) /* empty tag+value */
          || ( EMPTY( t->tag ) && EMPTY( tag ) && NOTEMPTY( value ) && NOTEMPTY( t->value ) && strcmp( value, t->value )==0 ) /* empty tag, same value */
          )
        {
        FreeIfAllocated( &(t->value) );
        if( value==NULL ) t->value = NULL;
        else t->value = strdup( value );
        t->type = VT_STR;
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  if( tag==NULL )
    n->tag = NULL;
  else
    n->tag = strdup( tag );

  n->type = VT_STR;
  if( value==NULL ) n->value = NULL;
  else n->value = strdup( value );
  n->subHeaders = NULL;
  n->next = list;
  return n;
  }

_TAG_VALUE* NewTagValueGuessType( char* tag, char* value, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( ( NOTEMPTY( t->tag ) && NOTEMPTY( tag ) && strcmp( t->tag,tag )==0 )
          || ( EMPTY( t->tag ) && EMPTY( tag ) ) )
        {
        FreeIfAllocated( &(t->value) );
        t->type = GuessType( value );
        switch( t->type )
          {
          case VT_INVALID:
            break;
          case VT_STR:
            t->value = strdup( value );
            break;
          case VT_INT:
            t->iValue = atoi( value );
            break;
          case VT_DOUBLE:
            t->dValue = atof( value );
            break;
          case VT_LIST:
            Error( "We do not guess the type of a list value in TV" );
            break;
          case VT_NULL:
            /* not a thing */
            break;
          }
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  if( EMPTY( tag ) )
    n->tag = NULL;
  else
    n->tag = strdup( tag );
  n->type = GuessType( value );
  switch( n->type )
    {
    case VT_INVALID:
      break;
    case VT_STR:
      n->value = strdup( value );
      break;
    case VT_INT:
      n->iValue = atoi( value );
      break;
    case VT_DOUBLE:
      n->dValue = atof( value );
      break;
    case VT_LIST:
      Error( "TV type guessing does not return LIST" );
      break;
    case VT_NULL:
      /* not a thing */
      break;
    }

  n->next = list;
  return n;
  }

_TAG_VALUE* NewTagValueNull( char* tag, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( ( NOTEMPTY( t->tag ) && NOTEMPTY( tag ) && strcmp( t->tag,tag )==0 )
          || ( EMPTY( t->tag ) && EMPTY( tag ) ) )
        {
        FreeIfAllocated( &(t->value) );
        t->type = VT_NULL;
        t->value = NULL;
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  if( EMPTY( tag ) )
    n->tag = NULL;
  else
    n->tag = strdup( tag );
  n->type = VT_NULL;

  n->next = list;
  return n;
  }

_TAG_VALUE* NewTagValueInt( char* tag, int value, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( NOTEMPTY( t->tag ) && strcmp( t->tag,tag )==0 )
        {
        t->iValue = value;
        t->type = VT_INT;
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  n->tag = strdup( tag );
  n->iValue = value;
  n->type = VT_INT;
  n->subHeaders = NULL;
  n->next = list;
  return n;
  }

_TAG_VALUE* NewTagValueDouble( char* tag, double value, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( NOTEMPTY( t->tag ) && strcmp( t->tag,tag )==0 )
        {
        t->dValue = value;
        t->type = VT_DOUBLE;
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  n->tag = strdup( tag );
  n->dValue = value;
  n->type = VT_DOUBLE;
  n->subHeaders = NULL;
  n->next = list;
  return n;
  }

_TAG_VALUE* NewTagValueList( char* tag, _TAG_VALUE* value, _TAG_VALUE* list, int replaceDup )
  {
  if( replaceDup )
    {
    /* do we already have this tag?  if so, replace the value */
    for( _TAG_VALUE* t=list; t!=NULL; t=t->next )
      {
      if( NOTEMPTY( t->tag ) && strcmp( t->tag,tag )==0 )
        {
        if( t->subHeaders!=NULL )
          {
          FreeTagValue( t->subHeaders );
          }
        t->subHeaders = value;
        t->type = VT_LIST;
        return list;
        }
      }
    }

  /* nope - it's new.  create a new item in the list */
  _TAG_VALUE* n = (_TAG_VALUE*)SafeCalloc(1, sizeof( _TAG_VALUE ), "_TAG_VALUE" );
  if( EMPTY( tag ) )
    n->tag = NULL;
  else
    n->tag = strdup( tag );
  n->subHeaders = value;
  n->type = VT_LIST;
  n->next = list;
  return n;
  }

_TAG_VALUE* DeleteTagValue( _TAG_VALUE* list, char* tag )
  {
  if( EMPTY( tag ) || list==NULL )
    return list;

  _TAG_VALUE** ptr = &list;
  while( ptr!=NULL && (*ptr)!=NULL )
    {
    _TAG_VALUE* current = *ptr;
    if( current!=NULL
        && NOTEMPTY( current->tag )
        && strcmp( current->tag, tag )==0 )
      {
      *ptr = current->next;
      FreeIfAllocated( &(current->tag) );
      FreeIfAllocated( &(current->value) );
      FreeTagValue( current->subHeaders );
      FREE( current );
      /* keep going as there may be other tag/value pairs with the same tag */
      }
    else
      {
      if( current==NULL )
        break;
      ptr = & (current->next);
      }
    }

  return list;
  }

char* GetTagValue( _TAG_VALUE* list, char* tagName )
  {
  char* retVal = NULL;
  if( EMPTY( tagName ) )
    {
    return NULL;
    }

  while( list!=NULL )
    {
    if( NOTEMPTY( list->tag)
        && strcasecmp(list->tag,tagName)==0
        && list->type==VT_STR )
      {
      retVal = list->value;
      break;
      }
    list = list->next;
    }

  return retVal;
  }

_TAG_VALUE* FindTagValue( _TAG_VALUE* list, char* tagName )
  {
  _TAG_VALUE* retVal = NULL;
  if( EMPTY( tagName ) )
    {
    return NULL;
    }

  while( list!=NULL )
    {
    if( NOTEMPTY( list->tag)
        && strcmp(list->tag,tagName)==0 )
      {
      retVal = list;
      break;
      }
    list = list->next;
    }

  return retVal;
  }

_TAG_VALUE* FindTagValueByValue( _TAG_VALUE* list, char* value )
  {
  _TAG_VALUE* retVal = NULL;
  if( EMPTY( value ) )
    {
    return NULL;
    }

  while( list!=NULL )
    {
    if( NOTEMPTY( list->value)
        && strcmp(list->value,value)==0 )
      {
      retVal = list;
      break;
      }
    list = list->next;
    }

  return retVal;
  }

_TAG_VALUE* FindTagValueNoCase( _TAG_VALUE* list, char* tagName )
  {
  _TAG_VALUE* retVal = NULL;
  if( EMPTY( tagName ) )
    {
    return NULL;
    }

  int n = 0;
  while( list!=NULL )
    {
    ++n;
    if( NOTEMPTY( list->tag )
        && strcasecmp( list->tag, tagName )==0 )
      {
      /* Notice( "FoundTagValueNoCase(%s) in list - case insensitive match", tagName ); */
      retVal = list;
      break;
      }
    list = list->next;
    }

  if( retVal==NULL )
    {
    /* Notice( "FoundTagValueNoCase(%s) - no match in %d element list", tagName, n ); */
    }

  return retVal;
  }

int GetTagValueInt( _TAG_VALUE* list, char* tagName )
  {
  int retVal = INVALID_INT;
  if( EMPTY( tagName ) )
    {
    return retVal;
    }

  while( list!=NULL )
    {
    if( NOTEMPTY( list->tag )
        && strcasecmp(list->tag,tagName)==0
        && list->type==VT_INT )
      {
      retVal = list->iValue;
      break;
      }
    list = list->next;
    }

  return retVal;
  }

double GetTagValueDouble( _TAG_VALUE* list, char* tagName )
  {
  double retVal = INVALID_DOUBLE;
  if( EMPTY( tagName ) )
    {
    return retVal;
    }

  while( list!=NULL )
    {
    if( NOTEMPTY( list->tag )
        && strcasecmp(list->tag,tagName)==0 )
      {
      if( list->type==VT_DOUBLE )
        {
        retVal = list->dValue;
        break;
        }
      else if( list->type==VT_INT )
        {
        retVal = (double)list->iValue;
        break;
        }
      }
    list = list->next;
    }

  return retVal;
  }

_TAG_VALUE* GetTagValueList( _TAG_VALUE* list, char* tagName )
  {
  _TAG_VALUE* retVal = NULL;
  if( EMPTY( tagName ) )
    {
    return NULL;
    }

  while( list!=NULL )
    {
    if( NOTEMPTY(list->tag)
        && strcasecmp(list->tag,tagName)==0 )
      {
      retVal = list->subHeaders;
      break;
      }
    list = list->next;
    }

  return retVal;
  }

int CompareTagValueList( _TAG_VALUE* a, _TAG_VALUE* b )
  {
  if( a==b )
    return 0;

  if( a!=NULL && b==NULL )
    return -1;

  if( a==NULL && b!=NULL )
    return -2;

  for( _TAG_VALUE* tv=a; tv!=NULL; tv=tv->next )
    {
    if( EMPTY( tv->tag ) )
      continue;

    switch( tv->type )
      {
      case VT_LIST:
        {
        _TAG_VALUE* cmp = GetTagValueList( b, tv->tag );
        if( cmp==NULL )
          return -10;
        if( tv->subHeaders==NULL && cmp->subHeaders==NULL )
          {
          /* okay - same */
          }
        else
          {
          int err = CompareTagValueList( tv->subHeaders, cmp->subHeaders );
          if( err!=0 )
            return -100 + err;
          }
        }
        break;

      case VT_INVALID:
        if( tv->subHeaders!=NULL )
          {}
        else
          Warning( "Comparing tags - [%s] has invalid type", NULLPROTECT( tv->tag ) );
        break;

      case VT_STR:
        if( EMPTY( tv->value ) )
          {
          Warning( "%s has empty string\n", NULLPROTECT( tv->tag ) );
          return -3;
          }

        char* str = GetTagValue( b, tv->tag );
        if( EMPTY( str ) )
          return -4;

        int c = strcmp( tv->value, str );
        if( c )
          return -5;
        break;

      case VT_INT:
        {
        int i = GetTagValueInt( b, tv->tag );
        if( i!=tv->iValue )
          return -6;
        }
        break;

      case VT_DOUBLE:
        {
        double d = GetTagValueDouble( b, tv->tag );
        if( d!=tv->dValue )
          return -7;
        }
        break;

      case VT_NULL:
        {
        _TAG_VALUE* sub = FindTagValue( b, tv->tag );
        if( sub==NULL || sub->type!=VT_NULL )
          return -8;
        }
        break;
      }

    if( tv->subHeaders!=NULL )
      { /* not really a list but there are sub-headers anyways? */
      _TAG_VALUE* sub = GetTagValueList( b, tv->tag );
      if( sub==NULL )
        return -9;
      int err = CompareTagValueList( tv->subHeaders, sub );
      if( err )
        return + err;
      }

    }

  return 0;
  }

int CompareTagValueListBidirectional( _TAG_VALUE* a, _TAG_VALUE* b )
  {
  int c1 = CompareTagValueList( a, b );
  int c2 = CompareTagValueList( b, a );
  return c1 | c2;
  }

void PrintTagValue( int indent, _TAG_VALUE* list )
  {
  if( indent<0 || indent>100 )
    {
    Warning( "PrintTagValue() - bad indent" );
    return;
    }

  if( list==NULL )
    {
    Warning( "PrintTagValue() - NULL list" );
    return;
    }

  char indentBuf[250];
  memset( indentBuf, ' ', indent );
  indentBuf[indent] = 0;

  char printLine[BUFLEN];
  char* ptr = printLine;
  char* end = printLine + sizeof(printLine) - 1;

  strncpy( ptr, indentBuf, end-ptr );
    ptr += indent;

  if( list->tag!=NULL )
    {
    strncpy( ptr, "\"", end-ptr );
      ptr += strlen( ptr );
    strncpy( ptr, list->tag, end-ptr );
      ptr += strlen( ptr );
    strncpy( ptr, "\"", end-ptr );
      ptr += strlen( ptr );
    strncpy( ptr, ": ", end-ptr );
      ptr += strlen( ptr );
    }
  else
    {
    strncpy( ptr, "\"\": ", end-ptr );
      ptr += strlen( ptr );
    }

  switch( list->type )
    {
    case VT_INVALID:
      if( list->subHeaders!=NULL )
        {}
      else if( list->tag==NULL )
        {}
      else
        Warning( "Printing tags - [%s] has invalid type", NULLPROTECT( list->tag ) );
      break;
    case VT_LIST:
      snprintf( ptr, (int)(end-ptr), "(list) " );
        ptr += strlen( ptr );
      if( list->subHeaders==NULL )
        {
        strncpy( ptr, "[]", end-ptr );
          ptr += strlen( ptr );
        }
      break;
    case VT_NULL:
      strncpy( ptr, "null", end-ptr );
      ptr += strlen( ptr );
      break;
    case VT_STR:
      if( EMPTY( list->value ) )
        {
        strncpy( ptr, "\"\"", end-ptr );
          ptr += strlen( ptr );
        }
      else
        {
        strncpy( ptr, "\"", end-ptr );
          ptr += strlen( ptr );
        strncpy( ptr, list->value, end-ptr );
          ptr += strlen( ptr );
        strncpy( ptr, "\"", end-ptr );
          ptr += strlen( ptr );
        }
      break;
    case VT_INT:
      snprintf( ptr, (int)(end-ptr), "(int) %d", list->iValue );
      ptr += strlen( ptr );
      break;
    case VT_DOUBLE:
      snprintf( ptr, (int)(end-ptr), "(double) %lf", list->dValue );
      ptr += strlen( ptr );
      break;
    }
  /*
  strncpy( ptr,end-ptr, "\n" );
    ptr += strlen( ptr );
  */
  Notice( printLine );
  if( list->subHeaders!=NULL )
    {
    PrintTagValue( indent+2, list->subHeaders );
    }
  if( list->next!=NULL )
    {
    PrintTagValue( indent, list->next );
    }
  }

void PrintTagValueFromList( _TAG_VALUE* list, char* tag )
  {
  if( list==NULL )
    {
    Warning( "PrintTagValueFromList() - NULL list" );
    return;
    }
  if( EMPTY( tag ) )
    {
    Warning( "PrintTagValueFromList() - empty tag" );
    return;
    }

  for( _TAG_VALUE* tv=list; tv!=NULL; tv=tv->next )
    {
    if( NOTEMPTY( tv->tag ) && strcasecmp( tv->tag, tag )==0 )
      {
      printf( "%s=", tv->tag );
      switch( tv->type )
        {
        case VT_LIST:
          printf( "[list]\n" );
          break;
        case VT_NULL:
          printf( "NULL\n" );
          break;
        case VT_STR:
          printf( "\"%s\"\n", NULLPROTECT( tv->value ) );
          break;
        case VT_INT:
          printf( "(int)%d\n", tv->iValue );
          break;
        case VT_DOUBLE:
          printf( "(double)%lf\n", tv->dValue );
          break;
        default:
          printf( "{default}\n" );
          break;
        }
      }
    }
  }

char* GetTagValueSafe( _TAG_VALUE* list, char* tagName, char* expr )
  {
  char* val = NULL;
  val = GetTagValue( list, tagName );

  if( EMPTY( expr ) )
    {
    return val;
    }

  if( StringMatchesRegex( expr, val )==0 )
    {
    return val;
    }

  /* no match - return empty string as original is dangerous */
  return NULL;
  }

_TAG_VALUE* CopyTagValueList( _TAG_VALUE* list )
  {
  if( list==NULL )
    return NULL;

  _TAG_VALUE* newList = NULL;
  for( ; list!=NULL; list=list->next )
    {
    switch( list->type )
      {
      case VT_INVALID:
        Error( "Cannot copy a _TAG_VALUE list with an invalid-type entry" );

      case VT_STR:
        newList = NewTagValue( list->tag, list->value, newList, 0 );
        break;

      case VT_INT:
        newList = NewTagValueInt( list->tag, list->iValue, newList, 0 );
        break;

      case VT_DOUBLE:
        newList = NewTagValueDouble( list->tag, list->dValue, newList, 0 );
        break;

      case VT_LIST:
        newList = NewTagValueList( list->tag, CopyTagValueList( list->subHeaders ), newList, 0 );
        break;

      case VT_NULL:
        newList = NewTagValueNull( list->tag, newList, 0 );
        break;
      }
    }

  return newList;
  }

/* FreeTagValue()
 * Recurively free up memory in a _TAG_VALUE nested linked list.
 * Recommended to set the pointer to the list to NULL after calling this.
 */
void FreeTagValue( _TAG_VALUE* list )
  {
  if( list==NULL )
    {
    return;
    }
  if( list->subHeaders!=NULL )
    {
    FreeTagValue( list->subHeaders );
    list->subHeaders = NULL;
    }
  if( list->next!=NULL )
    {
    FreeTagValue( list->next );
    list->next = NULL;
    }
  int isPassword = 0;
  if( list->tag!=NULL )
    {
    if( strstr( list->tag, "pass" )!=NULL
        || strstr( list->tag, "PASS" )!=NULL )
      isPassword = 1;

    FREE( list->tag );
    }
  if( list->value!=NULL )
    {
    if( isPassword )
      memset( list->value, 0, strlen( list->value )-1 );
    FREE( list->value );
    }
  FREE( list );
  }

/* ExpandMacros
 * Inputs: src string, _TAG_VALUE linked list of search/replace pairs
 * Outputs: dst buffer of dstLen size
 * Returns: negative=error, else # of string replacements.
 * %MACRO% or $MACRO[^a-z] replaced as per search/replace data.
 */
int ExpandMacros( char* src, char* dst, int dstLen, _TAG_VALUE* patterns )
  {
  if( src==NULL || dst==NULL )
    {
    return -1;
    }

  /*
  Notice("ExpandMacros( %s )", NULLPROTECT( src ) );
  for( _TAG_VALUE* t=patterns; t!=NULL; t=t->next )
    {
    Notice("... %s : %s", NULLPROTECT(t->tag), NULLPROTECT(t->value) );
    }
  */

  char* endp = dst + dstLen;
  char* ends = src + strlen(src);
  int c;
  int nReplacements = 0;
  while( (*src)!=0 )
    {
    c = *src;
    if( c=='%' )
      {
      int nextC = *(src+1);
      if( nextC=='%' )
        {
        *dst = '%';
        ++dst;
        src+=2;
        }
      else
        {
        int gotReplacement = 0;
        for( _TAG_VALUE* t = patterns; t!=NULL; t=t->next )
          {
          if( NOTEMPTY( t->tag ) )
            {
            int l = strlen(t->tag);
            int m = strlen(NULLPROTECTE(t->value));
            /*   %MACRO% */
            if( t->value!=NULL 
                && dst+m+1<endp
                && src+l+1<ends
                && strncmp( src+1, t->tag, l )==0 )
              {
              int trailP = *(src+l+1);
              if( trailP=='%' )
                {
                strcpy( dst, NULLPROTECTE( t->value ) );
                dst += m;
                src += l+2;
                ++gotReplacement;
                ++nReplacements;
                break;
                }
              else
                {
                /* no trailing % - not this macro anyways */
                }
              }
            else
              {
              /* empty search string or not enough space or no match */
              }
            }
          else
            {
            /* empty search string */
            }
          } /* for loop of tag/value pairs */

        if( gotReplacement==0 ) /* no match - just a % sign */
          {
          *(dst++) = *(src++);
          }
        } /* char after % is not another % */
      } /* char was '%' */
    else if( c=='$' )
      {
      int nextC = *(src+1);
      if( nextC=='$' )
        {
        *dst = '$';
        ++dst;
        src+=2;
        }
      else
        {
        int gotReplacement = 0;
        for( _TAG_VALUE* t = patterns; t!=NULL; t=t->next )
          {
          if( NOTEMPTY( t->tag ) )
            {
            int l = strlen(t->tag);
            int m = strlen(NULLPROTECTE(t->value));
            /* $MACRO */
            if( t->value!=NULL 
                && dst+m+1<endp
                && src+l<ends
                && strncmp( src+1, t->tag, l )==0 )
              {
              int trailP = *(src+l+1);
              if( !isalpha( trailP ) )
                {
                strcpy( dst, NULLPROTECTE( t->value ) );
                dst += m;
                *(dst++) = trailP;
                src += l+2;
                ++gotReplacement;
                ++nReplacements;
                break;
                }
              else
                {
                /* no trailing (non-alpha) chars - not this macro anyways */
                }
              }
            else
              {
              /* empty search string or not enough space or no match */
              }
            }
          else
            {
            /* empty search string */
            }
          } /* for loop of tag/value pairs */

        if( gotReplacement==0 ) /* no match - just a % sign */
          {
          *(dst++) = *(src++);
          }
        } /* char after $ is not another $ */
      } /* char was '$' */
    else
      {
      *(dst++) = *(src++);
      }
    }

  *dst = 0;

  return nReplacements;
  }

int ExpandMacrosVA( char* src, char* dst, int dstLen, ... )
  {
  _TAG_VALUE* patterns = NULL;

  va_list argptr;
  va_start( argptr, dstLen );
  for(;;)
    {
    char* tag = NULL;
    char* value = NULL;

    tag = va_arg( argptr, char* );
    if( tag==NULL )
      {
      break;
      }

    value = va_arg( argptr, char* );
    if( value!=NULL )
      {
      patterns = NewTagValue( tag, value, patterns, 1 );
      }
    }

  va_end( argptr );

  int n = ExpandMacros( src, dst, dstLen, patterns );
  FreeTagValue( patterns );

  return n;
  }

_TAG_VALUE* TagIntList( char* placeHolderPtr, ... )
  {
  _TAG_VALUE* list = NULL;

  va_list argptr;
  va_start( argptr, placeHolderPtr );
  for(;;)
    {
    char* tag = NULL;

    tag = va_arg( argptr, char* );
    if( tag==NULL )
      {
      break;
      }

    int value = va_arg( argptr, int );
    list = NewTagValueInt( tag, value, list, 1 );
    }

  va_end( argptr );

  return list;
  }

char* AggregateMessages( _TAG_VALUE* messages )
  {
  int length = 0;
  int n = 0;
  for( _TAG_VALUE* tv = messages; tv!=NULL; tv=tv->next )
    {
    if( NOTEMPTY( tv->value ) )
      {
      ++n;
      length += strlen( tv->value );
      }
    }

  if( n==0 || length==0 )
    return NULL;

  char* buf = (char*)calloc( length + n*5, sizeof(char) );
  char* ptr = buf;

  for( _TAG_VALUE* tv = messages; tv!=NULL; tv=tv->next )
    {
    if( NOTEMPTY( tv->value ) )
      {
      strcpy( ptr, tv->value );
      ptr += strlen( tv->value );
      strcpy( ptr, ". " );
      ptr += strlen( tv->value );
      }
    }

  return buf;
  }

_TAG_VALUE* AppendValue( char* buf, _TAG_VALUE* workingHeaders )
  {
  for( char *ptr=buf; *ptr!=0; ++ptr )
    {
    if( *ptr == CR )
      {
      *ptr = 0;
      break;
      }
    }

  if( workingHeaders->tag!=NULL
      && workingHeaders->value!=NULL
      && strcmp( workingHeaders->tag,"value")==0 )
    {
    int l = strlen( workingHeaders->value ) + strlen( buf ) + 5;
    char* tmp = calloc( l, sizeof( char ) );
    strcpy( tmp, workingHeaders->value );
    strcat( tmp, "\n" );
    strcat( tmp, buf );
    free( workingHeaders->value );
    workingHeaders->value = tmp;
    }

  return workingHeaders;
  } 

_TAG_VALUE* AppendTagValue( _TAG_VALUE* list, _TAG_VALUE* newItem )
  {
  if( list==NULL )
    return newItem;

  _TAG_VALUE** ptr = NULL;
  for( ptr = &list; ptr!=NULL && *ptr!=NULL; ptr = &( (*ptr)->next ) )
    {}

  if( ptr!=NULL && *ptr==NULL )
    {
    *ptr = newItem;
    return list;
    }

  Error( "Failed to append item to tag-value list" );
  return NULL;
  }

_TAG_VALUE* ExtractValueFromPath( _TAG_VALUE* tree, _TAG_VALUE* path )
  {
  for( _TAG_VALUE* step = path; step!=NULL; step=step->next )
    {
    int isTerminal = step->next==NULL ? 1 : 0;

    if( step->type==VT_STR )
      {
      int foundIt = 0;
      for( _TAG_VALUE* item=tree; item!=NULL; item=item->next )
        {
        if( NOTEMPTY( item->tag )
            && NOTEMPTY( step->value )
            && strcasecmp( item->tag, step->value )==0 )
          {
          if( isTerminal )
            return item;
          
          tree = item->subHeaders;
          foundIt = 1;
          break;
          }
        }
      if( foundIt==0 )
        {
        Warning( "Failed to find item labeled %s", step->value );
        return NULL;
        }
      }
    else if( step->type==VT_INT )
      {
      int i = 0;
      _TAG_VALUE* item = tree;
      int foundIt = 0;
      for( ; item!=NULL; item=item->next, ++i )
        {
        if( i==step->iValue )
          {
          if( isTerminal )
            return item;
          
          tree = item->subHeaders;
          foundIt = 1;
          break;
          }
        }
      if( foundIt==0 )
        {
        Warning( "Failed to find item at position %d", step->iValue );
        return NULL;
        }
      }
    else
      Error( "Only expecting STR or INT steps in path" );
    }

  Error( "Failed to find item at path specified" );
  return NULL;
  }

enum pState
  {
  ps_item_scan,
  ps_quoted_value,
  ps_int_value,
  ps_pre_comma
  };

_TAG_VALUE* ParsePath( char* textPath )
  {
  _TAG_VALUE* path = NULL;

  enum pState state = ps_item_scan;
  char* string = NULL;
  for( char* ptr = textPath; *ptr!=0; ++ptr )
    {
    int c = *ptr;
    switch( state )
      {
      case ps_item_scan:
        if( c==QUOTE )
          {
          string = ptr+1;
          state = ps_quoted_value;
          }
        else if( isdigit( c ) )
          {
          string = ptr;
          state = ps_int_value;
          }
        else if( isspace( c ) )
          {}
        else
          Error( "State @ %s is ps_item_scan but not a quote or digit", ptr );
        break;

      case ps_quoted_value:
        if( c==QUOTE )
          {
          char buf[BUFLEN];
          strncpy( buf, string, ptr-string );
          buf[ptr-string] = 0;
          _TAG_VALUE* newOne = NewTagValue( "string", buf, NULL, 0 );
          path = AppendTagValue( path, newOne );
          state = ps_pre_comma;
          }
        else if( c==BACKSLASH )
          {
          ++ptr;
          }
        break;

      case ps_int_value:
        if( ! isdigit( c ) )
          {
          char buf[BUFLEN];
          strncpy( buf, string, ptr-string );
          buf[ptr-string] = 0;
          int iVal = atoi(buf);
          _TAG_VALUE* newOne = NewTagValueInt( "Int", iVal, NULL, 0 );
          path = AppendTagValue( path, newOne );
          if( c==COMMA )
            state = ps_item_scan;
          else
            state = ps_pre_comma;
          }
        break;

      case ps_pre_comma:
        if( c==COMMA )
          state = ps_item_scan;
        break;

      default:
        Error( "Weird state in parser" );
      }
    }

  return path;
  }

void PopulateStringValueFromNumeric( _TAG_VALUE* t )
  {
  if( t==NULL )
    return;

  if( NOTEMPTY( t->value ) )
    return;

  if( t->iValue != INVALID_INT )
    {
    char buf[100];
    snprintf( buf, sizeof(buf)-2, "%d", t->iValue );
    if( t->value != NULL )
      free( t->value );
    t->value = strdup( buf );
    return;
    }

  if( t->dValue != INVALID_DOUBLE )
    {
    char buf[100];
    snprintf( buf, sizeof(buf)-2, "%.0lf", t->dValue );
    if( t->value != NULL )
      free( t->value );
    t->value = strdup( buf );
    return;
    }
  }
