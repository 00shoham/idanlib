#include "utils.h"

FILE* debugOutput = NULL;
FILE* remoteReadHandle = NULL;
FILE* remoteWriteHandle = NULL;
pthread_t timerThread = 0;

int glob_argc = 0;
char** glob_argv = NULL;
int inDaemon = 0;
int inCGI = 0;
int printedContentType = 0;

void EmergencyCloseHandles()
  {
  if( remoteReadHandle != NULL )
    {
    fclose( remoteReadHandle );
    remoteReadHandle = NULL;
    }
  if( remoteWriteHandle != NULL )
    {
    fclose( remoteWriteHandle );
    remoteWriteHandle = NULL;
    }
  if( timerThread!=0 )
    {
    pthread_join( timerThread, NULL );
    }
  }
