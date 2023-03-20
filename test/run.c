#include "utils.h"

void Test()
  {
  char* cmd = "ls -l > /tmp/ls.log";

  int err = ASyncRunShellNoIO( cmd );
  sleep(1);
  }

void main()
  {
  Test();
  }
