#ifndef _LUA_WRAPPER
#define _LUA_WRAPPER

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

const char* LuaTypeName( int type );
int LuaPrintTable( lua_State* L );
int LuaPrintStack( lua_State* L );
int TagValueTableOnLuaStack( lua_State* L, _TAG_VALUE* list );
_TAG_VALUE* LuaTableToTagValue( lua_State *L );
int TableToJSON( lua_State* L );
int JSONToTable( lua_State* L );
int GetTableSizeFunction( lua_State* L );
int LUAWebTransaction( lua_State* L );
lua_State* LUAInit();
void LuaFree( lua_State* L );
void LUALoadScript( lua_State *L, char* fileName );
_TAG_VALUE* LUAFunctionCall( lua_State *L, char* functionName, _TAG_VALUE* args );

#endif
