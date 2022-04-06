#ifndef _LUA_WRAPPER
#define _LUA_WRAPPER

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

const char* LuaTypeName( int type );
void PrintStackLocation( lua_State* L, int index );
int PrintLuaStack( lua_State* L );
int TagValueTableOnLuaStack( lua_State* L, _TAG_VALUE* list );
_TAG_VALUE* LuaTableToTagValue( lua_State *L );
int TableToJSON( lua_State* L );
int JSONToTable( lua_State* L );
int GetTableSizeFunction( lua_State* L );
lua_State* LUAInit();

#endif
