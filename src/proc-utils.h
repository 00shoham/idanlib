#ifndef _INCLUDE_PROC_UTILS
#define _INCLUDE_PROC_UTILS

int ProcessExistsAndIsMine( pid_t p );
int POpenAndRead( const char *cmd, int* readPtr, pid_t* childPtr );
int POpenAndSearch( const char *cmd, char* subString, char** result );
int POpenAndWrite( const char *cmd, int* writePtr, pid_t* childPtr );
int POpenAndReadWrite( const char* cmd, int* readFD, int* writeFD, pid_t* child );
int AsyncReadFromChildProcess( char* cmd,
                               int sleepSeconds,
                               void* params,
                               void (*GotLineCallback)( void*, char* ),
                               void (*TimeoutCallback)( void* )
                               );
int ReadLineFromCommand( char* cmd, char* buf, int bufSize, int timeoutSeconds, int maxtimeSeconds );
int ReadLinesFromCommand( char* cmd, char** bufs, int nBufs, int bufSize, int timeoutSeconds, int maxtimeSeconds );
int WriteLineToCommand( char* cmd, char* stdinLine, int timeoutSeconds, int maxtimeSeconds );
int WriteReadLineToFromCommand( char* cmd, char* stdinLine, char* buf, int bufSize, int timeoutSeconds, int maxtimeSeconds );
int AsyncRunCommandNoIO( char* cmd );
int SyncRunCommandNoIO( char* cmd );
void SignalHandler( int signo );
void KillExistingCommandInstances( char* commandLine );

uid_t GetUID( const char* logName );
gid_t GetGID( const char* groupName );

#endif
