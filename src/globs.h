#ifndef _INCLUDE_GLOBS
#define _INCLUDE_GLOBS

extern FILE* debugOutput;
extern FILE* remoteReadHandle;
extern FILE* remoteWriteHandle;
extern pthread_t timerThread;

extern int glob_argc;
extern char** glob_argv;
extern int inDaemon;
extern int inCGI;
extern int printedContentType;

void EmergencyCloseHandles();

#endif
