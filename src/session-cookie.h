#ifndef _INCLUDE_COOKIE
#define _INCLUDE_COOKIE

#define MIN_SESSION_TTL     60  /* 1 minute */
#define COOKIE_ID           "IL-SESSION-STATE"
#define HTTP_COOKIE_PREFIX  "HTTP_COOKIE"

extern char **environ;

char* SimpleHash( char* string, int nBytes );
void ClearCookie( char* cookie );
void ClearSessionCookie();
char* GetCookieFromEnvironment( char* cookie );
char* GetSessionCookieFromEnvironment();

char* EncodeIdentityInCookie( char* userID, char* remoteAddr, char* userAgent, long ttlSeconds, uint8_t* key );
int GetIdentityFromCookie( char* cookie, char** userID, char* remoteAddr, uint8_t* key );
int PrintSessionCookie( char* userID, long ttlSeconds, char* remoteAddrVariable, char* userAgentVariable, uint8_t* key );
void ClearSessionCookie();
char* GetValidatedUserIDFromHttpHeaders( uint8_t* key );

#endif

