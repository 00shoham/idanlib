#ifndef _INCLUDE_COOKIE
#define _INCLUDE_COOKIE

#define MIN_SESSION_TTL     60  /* 1 minute */
#define COOKIE_ID           "IL-SESSION-STATE"
#define HTTP_COOKIE_PREFIX  "HTTP_COOKIE"

extern char **environ;

char* EncodeIdentityInCookie( char* userID, char* remoteAddr, long ttlSeconds, uint8_t* key );
int GetIdentityFromCookie( char* cookie, char** userID, char* remoteAddr, uint8_t* key );
int PrintSessionCookie( char* userID, long ttlSeconds, char* remoteAddrVariable, uint8_t* key );
void ClearSessionCookie();
char* GetSessionCookieFromEnvironment();
char* GetValidatedUserIDFromHttpHeaders( uint8_t* key );

#endif

