#include "utils.h"

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
    Warning( "Top of LUA stack is not a table" );
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
      return 0;
      }
    }

  char* postData = GetTagValue( tv, "POST_DATA" );
  if( EMPTY( postData ) )
    postData = NULL;

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
  if( timeoutSeconds<0 )
    timeoutSeconds = 0;

  _TAG_VALUE* httpHeaders = NULL;
  char* userAgent = GetTagValue( tv, "USER_AGENT" );
  if( EMPTY( userAgent ) )
    userAgent = NULL;
  else
    httpHeaders = NewTagValue( "user-agent", userAgent, httpHeaders, 1 );

  char* cookiesFile = GetTagValue( tv, "COOKIES_FILE" );
  if( EMPTY( cookiesFile ) )
    cookiesFile = DEFAULT_COOKIES_FILE;

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
    FreeData( &d);
    return 0;
    }
  else
    {
    lua_pushstring( L, (char*)d.data );
    FreeData( &d);
    return 1;
    }
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
  int err = luaL_loadbuffer( L, (char*)code, nBytes, fileName );
  if( err )
    Error( "Failed to load buffer from %s - error %d - %s", fileName, err, lua_tostring(L,-1) );

  /* not sure why we need this, but it seems to help
     with subsequent function calls */
  if( lua_pcall( L, 0, 0, 0 ) ) /* prime the pump? */
    Error( "Initial lua_pcall() failed");
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

  int nArgs = 0;
  if( args!=NULL )
    nArgs = TagValueTableOnLuaStack( L, args );

  if( ( err = lua_pcall( L,
                         nArgs /* args table */,
                         1 /* results table */,
                         0 /* no special errhandler */ ) ) )
    Error( "luaL_pcall(%s) returned error %d - %s", functionName, err, lua_tostring(L,-1) );

  _TAG_VALUE* response = LuaTableToTagValue( L );

  lua_remove( L, -1 );
  return response;
  }
