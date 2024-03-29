#ifndef _CURL_WRAPPER
#define _CURL_WRAPPER

typedef struct _data
  {
  size_t sizeAllocated;
  unsigned char* data;
  unsigned char* ptr;
  } _DATA;

#define DEFAULT_USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"
#define DEFAULT_COOKIES_FILE "/tmp/cookies.txt"

void AllocateData( _DATA* d, size_t n );
void FreeData( _DATA* d );
size_t CurlWriteback( void *payload,
                      size_t size, size_t nmemb,
                      void *data );
CURLcode WebTransaction( char* url,
                         enum httpMethod method,
                         char* postData,
                         int postDataBinarySize, /* set *only* if postData includes \000s */
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
                         );

/* same as above but input args come from a _TAG_VALUE linked list */
int WebTransactionTV( _TAG_VALUE* args,
                      _DATA* postResult,
                      char** errorMsg );


#endif
