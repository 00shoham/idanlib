#ifndef _INCLUDE_PROC_UTILS
#define _INCLUDE_PROC_UTILS

#define DEFAULT_READLINE_NUM_BUFFERS         1000
#define DEFAULT_READLINE_LINE_LEN            500
#define DEFAULT_READLINE_TIMEOUT_PER_READ    3
#define DEFAULT_READLINE_TIMEOUT_TOTAL       (DEFAULT_READLINE_TIMEOUT_PER_READ * 5)

int ProcessExistsAndIsMine( pid_t p );
int POpenAndRead( const char *cmd, int* readPtr, pid_t* childPtr );
int POpenAndSearch( const char *cmd, char* subString, char** result );
int POpenAndSearchRegEx( const char *cmd, char* regex, char** result );
int POpenAndSearchMultipleResults( const char *cmd, char* subString, char** result );
int POpenAndWrite( const char *cmd, int* writePtr, pid_t* childPtr );
int POpenAndReadWrite( const char* cmd, int* readFD, int* writeFD, pid_t* child );
int AsyncReadFromChildProcess( char* cmd,
                               int sleepSeconds,
                               void* params,
                               void (*GotLineCallback)( void*, char* ),
                               void (*TimeoutCallback)( void* ),
                               void (*CallBetweenReads)( )
                               );
int ReadLineFromCommand( char* cmd, char* buf, int bufSize, int timeoutSeconds, int maxtimeSeconds );
int ReadLinesFromCommand( char* cmd, char** bufs, int nBufs, int bufSize, int timeoutSeconds, int maxtimeSeconds );
int ReadLinesFromCommandEx( char* cmd, char*** bufsPtr, int maxLineLen, int timeoutPerReadSeconds, int maxTimeoutSeconds );
int WriteLineToCommand( char* cmd, char* stdinLine, int timeoutSeconds, int maxtimeSeconds );
int WriteReadLineToFromCommand( char* cmd, char* stdinLine, char* buf, int bufSize, int timeoutSeconds, int maxtimeSeconds );
int AsyncRunCommandNoIO( char* cmd );
int SyncRunCommandNoIO( char* cmd );
int SyncRunCommandSingleFileStdin( char* cmd, char* fileNameStdin );
int SyncRunCommandManyFilesStdin( char* cmd, char* listFileName );
int ASyncRunShellNoIO( char* cmd );
int SyncRunShellNoIO( char* cmd );
void SignalHandler( int signo );
void KillEarlierInstancesOfThisProcess( int argc, char** argv, int sigNo );
void KillExistingCommandInstances( char* commandLine, int sigNo );
void KillExistingCommandInstancesGentleFirst( char* commandLine );

uid_t GetUID( const char* logName );
gid_t GetGID( const char* groupName );

int DoWeHaveATTY();
int SendEMail( char* recipient, char* subject, char* body );

char* MakeDebugFilename( int argc, char** argv );

#endif
