#include "utils.h"

#define USER_AGENT_HASH_LEN 4

char* SimpleHash( char* string, int nBytes )
  {
  if( EMPTY( string ) )
    return NULL;
  if( nBytes<1 || nBytes>8 )
    Error( "SimpleHash() - 1-8 bytes" );

  uint8_t* raw = (uint8_t*)string;
  uint8_t* hash = (uint8_t*)SafeCalloc( nBytes + 1, sizeof(uint8_t), "Hash" );
  for( int i=0; string[i]!=0; ++i )
    hash[ i%nBytes ] ^= raw[i];

  int encLen = 0;
  char* encoded = EncodeToBase64( hash, nBytes, &encLen );
  if( encLen<=0 )
    Error( "Failed to EncodeToBase64 in SimpleHash" );
  free( hash );

  return encoded;
  }

void ClearCookie( char* cookie )
  { 
  printf( "Set-Cookie: %s=; Max-Age=-1\n", cookie );
  }

void ClearSessionCookie()
  { 
  ClearCookie( COOKIE_ID  );
  }

char* GetCookieFromEnvironment( char* cookie )
  {
  if( EMPTY( cookie ) )
    {
    Warning( "Cannot GetCookieFromEnvironment() without specifying a cookie" );
    return NULL;
    }

  int l = strlen( HTTP_COOKIE_PREFIX );
  char* cookies = NULL;
  for( int i=0; environ[i]!=NULL; ++i )
    {
    char* env = environ[i];
    if( *env!=0 && strncmp( env, HTTP_COOKIE_PREFIX, l )==0 && env[l]=='=' )
      {
      cookies = env + l + 1;
      break;
      }
    }

  if( cookies==NULL )
    {
    /* no HTTP cookies were provided by the web server */
    return NULL;
    }

  char* ptr = NULL;
  l = strlen( cookie );
  char* myCookies = strdup( cookies );
  for( char* token = strtok_r( myCookies, ";", &ptr ); token!=NULL; token = strtok_r( NULL, ";", &ptr ) )
    {
    while( (*token)==' ' )
      ++token;

    if( strncasecmp( token, cookie, l )==0 && *(token+l)=='=' )
      {
      char* thisCookie = strdup(token + l + 1); 
      free( myCookies );
      return thisCookie;
      }
    }
  free( myCookies );

  return NULL;
  }

char* GetSessionCookieFromEnvironment()
  {
  return GetCookieFromEnvironment( COOKIE_ID );
  }

char* EncodeIdentityInCookie( char* userID, char* remoteAddr, char* userAgent, long ttlSeconds, uint8_t* key )
  {
  if( EMPTY( userID )
      || EMPTY( remoteAddr )
      || EMPTY( userAgent )
      || EMPTY( key )
      || ttlSeconds<=0 )
    {
    Warning( "Invalid arguments to EncodeIdentityInCookie" );
    return NULL;
    }

  time_t tNow = time(NULL);
  time_t tEnd = tNow + ttlSeconds;

  char plaintext[BUFLEN];
  snprintf( plaintext, sizeof(plaintext)-1, "USER:%s\nADDR:%s\nUAGT:%s\nEXPT:%ld\n",
            userID, remoteAddr, userAgent, (long)tEnd );

  char* crypto = NULL;
  int cryptoLen = 0;
  int err = EncryptAES256Base64Encode( (uint8_t*)plaintext, strlen(plaintext),
                                       key, AES_KEYLEN,
                                       &crypto, &cryptoLen );

  if( err || EMPTY( crypto ) || cryptoLen==0 )
    Error( "Failed to encrypt session state: %d", err );

  return strdup( crypto );
  }

int GetIdentityFromCookie( char* cookie, char** userID, char* remoteAddr, char* userAgent, uint8_t* key )
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
  int gotUagt = 0;
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
        Warning( "User has moved - IP changed (from %s to %s)", remoteAddr, pAddr );
        /* just a warning -- return -4; */
        }
      gotAddr = 1;
      }
    else if( strncmp( line, "UAGT:", 5 )==0 )
      {
      char* uAgent = line + 5;
      if( EMPTY( uAgent ) )
        {
        Warning( "Empty UAGT in session cookie" );
        return -5;
        }
      if( strcmp( uAgent, userAgent )!=0 )
        {
        Warning( "User has changed user agent (from %s to %s)", userAgent, uAgent );
        return -6;
        }
      gotUagt = 1;
      }
    else if( strncmp( line, "EXPT:", 5 )==0 )
      {
      char* pTime = line + 5;
      if( EMPTY( pTime ) )
        {
        Warning( "Empty ADDR in session cookie" );
        return -7;
        }
      long expT = 0;
      if( sscanf( pTime, "%ld", &expT )!=1 )
        {
        Warning( "Invalid EXPT (%s) in session cookie", pTime );
        return -8;
        }
      time_t tNow = time(NULL);
      if( (long)tNow >= expT )
        {
        Warning( "Session expired" );
        return -9;
        }
      gotTime = 1;
      }
    else if( EMPTY( line ) )
      { /* whatever */ }
    else
      {
      Warning( "Unexpected line in session state: %s", line );
      return -10;
      }
    }

  if( gotUser && gotAddr && gotTime && gotUagt )
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

  if( ! gotUagt )
    {
    Warning( "Session state does not specify the user agent" );
    return -11;
    }

  /* gotTime must be 0 to reach this */
  Warning( "Session state does not specify the time" );
  return -12;
  }

int PrintSessionCookie( char* userID, long ttlSeconds, char* remoteAddrVariable, char* userAgentVariable, uint8_t* key )
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

  varName = EMPTY( userAgentVariable ) ? DEFAULT_USER_AGENT_VAR : userAgentVariable;
  char* uagt = getenv( varName );
  if( EMPTY( uagt ) )
    {
    Warning( "PrintSessionCookie() - cannot discern user agent from HTTP header %s", varName );
    return -4;
    }
  char* userAgentHash = SimpleHash( uagt, USER_AGENT_HASH_LEN );

  char* cookie = EncodeIdentityInCookie( userID, addr, userAgentHash, ttlSeconds, key );

  printf( "Set-Cookie: %s=%s; Max-Age=%ld\n", COOKIE_ID, cookie, ttlSeconds);
  free( cookie );

  return 0;
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

  char* userAgent = getenv( DEFAULT_USER_AGENT_VAR );
  if( userAgent==NULL )
    {
    Warning( "No user agent" );
    return NULL;
    }
  char* userAgentHash = SimpleHash( userAgent, USER_AGENT_HASH_LEN );

  char* userID = NULL;
  int err = GetIdentityFromCookie( cookie, &userID, remoteAddr, userAgentHash, key );
  if( err )
    {
    Warning( "GetIdentityFromCookie failed - %d", err );
    return NULL;
    }

  return userID;
  }

