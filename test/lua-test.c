#include "base.h"

#define LUATEST "test.lua"

int main()
  {
  printf( "Init lua:\n");
  lua_State* L = LUAInit();

  printf( "Load %s:\n", LUATEST );
  LUALoadScript( L, LUATEST );

  _TAG_VALUE* args = NULL;
  args = NewTagValue( "string", "Hello, world!", args, 0 );
  args = NewTagValueInt( "integer", 3, args, 0 );
  args = NewTagValueDouble( "float", 3.141516, args, 0 );

  printf( "Arguments:\n" );
  PrintTagValue( 2, args );

  _TAG_VALUE* retVal = LUAFunctionCall( L, "TestFunction", args );

  printf( "Return values:\n" );
  PrintTagValue( 2, retVal );

  FreeTagValue( args );
  FreeTagValue( retVal );
  LuaFree( L );

  return 0;
  }
