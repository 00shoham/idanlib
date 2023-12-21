#ifndef _INCLUDE_COOKIE
#define _INCLUDE_COOKIE

#define MIN_SESSION_TTL     60  /* 1 minute */
#define COOKIE_ID           "IL-SESSION-STATE"
#define HTTP_COOKIE_PREFIX  "HTTP_COOKIE"

extern char **environ;

char* SimpleHash( char* string, int nBytes );
void ClearCookie( char* cookie );
void ClearSessionCookie();
char* GetSessionCookieFromEnvironment();

char* EncodeIdentityInCookie( char* userID, char* remoteAddr, char* userAgent, long ttlSeconds, uint8_t* key );
int GetIdentityFromCookie( char* cookie, char** userPtr,
                           long* expiryPtr, long* durationPtr,
                           char* remoteAddr, char* userAgent,
                           uint8_t* key );
int PrintSessionCookie( char* userID, long ttlSeconds, char* remoteAddrVariable, char* userAgentVariable, uint8_t* key );
void ClearSessionCookie();
char* GetValidatedUserIDFromHttpHeaders( uint8_t* key, char* cookieText );

char* ExtractUserIDOrDieEx( enum callMethod cm,
                            char* userVarName, char* cookieVarName,
                            char* myUrlVarName,
                            char* authURL,
                            uint8_t* key );
#endif

