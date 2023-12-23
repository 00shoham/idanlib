#ifndef _INCLUDE_HTTP
#define _INCLUDE_HTTP

#define MAX_ENCODE 2000

#define DEFAULT_USER_ENV_VAR         "REMOTE_USER"
#define DEFAULT_REQUEST_URI_ENV_VAR  "REQUEST_URI"
#define DEFAULT_HTTP_HOST_ENV_VAR    "HTTP_HOST"
#define DEFAULT_HTTP_REFERER         "HTTP_REFERER"
#define DEFAULT_AUTH_URL             "/cgi-bin/auth2cookie"


#define LF 10
#define CR 13

#define HEXDIGITS "0123456789abcdefABCDEF"

enum httpMethod { HTTP_INVALID, HTTP_GET, HTTP_POST };
enum state { ST_TEXT, ST_GOTCR, ST_GOTNL };

typedef struct _cgi_header
  {
  char* separatorString;
  _TAG_VALUE* files;
  _TAG_VALUE* headers;
  } _CGI_HEADER;

enum postState
  {
  ps_FIRSTHEADER,
  ps_SEPARATOR,
  ps_HEADERLINE,
  ps_VARIABLE,
  ps_DATA
  };

int ParsePostData( FILE* stream,
                   _CGI_HEADER *header,
                   int (*funcPtr)( _CGI_HEADER* ) );

/* did a bit of UI call get called via a UI or API? */
enum callMethod { cm_invalid, cm_ui, cm_api };

int GetSocketFromHostAndPort( char* serverName, int portNum,
                              struct addrinfo** aiPtr );
int TCPPortSendReceive( char* serverName, int portNum,
                        char* httpPostText,
                        char* responseBuffer, int responseBufLen );
int CountTextLines( char* rawResponse );
void ChopResponseIntoLines( char* buffer, char** lines, int n );
int HTTPSendReceive( char* serverName, int portNum,
                     char* post, char* response, int responseLen );
char* Encode( int nBytes, unsigned char* bytes );
unsigned char* Decode( char* string );
int GetArgumentFromQueryString( char** bufPtr, char* keyword, char* regExp );

/* apache2 specific: */
char* GetWebUser();
char* GetWebGroup();

void ClearCookie( char* cookie );
char* GetCookieFromEnvironment( char* cookie );

char* RemoveURLEncoding( char* src, char* dst );
_TAG_VALUE* ParseQueryString( _TAG_VALUE* list, char* string );

void CGIHeader( char* contentType,
                long contentLength,
                char* pageTitle,
                int nCSS, char** css,
                int nJS, char** js
                );
void CGIFooter();

_TAG_VALUE* ParseHeaderSubVariables( _TAG_VALUE* list, char* buf );
_TAG_VALUE* ParseHeaderLine( _TAG_VALUE* workingHeaders, char* buf );
_TAG_VALUE* HeadersContainTag( _TAG_VALUE* list, char* tag );
int HeadersContainTagAndSubTag( _TAG_VALUE* list, char* tag, char* subTag );
_TAG_VALUE* ParseValue( char* buf, _TAG_VALUE* workingHeaders );
void DownloadFile( long filesize, char* path, char* fileName );
void DownloadChunkedStream( int fd, char* fileName );

char* ExtractUserIDOrDie( enum callMethod cm, char* envVarName );

int StringMatchesUserIDFormat( char* userID );


char* URLEncode( char* raw );
char* URLDecode( char* encoded );
int IsURLEncoded( char* string ); /* 0==true */


char* MyRelativeRequestURL( char* reqVarName );
char* FullRequestURL( char* hostVarName, char* reqVarName );
void RedirectToUrl( char* url, char* cssPath );

#endif
