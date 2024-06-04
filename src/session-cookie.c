#include "utils.h"

#define USER_AGENT_HASH_LEN 4

void ClearSessionCookieSpecific( char* cookieId )
  {
  if( EMPTY( cookieId ) )
    return;

  ClearCookie( cookieId  );
  }

char* GetSessionCookieFromEnvironmentSpecific( char* cookieId )
  {
  if( EMPTY( cookieId ) )
    return NULL;

  return GetCookieFromEnvironment( cookieId );
  }

void ClearSessionCookieDefault()
  {
  ClearCookie( DEFAULT_ID_OF_AUTH_COOKIE  );
  }

char* GetSessionCookieFromEnvironmentDefault()
  {
  return GetCookieFromEnvironment( DEFAULT_ID_OF_AUTH_COOKIE );
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
    /* Notice( "GetIdentityFromCookie() - decrypted line: [%s]", line ); */
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
      if( NOTEMPTY( userAgent ) )
        hash = SimpleHash( userAgent, USER_AGENT_HASH_LEN );

#if 0
      if( strcmp( uAgent, userAgent )==0 )
        { /* okay - plaintext user agent the same */ }
      else
#endif
      if( NOTEMPTY( hash )
          && ( strcmp( uAgent, hash )==0 || strcmp( uAgent, userAgent )==0 ) )
        { /* okay - hash user agent the same */
        }
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

int PrintSessionCookie( char* cookieVarName,
                        char* userID,
                        long ttlSeconds,
                        char* remoteAddrVariable,
                        char* userAgentVariable,
                        uint8_t* key )
  {
  if( EMPTY( cookieVarName ) )
    {
    Warning( "PrintSessionCookie() - must specify cookie variable name" );
    return -1;
    }

  if( EMPTY( userID ) )
    {
    Warning( "PrintSessionCookie() - must specify user ID" );
    return -2;
    }

  if( ttlSeconds < MIN_SESSION_TTL )
    {
    Warning( "PrintSessionCookie() - must specify user TTL of at least %d", MIN_SESSION_TTL );
    return -3;
    }

  char* varName = EMPTY( remoteAddrVariable ) ? DEFAULT_REMOTE_ADDR : remoteAddrVariable;
  char* addr = getenv( varName );
  if( EMPTY( addr ) )
    {
    Warning( "PrintSessionCookie() - cannot discern remote address from HTTP header %s", varName );
    return -4;
    }

  varName = EMPTY( userAgentVariable ) ? DEFAULT_USER_AGENT_VAR : userAgentVariable;
  char* uagt = getenv( varName );
  if( EMPTY( uagt ) )
    {
    Warning( "PrintSessionCookie() - cannot discern user agent from HTTP header %s", varName );
    return -5;
    }


  char* userAgentHash = SimpleHash( uagt, USER_AGENT_HASH_LEN );


  /* QQQ */
  Notice( "PrintSessionCookie(): user=%s, addr=%s, uah=%s, ttl=%d, key=%s",
          NULLPROTECT( userID ),
          NULLPROTECT( addr ),
          NULLPROTECT( userAgentHash ),
          ttlSeconds,
          key==NULL || *key==0 ? "nil" : "value" );

  char* cookie = EncodeIdentityInCookie( userID, addr, userAgentHash, ttlSeconds, key );

  printf( "Set-Cookie: %s=%s; Max-Age=%ld\n", cookieVarName, cookie, ttlSeconds);

  /* DEBUG
  Notice( "Set-Cookie: %s=%s; Max-Age=%ld\n", cookieVarName, cookie, ttlSeconds);
  */
  free( cookie );

  if( userAgentHash )
    free( userAgentHash );

  return 0;
  }

/* pass in remote_addr and user_agent vars */
char* GetValidatedUserIDFromHttpHeaders( uint8_t* key,
                                         char* cookieName,
                                         char* cookieText,
                                         char* remoteAddrVar,
                                         char* userAgentVar )
  {
  if( EMPTY( cookieName ) )
    {
    Warning( "GetValidatedUserIDFromHttpHeaders() - must specify cookie name" );
    return NULL;
    }

  if( EMPTY( cookieText ) )
    {
    cookieText = GetSessionCookieFromEnvironmentSpecific( cookieName );
    if( cookieText==NULL )
      {
      Warning( "No session cookie" );
      return NULL;
      }
    }

  char* remoteAddr = getenv( EMPTY( remoteAddrVar ) ? DEFAULT_REMOTE_ADDR : remoteAddrVar );
  if( remoteAddr==NULL )
    {
    Warning( "No remote address" );
    return NULL;
    }
  else
    Notice( "GetValidatedUserIDFromHttpHeaders() - remoteAddr == [%s]", NULLPROTECT( remoteAddr ) );

  char* userAgent = getenv( EMPTY( userAgentVar ) ? DEFAULT_USER_AGENT_VAR : userAgentVar );
  if( EMPTY( userAgent ) )
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
    Warning( "GetIdentityFromCookie from [%s] failed - %d", cookieName, err );
    if( userAgentHash )
      free( userAgentHash );
    Warning( "Clearing cookie %s as it's likely corrupt", cookieName );
    ClearCookie( cookieName );
    return NULL;
    }

  if( expiry>0 && duration>0 )
    {
    time_t tNow = time(NULL);
    time_t tExpiry = tNow + duration;
    int autoRenewSeconds = MIN_RENEW_SESSION_INTERVAL; /* default - short sessions should renew within 30 seconds of expiry */

    if( duration > 3600 ) /* heuristic - long sessions should renew on activity within 5 minutes of expiry */
      autoRenewSeconds = 600;

    if( tExpiry - expiry >= autoRenewSeconds )
      {
      Notice( "Extending session of %s on Hash(%s) at %s by %d seconds",
          NULLPROTECT( userID ),
          NULLPROTECT( userAgentHash ),
          NULLPROTECT( remoteAddr ),
          (int)(tExpiry - expiry) );

      err = PrintSessionCookie( cookieName,
                                userID,
                                duration,
                                remoteAddrVar,
                                userAgentVar,
                                key );
      }
    }

  if( userAgentHash )
    free( userAgentHash );

  return userID;
  }

char* ExtractUserIDOrDieEx( enum callMethod cm,
                            char* userVarName,
                            char* remoteAddrVarName,
                            char* userAgentVarName,
                            char* cookieVarName,
                            char* myUrlVarName,
                            char* authURL,
                            uint8_t* key,
                            char* cssPath )
  {
  char* userVar = EMPTY(userVarName) ? DEFAULT_USER_ENV_VAR : userVarName;
  char* remoteAddrVar = EMPTY(remoteAddrVarName) ? DEFAULT_REMOTE_ADDR : remoteAddrVarName;
  char* userAgentVar = EMPTY(userAgentVarName) ? DEFAULT_USER_AGENT_VAR : userAgentVarName;
  char* cookieVar = EMPTY(cookieVarName) ? DEFAULT_ID_OF_AUTH_COOKIE : cookieVarName;
  char* authLocation = EMPTY( authURL ) ? DEFAULT_AUTH_URL : authURL;

  /* DEBUG
  Notice( "ExtractUserIDOrDieEx()" );
  Notice( "ExtractUserIDOrDieEx() - userVar = %s", userVar );
  Notice( "ExtractUserIDOrDieEx() - remoteAddrVar = %s", remoteAddrVar );
  Notice( "ExtractUserIDOrDieEx() - userAgentVar = %s", userAgentVar );
  Notice( "ExtractUserIDOrDieEx() - cookieVar = %s", cookieVar );
  Notice( "ExtractUserIDOrDieEx() - authLocation = %s", authLocation );
  */

  if( EMPTY( userVar ) && EMPTY( cookieVar ) )
    {
    if( cm==cm_ui )
      {
      printf("Content-Type: text/html\r\n\r\n");
      printedContentType = 1;
      printf( "<html><body><b>Configuration problem: what env variable carries the user ID and/or authentication cookie?</b></body></html>\n" );
      exit(0);
      }
    else
      APIError( "API basics", -1, "Configuration problem: what env variable carries the user ID and/or authentication cookie?" );
    }

  char* userName = NULL;

  char* cookieValue = GetCookieFromEnvironment( cookieVar );
  if( NOTEMPTY( cookieValue ) )
    {
    /* DEBUG Notice( "ExtractUserIDOrDieEx() - got cookie [%s] from [%s]", cookieValue, cookieVar ); */
    userName = GetValidatedUserIDFromHttpHeaders( key, cookieVar, cookieValue,
                                                  remoteAddrVar, userAgentVar );
    }
  else
    {
    Notice( "ExtractUserIDOrDieEx() - cookie is empty" );
    }

  if( cookieValue!=NULL )
    free( cookieValue );

  if( NOTEMPTY( userName ) )
    return userName;

  if( NOTEMPTY( cookieVar ) )
    { /* there is a cookie but we don't know who the user is. */
    char* myURL = MyRelativeRequestURL( myUrlVarName );
    Notice( "MyRelativeRequestURL() says we are at %s", NULLPROTECT( myURL ) );
    char* encURL = URLEncode( myURL );
    char gotoURL[BUFLEN];
    snprintf( gotoURL, sizeof(gotoURL)-1, "%s?URL=%s", authLocation, encURL );
    free( myURL );
    free( encURL );
    Notice( "Redirecting to %s", NULLPROTECT( gotoURL ) );
    RedirectToUrl( gotoURL, cssPath );
    exit(0);
    }

  userName = getenv( userVar );

  if( EMPTY( userName ) )
    {
    if( cm==cm_ui )
      {
      printf("Content-Type: text/html\r\n\r\n");
      printedContentType = 1;
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

