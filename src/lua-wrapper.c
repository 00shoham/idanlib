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

void PrintStackLocation( lua_State* L, int index )
  {
  int type = lua_type( L, index );
  printf( "@ %d: %s = ", index, LuaTypeName( type ) );
  fflush(stdout);
  switch( type )
    {
    case LUA_TNIL:
      printf( "nil\n" );
      break;

    case LUA_TBOOLEAN:
      printf( "%s\n", lua_toboolean( L, index) ? "true" : "false" );
      break;

    case LUA_TNUMBER:
      printf( "%lg\n", lua_tonumber( L, index) );
      break;

    case LUA_TSTRING:
      printf( "%s\n", lua_tostring( L, index) );
      break;

    case LUA_TTABLE:
      printf( "{table}\n" );
      break;

    default:
      printf( "{other data}\n" );
      break;
    }
  }

int PrintLuaStack( lua_State* L )
  {
  int n = lua_gettop( L );
  printf( "Stack has %d items\n", n );
  for( int i=1; i<=n; ++i )
    PrintStackLocation( L, -1 * i );
  return 0;
  }

int TagValueTableOnLuaStack( lua_State* L, _TAG_VALUE* list )
  {
  lua_newtable( L );
  for( _TAG_VALUE* t = list; t!=NULL; t=t->next )
    {
    if( EMPTY( t->tag ) )
      {
      Warning( "TAG_VALUE empty tag" );
      continue;
      }
    lua_pushstring( L, t->tag );

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
        TagValueTableOnLuaStack( L, list->subHeaders );
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

  /* we have table @ -2, key @ -2 initially */
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

lua_State* LUAInit()
  {
  lua_State *L = luaL_newstate();    /* opens Lua */

  lua_pushcfunction( L, TableToJSON);
  lua_setglobal( L, "TableToJSON" );

  lua_pushcfunction( L, JSONToTable );
  lua_setglobal( L, "JSONToTable" );

  lua_pushcfunction( L, GetTableSizeFunction );
  lua_setglobal( L, "GetTableSize" );

  lua_pushcfunction( L, PrintLuaStack );
  lua_setglobal( L, "PrintLuaStack " );

  // load the libs
  luaL_openlibs(L);

  return L;
  }
