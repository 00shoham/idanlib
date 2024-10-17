#include "utils.h"

/* #define DEBUG 1 */

pthread_mutex_t luaLock = PTHREAD_MUTEX_INITIALIZER;

const char* LuaTypeName( int type )
  {
  switch( type )
    {
    case LUA_TNIL: return "nil";
    case LUA_TBOOLEAN: return "bool";
    case LUA_TNUMBER: return "number";
    case LUA_TSTRING: return "string";
    case LUA_TTABLE: return "table";
    default: return "other";
    }
  }

int LuaPrintTable( lua_State* L )
  {
  lua_pushnil( L );  /* first key */
  printf( "{ " );
  int first = 1;
  while( lua_next(L, -2 ) != 0 )
    {
    if( first )
      {}
    else
      printf( ", " );
    if( lua_isstring( L, -1 ) )
      printf( "%s = %s", lua_tostring( L, -2 ), lua_tostring( L, -1 ) );
    else if( lua_isnumber( L, -1) )
      printf( "%s = %lf", lua_tostring( L, -2 ), lua_tonumber( L, -1 ) );
    else if( lua_isboolean( L, -1) )
      printf( "%s = %s", lua_tostring( L, -2 ), lua_toboolean( L, -1 ) ? "true":"false" );
    else if( lua_istable( L, -1 ) )
      {
      printf( "%s = ", lua_tostring( L, -2 ) );
      (void)LuaPrintTable( L );
      }
    lua_remove( L, -1 );
    first = 0;
    }
  printf( " }" );

  return 0;
  }

int LuaPrintStack( lua_State* L )
  {
  for( int i=1; i<=lua_gettop( L ); ++i )
    {
    int type = lua_type( L, i );
    printf( "  @%d %s: ", i, LuaTypeName( type ) );
    switch( type )
      {
      case LUA_TNIL:
        printf( "nil\n" );
        break;

      case LUA_TBOOLEAN:
        printf( "%s\n", lua_toboolean( L, i ) ? "true" : "false" );
        break;

      case LUA_TNUMBER:
        printf( "%lg\n", lua_tonumber( L, i ) );
        break;

      case LUA_TSTRING:
        printf( "%s\n", lua_tostring( L, i ) );
        break;

      case LUA_TTABLE:
        lua_pushvalue( L, i );
        (void)LuaPrintTable( L );
        lua_remove( L, -1 );
        printf( "\n" );
        break;

      default:
        printf( "{other data}\n" );
        break;
      }
    }

  return 0;
  }

int TagValueTableOnLuaStack( lua_State* L, _TAG_VALUE* list )
  {
  lua_newtable( L );
  int index = 1;
  for( _TAG_VALUE* t = list; t!=NULL; t=t->next, ++index )
    {
    if( EMPTY( t->tag ) )
      {
      char buf[100];
      snprintf( buf, sizeof(buf)-1, "no-tag-index-%d", index );
      lua_pushstring( L, buf );
      }
    else
      {
      lua_pushstring( L, t->tag );
      }

    switch( t->type )
      {
      case VT_STR:
        if( t->value==NULL )
          lua_pushstring( L, "" );
        else
          lua_pushstring( L, t->value );
        break;

      case VT_INT:
        lua_pushnumber( L, (double)t->iValue );
        break;

      case VT_DOUBLE:
        lua_pushnumber( L, t->dValue );
        break;

      case VT_NULL:
        lua_pushnil( L );
        break;

      case VT_LIST:
        TagValueTableOnLuaStack( L, t->subHeaders );
        break;

      default:
        Warning( "Invalid item type in TAG_VALUE list" );
      }

    /* table = -3, key = -2, value = -1 */
    lua_settable(L, -3);
    }

  return 1;
  }

_TAG_VALUE* LuaTableToTagValue( lua_State *L )
  {
  if( ! lua_istable(L, -1) )
    {
    Warning( "Top of LUA stack is not a table (gettop=%d)", lua_gettop(L) );
    return NULL;
    }

  _TAG_VALUE* list = NULL;

  /* table is in the stack at index -1 */
  lua_pushnil( L );  /* first key */

  /* we have table @ -2, key @ -1 initially */
  while( lua_gettop( L ) >= 2 && lua_next(L, -2) != 0 )
    {
    /* lua_next() yields table @ -3, key @ -2, value @ -1 */
    char* tag = NULL;
    if( lua_type( L, -2 )==LUA_TSTRING )
      {
      tag = strdup( lua_tostring( L, -2 ) );
      }
    else if( lua_type( L, -2 )==LUA_TNUMBER )
      {
      double d = lua_tonumber( L, -2 );
      char buf[100];
      snprintf( buf, sizeof(buf)-1, "%lg", d );
      tag = strdup( buf );
      }
    else
      Error( "Invalid index type (not str or num) in table" );

    switch( lua_type( L, -1 ) )
      {
      case LUA_TNIL:
        list = NewTagValueNull( tag, list, 0 );
        break;

      case LUA_TBOOLEAN:
        list = NewTagValueInt( tag, lua_toboolean( L, -1 ), list, 0 );
        break;

      case LUA_TNUMBER:
        list = NewTagValueDouble( tag, lua_tonumber( L, -1 ), list, 0 );
        break;

      case LUA_TSTRING:
        list = NewTagValue( tag, lua_tostring( L, -1 ), list, 0 );
        break;

      case LUA_TTABLE:
        lua_pushvalue( L, -1 );
        list = NewTagValueList( tag, LuaTableToTagValue( L ), list, 0 );
        lua_remove( L, -1 );
        break;

      default:
        Warning( "Unsupported type in LUA table->tag/value" );
        break;
      }

    FREE( tag );

    /* the value @ -1 should be removed from the stack now */
    lua_remove( L, -1 );
    }

  return list;
  }

int TableToJSON( lua_State* L )
  {
  if( lua_gettop( L )<1 || ! lua_istable(L, -1) )
    {
    Warning( "Top of LUA stack is not a table" );
    return 0;
    }

  _TAG_VALUE* tv = LuaTableToTagValue( L );
  if( tv==NULL )
    {
    Warning( "Failed to convert LUA table to _TAG_VALUE" );
    return 0;
    }

  char buf[BUFLEN];
  int err = ListToJSON( tv, buf, sizeof(buf)-1 );
  if( err )
    {
    Warning( "Failed to _TAG_VALUE to JSON (%d)", err );
    FreeTagValue( tv );
    return 0;
    }

  lua_pushstring( L, buf );

  FreeTagValue( tv );
  return 1;
  }

int JSONToTable( lua_State* L )
  {
  if( lua_gettop( L )<1 || ! lua_isstring(L, -1) )
    {
    Warning( "Top of LUA stack is not a table" );
    return 0;
    }

  const char* json = lua_tolstring( L, -1, NULL );
  if( json==NULL )
    {
    Warning( "Failed to get string from LUA stack" );
    return 0;
    }

  _TAG_VALUE* tv = ParseJSON( json );
  if( tv==NULL )
    {
    Warning( "Failed to convert JSON string to _TAG_VALUE" );
    return 0;
    }

  int retVal = TagValueTableOnLuaStack( L, tv );
  FreeTagValue( tv );

  return retVal;
  }

int GetTableSizeFunction( lua_State* L )
  {
  int retVal = -1;

  if( lua_gettop( L )<1 || ! lua_istable(L, -1) )
    {
    Warning( "Top of LUA stack is not a table" );
    }
  else
    {
    retVal = 0;
    lua_pushnil( L );  /* first key */
    while( lua_next(L, -2) != 0 )
      {
      ++retVal;
      lua_remove( L, -1 );
      }
    }

  lua_pushnumber( L, (double)retVal );
  return 1;
  }

int LUAWebTransaction( lua_State* L )
  {
  char* me = "LUAWebTransaction";
  if( lua_gettop( L )<1 || ! lua_istable(L, -1) )
    {
    Warning( "%s: Top of LUA stack is not a table", me );
    return 0;
    }

  _TAG_VALUE* tv = LuaTableToTagValue( L );

  char* url = GetTagValue( tv, "URL" );
  if( EMPTY( url ) )
    {
    Warning( "%s: URL not set", me );
    FreeTagValue( tv );
    return 0;
    }

  enum httpMethod method = HTTP_GET;
  char* methodString = GetTagValue( tv, "METHOD" );
  if( !EMPTY( methodString ) )
    {
    if( strcasecmp( methodString, "GET" )==0 )
      method = HTTP_GET;
    else if( strcasecmp( methodString, "POST" )==0 )
      method = HTTP_POST;
    else
      {
      Warning( "%s: METHOD must be GET|POST", me );
      FreeTagValue( tv );
      return 0;
      }
    }

  char* postData = GetTagValue( tv, "POST_DATA" );
  if( EMPTY( postData ) )
    postData = NULL;
  else
    { /* if we are posting anything, the method must be HTTP_POST... */
    method = HTTP_POST;
    }

  char* postContentType = GetTagValue( tv, "CONTENT_TYPE" );
  if( EMPTY( postContentType ) )
    postContentType = NULL;

  char* urlUserID = GetTagValue( tv, "URL_USER_ID" );
  if( EMPTY( urlUserID ) )
    urlUserID = NULL;

  char* urlPassword = GetTagValue( tv, "URL_PASSWORD" );
  if( EMPTY( urlPassword ) )
    urlPassword = NULL;

  char* proxyURL = GetTagValue( tv, "PROXY_URL" );
  if( EMPTY( proxyURL ) )
    proxyURL = NULL;

  char* proxyUserID = GetTagValue( tv, "PROXY_USER_ID" );
  if( EMPTY( proxyUserID ) )
    proxyUserID = NULL;

  char* proxyPassword = GetTagValue( tv, "PROXY_PASSWORD" );
  if( EMPTY( proxyPassword ) )
    proxyPassword = NULL;

  int timeoutSeconds = GetTagValueInt( tv, "TIMEOUT_SECONDS" );
  if( timeoutSeconds<=0 )
    {
    timeoutSeconds = (int)GetTagValueDouble( tv, "TIMEOUT_SECONDS" );
    if( timeoutSeconds<=0 )
      {
      timeoutSeconds = 60;
      }
    }

  _TAG_VALUE* httpHeaders = NULL;
  char* userAgent = GetTagValue( tv, "USER_AGENT" );
  if( EMPTY( userAgent ) )
    userAgent = NULL;
  else
    httpHeaders = NewTagValue( "user-agent", userAgent, httpHeaders, 1 );

  for( _TAG_VALUE* head=tv; head!=NULL; head=head->next )
    {
    if( NOTEMPTY( head->tag )
        && strncasecmp( head->tag, "HEADER_", 7 )==0
        && NOTEMPTY( head->value ) )
      {
      char* headerName = head->tag + 7;
      char* headerValue = head->value;
      httpHeaders = NewTagValue( headerName, headerValue, httpHeaders, 1 );
      }
    }

  char* cookiesFile = GetTagValue( tv, "COOKIES_FILE" );
  if( EMPTY( cookiesFile ) )
    cookiesFile = DEFAULT_COOKIES_FILE;

  /* check that we can write to the cookies file - else we could get errors... */
  int fd = open( cookiesFile, O_RDWR | O_CREAT, 0666 );
  if( fd==-1 )
    Error( "Cannot open/create cookies file [%s] with read/write permissions", cookiesFile );
  close( fd );
  fd = 0;

  int skipPeerVerify = 0;
  char* skipPeerVerifyString = GetTagValue( tv, "SKIP_PEER_VERIFY" );
  if( ! EMPTY( skipPeerVerifyString ) )
    skipPeerVerify = 1;

  int skipHostVerify = 0;
  char* skipHostVerifyString = GetTagValue( tv, "SKIP_HOST_VERIFY" );
  if( ! EMPTY( skipHostVerifyString ) )
    skipHostVerify = 1;

  _DATA d = { 0, NULL, NULL };
  char* errMsg = NULL;

  /*
  Notice( "WebTransaction: url=%s, method=%s", url, method==HTTP_POST?"POST":"GET" );
  for( _TAG_VALUE* head=httpHeaders; head!=NULL; head=head->next )
    Notice( "WebTransaction header: %s: %s", head->tag, head->value );
  */

  CURLcode err =
    WebTransaction( url, method,                           /* url and method */
                    postData, 0, postContentType,          /* postData, postBinLen */
                    &d,                                    /* data back */
                    urlUserID, urlPassword,                /* url creds */
                    proxyURL, proxyUserID, proxyPassword,  /* proxy url and creds */
                    timeoutSeconds,                        /* timeout */
                    httpHeaders,                           /* user agent */
                    cookiesFile,                           /* cookies file */
                    skipPeerVerify,                        /* do not skip peer verify */
                    skipHostVerify,                        /* do not skip host verify */
                    &errMsg
                    );
                         
  if( err != CURLE_OK )
    {
    Warning( "%s: CURL says - %s", me, errMsg );
    FreeData( &d );
    FreeTagValue( tv );
    return 0;
    }

#ifdef DEBUG
  Notice( "LUA WebTransaction().  URL=%s POST_DATA=[%s] retVal=200/OK retData=[%s]",
          NULLPROTECT( url ),
          NULLPROTECT( postData ),
          NULLPROTECT( (char*)d.data ) );
#endif

  if( d.data==NULL )
    {
    /* Notice( "LUA WebTransaction() - empty results" ); */
    (void)lua_pushstring( L, "" );
    }
  else
    {
    /* Notice( "LUA WebTransaction() - results = [%s]", d.data ); */
    (void)lua_pushstring( L, (const char*)d.data );
    }

  /* web transaction logs this same content already...
     Notice( "Pushed string onto lua stack (%s)", NULLPROTECT( d.data ) ); */
  FreeData( &d );
  FreeTagValue( tv );
  return 1;
  }

int LUASleep( lua_State* L )
  {
  if( lua_gettop( L )<1 || ! lua_isnumber(L, -1) )
    {
    Warning( "LUASleep: Top of LUA stack is not a number" );
    return 0;
    }

  double n = lua_tonumber( L, -1 );
  lua_remove( L, -1 );
  if( n<=0 )
    {
    Warning( "%s: Cannot sleep for %lg seconds", n );
    return 0;
    }

  int in = (int)n;
  sleep( in );

  return 0;
  }

int LUATailFile( lua_State* L )
  {
  char* me = "LUATailFile";
  if( lua_gettop( L )<1 || ! lua_istable(L, -1) )
    {
    Warning( "%s: Top of LUA stack is not a table", me );
    return 0;
    }

  _TAG_VALUE* tv = LuaTableToTagValue( L );

  char* fileName = GetTagValue( tv, "filename" );
  if( EMPTY( fileName ) )
    {
    FreeTagValue( tv );
    Warning( "%s: filename not set", me );
    return 0;
    }

  int nLines = GetTagValueInt( tv, "nlines" );
  if( nLines<1 )
    nLines = (int)GetTagValueDouble( tv, "nlines" );

  if( nLines<1 )
    {
    FreeTagValue( tv );
    Warning( "%s: nlines not set", me );
    return 0;
    }

  if( nLines>100 )
    {
    FreeTagValue( tv );
    Warning( "%s: nlines too large (max 100)", me );
    return 0;
    }

  FILE* f = fopen( fileName, "r" );
  if( f==NULL )
    {
    FreeTagValue( tv );
    Warning( "%s: could not open %s", me, fileName );
    return 0;
    }

  char buf[BUFLEN];
  TailFile( f, nLines, buf, sizeof(buf)-1 );
  fclose( f );

  FreeTagValue( tv );

  if( strlen(buf)>0 )
    {
    lua_pushstring( L, (char*)buf );
    return 1;
    }

  Warning( "%s: empty result", me );
  return 0;
  }

int LUAHexStringToNumber( lua_State* L )
  {
  char* me = "LUAHexStringToNumber";
  if( lua_gettop( L )<1 || ! lua_isstring(L, -1) )
    {
    Warning( "%s: Top of LUA stack is not a string", me );
    return 0;
    }

  const char* str = lua_tostring( L, -1 );

  if( EMPTY( str ) )
    {
    Warning( "%s: Cannot call HexStringToNumber with empty string" );
    lua_remove( L, -1 );
    return 0;
    }

  long x;
  if( sscanf( str, "%lx", &x )==1 )
    {
    lua_remove( L, -1 );
    lua_pushnumber( L, x );
    return 1;
    }

  Warning( "Failed to get a number from %s", str );
  lua_remove( L, -1 );
  return 0;
  }

int LUARegExExtract( lua_State* L )
  {
  char* me = "LUARegExExtract";
  if( lua_gettop( L )<1 || ! lua_istable(L, -1) )
    {
    Warning( "%s: Top of LUA stack is not a table", me );
    return 0;
    }

  _TAG_VALUE* tv = LuaTableToTagValue( L );

  char* string = GetTagValue( tv, "string" );
  if( EMPTY( string ) )
    {
    FreeTagValue( tv );
    Warning( "%s: string not set", me );
    return 0;
    }

  char* pattern = GetTagValue( tv, "pattern" );
  if( EMPTY( pattern ) )
    {
    FreeTagValue( tv );
    Warning( "%s: pattern not set", me );
    return 0;
    }

  char* result = ExtractRegexFromString( pattern, string );

  if( EMPTY( result ) )
    {
    lua_pushstring( L, "" );
    if( result!=NULL )
      free( result );
    }
  else
    {
    lua_pushstring( L, result );
    }
  FreeTagValue( tv );

  return 1;
  }

int LUARegExMatch( lua_State* L )
  {
  char* me = "LUARegExMatch";
  if( lua_gettop( L )<1 || ! lua_istable(L, -1) )
    {
    Warning( "%s: Top of LUA stack is not a table", me );
    return 0;
    }

  _TAG_VALUE* tv = LuaTableToTagValue( L );

  char* string = GetTagValue( tv, "string" );
  if( EMPTY( string ) )
    {
    FreeTagValue( tv );
    Warning( "%s: string not set", me );
    return 0;
    }

  char* pattern = GetTagValue( tv, "pattern" );
  if( EMPTY( pattern ) )
    {
    FreeTagValue( tv );
    Warning( "%s: pattern not set", me );
    return 0;
    }

  int result = StringMatchesRegex( pattern, string );
  FreeTagValue( tv );
  lua_pushnumber( L, result );

  return 1;
  }

int LUAReadLineFromCommand( lua_State* L )
  {
  char* me = "LUAReadLineFromCommand";
  if( lua_gettop( L )<1 || ! lua_istable(L, -1) )
    {
    Warning( "%s: Top of LUA stack is not a table", me );
    lua_pushstring( L, "" );
    lua_pushnumber( L, -1 );
    return 2;
    }

  _TAG_VALUE* tv = LuaTableToTagValue( L );

  char* cmd = GetTagValue( tv, "command" );
  if( EMPTY( cmd ) )
    {
    FreeTagValue( tv );
    Warning( "%s: command not set", me );
    lua_pushstring( L, "" );
    lua_pushnumber( L, -2 );
    return 2;
    }

  int timeout = GetTagValueInt( tv, "timeout" );
  if( timeout<1 )
    timeout = (int)GetTagValueDouble( tv, "timeout" );
  if( timeout<5 )
    {
    timeout = 5;
    Notice( "%s: timeout not set - setting to 5 seconds", me );
    }

  char buf[BUFLEN];
  buf[0] = 0;
  int result = ReadLineFromCommand( cmd, buf, sizeof(buf)-1, 1, timeout );
  FreeTagValue( tv );

  lua_pushstring( L, buf );
  lua_pushnumber( L, result );

  return 2;
  }

int LUAReadOutputFromCommand( lua_State* L )
  {
  char* me = "LUAReadOutputFromCommand";
  if( lua_gettop( L )<1 || ! lua_istable(L, -1) )
    {
    Warning( "%s: Top of LUA stack is not a table", me );
    lua_pushstring( L, "" );
    lua_pushnumber( L, -1 );
    return 2;
    }

  _TAG_VALUE* tv = LuaTableToTagValue( L );

  char* cmd = GetTagValue( tv, "command" );
  if( EMPTY( cmd ) )
    {
    FreeTagValue( tv );
    Warning( "%s: command not set", me );
    lua_pushstring( L, "" );
    lua_pushnumber( L, -2 );
    return 2;
    }

  int maxLineLen = GetTagValueInt( tv, "max_line_len" );
  if( maxLineLen<1 )
    maxLineLen = (int)GetTagValueDouble( tv, "max_line_len" );
  if( maxLineLen<500 )
    {
    maxLineLen = 500;
    Notice( "%s: max_line_len not set - setting to 500 chars", me );
    }

  int readTimeout = GetTagValueInt( tv, "read_timeout" );
  if( readTimeout<1 )
    readTimeout = (int)GetTagValueDouble( tv, "read_timeout" );
  if( readTimeout<5 )
    {
    readTimeout = 5;
    Notice( "%s: read_timeout not set - setting to 5 seconds", me );
    }

  int maxTimeout = GetTagValueInt( tv, "max_timeout" );
  if( maxTimeout<1 )
    maxTimeout = (int)GetTagValueDouble( tv, "max_timeout" );
  if( maxTimeout<15 )
    {
    maxTimeout = 15;
    Notice( "%s: max_timeout not set - setting to 15 seconds", me );
    }

  char** buffers = NULL;

  Notice( "cmd = %s", cmd );
  Notice( "maxLineLen = %d", maxLineLen );
  Notice( "readTimeout = %d", readTimeout );
  Notice( "maxTimeout = %d", maxTimeout );

  int nLines = ReadLinesFromCommandEx( cmd,
                                       &buffers,
                                       maxLineLen,
                                       readTimeout,
                                       maxTimeout );

  FreeTagValue( tv );

  if( nLines>=1 )
    {
    _TAG_VALUE* linesList = NULL;
    for( int i=0; i<nLines; ++i )
      {
      char numTag[20];
      snprintf( numTag, sizeof(numTag)-2, "%07d", i );
      _TAG_VALUE* singleLine = NewTagValue( numTag, buffers[i], NULL, 0 );
      linesList = AppendTagValue( linesList, singleLine );
      if( buffers[i]!=NULL )
        free( buffers[i] );
      }
    free( buffers );
    buffers = NULL;
    (void)TagValueTableOnLuaStack( L, linesList );
    FreeTagValue( linesList );
    }
  else
    { /* push an empty table if we read nothing */
    TagValueTableOnLuaStack( L, NULL );
    }

  lua_pushnumber( L, nLines );

  return 2;
  }

int LUANotice( lua_State* L )
  {
  if( lua_gettop( L )<1 || ! lua_isstring(L, -1) )
    {
    Warning( "LUANotice: Top of LUA stack is not a string" );
    return 0;
    }

  const char* str = lua_tostring( L, -1 );
  if( str==NULL )
    str = "";

  Notice( str );

  lua_remove( L, -1 );
  return 0;
  }

int LUAWarning( lua_State* L )
  {
  if( lua_gettop( L )<1 || ! lua_isstring(L, -1) )
    {
    Warning( "LUAWarning: Top of LUA stack is not a string" );
    return 0;
    }

  const char* str = lua_tostring( L, -1 );
  if( str == NULL )
    str = "";

  Warning( str );

  lua_remove( L, -1 );
  return 0;
  }

int LUAError( lua_State* L )
  {
  if( lua_gettop( L )<1 || ! lua_isstring(L, -1) )
    {
    Warning( "LUAWarning: Top of LUA stack is not a string" );
    return 0;
    }

  const char* str = lua_tostring( L, -1 );
  if( str==NULL )
    str = "";

  Error( str );

  lua_remove( L, -1 );
  return 0;
  }

lua_State* LUAInit()
  {
  lua_State *L = luaL_newstate();    /* opens Lua */

  lua_pushcfunction( L, TableToJSON);
  lua_setglobal( L, "TableToJSON" );

  lua_pushcfunction( L, JSONToTable );
  lua_setglobal( L, "JSONToTable" );

  lua_pushcfunction( L, GetTableSizeFunction );
  lua_setglobal( L, "GetTableSize" );

  lua_pushcfunction( L, LuaPrintTable );
  lua_setglobal( L, "PrintTable" );

  lua_pushcfunction( L, LuaPrintStack );
  lua_setglobal( L, "PrintStack" );

  lua_pushcfunction( L, LUAWebTransaction );
  lua_setglobal( L, "WebTransaction" );

  lua_pushcfunction( L, LUASleep );
  lua_setglobal( L, "Sleep" );

  lua_pushcfunction( L, LUANotice );
  lua_setglobal( L, "Notice" );

  lua_pushcfunction( L, LUAWarning );
  lua_setglobal( L, "Warning" );

  lua_pushcfunction( L, LUAError );
  lua_setglobal( L, "Error" );

  lua_pushcfunction( L, LUATailFile );
  lua_setglobal( L, "TailFile" );

  lua_pushcfunction( L, LUAHexStringToNumber );
  lua_setglobal( L, "HexStringToNumber" );

  lua_pushcfunction( L, LUARegExExtract );
  lua_setglobal( L, "RegExExtract" );

  lua_pushcfunction( L, LUARegExMatch );
  lua_setglobal( L, "RegExMatch" );

  lua_pushcfunction( L, LUAReadLineFromCommand );
  lua_setglobal( L, "ReadLineFromCommand" );

  lua_pushcfunction( L, LUAReadOutputFromCommand );
  lua_setglobal( L, "ReadOutputFromCommand" );

  // load the libs
  luaL_openlibs(L);

  return L;
  }

void LuaFree( lua_State* L )
  {
  if( L!=NULL )
    lua_close(L);
  }

void LUALoadScript( lua_State *L, char* fileName )
  {
  /* read the lua code */
  unsigned char* code = NULL;
  long nBytes = FileRead( fileName, &code );
  if( nBytes>0 )
    {
    int err = luaL_loadbuffer( L, (char*)code, nBytes, fileName );
    if( err )
      Error( "Failed to load buffer from %s - error %d - %s", fileName, err, lua_tostring(L,-1) );
    FREE( code );
    }

  /* not sure why we need this, but it seems to help
     with subsequent function calls */
  int err = lua_pcall( L, 0, 0, 0 );
  if( err ) /* prime the pump? */
    Warning( "Initial lua_pcall() failed - %d - %s", err, lua_tostring(L,-1));
  }

/* The convention is to pass in a _TAG_VALUE* list of arguments, where
   each argument has a name and a value (so not ordered, named!) and
   to return a _TAG_VALUE* list of returns.  The caller has to free
   the return value list */
_TAG_VALUE* LUAFunctionCall( lua_State *L, char* functionName, _TAG_VALUE* args )
  {
  int err = 0;
  if( L==NULL )
    Error( "LUAFunctionCall with NULL lua_State" );

  if( EMPTY( functionName ) )
    Error( "LUAFunctionCall with no function name" );

  if( lua_getglobal( L, functionName )<0 )
    {
    Warning( "Could not find function %s", functionName );
    return NULL;
    }

#ifdef DEBUG
  char buf[BUFLEN];
  char* ptr = buf;
  char* end = ptr + sizeof(buf) - 2;
  strncpy( ptr, "LUAFunctionCall: ", end-ptr ); ptr += strlen( ptr );
  strncpy( ptr, functionName, end-ptr ); ptr += strlen( ptr );
  strncpy( ptr, "( ", end-ptr ); ptr += strlen( ptr );
  for( _TAG_VALUE* arg=args; arg!=NULL; arg=arg->next )
    {
    strncpy( ptr, NULLPROTECT( arg->tag ), end-ptr ); ptr += strlen( ptr );
    strncpy( ptr, ": ", end-ptr ); ptr += strlen( ptr );
    switch( arg->type )
      {
      case VT_STR: 
        strncpy( ptr, NULLPROTECT( arg->value ), end-ptr ); ptr += strlen( ptr );
        break;
      case VT_INT: 
        snprintf( ptr, end-ptr, "%d", arg->iValue ); ptr += strlen( ptr );
        break;
      case VT_DOUBLE: 
        snprintf( ptr, end-ptr, "%.1lf", arg->dValue ); ptr += strlen( ptr );
        break;
      case VT_LIST: 
        strncpy( ptr, "<LIST>", end-ptr ); ptr += strlen( ptr );
        break;
      default: 
        strncpy( ptr, "<OTHER>", end-ptr ); ptr += strlen( ptr );
        break;
      }
    if( arg->next!=NULL )
      {
      strncpy( ptr, ", ", end-ptr );
      ptr += strlen( ptr );
      }
    }
  strncpy( ptr, " )", end-ptr ); ptr += strlen( ptr );

  int stackDepth = lua_gettop( L );
  snprintf( ptr, end-ptr, " stack depth=%d", stackDepth );
  ptr += strlen( ptr );

  Notice( buf );
#endif

  int nArgs = 0;
  if( args!=NULL )
    nArgs = TagValueTableOnLuaStack( L, args );

  if( ( err = lua_pcall( L,
                         nArgs /* args table */,
                         1 /* results table */,
                         0 /* no special errhandler */ ) ) )
    {
    Warning( "lua_pcall(%s) returned error %d - %s", functionName, err, lua_tostring(L,-1) );
    lua_remove( L, -1 );
    return NULL;
    }

#ifdef DEBUG
  Notice( "Returned from lua_pcall(%s)", functionName );
#endif

  _TAG_VALUE* response = LuaTableToTagValue( L );

#ifdef DEBUG
  Notice( "Got response (%p) from Lua table", response==NULL?"NULL":"valid-ptr" );
#endif

  lua_remove( L, -1 );
  return response;
  }
