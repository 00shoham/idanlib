#ifndef _INCLUDE_COOKIE
#define _INCLUDE_COOKIE

#define MIN_SESSION_TTL             60  /* 1 minute */
#define DEFAULT_ID_OF_AUTH_COOKIE   "IL-SESSION-STATE"
#define HTTP_COOKIE_PREFIX          "HTTP_COOKIE"

extern char **environ;

char* SimpleHash( char* string, int nBytes );

void ClearSessionCookieSpecific( char* cookieId );
char* GetSessionCookieFromEnvironmentSpecific( char* cookieId );
void ClearSessionCookieDefault();
char* GetSessionCookieFromEnvironmentDefault();

char* EncodeIdentityInCookie( char* userID, char* remoteAddr, char* userAgent, long ttlSeconds, uint8_t* key );
int GetIdentityFromCookie( char* cookie, char** userPtr,
                           long* expiryPtr, long* durationPtr,
                           char* remoteAddr, char* userAgent,
                           uint8_t* key );
int PrintSessionCookie( char* cookieVarName,
                        char* userID,
                        long ttlSeconds,
                        char* remoteAddrVariable,
                        char* userAgentVariable,
                        uint8_t* key );
void ClearSessionCookie();
char* GetValidatedUserIDFromHttpHeaders( uint8_t* key, char* cookieText );

char* ExtractUserIDOrDieEx( enum callMethod cm,
                            char* userVarName, char* cookieVarName,
                            char* myUrlVarName,
                            char* authURL,
                            uint8_t* key,
                            char* cssPath );
#endif

