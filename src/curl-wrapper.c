#include "utils.h"

void AllocateData( _DATA* d, size_t n )
  {
  if( d==NULL )
    Error( "Cannot allocate NULl _DATA structure" );
  d->data = (unsigned char*)SafeCalloc( n, sizeof(unsigned char*), "HTTP response" );
  d->ptr = d->data;
  d->sizeAllocated = n;
  }

void FreeData( _DATA* d )
  {
  if( d==NULL )
    {
    Warning( "Cannot free NULl _DATA structure" );
    return;
    }
  if( d->data==NULL )
    Warning( "_DATA structure has no data" );
  else
    {
    free( d->data );
    d->data = NULL;
    d->ptr = NULL;
    d->sizeAllocated = 0;
    }
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
    Warning( "CurlWriteback with NULl response buffer" );
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

CURLcode WebTransaction( char* url,
                         enum httpMethod method,
                         char* queryString,
                         char* postData,
                         int postDataBinarySize, /* set if postData includes \000s */
                         _DATA* postResult,
                         char* urlUsername,
                         char* urlPassword,
                         char* proxyURL,
                         char* proxyUsername,
                         char* proxyPassword,
                         int timeoutSeconds,
                         char* userAgent,
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
  char* whatToPost = NULL;
  if( method==HTTP_POST )
    {
    curl_easy_setopt( curl, CURLOPT_POST, 1 );
    if( NOTEMPTY( postData ) )
      {
      whatToPost = postData;
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
    curl_easy_setopt( curl, CURLOPT_TIMEOUT, timeoutSeconds );

  if( NOTEMPTY( userAgent ) )
    curl_easy_setopt( curl, CURLOPT_USERAGENT, userAgent );
  else
    curl_easy_setopt( curl, CURLOPT_USERAGENT, DEFAULT_USER_AGENT );

  curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, skipVerifyHost ? 0 : 2 );
  curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, skipVerifyPeer ? 0 : 1 );

  curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L);

  if( postResult!=NULL )
    {
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)postResult );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, CurlWriteback );
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

