#include "utils.h"

char generatedIdentifierChars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
char validIdentifierChars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_";

char* upperHexDigits = "0123456789ABCDEF";

void FreeArrayOfStrings( char** array, int len )
  {
  if( array==NULL )
    return;

  for( int i=0; i<len; ++i )
    {
    char* p = array[i];
    if( p!=NULL )
      {
      FREE( p );
      p = NULL;
      }
    }

  FREE( array );
  }

/* Return a pointer to the first non-whitespace character in a string.
 */
char* TrimHead( char* buf )
  {
  if( EMPTY( buf ) )
    return buf;

  /* printf("TrimHead( %s )\n", buf ); */

  char* ptr = buf;
  while( *ptr!=0 )
    {
    if( *ptr==' ' || *ptr=='\t' || *ptr=='\r' || *ptr=='\n' )
      {
      ++ptr;
      }
    else
      {
      break;
      }
    }

  /* printf("TrimHead --> ( %s )\n", ptr ); */

  return ptr;
  }

/* Remove trailing LF and CR characters from a string.
 * Note that the input argument is modified - all LF or CR
 * in a sequence that appears at the end of the string are
 * replaced with '\0'.
 */
void TrimTail( char* ptr )
  {
  if( EMPTY( ptr ) )
    return;

  for( char* eolc = ptr+strlen(ptr)-1; eolc>=ptr; --eolc )
    {
    if( *eolc == '\r' || *eolc == '\n' || *eolc==' ' || *eolc=='\t' )
      {
      *eolc = 0;
      }
    else
      {
      break;
      }
    }
  }

int IsSpace( int c, int noQuotes )
  {
  if( c==' ' || c=='\t' || c=='\r' || c=='\n' )
    return 1;
  if( noQuotes && c=='"' )
    return 1;
  return 0;
  }

void ShiftLeft( char* ptr )
  {
  do
    {
    *(ptr) = *(ptr+1);
    ++ptr;
    } while( *(ptr+1)!=0 );
  *ptr = 0;
  }

/* Remove leading and trailing spaces; replace double spaces with single
   Modifies the original string.
 */
char* RemoveExtraSpaces( char* raw, int noQuotes )
  {
  char* retPtr = raw;

  while( IsSpace( *retPtr, noQuotes ) )
    ++retPtr;

  int lastWasSpace = 0;
  for( char* ptr=retPtr; *ptr!=0; ++ptr )
    {
    if( lastWasSpace )
      {
      while( IsSpace( *ptr, noQuotes ) )
        ShiftLeft( ptr );
      lastWasSpace = 0;
      }
    else
      {
      if( IsSpace( *ptr, noQuotes ) )
        lastWasSpace = 1;
      }
    }

  for( char* eolc = retPtr+strlen(retPtr)-1; eolc>=retPtr; --eolc )
    {
    if( IsSpace( *eolc, noQuotes ) )
      *eolc = 0;
    else
      break;
    }

  return retPtr;
  }

int randSeeded = 0;

/* Generate a random identifier of N letters and digits in a provided buffer */
void GenerateIdentifier( char* buf, int nChars )
  {
  if( randSeeded==0 )
    {
    srand48( time( NULL ) );
    randSeeded = 1;
    }

  int l = strlen( generatedIdentifierChars );
  for( int i=0; i<nChars; ++i )
    {
    int c = lrand48() % l;
    *(buf++) = generatedIdentifierChars[c];
    }
  *buf = 0;
  }

int StringIsAnIdentifier( char* str )
  {
  if( EMPTY( str ) )
    {
    return -1;
    }
  else
    {
    while( *str!=0 )
      {
      int c = *str;
      char* ptr = strchr( validIdentifierChars, c );
      if( ptr==NULL )
        {
        return -2;
        }
      ++str;
      }
    }

  return 0;
  }

/* Modify a string such that any trailing " marks are replaced with
 * '\0' and return a pointer to the first non-" characters in it.
 * Effectively the return string is a version of the input string
 * but without quotes.  Note that the input string is modified.
 */
char* StripQuotes( char* buf )
  {
  if( EMPTY( buf ) )
    {
    return buf;
    }

  if( *buf == '"' )
    {
    ++buf;
    }

  if( buf[strlen(buf)-1]=='"' )
    {
    buf[strlen(buf)-1] = 0;
    }

  return buf;
  }

/* Remove leading and trailing CR and LF chars from a string.
 * Return a pointer to within the original string, which has been
 * modified (at the tail end).
 */
char* StripEOL( char* buf )
  {
  if( EMPTY( buf ) )
    {
    return buf;
    }

  while( *buf == '\r' || *buf=='\n' )
    {
    ++buf;
    }

  if( buf[0]!=0 )
    {
    char* eos = buf + strlen(buf) - 1;
    while( (*eos=='\r' || *eos=='\n') && eos>=buf )
      {
      *eos = 0;
      --eos;
      }
    }

  return buf;
  }

int StringEndsWith( const char* string, const char* suffix, int caseSensitive )
  {
  if( EMPTY( string ) || EMPTY( suffix ) )
    {
    return -1;
    }

  int ls = strlen( string );
  int lu = strlen( suffix );

  if( lu>ls )
    {
    return -2;
    }

  if( caseSensitive )
    {
    return strcasecmp( string + ls - lu, suffix );
    }

  return strcmp( string + ls - lu, suffix );
  }

int StringStartsWith( const char* string, const char* prefix, int caseSensitive )
  {
  if( EMPTY( string ) || EMPTY( prefix ) )
    {
    return -1;
    }

  int ls = strlen( string );
  int lp = strlen( prefix );

  if( lp>ls )
    {
    return -2;
    }

  if( caseSensitive )
    {
    return strncasecmp( string, prefix, strlen(prefix) );
    }

  return strncmp( string, prefix, strlen(prefix) );
  }

int CountInString( const char* string, const int c )
  {
  if( EMPTY( string ) )
    {
    return 0;
    }
  int n=0;
  while( *string!=0 )
    {
    if( *string ==c )
      {
      ++n;
      }
    ++string;
    }

  return n;
  }

int AllDigits( char* str )
  {
  while( *str!=0 )
    {
    int c = *str;
    if( ! isdigit( c ) )
      return 0;
    ++str;
    }

  return 1;
  }

int AllDigitsSingleDot( char* str )
  {
  int nDots = 0;
  while( (*str)!=0 )
    {
    int c = *str;
    if( ! isdigit( c ) )
      {
      if( c == '.' )
        {
        ++nDots;
        }
      else
        {
        return 0;
        }

      if( nDots>1 )
        {
        return 0;
        }
      }

    ++str;
    }

  if( nDots==1 )
    {
    return 1;
    }

  return 0;
  }

void LowerCase( char* dst, int dstSize, char* src )
  {
  if( dst==NULL )
    return;

  if( dst!=src )
    *dst = 0;

  if( EMPTY( src ) )
    return;

  char* endp = dst + dstSize;
  while( (*src)!=0 && dst<endp )
    {
    int c = *src;
    c = tolower( c );
    *dst = c;
    ++dst;
    ++src;
    }

  *dst = 0;
  }

void UpperCase( char* dst, int dstSize, char* src )
  {
  if( dst==NULL )
    return;

  if( dst!=src )
    *dst = 0;

  if( EMPTY( src ) )
    return;

  char* endp = dst + dstSize;
  while( (*src)!=0 && dst<endp )
    {
    int c = *src;
    c = toupper( c );
    *dst = c;
    ++dst;
    ++src;
    }

  *dst = 0;
  }

/* Regular expression test.
 * 0 = match.
 */
int StringMatchesRegex( char* expr, char* str )
  {
  if( EMPTY( expr ) )
    {
    return 0;
    }

  if( EMPTY( str ) )
    {
    return 0;
    }

  regex_t re;
  int err = regcomp( &re, expr, REG_EXTENDED|REG_NOSUB );
  if( err!=0 )
    {
    Warning("Cannot compile e-mail RegExp: [%s] (%d)", expr, err);
    return -100 + err;
    }

  int status = regexec( &re, str, 0, NULL, 0 );
  regfree( &re );

  return status;
  }

char* ExtractRegexFromString( char* expr, char* str )
  {
  if( EMPTY( expr ) )
    {
    return NULL;
    }

  if( EMPTY( str ) )
    {
    return NULL;
    }

  regex_t re;
  int err = regcomp( &re, expr, REG_EXTENDED );
  if( err!=0 )
    {
    Warning("Cannot compile e-mail RegExp: [%s] (%d)", expr, err);
    return NULL;
    }

  regmatch_t matches[100];
  int status = regexec( &re, str, 1, matches, 0 );
  regfree( &re );
  if( status!=0 )
    {
    return NULL;
    }

  /* printf( "Match found at %d ~ %d\n",
             (int)(matches[0].rm_so), (int)(matches[0].rm_eo) );
  */
  int l = matches[0].rm_eo - matches[0].rm_so;
  char* buf = calloc( l+1, sizeof(char) );
  strncpy( buf, str+matches[0].rm_so, l );
  return buf;
  }

char* GenerateAllocateGUID()
  {
  char buf[100];

  uuid_t x;
  uuid_generate( x );
  uuid_unparse_lower( x, buf );

  return strdup( buf );
  }

int CompareStrings( const void* vA, const void* vB)
  {
  const char** pa = (const char**)vA;
  const char** pb = (const char**)vB;
  const char* a = *pa;
  const char* b = *pb;

  return strcmp( a, b );
  }

void MaskNonPrintableChars( unsigned char* str )
  {
  if( EMPTY( str ) )
    return;
  while( (*str)!=0 )
    {
    int c = *str;
    if( ! isprint( c ) )
      *str = '?';
    ++str;
    }
  }

char* EncodeNonPrintableChars( unsigned char* str, unsigned char* omitChars )
  {
  if( EMPTY( str ) )
    return (char*)str;

  int newLen = 0;
  for( unsigned char* ptr=str; (*ptr)!=0; ++ptr )
    {
    int c = *ptr;
    if( omitChars!=NULL && strchr( (char*)omitChars, c )!=NULL )
      continue;

    if( ! isprint( c ) )
      newLen += 3;
    else
      newLen += 1;
    }

  char* newStr = (char*)SafeCalloc( newLen+5, sizeof(char), "EncodeNonPrintableChars" );
  char* dst = newStr;
  for( unsigned char* src=str; (*src)!=0; ++src )
    {
    int c = *src;
    if( omitChars!=NULL && strchr( (char*)omitChars, c )!=NULL )
      continue;

    if( ! isprint( c ) )
      {
      int c1 = (c & 0xf0) >> 4;
      int c2 = (c & 0x0f);
      *(dst++) = '=';
      *(dst++) = upperHexDigits[c1];
      *(dst++) = upperHexDigits[c2];
      }
    else
      *(dst++) = *src;
    }
  *dst = 0;

  return newStr;
  }
