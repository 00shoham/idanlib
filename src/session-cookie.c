#include "utils.h"

char* EncodeIdentityInCookie( char* userID, char* remoteAddr, long ttlSeconds, uint8_t* key )
  {
  if( EMPTY( userID ) || EMPTY( remoteAddr) || EMPTY( key ) || ttlSeconds<=0 )
    {
    Warning( "Invalid arguments to EncodeIdentityInCookie" );
    return NULL;
    }

  time_t tNow = time(NULL);
  time_t tEnd = tNow + ttlSeconds;

  char plaintext[BUFLEN];
  snprintf( plaintext, sizeof(plaintext)-1, "USER:%s\nADDR:%s\nEXPT:%ld\n",
            userID, remoteAddr, (long)tEnd );

  char* crypto = NULL;
  int cryptoLen = 0;
  int err = EncryptAES256Base64Encode( (uint8_t*)plaintext, strlen(plaintext),
                                       key, AES_KEYLEN,
                                       &crypto, &cryptoLen );

  if( err || EMPTY( crypto ) || cryptoLen==0 )
    Error( "Failed to encrypt session state: %d", err );

  return strdup( crypto );
  }

int GetIdentityFromCookie( char* cookie, char** userID, char* remoteAddr, uint8_t* key )
  {
  if( EMPTY( cookie ) || userID==NULL || EMPTY( remoteAddr) || EMPTY( key ) )
    {
    Warning( "Invalid arguments to ValidateIdentityFromCookie" );
    return -1;
    }

  uint8_t* plaintext = NULL;
  size_t plaintextLen = 0;

  int err = Base64DecodeDecryptAES256( cookie, strlen(cookie), key, AES_KEYLEN,
                                       &plaintext, &plaintextLen );

  if( err || EMPTY( plaintext ) || plaintextLen==0 )
    {
    Warning( "Failed to decrypt session state: %d", err );
    return -1000 + err;
    }

  int gotUser = 0;
  int gotAddr = 0;
  int gotTime = 0;

  char* ptr = NULL;
  for( char* line = strtok_r( (char*)plaintext, "\r\n", &ptr );
       line!=NULL; line = strtok_r( NULL, "\r\n", &ptr ) )
    {
    if( strncmp( line, "USER:", 5 )==0 )
      {
      char* pUser = line + 5;
      if( EMPTY( pUser ) )
        {
        Warning( "Empty USER in session cookie" );
        return -2;
        }
      *userID = strdup( pUser );
      gotUser = 1;
      }
    else if( strncmp( line, "ADDR:", 5 )==0 )
      {
      char* pAddr = line + 5;
      if( EMPTY( pAddr ) )
        {
        Warning( "Empty ADDR in session cookie" );
        return -3;
        }
      if( strcmp( pAddr, remoteAddr )!=0 )
        {
        Warning( "User has changed remote address (from %s to %s)", pAddr, remoteAddr );
        return -4;
        }
      gotAddr = 1;
      }
    else if( strncmp( line, "EXPT:", 5 )==0 )
      {
      char* pTime = line + 5;
      if( EMPTY( pTime ) )
        {
        Warning( "Empty ADDR in session cookie" );
        return -5;
        }
      long expT = 0;
      if( sscanf( pTime, "%ld", &expT )!=1 )
        {
        Warning( "Invalid EXPT (%s) in session cookie", pTime );
        return -6;
        }
      time_t tNow = time(NULL);
      if( (long)tNow >= expT )
        {
        Warning( "Session expired" );
        return -7;
        }
      gotTime = 1;
      }
    else if( EMPTY( line ) )
      { /* whatever */ }
    else
      {
      Warning( "Unexpected line in session state: %s", line );
      return -8;
      }
    }

  if( gotUser && gotAddr && gotTime)
    return 0;

  if( ! gotUser )
    {
    Warning( "Session state does not specify the user" );
    return -9;
    }

  if( ! gotAddr )
    {
    Warning( "Session state does not specify the address" );
    return -10;
    }

  /* gotTime must be 0 to reach this */
  Warning( "Session state does not specify the time" );
  return -11;
  }

int PrintSessionCookie( char* userID, long ttlSeconds, char* remoteAddrVariable, uint8_t* key )
  { 
  if( EMPTY( userID ) )
    { 
    Warning( "PrintSessionCookie() - must specify user ID" );
    return -1;
    }

  if( ttlSeconds < MIN_SESSION_TTL )
    { 
    Warning( "PrintSessionCookie() - must specify user TTL of at least %d", MIN_SESSION_TTL );
    return -2;
    }

  char* varName = EMPTY( remoteAddrVariable ) ? DEFAULT_REMOTE_ADDR : remoteAddrVariable;
  char* addr = getenv( varName );
  if( EMPTY( addr ) )
    {
    Warning( "PrintSessionCookie() - cannot discern remote address from HTTP header %s", varName );
    return -3;
    }

  char* cookie = EncodeIdentityInCookie( userID, addr, ttlSeconds, key );

  printf( "Set-Cookie: %s=%s; Max-Age=%ld\n", COOKIE_ID, cookie, ttlSeconds);
  free( cookie );

  return 0;
  }

void ClearSessionCookie()
  { 
  printf( "Set-Cookie: %s=; Max-Age=-1\n", COOKIE_ID );
  }

char* GetSessionCookieFromEnvironment()
  {
  char cookiePrefix[BUFLEN];
  snprintf( cookiePrefix, sizeof(cookiePrefix)-1, "%s=%s=",
            HTTP_COOKIE_PREFIX, COOKIE_ID );

  int l = strlen( cookiePrefix );
  for( int i=0; environ[i]!=NULL; ++i )
    {
    char* env = environ[i];
    if( *env!=0 && strncmp( env, cookiePrefix, l )==0 )
      {
      return env + l;
      }
    }
  return NULL;
  }

char* GetValidatedUserIDFromHttpHeaders( uint8_t* key )
  {
  char* cookie = GetSessionCookieFromEnvironment();
  if( cookie==NULL )
    {
    Warning( "No session cookie" );
    return NULL;
    }

  char* remoteAddr = getenv( DEFAULT_REMOTE_ADDR );
  if( remoteAddr==NULL )
    {
    Warning( "No remote address" );
    return NULL;
    }

  char* userID = NULL;
  int err = GetIdentityFromCookie( cookie, &userID, remoteAddr, key );
  if( err )
    {
    Warning( "GetIdentityFromCookie failed - %d", err );
    return NULL;
    }

  return userID;
  }

