#include "utils.h"

#define USER_AGENT_HASH_LEN 4

void ClearCookie( char* cookie )
  { 
  printf( "Set-Cookie: %s=; Max-Age=-1\n", cookie );
  }

void ClearSessionCookie()
  { 
  ClearCookie( COOKIE_ID  );
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
  snprintf( plaintext, sizeof(plaintext)-1, "USER:%s\nADDR:%s\nUAGT:%s\nEXPT:%ld\nDRTN:%ld\n",
            userID, remoteAddr, userAgent, (long)tEnd, (long)ttlSeconds );

  char* crypto = NULL;
  int cryptoLen = 0;
  int err = EncryptAES256Base64Encode( (uint8_t*)plaintext, strlen(plaintext),
                                       key, AES_KEYLEN,
                                       &crypto, &cryptoLen );

  if( err || EMPTY( crypto ) || cryptoLen==0 )
    Error( "Failed to encrypt session state: %d", err );

  return crypto;
  }

int GetIdentityFromCookie( char* cookie, char** userPtr,
                           long* expiryPtr, long* durationPtr,
                           char* remoteAddr, char* userAgent,
                           uint8_t* key )
  {
  if( EMPTY( cookie ) )
    {
    Warning( "Invalid arguments to GetIdentityFromCookie - no cookie" );
    return -1;
    }

  if( userPtr==NULL )
    {
    Warning( "Invalid arguments to GetIdentityFromCookie - no return PTR" );
    return -2;
    }

  if( EMPTY( remoteAddr ) )
    {
    Warning( "Invalid arguments to GetIdentityFromCookie - no remoteAddr" );
    return -3;
    }

  if( EMPTY( userAgent ) )
    {
    Warning( "Invalid arguments to GetIdentityFromCookie - no userAgent" );
    return -4;
    }

  if( key==NULL )
    {
    Warning( "Invalid arguments to GetIdentityFromCookie - no key" );
    return -5;
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
  int gotExpr = 0;
  /* int gotDrtn = 0; */

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
        free( plaintext );
        return -6;
        }
      *userPtr = strdup( pUser );
      gotUser = 1;
      }
    else if( strncmp( line, "ADDR:", 5 )==0 )
      {
      char* pAddr = line + 5;
      if( EMPTY( pAddr ) )
        {
        Warning( "Empty ADDR in session cookie" );
        free( plaintext );
        return -7;
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
        free( plaintext );
        return -8;
        }
      char* hash = NULL;
      if( userAgent!=NULL )
        hash = SimpleHash( userAgent, USER_AGENT_HASH_LEN );

      if( strcmp( uAgent, userAgent )==0 )
        { /* okay - plaintext user agent the same */ }
      else if( hash!=NULL && strcmp( uAgent, hash )==0)
        { /* okay - hash user agent the same */ }
      else
        {
        Warning( "User has changed user agent (from %s to %s / %s)",
                 userAgent, uAgent, NULLPROTECT( hash ) );
        if( hash )
          free( hash );
        free( plaintext );
        return -9;
        }

      if( hash )
        free( hash );

      gotUagt = 1;
      }
    else if( strncmp( line, "EXPT:", 5 )==0 )
      {
      char* pTime = line + 5;
      if( EMPTY( pTime ) )
        {
        Warning( "Empty EXPT in session cookie" );
        free( plaintext );
        return -10;
        }
      long expT = 0;
      if( sscanf( pTime, "%ld", &expT )!=1 )
        {
        Warning( "Invalid EXPT (%s) in session cookie", pTime );
        free( plaintext );
        return -11;
        }
      if( expiryPtr != NULL )
        {
        *expiryPtr = expT;
        }
      time_t tNow = time(NULL);
      if( (long)tNow >= expT )
        {
        Warning( "Session expired" );
        free( plaintext );
        return -12;
        }
      Notice( "Cookie expires at %08lx (%ds in future)",
              (long)expT, (int)(expT - tNow) );
      gotExpr = 1;
      }
    else if( strncmp( line, "DRTN:", 5 )==0 )
      {
      char* pTime = line + 5;
      if( EMPTY( pTime ) )
        {
        Warning( "Empty DRTN in session cookie" );
        free( plaintext );
        return -13;
        }
      long duration = 0;
      if( sscanf( pTime, "%ld", &duration )!=1 )
        {
        Warning( "Invalid DRTN (%s) in session cookie", pTime );
        free( plaintext );
        return -14;
        }
      if( durationPtr!=NULL && duration>0 )
        *durationPtr = duration;
      Notice( "Cookie duration is %ld seconds", duration );
      /* gotDrtn = 1; */
      }
    else if( EMPTY( line ) )
      { /* whatever */
      }
    else
      {
      Warning( "Unexpected line in session state: %s", line );
      free( plaintext );
      return -15;
      }
    }

  if( gotUser && gotAddr && gotExpr && gotUagt )
    {
    free( plaintext );
    return 0;
    }

  if( ! gotUser )
    {
    Warning( "Session state does not specify the user" );
    free( plaintext );
    return -16;
    }

  if( ! gotAddr )
    {
    Warning( "Session state does not specify the address" );
    free( plaintext );
    return -17;
    }

  if( ! gotUagt )
    {
    Warning( "Session state does not specify the user agent" );
    free( plaintext );
    return -18;
    }

  /* gotTime must be 0 to reach this */
  Warning( "Session state does not specify the expiry time" );
  free( plaintext );
  return -19;
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

  if( userAgentHash )
    free( userAgentHash );

  return 0;
  }

char* GetValidatedUserIDFromHttpHeaders( uint8_t* key, char* cookieText )
  {
  if( EMPTY( cookieText ) )
    {
    cookieText = GetSessionCookieFromEnvironment();
    if( cookieText==NULL )
      {
      Warning( "No session cookie" );
      return NULL;
      }
    }

  char* remoteAddr = getenv( DEFAULT_REMOTE_ADDR );
  if( remoteAddr==NULL )
    {
    Warning( "No remote address" );
    return NULL;
    }
  else
    Notice( "GetValidatedUserIDFromHttpHeaders() - remoteAddr == [%s]", NULLPROTECT( remoteAddr ) );

  char* userAgent = getenv( DEFAULT_USER_AGENT_VAR );
  if( userAgent==NULL )
    {
    Warning( "No user agent" );
    return NULL;
    }
  char* userAgentHash = SimpleHash( userAgent, USER_AGENT_HASH_LEN );

  char* userID = NULL;
  long expiry = -1;
  long duration = -1;
  int err = GetIdentityFromCookie( cookieText, &userID,
                                   &expiry, &duration,
                                   remoteAddr, userAgentHash,
                                   key );

  if( err )
    {
    Warning( "GetIdentityFromCookie failed - %d", err );
    if( userAgentHash )
      free( userAgentHash );
    return NULL;
    }

  if( expiry>0 && duration>0 )
    { /* QQQ possibly write updated expiry back to cookie */
    time_t tNow = time(NULL);
    time_t tExpiry = tNow + duration;
    if( tExpiry > expiry )
      {
      Notice( "Extending session of %s on Hash(%s) at %s by %d seconds",
          NULLPROTECT( userID ),
          NULLPROTECT( userAgentHash ),
          NULLPROTECT( remoteAddr ),
          (int)(tExpiry - expiry) );
      err = PrintSessionCookie( userID, duration, DEFAULT_REMOTE_ADDR, DEFAULT_USER_AGENT_VAR, key );
      }
    }

  if( userAgentHash )
    free( userAgentHash );

  return userID;
  }

char* ExtractUserIDOrDieEx( enum callMethod cm,
                            char* userVarName, char* cookieVarName,
                            char* myUrlVarName,
                            char* authURL,
                            uint8_t* key )
  {
  char* userVar = EMPTY(userVarName) ? DEFAULT_USER_ENV_VAR : userVarName;
  char* cookieVar = EMPTY(cookieVarName) ? COOKIE_ID : cookieVarName;
  char* authLocation = EMPTY( authURL ) ? DEFAULT_AUTH_URL : authURL;

  if( EMPTY( userVar ) && EMPTY( cookieVar ) )
    {
    if( cm==cm_ui )
      {
      printf("Content-Type: text/html\r\n\r\n");
      printf( "<html><body><b>Configuration problem: what env variable carries the user ID and/or authentication cookie?</b></body></html>\n" );
      exit(0);
      }
    else
      APIError( "API basics", -1, "Configuration problem: what env variable carries the user ID and/or authentication cookie?" );
    }

  char* userName = NULL;

  char* cookieValue = GetCookieFromEnvironment( cookieVar );
  if( NOTEMPTY( cookieValue ) )
    userName = GetValidatedUserIDFromHttpHeaders( key, cookieValue );

  if( cookieValue!=NULL )
    free( cookieValue );

  if( NOTEMPTY( userName ) )
    return userName;

  if( NOTEMPTY( cookieVar ) )
    { /* there is a cookie but we don't know who the user is. */
    char* myURL = MyRelativeRequestURL( myUrlVarName );
    char* encURL = URLEncode( myURL );
    char gotoURL[BUFLEN];
    snprintf( gotoURL, sizeof(gotoURL)-1, "%s?URL=%s", authLocation, encURL );
    free( myURL );
    free( encURL );
    RedirectToUrl( gotoURL );
    exit(0);
    }

  userName = getenv( userVar );

  if( EMPTY( userName ) )
    {
    if( cm==cm_ui )
      {
      printf("Content-Type: text/html\r\n\r\n");
      printf( "<html><body><b>Cannot discern user name from variable %s</b></body></html>\n", userVar );
      exit(0);
      }
    else
      {
      APIError( "API basics", -2, "Cannot discern user name from HTTP variable (%s)", userVar );
      }
    }

  return userName;
  }

