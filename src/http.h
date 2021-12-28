#ifndef _INCLUDE_HTTP
#define _INCLUDE_HTTP

#define MAX_ENCODE 2000

#define DEFAULT_USER_ENV_VAR "REMOTE_USER"

#define LF 10
#define CR 13

#define HEXDIGITS "0123456789abcdefABCDEF"

enum httpMethod { HTTP_INVALID, HTTP_GET, HTTP_POST };
enum state { ST_TEXT, ST_GOTCR, ST_GOTNL };

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

char* ExtractUserIDOrDie( enum callMethod cm, char* envVarName );

int StringMatchesUserIDFormat( char* userID );

#endif
