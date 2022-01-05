#include "utils.h"

int ProcessExistsAndIsMine( pid_t p )
  {
  if( getpgid(p) < 0 )
    {
    /* often this is normal */
    Warning("getpgid(%ld) error -- %d:%s", (long)p, errno, strerror( errno ) );
    return -1; /* either does not exist or not my child */
    }

  if( kill( p, 0 ) != 0 )
    {
    Warning("kill(%ld) error -- %d:%s", (long)p, errno, strerror( errno ) );
    return -2; /* p does not seem to exist */
    }

  return 0;
  }

int POpenAndRead( const char *cmd, int* readPtr, pid_t* childPtr )
  {
  int fd[2];
  int readFD = -1;
  int writeFD = -1;
  int pid = -1;

  /* Create a pipe to talk to the child */
  int err = pipe( fd );
  if( err<0 )
    Error( "Failed to pipe() - %d:%s", errno, strerror( errno ) );

  readFD = fd[0];
  writeFD = fd[1];

  /* Fork so we have parent/child processes */
  pid = fork();
  if( pid<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( pid == 0 ) /* child */
    {
    NARGV* args = nargv_parse( cmd );
    if( args==NULL )
      Error( "Failed to parse cmd line [%s]", cmd );
    fflush( stdout );
    close( readFD );
    dup2( writeFD, 1 );
    close( writeFD );
    (void)execv( args->argv[0], args->argv );
    /* end of code */
    }
  else /* parent */
    {
    *childPtr = pid;
    close( writeFD );
    *readPtr = readFD;
    fcntl( readFD, F_SETFL, O_NONBLOCK );
    }

  return 0;
  }

int POpenAndSearch( const char *cmd, char* subString, char** result )
  {
  if( EMPTY( cmd ) || EMPTY( subString ) )
    return -1;

  int fileDesc = -1;
  pid_t child = -1;
  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child [%s].", cmd );

  int flags = fcntl( fileDesc, F_GETFL);
  flags &= ~O_NONBLOCK;
  fcntl( fileDesc, F_SETFL, flags);

  FILE* f = fdopen( fileDesc, "r" );
  char buf[BUFLEN];
  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    if( strstr( buf, subString )!=NULL )
      {
      fclose( f );
      if( result!=NULL )
        *result = strdup( buf );
      return 0;
      }
    }

  fclose( f );
  return -2;
  }

int POpenAndReadWrite( const char* cmd, int* readFD, int* writeFD, pid_t* child )
  {
  int err = 0;
  int input[2];
  int inputRead = -1;
  int inputWrite = -1;
  int output[2];
  int outputRead = -1;
  int outputWrite = -1;
  int pid = -1;

  /* Create pipes to talk to the child */
  err = pipe( input );
  if( err<0 )
    Error( "Failed to pipe() - %d:%s", errno, strerror( errno ) );

  inputRead = input[0];
  inputWrite = input[1];

  err = pipe( output );
  if( err<0 )
    Error( "Failed to pipe() - %d:%s", errno, strerror( errno ) );

  outputRead = output[0];
  outputWrite = output[1];

  /* Fork so we have parent/child processes */
  pid = fork();
  if( pid<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( pid == 0 ) /* child */
    {
    NARGV* args = nargv_parse( cmd );
    if( args==NULL )
      Error( "Failed to parse cmd line [%s]", cmd );
    close( inputWrite );
    close( outputRead );
    dup2( inputRead, 0 );
    close( inputRead );
    dup2( outputWrite, 1 );
    dup2( outputWrite, 2 );
    close( outputWrite );
    (void)execv( args->argv[0], args->argv );
    /* end of code */
    }
  else /* parent */
    {
    *child = pid;
    close( inputRead );
    close( outputWrite );
    *readFD = outputRead;
    *writeFD = inputWrite;
    fcntl( outputRead, F_SETFL, O_NONBLOCK );
    }

  return 0;
  }

int POpenAndWrite( const char *cmd, int* writePtr, pid_t* childPtr )
  {
  int fd[2];
  int readFD = -1;
  int writeFD = -1;
  int pid = -1;

  /* Create a pipe to talk to the child */
  int err = pipe( fd );
  if( err<0 )
    Error( "Failed to pipe() - %d:%s", errno, strerror( errno ) );

  readFD = fd[0];
  writeFD = fd[1];

  /* Fork so we have parent/child processes */
  pid = fork();
  if( pid<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( pid == 0 ) /* child */
    {
    NARGV* args = nargv_parse( cmd );
    if( args==NULL )
      Error( "Failed to parse cmd line [%s]", cmd );
    fflush( stdout );
    close( writeFD );
    dup2( readFD, 0 );
    close( readFD );
    close( 2 );
    (void)execv( args->argv[0], args->argv );
    /* end of code */
    }
  else /* parent */
    {
    *childPtr = pid;
    close( readFD );
    *writePtr = writeFD;
    fcntl( writeFD, F_SETFL, O_NONBLOCK );
    }

  return 0;
  }

int AsyncReadFromChildProcess( char* cmd,
                               int sleepSeconds,
                               void* params,
                               void (*GotLineCallback)( void*, char* ),
                               void (*TimeoutCallback)( void* )
                               )
  {
  int fileDesc = -1;
  pid_t child = -1;
  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child." );

  char buf[BUFLEN];
  char* ptr = NULL;
  char* endPtr = buf + sizeof(buf) - 2;

  int retVal = 0;
  int exited = 0;

  for(;;)
    {
    int nBytesRead = 0;
    ptr = buf;
    *ptr = 0;

    for( ptr=buf; ptr<endPtr; ++ptr )
      {
      nBytesRead = read( fileDesc, ptr, 1 );
      if( nBytesRead<=0 )
        {
        *ptr = 0;
        break;
        }
      if( *ptr=='\n' )
        {
        ++ptr;
        *ptr = 0;
        break;
        }
      }

    if( nBytesRead>0 && buf[0]!=0 )
      {
      (*GotLineCallback)( params, buf );
      buf[0] = 0;
      }
/*
    else
      {
      printf("Read %d bytes starting with a %d byte\n", (int)(ptr-buf), (int)buf[0] );
      }
*/

    retVal = 0;
    int wstatus;
    if( waitpid( child, &wstatus, WNOHANG )==-1 )
      {
      Notice( "waitpid returned -1.");
      retVal = -1;
      break;
      }
    if( WIFEXITED( wstatus ) )
      {
      exited = 1;
      Notice( "child exited.");
      retVal = 0;
      break;
      }

    if( nBytesRead<=0 )
      {
      sleep( sleepSeconds );
      (*TimeoutCallback)( params );
      }
    }

  close( fileDesc );
  if( ! exited )
    {
    kill( child, SIGHUP );
    }

  return retVal;
  }

int ReadLineFromCommand( char* cmd, char* buf, int bufSize, int timeoutSeconds, int maxtimeSeconds )
  {
  int fileDesc = -1;
  pid_t child = -1;

  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child to run [%s].", cmd );

  char* ptr = buf;
  char* endPtr = buf + bufSize - 2;

  int retVal = 0;
  time_t tStart = time(NULL);
  int exited = 0;

  for(;;)
    {
    if( (int)(time(NULL) - tStart) >= maxtimeSeconds )
      {
      retVal = -3;
      break;
      }

    fd_set readSet;
    fd_set exceptionSet;
    struct timeval timeout;

    FD_ZERO( &readSet );
    FD_SET( fileDesc, &readSet );
    FD_ZERO( &exceptionSet );
    FD_SET( fileDesc, &exceptionSet );
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    
    int result = select( fileDesc+1, &readSet, NULL, &exceptionSet, &timeout );
    if( result>0 )
      {
      int nBytes = read( fileDesc, ptr, endPtr-ptr );
      if( nBytes>0 )
        {
        ptr += nBytes;
        *ptr = 0;
        if( strchr( buf, '\n' )!=NULL )
          {
          break;
          }
        }
      }

    int wStatus;
    if( waitpid( child, &wStatus, WNOHANG )==-1 )
      {
      Notice( "waitpid returned -1 (error.  errno=%d/%s).", errno, strerror( errno ));
      retVal = -1;
      break;
      }

    if( WIFEXITED( wStatus ) )
      {
      exited = 1;
      Notice( "child exited.");
      retVal = 0;
      break;
      }
    }

  close( fileDesc );

  if( ! exited )
    {
    kill( child, SIGHUP );
    }

  return retVal;
  }

int ReadLinesFromCommand( char* cmd, char** bufs, int nBufs, int bufSize, int timeoutSeconds, int maxtimeSeconds )
  {
  int fileDesc = -1;
  pid_t child = -1;

  for( int i=0; i<nBufs; ++i )
    *(bufs[i]) = 0;

  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child to run [%s].", cmd );

  int lineNo = 0;
  char* ptr = bufs[lineNo];
  char* endPtr = ptr + bufSize - 2;

  int retVal = 0;
  time_t tStart = time(NULL);

  int exited = 0;

  while( fileDesc > 0 )
    {
    if( (int)(time(NULL) - tStart) >= maxtimeSeconds )
      {
      retVal = -3;
      break;
      }

    fd_set readSet;
    fd_set exceptionSet;
    struct timeval timeout;

    FD_ZERO( &readSet );
    FD_SET( fileDesc, &readSet );
    FD_ZERO( &exceptionSet );
    FD_SET( fileDesc, &exceptionSet );
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    
    int result = select( fileDesc+1, &readSet, NULL, &exceptionSet, &timeout );
    if( result>0 )
      {
      char tinyBuf[2];
      tinyBuf[0] = 0;
      int n = 0;
      while( ptr < endPtr && (n=read( fileDesc, tinyBuf, 1 ))==1 )
        {
        int c = tinyBuf[0];
        if( c=='\n' )
          {
          ++lineNo;
          if( lineNo >= nBufs )
            {
            close( fileDesc );
            fileDesc = -1;
            break;
            }
          ptr = bufs[lineNo];
          endPtr = ptr + bufSize - 2;
          }
        else
          {
          *ptr = c;
          ++ptr;
          *ptr = 0;
          }
        }
      }

    int wStatus;
    if( waitpid( child, &wStatus, WNOHANG )==-1 )
      {
      Notice( "waitpid returned -1 (error.  errno=%d/%s).", errno, strerror( errno ));
      retVal = -1;
      break;
      }

    if( WIFEXITED( wStatus ) )
      {
      exited = 1;
      Notice( "child exited.");
      retVal = 0;
      break;
      }
    }

  if( fileDesc>0 )
    {
    close( fileDesc );
    }

  if( exited==0 )
    {
    kill( child, SIGHUP );
    }

  return retVal;
  }

int WriteReadLineToFromCommand( char* cmd, char* stdinLine, char* buf, int bufSize, int timeoutSeconds, int maxtimeSeconds )
  {
  int readFD = -1;
  int writeFD = -1;
  pid_t child = -1;

  int err = POpenAndReadWrite( cmd, &readFD, &writeFD, &child );
  if( err ) Error( "Cannot popen child to run [%s].", cmd );

  int l = strlen(stdinLine);
  int nBytes = write( writeFD, stdinLine, l );
  if( nBytes>0 )
    {
    if( nBytes!=l )
      Warning( "Tried to write %d bytes but only managed %d", l, nBytes );
    }
  close( writeFD );

  char* ptr = buf;
  char* endPtr = buf + bufSize - 2;

  int retVal = 0;
  time_t tStart = time(NULL);
  int exited = 0;

  for(;;)
    {
    if( (int)(time(NULL) - tStart) >= maxtimeSeconds )
      {
      retVal = -3;
      break;
      }

    fd_set readSet;
    fd_set exceptionSet;
    struct timeval timeout;

    FD_ZERO( &readSet );
    FD_SET( readFD, &readSet );
    FD_ZERO( &exceptionSet );
    FD_SET( readFD, &exceptionSet );
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    
    int result = select( readFD+1, &readSet, NULL, &exceptionSet, &timeout );
    if( result>0 )
      {
      int nBytes = read( readFD, ptr, endPtr-ptr );
      if( nBytes>0 )
        {
        ptr += nBytes;
        *ptr = 0;
        if( strchr( buf, '\n' )!=NULL )
          {
          break;
          }
        }
      }

    int wStatus;
    if( waitpid( child, &wStatus, WNOHANG )==-1 )
      {
      Notice( "waitpid returned -1 (error.  errno=%d/%s).", errno, strerror( errno ));
      retVal = -1;
      break;
      }

    if( WIFEXITED( wStatus ) )
      {
      exited = 1;
      Notice( "child exited.");
      retVal = 0;
      break;
      }
    }

  close( readFD );
  if( ! exited )
    {
    kill( child, SIGHUP );
    }

  return retVal;
  }

int WriteLineToCommand( char* cmd, char* stdinLine, int timeoutSeconds, int maxtimeSeconds )
  {
  int fileDesc = -1;
  pid_t child = -1;

  int err = POpenAndWrite( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child to run [%s].", cmd );

  int retVal = 0;
  time_t tStart = time(NULL);
  while( *stdinLine != 0 )
    {
    if( (int)(time(NULL) - tStart) >= maxtimeSeconds )
      {
      retVal = -3;
      break;
      }

    fd_set writeSet;
    fd_set exceptionSet;
    struct timeval timeout;

    FD_ZERO( &writeSet );
    FD_SET( fileDesc, &writeSet );
    FD_ZERO( &exceptionSet );
    FD_SET( fileDesc, &exceptionSet );
    timeout.tv_sec = timeoutSeconds;
    timeout.tv_usec = 0;
    
    int result = select( fileDesc+1, NULL, &writeSet, &exceptionSet, &timeout );
    if( result>0 )
      {
      int l = strlen( stdinLine );
      int nBytes = write( fileDesc, stdinLine, l );
      if( nBytes>0 )
        {
        if( nBytes!=l )
          Warning( "Tried to write %d bytes but only managed %d", l, nBytes );
        stdinLine += nBytes;
        }
      }
    else
      {
      Warning( "Timeout waiting for child - %d:%d:%s", err, errno, strerror( errno ) );
      }
    }

  close( fileDesc );

  int wStatus;
  if( waitpid( child, &wStatus, WNOHANG )==-1 )
    {
    Notice( "waitpid returned -1 (error.  errno=%d/%s).", errno, strerror( errno ));
    retVal = -1;
    }

  if( WIFEXITED( wStatus ) )
    {
    Notice( "child exited.");
    retVal = 0;
    }

  if( retVal != 0 )
    {
    kill( child, SIGHUP );
    }

  return retVal;
  }

/* 0 return only indicates that child forked successfully */
int AsyncRunCommandNoIO( char* cmd )
  {
  if( EMPTY( cmd ) )
    return -1;

  NARGV* args = nargv_parse( cmd );
  if( args==NULL )
    Error( "Failed to parse cmd line [%s]", cmd );

  pid_t pid = fork();
  if( pid<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( pid == 0 ) /* child */
    {
    close( 0 );
    close( 1 );
    close( 2 );
    (void)execv( args->argv[0], args->argv );
    /* end of code */
    }

  nargv_free( args );

  return 0;
  }

int SyncRunCommandNoIO( char* cmd )
  {
  if( EMPTY( cmd ) )
    return -1;

  NARGV* args = nargv_parse( cmd );
  if( args==NULL )
    Error( "Failed to parse cmd line [%s]", cmd );

  pid_t pid = fork();
  if( pid<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( pid == 0 ) /* child */
    {
    close( 0 );
    close( 1 );
    close( 2 );
    if( execv( args->argv[0], args->argv ) )
      Error( "Tried to exec [%s] - got error %d:%s",
             args->argv[0], errno, strerror(errno) );
    /* end of code */
    }

  nargv_free( args );

  int retVal = 0;
  int wStatus = 0;
  if( waitpid( pid, &wStatus, 0 )==-1 )
    {
    Warning( "waitpid returned -1 (error.  errno=%d/%s).\n", errno, strerror( errno ));
    retVal = -1;
    }

  if( WIFEXITED( wStatus ) )
    {
    Notice( "child exited.\n");
    }

  return retVal;
  }

void SignalHandler( int signo )
  {
  if( signo == SIGHUP )
    {
    syslog( LOG_NOTICE, "Received SIGHUP - exiting" );
    exit( 0 );
    }
  else if( signo == SIGUSR1 )
    {
    syslog( LOG_NOTICE, "Received SIGUSR1 - closing handle" );
    void EmergencyCloseHandles();
    }
  }

void KillExistingCommandInstances( char* commandLine )
  {
  if( EMPTY( commandLine ) )
    return;

  Notice( "KillExistingCommandInstances( %s )", commandLine );
  char* procLine = NULL;
  int nTries = 10;
  while( POpenAndSearch( "/bin/ps -efww", commandLine, &procLine )==0
         && nTries-- )
    {
    Notice( "Command [%s] already running - will try to stop it", commandLine );
    if( NOTEMPTY( procLine ) )
      {
      char* ptr = NULL;
      char* userID = strtok_r( procLine, " \t\r\n", &ptr );
      char* processID = strtok_r( NULL, " \t\r\n", &ptr );
      long pidNum = -1;
      if( NOTEMPTY( processID ) )
        pidNum = atol( processID );

      Notice( "Killing process %ld which belongs to %s", pidNum, userID );
      if( pidNum>0 )
        {
        int err = kill( (pid_t)pidNum, SIGHUP );
        if( err )
          {
          Warning( "Failed to send HUP signal to process %d: %d:%d:%s",
                   pidNum, err, errno, strerror( errno ) );
          break;
          }
        else
          sleep(1); /* might take a while to stop it */
        }
      FREE( procLine );
      }
    }
  }

uid_t GetUID( const char* logName )
  {
  if( EMPTY( logName ) )
    Error( "GetUID(NULL)" );
  struct passwd* p = getpwnam( logName );
  if( p==NULL )
    Error( "GetUID(%s) -> invalid user", logName );
  return p->pw_uid;
  }

gid_t GetGID( const char* groupName )
  {
  if( EMPTY( groupName ) )
    Error( "GetjID(NULL)" );
  struct group* g = getgrnam( groupName );
  if( g==NULL )
    Error( "GetGID(%s) -> invalid group", groupName );
  return g->gr_gid;
  }

