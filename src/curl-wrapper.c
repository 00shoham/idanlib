#include "utils.h"

/* #define DEBUG 1 */
#undef DEBUG

void AllocateData( _DATA* d, size_t n )
  {
  if( d==NULL )
    Error( "Cannot allocate NULL _DATA structure" );
  d->data = (unsigned char*)SafeCalloc( n, sizeof(unsigned char*), "HTTP response" );
  d->ptr = d->data;
  d->sizeAllocated = n;
  }

void FreeData( _DATA* d )
  {
  if( d==NULL )
    {
    Warning( "Cannot free NULL _DATA structure" );
    return;
    }

  if( d->data==NULL )
    Warning( "_DATA structure has no data" );
  else
    free( d->data );

  d->data = NULL;
  d->ptr = NULL;
  d->sizeAllocated = 0;
  /* parent has to handle free( d ); */
  }

size_t CurlWriteback( void *payload,
                      size_t size, size_t nmemb,
                      void *data )
  {
  size_t messageSize = size * nmemb;
  if( messageSize<=0 )
    {
    Warning( "CurlWriteback with no data" );
    return 0;
    }

  _DATA* buf = (_DATA*)data;
  if( buf==NULL )
    {
    Warning( "CurlWriteback with NULL response buffer" );
    return 0;
    }

  /* (Re)allocate if needed */
  if( buf->data==NULL )
    {
    size_t n = messageSize * 2 + 10;
    AllocateData( buf, n );
    }
  else
    {
    size_t spaceUsed = buf->ptr - buf->data;
    size_t spaceRemaining = buf->sizeAllocated - spaceUsed;
    -- spaceRemaining; /* zero termination and safety */
    if( spaceRemaining <= messageSize )
      {
      size_t newSize = buf->sizeAllocated + messageSize * 2 + 10;
      buf->data = (unsigned char*)realloc( buf->data, newSize );
      if( buf->data==NULL )
        Error( "Failed to reallocate %ld buffer for HTTP response", (long)newSize );
      buf->ptr = buf->data + spaceUsed;
      buf->sizeAllocated = newSize;
      }
    }

  /* Update the buffer */
  if( memcpy( buf->ptr, payload, messageSize ) != buf->ptr )
    Error( "Failed to copy %ld bytes of HTTP response", (long)messageSize );

  buf->ptr += messageSize;
  *( buf->ptr ) = 0;

  return messageSize;
  }

#ifdef DEBUG
struct curlDebugData
  {
  int nohex; /* 1 or 0 */
  };
 
static
void CurlDebugDump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          int nohex)
  {
  size_t i;
  size_t c;
 
  unsigned int width = 0x10;
 
  /* without the hex output, we can fit more on screen */
  if(nohex)
    width = 0x40;
 
  fprintf( stream, "%s, %10.10lu bytes (0x%8.8lx)\n",
           text, (unsigned long)size, (unsigned long)size );
 
  for( i=0; i<size; i += width )
    {
    fprintf(stream, "%4.4lx: ", (unsigned long)i);
 
    if( !nohex )
      {
      /* hex not disabled, show it */
      for( c = 0; c < width; c++ )
        if( i + c < size )
          fprintf(stream, "%02x ", ptr[i + c]);
        else
          fputs("   ", stream);
      }
 
    for( c = 0; (c < width) && (i + c < size); c++ )
      {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if( nohex
          && (i + c + 1 < size)
          && ptr[i + c] == 0x0d
          && ptr[i + c + 1] == 0x0a )
        {
        i += (c + 2 - width);
        break;
        }

      fprintf( stream, "%c",
               (ptr[i + c] >= 0x20) && (ptr[i + c]<0x80) ? ptr[i + c] : '.' );

      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if( nohex
          && (i + c + 2 < size)
          && ptr[i + c + 1] == 0x0d
          && ptr[i + c + 2] == 0x0a)
        {
        i += (c + 3 - width);
        break;
        }
      }

    fputc('\n', stream); /* newline */
    }

  fflush(stream);
  }
 
static int CurlDebugTrace( CURL *handle, curl_infotype type,
                           char *data, size_t size,
                           void *userp )
  {
  struct curlDebugData *config = (struct curlDebugData *)userp;
  const char *text;
  (void)handle; /* prevent compiler warning */
 
  switch(type)
    {
    case CURLINFO_TEXT:
      fprintf(stderr, "== Info: %s", data);
      /* FALLTHROUGH */
    default: /* in case a new one is introduced to shock us */
      return 0;
   
    case CURLINFO_HEADER_OUT:
      text = "=> Send header";
      break;

    case CURLINFO_DATA_OUT:
      text = "=> Send data";
      break;

    case CURLINFO_SSL_DATA_OUT:
      text = "=> Send SSL data";
      break;

    case CURLINFO_HEADER_IN:
      text = "<= Recv header";
      break;

    case CURLINFO_DATA_IN:
      text = "<= Recv data";
      break;

    case CURLINFO_SSL_DATA_IN:
      text = "<= Recv SSL data";
      break;
    }
 
  CurlDebugDump(text, stderr, (unsigned char *)data, size, config->nohex);
  return 0;
  }
#endif 

CURLcode WebTransaction( char* url,
                         enum httpMethod method,
                         char* postData,
                         int postDataBinarySize, /* set if postData includes \000s */
                         char* postContentType,
                         _DATA* postResult,
                         char* urlUsername,
                         char* urlPassword,
                         char* proxyURL,
                         char* proxyUsername,
                         char* proxyPassword,
                         int timeoutSeconds,
                         _TAG_VALUE* httpHeaders,
                         char* cookieFile,
                         int skipVerifyPeer,
                         int skipVerifyHost,
                         char** errorMsg
                         )
  {
  char errorBuffer[ CURL_ERROR_SIZE+10 ];
  errorBuffer[0] = 0;

  if( EMPTY( url ) )
    {
    Warning( "WebTransaction with no URL" );
    return -100;
    }

  if( method != HTTP_GET && method != HTTP_POST )
    {
    Warning( "WebTransaction with invalid httpMethod" );
    return -101;
    }

  if( NOTEMPTY( postData ) && method!=HTTP_POST )
    {
    Warning( "WebTransaction with post data but not HTTP POST method" );
    return -102;
    }

  if( ( NOTEMPTY( urlUsername ) && EMPTY( urlPassword ) )
      || ( EMPTY( urlUsername ) && NOTEMPTY( urlPassword ) ) )
    {
    Warning( "WebTransaction - username/password must be set as a pair" );
    return -103;
    }

  if( ( NOTEMPTY( proxyUsername ) && EMPTY( proxyPassword ) )
      || ( EMPTY( proxyUsername ) && NOTEMPTY( proxyPassword ) ))
    {
    Warning( "WebTransaction - proxy username/password must be set as a pair" );
    return -104;
    }

  if( NOTEMPTY( proxyUsername ) && EMPTY( proxyURL ) )
    {
    Warning( "WebTransaction - proxy creds set but no proxy URL" );
    return -105;
    }

  CURLcode res = CURLE_OK;

  res = curl_global_init( CURL_GLOBAL_ALL );
  if( res )
    {
    Warning( "Failed to init CURL with curl_global_init" );
    return -200;
    }

  CURL* curl = curl_easy_init();
  if( curl==NULL )
    {
    Warning( "Failed to init CURL with curl_easy_init" );
    curl_global_cleanup();
    return -201;
    }

  if( postResult!=NULL )
    {
    if( postResult->data!=NULL )
      free( postResult->data );
    postResult->data = NULL;
    postResult->ptr = NULL;
    postResult->sizeAllocated = 0;
    }

  curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, errorBuffer );

  curl_easy_setopt( curl, CURLOPT_URL, url );

  struct curl_slist *hs=NULL;
  int nHeaders = 0;
  for( _TAG_VALUE* tv=httpHeaders; tv!=NULL; tv=tv->next )
    {
    if( NOTEMPTY( tv->tag ) && NOTEMPTY( tv->value ) )
      {
      if( strcasecmp( tv->tag, "user-agent" )==0 )
        {
        curl_easy_setopt( curl, CURLOPT_USERAGENT, tv->value );
        }
      else
        {
        char buf[BUFLEN];
        snprintf( buf, sizeof(buf)-1, "%s=%s", tv->tag, tv->value );
        hs = curl_slist_append( hs, buf );
        ++nHeaders;
        }
      }
    }

  char* whatToPost = NULL;
  if( method==HTTP_POST )
    {
    curl_easy_setopt( curl, CURLOPT_POST, 1 );
    if( NOTEMPTY( postData ) )
      {
      whatToPost = postData;
      if( NOTEMPTY( postContentType ) )
        {
        char buf[BUFLEN];
        snprintf( buf, sizeof(buf)-1, "Content-Type: %s", postContentType );
        hs = curl_slist_append( hs, buf );
        ++nHeaders;
        }

      if( postDataBinarySize )
        {
        whatToPost = curl_easy_escape( curl, postData, postDataBinarySize );
        }
      curl_easy_setopt( curl, CURLOPT_POSTFIELDS, whatToPost );
      /* curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, (longL );
       * - if postData not 0-terminated
       */
      }
    else
      {
      curl_easy_setopt( curl, CURLOPT_POST, 0 );
      }
    }

  if( NOTEMPTY( urlUsername ) && NOTEMPTY( urlPassword ) )
    {
    curl_easy_setopt( curl, CURLOPT_USERNAME, urlUsername );
    curl_easy_setopt( curl, CURLOPT_PASSWORD, urlPassword );
    }

  if( NOTEMPTY( proxyURL ) )
    {
    curl_easy_setopt( curl, CURLOPT_PROXY, proxyURL );
    if( NOTEMPTY( proxyUsername ) && NOTEMPTY( proxyPassword ) )
      {
      curl_easy_setopt( curl, CURLOPT_PROXYUSERNAME, proxyUsername );
      curl_easy_setopt( curl, CURLOPT_PROXYPASSWORD, proxyPassword );
      }
    }

  curl_easy_setopt( curl, CURLOPT_ACCEPT_ENCODING, "" );
  if( NOTEMPTY( cookieFile ) )
    {
    curl_easy_setopt( curl, CURLOPT_COOKIEFILE, cookieFile );
    curl_easy_setopt( curl, CURLOPT_COOKIEJAR, cookieFile );
    }

  if( timeoutSeconds>0 )
    {
    curl_easy_setopt( curl, CURLOPT_TIMEOUT, timeoutSeconds );
    curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, timeoutSeconds );
    }
  else
    Warning( "WebTransaction: no timeout specified!" );

  curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, skipVerifyHost ? 0 : 2 );
  curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, skipVerifyPeer ? 0 : 1 );

  curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L);

  if( postResult!=NULL )
    {
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)postResult );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, CurlWriteback );
    }

#ifdef DEBUG
  struct curlDebugData config;
  config.nohex = 1; /* enable ascii tracing */
  curl_easy_setopt( curl, CURLOPT_DEBUGFUNCTION, CurlDebugTrace );
  curl_easy_setopt( curl, CURLOPT_DEBUGDATA, &config );
  curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L );
#endif

  if( nHeaders )
    {
    curl_easy_setopt( curl, CURLOPT_HTTPHEADER, hs );
    }

  res = curl_easy_perform( curl );
  if( res != CURLE_OK )
    {
    Warning( "curl returned error %s", errorBuffer );
    if( errorMsg )
      *errorMsg = strdup( errorBuffer );
    }

  curl_easy_cleanup( curl );
  curl_global_cleanup();

  /* did we url encode the post data?  free it then. */
  if( whatToPost != NULL
      && postData != NULL
      && postData != whatToPost )
    {
    free( whatToPost );
    whatToPost = NULL;
    }

  return (int)res;
  }

int WebTransactionTV( _TAG_VALUE* args,
                      _DATA* postResult,
                      char** errorMsg )
  {
  if( args==NULL )
    {
    Warning( "WebTransactionTV with no arguments" );
    return -1;
    }

  char* url = GetTagValue( args, "url" );
  if( url==NULL )
    {
    Warning( "WebTransactionTV with no 'url' argument" );
    return -2;
    }

  char* methodString = GetTagValue( args, "method" );
  if( methodString==NULL )
    {
    Warning( "WebTransactionTV with no 'method' argument" );
    return -3;
    }
  if( strcmp( methodString, "get" )!=0
      && strcmp( methodString, "post" )!=0 )
    {
    Warning( "WebTransactionTV 'method' argument must be 'get' or 'post'" );
    return -4;
    }

  enum httpMethod method = HTTP_GET;
  if( strcmp( methodString, "post" )==0 )
    method = HTTP_POST;

  char* postData = GetTagValue( args, "post_data" );
  if( postData!=NULL && method != HTTP_POST )
    {
    Warning( "WebTransactionTV 'method' is not 'post' but post data provided" );
    return -5;
    }

  int postDataBinarySize = GetTagValueInt( args, "post_size" );
  if( postData!=NULL )
    {
    if( postDataBinarySize==INVALID_INT )
      postDataBinarySize = 0;
    }
  else
    {
    if( postDataBinarySize==INVALID_INT )
      postDataBinarySize = 0;
    else
      {
      Warning( "WebTransactionTV: post_size provided for non-POST request" );
      return -6;
      }
    }

  char* postContentType = GetTagValue( args, "CONTENT_TYPE" );

  char* urlUsername = GetTagValue( args, "url_username" );
  char* urlPassword = GetTagValue( args, "url_password" );

  char* proxyURL = GetTagValue( args, "proxy_url" );
  char* proxyUsername = GetTagValue( args, "proxy_username" );
  char* proxyPassword = GetTagValue( args, "proxy_password" );

  int timeoutSeconds = GetTagValueInt( args, "timeout_seconds" );;
  if( timeoutSeconds==INVALID_INT )
    timeoutSeconds = 0;

  char* cookieFile = GetTagValue( args, "cookie_file" );

  int skipVerifyPeer = GetTagValueInt( args, "skip_verify_peer" );
  if( skipVerifyPeer==INVALID_INT )
    skipVerifyPeer = 0;

  int skipVerifyHost = GetTagValueInt( args, "skip_verify_host" );
  if( skipVerifyHost==INVALID_INT )
    skipVerifyHost = 0;

  _TAG_VALUE* httpHeaders = FindTagValue( args, "http_headers" );
  if( httpHeaders!=NULL )
    httpHeaders = httpHeaders->subHeaders;

  return WebTransaction( url,
                         method,
                         postData,
                         postDataBinarySize, /* set if postData includes \000s */
                         postContentType,
                         postResult,
                         urlUsername,
                         urlPassword,
                         proxyURL,
                         proxyUsername,
                         proxyPassword,
                         timeoutSeconds,
                         httpHeaders,
                         cookieFile,
                         skipVerifyPeer,
                         skipVerifyHost,
                         errorMsg
                         );
  }

extern char* sha1hexdigits;

char* URLEncode( char* raw )
  {
  if( EMPTY( raw ) )
    return raw;

  int l = strlen( raw );
  char* encoded = (char*)SafeCalloc( l*3+1, sizeof(char), "URL encoded string" );
  char* dst = encoded;

  for( char* src = raw; *raw!=0; ++raw )
    {
    int c = *src;
    if( c == ' ' )
      *(dst++) = '+';
    else if( isalnum( c ) )
      *(dst++) = c;
    else
      {
      *(dst++) = '%';
      int h = (c & 0xf0) >> 4;
      int l = (c & 0x0f);
      *(dst++) = sha1hexdigits[h];
      *(dst++) = sha1hexdigits[l];
      }
    }
  *(dst) = 0;

  return encoded;
  }

char* URLDecode( char* encoded )
  {
  if( EMPTY( encoded ) )
    return encoded;

  int l = strlen( encoded );
  char* decoded = (char*)SafeCalloc( l, sizeof(char), "URL decoded string" );
  char* src = encoded;
  char* dst = decoded;
  for( ; *src!=0; ++src )
    {
    if( *src=='+' )
      {
      *(dst++) = ' ';
      }
    else if( *src=='%'
             && isxdigit( *(src+1) )
             && isxdigit( *(src+2) ) )
      {
      int ch = *(++src);
      int cl = *(++src);
      int nh = HexDigitNumber( ch );
      int nl = HexDigitNumber( cl );
      int byte = nh << 4 | nl;
      *(dst++) = byte;
      }
    else
      *(dst++) = *src;
    }

  *dst = 0;
  return decoded;
  }
