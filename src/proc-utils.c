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

  if( EMPTY( cmd ) )
    Error( "POpenAndRead(): no command specified" );
  NARGV* args = nargv_parse( cmd );
  if( args==NULL )
    Error( "POpenAndRead():Failed to parse cmd line [%s]", cmd );
  if( args->argv==NULL || EMPTY( args->argv[0] ) )
    Error( "POpenAndRead(): Command [%s] does not parse", cmd );
  if( FileExists( args->argv[0] )!=0 )
    Error( "POpenAndRead(): Command [%s] does not exist", args->argv[0] );

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
    /* Linux-specific - terminate via SIGHUP if parent exits */
    prctl( PR_SET_PDEATHSIG, SIGHUP );
    close( readFD );
    dup2( writeFD, 1 );
    dup2( writeFD, 2 );
    close( writeFD );
    /*
    Notice( "Calling execv() with cmd %s", args->argv[0] );
    for( int i=0; args->argv[i]!=NULL; ++i )
      Notice( " .. argv[%d] = [%s]", i, args->argv[i] );
    */
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

int POpenAndSearchRegEx( const char *cmd, char* regex, char** result )
  {
  if( EMPTY( cmd ) || EMPTY( regex ) )
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
    if( StringMatchesRegex( regex, buf )==0 )
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

int POpenAndSearchMultipleResults( const char *cmd, char* subString, char** result )
  {
  if( EMPTY( cmd ) || EMPTY( subString ) )
    return -1;

  if( result==NULL )
    return -2;

  int fileDesc = -1;
  pid_t child = -1;
  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child [%s].", cmd );

  int flags = fcntl( fileDesc, F_GETFL);
  flags &= ~O_NONBLOCK;
  fcntl( fileDesc, F_SETFL, flags);

  FILE* f = fdopen( fileDesc, "r" );

  char resultBuf[BIGBUF];
  char* copyPtr = resultBuf;
  char* endPtr = resultBuf + sizeof( resultBuf ) - 1;

  char line[BUFLEN];
  *copyPtr = 0;

  while( fgets( line, sizeof(line)-1, f )==line )
    {
    if( strstr( line, subString )!=NULL )
      {
      (void)StripEOL( line );
      int l = strlen( line );
      if( endPtr-copyPtr > l )
        {
        strcpy( copyPtr, line );
        copyPtr += l;
        strcpy( copyPtr, "\n" );
        copyPtr += 1;
        }
      else
        {
        Warning( "Not enough buffer space for line from command [%s]", cmd );
        break;
        }
      }
    }

  *copyPtr = 0;

  if( copyPtr > resultBuf )
    *result = strdup( resultBuf );
  else
    *result = NULL;

  fclose( f );
  sleep(1);

  int wstatus = -1;
  if( waitpid( child, &wstatus, WNOHANG )==-1 )
    {
    Warning( "POpenAndSearchMultipleResults - waitpid waiting for child running [%s] returned -1.", cmd);
    return -3;
    }

  if( WIFEXITED( wstatus ) )
    {
    return 0;
    }

  Warning( "POpenAndSearchMultipleResults - child running [%s] still running.", cmd);
  return -4;
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
    /* Linux-specific - terminate via SIGHUP if parent exits */
    prctl( PR_SET_PDEATHSIG, SIGHUP );
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
    /* Linux-specific - terminate via SIGHUP if parent exits */
    prctl( PR_SET_PDEATHSIG, SIGHUP );
    NARGV* args = nargv_parse( cmd );
    if( args==NULL )
      Error( "Failed to parse cmd line [%s]", cmd );
    /* could be dup - in parent *and* child: */
    /* fflush( stdout ); */
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
                               void (*TimeoutCallback)( void* ),
                               void (*CallBetweenReads)( )
                               )
  {
  int fileDesc = -1;
  pid_t child = -1;
  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child." );
  if( child <= 0 ) Error( "AsyncReadFromChildProcess() - invalid child pid." );

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
      /* Notice( "child %d exited.", (int)child ); */
      retVal = 0;
      break;
      }

    if( nBytesRead<=0 )
      {
      sleep( sleepSeconds );
      (*TimeoutCallback)( params );
      }

    if( CallBetweenReads!=NULL )
      (*CallBetweenReads)();
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
  if( EMPTY( cmd ) )
    return -10;
  if( buf==NULL )
    return -11;
  if( bufSize<10 )
    return -12;
  if( timeoutSeconds<1 )
    return -13;
  if( maxtimeSeconds<=timeoutSeconds )
    return -14;

  int fileDesc = -1;
  pid_t child = -1;

  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child to run [%s].", cmd );

  /* Notice( "ReadLineFromCommand(%s) - fileDesc=%d", cmd, fileDesc ); */

  char* ptr = buf;
  char* endPtr = buf + bufSize - 2;

  int retVal = 0;
  time_t tStart = time(NULL);
  int exited = 0;

  for(;;)
    {
    if( (int)(time(NULL) - tStart) >= maxtimeSeconds )
      {
      /* Notice( "ReadLineFromCommand(%s) - timeout", cmd ); */
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
      /* Notice( "ReadLineFromCommand(%s) - read %d bytes", cmd, nBytes ); */
      if( nBytes>0 )
        {
        ptr += nBytes;
        *ptr = 0;
        if( strchr( buf, '\n' )!=NULL )
          {
          /* Notice( "ReadLineFromCommand(%s) - read \\n - all done", cmd ); */
          break;
          }
        }
      }

    /* Notice( "ReadLineFromCommand(%s) - waitpid()", cmd ); */
    int wStatus;
    if( waitpid( child, &wStatus, WNOHANG )==-1 )
      {
      Warning( "waitpid returned -1 (error.  errno=%d/%s).", errno, strerror( errno ));
      retVal = -1;
      break;
      }

    /* Notice( "ReadLineFromCommand(%s) - WIFEXITED( %d )", cmd, wStatus ); */
    if( WIFEXITED( wStatus ) )
      {
      exited = 1;
      /* Notice( "child %d exited.", (int)child ); */
      retVal = 0;
      break;
      }
    }

  close( fileDesc );

  if( ! exited )
    {
    /* Notice( "ReadLineFromCommand(%s) - child did not exit - kill it", cmd ); */
    kill( child, SIGHUP );
    }

  /* Notice( "ReadLineFromCommand(%s) - returning %d", cmd, retVal ); */
  return retVal;
  }

int ReadLinesFromCommandEx( char* cmd, char*** bufsPtr, int maxLineLen, int timeoutPerReadSeconds, int maxTimeoutSeconds )
  {
  int fileDesc = -1;
  pid_t child = -1;

  if( EMPTY( cmd ) )
    {
    Warning( "ReadLinesFromCommandEx(): no command specified" );
    return -100;
    }

  if( bufsPtr==NULL )
    {
    Warning( "ReadLinesFromCommandEx(): no pointer provided to array of strings" );
    return -101;
    }

  if( maxLineLen<=0 )
    {
    Warning( "ReadLinesFromCommandEx(): maxLineLen not specified.  Using default." );
    maxLineLen = DEFAULT_READLINE_LINE_LEN;
    }

  if( timeoutPerReadSeconds<=0 )
    {
    Warning( "ReadLinesFromCommandEx(): timeoutPerReadSecond snot specified.  Using default." );
    timeoutPerReadSeconds = DEFAULT_READLINE_TIMEOUT_PER_READ;
    }

  if( maxTimeoutSeconds<=0 )
    {
    Warning( "ReadLinesFromCommandEx(): timeoutPerReadSecond snot specified.  Using default." );
    maxTimeoutSeconds = DEFAULT_READLINE_TIMEOUT_TOTAL;
    }

  if( maxTimeoutSeconds<timeoutPerReadSeconds )
    {
    Warning( "ReadLinesFromCommandEx(): maxTimeoutSeconds<timeoutPerReadSeconds.  Adjusting." );
    maxTimeoutSeconds = timeoutPerReadSeconds * 2;
    }

  int nBuffersAllocated = DEFAULT_READLINE_NUM_BUFFERS;
  char** bufs = (char**)SafeCalloc( nBuffersAllocated, sizeof( char* ), "ReadLinesFromCommandEx(): pointers to line buffers" );

  char* singleBuffer = (char*)SafeCalloc( maxLineLen, sizeof(char), "ReadLinesFromCommandEx(): single line buffer" );

  /*
  Notice( "Allocated %d buffers (%p) and a single %d buffer (%p)",
          nBuffersAllocated, bufs, maxLineLen, singleBuffer );
  */

  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err || fileDesc<=0 || child==-1 )
    {
    Warning( "Cannot popen child to run [%s] - %d - %d - %s",
             cmd, err, errno, strerror( errno ) );
    return -102;
    }

  /*
  Notice( "POpenAndRead( %s ) returned %d; fileDesc = %d; child = %d", cmd, err, fileDesc, (int)child );
  */

  int lineNo = 0;
  int nCharsRead = 0;
  char* ptr = singleBuffer;
  *ptr = 0;
  char* endPtr = ptr + maxLineLen - 2;

  int retVal = 0;
  time_t tStart = time(NULL);

  int exited = 0;

  for( int j=0; j<5; ++j )
    {
    int tElapsed = (int)(time(NULL) - tStart);
    if( tElapsed >= maxTimeoutSeconds )
      {
      Warning( "ReadLinesFromCommandEx(): reached timeout (%d seconds) after reading %d characters", tElapsed, nCharsRead );
      retVal = -3;
      break;
      }

    /* Notice( "tElapsed = %d", tElapsed ); */

    fd_set readSet;
    fd_set exceptionSet;
    struct timeval timeout;

    FD_ZERO( &readSet );
    FD_SET( fileDesc, &readSet );
    FD_ZERO( &exceptionSet );
    FD_SET( fileDesc, &exceptionSet );
    timeout.tv_sec = timeoutPerReadSeconds;
    timeout.tv_usec = 0;
    
    /* Notice( "pre-select()" ); */

    int result = select( fileDesc+1, &readSet, NULL, &exceptionSet, &timeout );

    /* Notice( "select() returned %d", result ); */

    if( result>0 )
      {
      char tinyBuf[2];
      tinyBuf[0] = 0;
      int n = 0;
      while( ptr < endPtr && (n=read( fileDesc, tinyBuf, 1 ))==1 )
        {
        int c = tinyBuf[0];
        /* Notice( "read [%c]", c ); */
        if( c=='\n' )
          {
          bufs[lineNo] = strdup( singleBuffer );
          ptr = singleBuffer;
          *ptr = 0;
          endPtr = ptr + maxLineLen - 2;

          /* Notice( "Completed reading line %d", lineNo ); */

          ++lineNo;
          if( lineNo >= nBuffersAllocated )
            {
            nBuffersAllocated += DEFAULT_READLINE_NUM_BUFFERS;
            bufs = (char**)realloc( bufs, nBuffersAllocated * sizeof(char*) );
            if( bufs==NULL )
              {
              Warning( "ReadLinesFromCommandEx(): failed to allocate array of strings" );
              return -103;
              }
            }
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
      /* Notice( "child %d exited.", (int)child ); */
      retVal = 0;
      break;
      }

    /* Notice( "Loop around.." ); */
    }

  if( fileDesc>0 )
    {
    close( fileDesc );
    fileDesc = -1;
    }

  if( exited==0 )
    {
    /* Notice( "Did not exit?  Kill child." ); */
    kill( child, SIGHUP );
    }

  *bufsPtr = bufs;
  if( retVal==0 )
    {
    /* Notice( "retVal == 0 - so returning lineNo (%d)", lineNo ); */
    retVal = lineNo;
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
      /* Notice( "child %d exited.", (int)child ); */
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
      /* Notice( "child %d exited.", (int)child ); */
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
    /* Notice( "child %d exited.", (int)child ); */
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

  pid_t pid = LaunchDaemon( 1 );
  if( pid<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( pid == 0 ) /* child */
    {
    /* Linux-specific - terminate via SIGHUP if parent exits */
    /* prctl( PR_SET_PDEATHSIG, SIGHUP ); */
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

  pid_t child = fork();
  if( child<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( child == 0 ) /* child */
    {
    /* Linux-specific - terminate via SIGHUP if parent exits */
    prctl( PR_SET_PDEATHSIG, SIGHUP );
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
  if( waitpid( child, &wStatus, 0 )==-1 )
    {
    Warning( "waitpid returned -1 (error.  errno=%d/%s).\n", errno, strerror( errno ));
    retVal = -1;
    }

  if( WIFEXITED( wStatus ) )
    {
    /* Notice( "child %d exited.", (int)child ); */
    }

  return retVal;
  }

int SyncRunCommandSingleFileStdin( char* cmd, char* fileNameStdin )
  {
  if( EMPTY( cmd ) 
      || EMPTY( fileNameStdin )
      || FileExists( fileNameStdin )!=0 )
    return -1;

  NARGV* args = nargv_parse( cmd );
  if( args==NULL )
    Error( "Failed to parse cmd line [%s]", cmd );

  int fd[2];
  int readFD = -1;
  int writeFD = -1;
  int err = pipe( fd );
  if( err<0 )
    Error( "Failed to pipe() - %d:%s", errno, strerror( errno ) );
  readFD = fd[0];
  writeFD = fd[1];

  pid_t child = fork();
  if( child<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( child == 0 ) /* child */
    {
    /* Linux-specific - terminate via SIGHUP if parent exits */
    prctl( PR_SET_PDEATHSIG, SIGHUP );
    close( 0 );
    dup2( readFD, 0 );
    close( writeFD );
    close( 1 );
    close( 2 );
    (void)execv( args->argv[0], args->argv );
    /* end of code */
    }

  /* we are in the parent / original process */
  nargv_free( args );

  close( readFD );

  int readStream = open( fileNameStdin, O_RDONLY );
  if( readStream<0 )
    Warning( "Cannot open file %s to stream to command", fileNameStdin );
  else
    {
    char buf[BUFLEN];
    size_t nBytes = -1;
    do
      {
      nBytes = read( readStream, buf, sizeof(buf)-1 );
      if( nBytes>0 )
        {
        size_t nWritten = write( writeFD, buf, nBytes );
        if( nWritten!=nBytes )
          {
          Warning( "Tried to write %d bytes but only managed %d",
                   (int)nBytes, (int)nWritten );
          break;
          }
        }
      } while( nBytes>0 );

    close( readStream );
    }
  close( writeFD ); /* tell child we are done */

  int retVal = 0;
  int wStatus;
  if( waitpid( child, &wStatus, 0 )==-1 )
    {
    Warning( "waitpid returned -1 (error.  errno=%d/%s).\n", errno, strerror( errno ));
    retVal = -1;
    }

  if( WIFEXITED( wStatus ) )
    {
    /* Notice( "child %d exited.\n", (int)child ); */
    }

  return retVal;
  }

int SyncRunCommandManyFilesStdin( char* cmd, char* listFileName )
  {
  if( EMPTY( cmd ) 
      || EMPTY( listFileName )
      || FileExists( listFileName )!=0 )
    return -1;

  NARGV* args = nargv_parse( cmd );
  if( args==NULL )
    Error( "Failed to parse cmd line [%s]", cmd );

  int fd[2];
  int readFD = -1;
  int writeFD = -1;
  int err = pipe( fd );
  if( err<0 )
    Error( "Failed to pipe() - %d:%s", errno, strerror( errno ) );
  readFD = fd[0];
  writeFD = fd[1];

  pid_t child = fork();
  if( child<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( child == 0 ) /* child */
    {
    /* Linux-specific - terminate via SIGHUP if parent exits */
    prctl( PR_SET_PDEATHSIG, SIGHUP );
    close( 0 );
    dup2( readFD, 0 );
    close( writeFD );
    close( 1 );
    close( 2 );
    (void)execv( args->argv[0], args->argv );
    /* end of code */
    }

  nargv_free( args );

  close( readFD );

  FILE* listF = fopen( listFileName, "r" );
  if( listF==NULL )
    {
    Warning( "Failed to open %s (list of files to read into cmd) - %d:%s",
             listFileName, errno, strerror( errno ) );
    }
  else
    {
    char fileName[BUFLEN];
    while( fgets( fileName, sizeof(fileName)-1, listF )==fileName )
      {
      StripEOL( fileName );

      int readStream = open( fileName, O_RDONLY );
      if( readStream<0 )
        Warning( "Cannot open file %s to stream to command", fileName );
      else
        {
        char buf[BUFLEN];
        size_t nBytes = -1;
        do
          {
          nBytes = read( readStream, buf, sizeof(buf)-1 );
          if( nBytes>0 )
            {
            size_t nWritten = write( writeFD, buf, nBytes );
            if( nWritten!=nBytes )
              {
              Warning( "Tried to write %d bytes but only managed %d",
                       (int)nBytes, (int)nWritten );
              break;
              }
            }
          } while( nBytes>0 );

        close( readStream );
        }
      }

    fclose( listF );
    }
  close( writeFD );

  int retVal = 0;
  int wStatus;
  if( waitpid( child, &wStatus, 0 )==-1 )
    {
    Warning( "waitpid returned -1 (error.  errno=%d/%s).\n", errno, strerror( errno ));
    retVal = -1;
    }

  if( WIFEXITED( wStatus ) )
    {
    /* Notice( "child %d exited.\n", (int)child ); */
    }

  return retVal;
  }

/* 0 return only indicates that child forked successfully */
int ASyncRunShellNoIO( char* cmd )
  {
  if( EMPTY( cmd ) )
    return -1;

  char oldpath[BUFLEN], *oldp=NULL;
  oldp = getcwd( oldpath, sizeof(oldpath)-1 );
  if( oldp!=oldpath )
    Error( "Failed to get old path" );

  pid_t child = LaunchDaemon( 1 );
  if( child<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( child == 0 ) /* child */
    {
    /* Linux-specific - terminate via SIGHUP if parent exits */
    /* Notice( "Forked.  About to exec [%s] in a shell", cmd ); */

    if( chdir( oldp ) )
      Error( "Failed to return to path %s", oldp );

#if 0
    int fd = open( "/tmp/ASyncRunShellNoIO.log",
                   O_APPEND | O_WRONLY | O_CREAT, 0666 );
    if( fd<0 )
      {
      Warning( "Failed to open log file /tmp/ASyncRunShellNoIO.log" );
      }
    else
      {
      char buf[BIGBUF];
      snprintf( buf, sizeof(buf)-1, "Logging output from [%s]\n", cmd );
      int n = strlen( buf );
      if( write( fd, buf, n )!=n )
        Warning( "Failed to write initial item to log file" );

      snprintf( buf, sizeof(buf)-1, "Running in [%s]\n", oldp );
      n = strlen( buf );
      if( write( fd, buf, n )!=n )
        Warning( "Failed to write second item to log file" );

      dup2( fd, 1 );
      dup2( fd, 2 );
      }
#endif
    execl( "/bin/sh", "sh", "-c", cmd, (char*)NULL );

    /* should not reach this - so it's an error */
    Error( "Failed to run /bin/sh sh -c \"%s\" - %d:%s",
           cmd, errno, strerror( errno ) );
    /* end of code */
    }

  return 0;
  }

int SyncRunShellNoIO( char* cmd )
  {
  if( EMPTY( cmd ) )
    return -1;

  pid_t child = fork();
  if( child<0 )
    Error( "Failed to fork() - %d:%s", errno, strerror( errno ) );

  if( child == 0 ) /* child */
    {
    /* Linux-specific - terminate via SIGHUP if parent exits */
    prctl( PR_SET_PDEATHSIG, SIGHUP );
    close( 0 );
    close( 1 );
    close( 2 );
    /* Notice( "Forked.  About to exec [%s] in a shell", cmd ); */
    execl( "/bin/sh", "sh", "-c", cmd, (char*)NULL );
    /* should not reach this - so it's an error */
    Error( "Failed to run /bin/sh sh -c \"%s\" - %d:%s",
           cmd, errno, strerror( errno ) );
    /* end of code */
    }

  int retVal = 0;
  int wStatus;
  if( waitpid( child, &wStatus, 0 )==-1 )
    {
    Warning( "waitpid returned -1 (error.  errno=%d/%s).\n", errno, strerror( errno ));
    retVal = -1;
    }

  if( WIFEXITED( wStatus ) )
    {
    /* Notice( "child %d exited.\n", (int)child ); */
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

void KillExistingCommandInstances( char* commandLine, int sigNo )
  {
  if( EMPTY( commandLine ) )
    return;

  /* Notice( "KillExistingCommandInstances( %s )", commandLine ); */
  char* procLine = NULL;
  int nTries = 10;
  while( POpenAndSearch( "/bin/ps -efww", commandLine, &procLine )==0
         && nTries-- )
    {
    /* Notice( "Command [%s] already running - will try to stop it", commandLine ); */
    if( NOTEMPTY( procLine ) )
      {
      char* ptr = NULL;
      /* char* userID = strtok_r( procLine, " \t\r\n", &ptr ); */
      char* processID = strtok_r( NULL, " \t\r\n", &ptr );
      long pidNum = -1;
      if( NOTEMPTY( processID ) )
        pidNum = atol( processID );

      /* Notice( "Killing process %ld which belongs to %s", pidNum, userID ); */
      if( pidNum>0 )
        {
        int err = kill( (pid_t)pidNum, sigNo );
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

void KillExistingCommandInstancesGentleFirst( char* commandLine )
  {
  KillExistingCommandInstances( commandLine, SIGHUP );
  sleep(1);
  KillExistingCommandInstances( commandLine, SIGKILL );
  }

void KillEarlierInstancesOfThisProcess( int argc, char** argv, int sigNo )
  {
  if( argc<=0 || argv==NULL )
    return;

  pid_t me = getpid();
  char meBuf[100];
  snprintf( meBuf, sizeof(meBuf)-1, "%ld", (long)me );

  pid_t parent = getppid();
  char parentBuf[100];
  snprintf( parentBuf, sizeof(parentBuf)-1, "%ld", (long)parent );

  /* Notice( "KillEarlierInstancesOfThisProcess( %s )", argv[0] ); */

  int fileDesc = -1;
  pid_t child = -1;
  char* cmd = "/bin/ps -efww";
  int err = POpenAndRead( cmd, &fileDesc, &child );
  if( err ) Error( "Cannot popen child [%s].", cmd );

  int flags = fcntl( fileDesc, F_GETFL );
  flags &= ~O_NONBLOCK;
  fcntl( fileDesc, F_SETFL, flags);

  FILE* f = fdopen( fileDesc, "r" );
  char buf[BUFLEN];
  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    if( strstr( buf, meBuf )!=NULL
        && strstr( buf, parentBuf )!=NULL )
      { /* that's me - don't kill myself! */
      continue;
      }

    int found = 1;
    for( int i=0; i<argc; ++i )
      {
      if( strstr( buf, argv[i] )==NULL )
        {
        found = -1;
        break;
        }
      }

    if( found==1 )
      { /* cmd line appears to match */
      char* ptr = NULL;
      char* logName = strtok_r( buf, " \t\r\n", &ptr );
      char* procNumStr = strtok_r( NULL, " \t\r\n", &ptr );
      if( GetUID( logName ) == getuid() )
        { /* yes, it's mine */
        long procNum = atol( procNumStr );
        if( procNum>0 )
          { /* and it's a real process number */
          /* Notice( "Killing predecessor process %ld", procNum ); */
          kill( (pid_t)procNum, sigNo );
          }
        }
      }
    }

  fclose( f );
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

int DoWeHaveATTY()
  {
  char buf[BUFLEN];
  buf[0] = 0;
  int err = ReadLineFromCommand( "/usr/bin/tty", buf, sizeof(buf)-1, 1, 1 );
  if( err!=0 )
    return -100 + err; /* error running the command */
  if( strstr( buf, "not a tty" )!=NULL )
    return -1; /* definitely not a TTY */
  if( strstr( buf, "/dev/" )!=NULL )
    return 0; /* definitely a tty */
  return -2; /* ambiguous - lets assume not */
  }

/*  echo "big problem" | mail -s "Big problem" idan */
int SendEMail( char* recipient, char* subject, char* body )
  {
  if( EMPTY( recipient ) )
    return -1;
  if( EMPTY( subject ) )
    return -2;
  if( EMPTY( body ) )
    return -3;

  char mailCommand[BUFLEN];
  snprintf( mailCommand, sizeof(mailCommand)-1, "/usr/bin/mail -s '%s' '%s'",
            subject, recipient );

  int writeHandle = -1;
  pid_t child = -1;
  int err = POpenAndWrite( mailCommand, &writeHandle, &child );
  if( err )
    Error( "Cannot popen child to run [%s].", mailCommand );

  int l = strlen( body );
  int nBytes = write( writeHandle, body, l );
  if( nBytes<l )
    Warning( "Only managed to write %d of %d bytes to mail command", nBytes, l );
  l = write( writeHandle, "\n\n", 2 );
  if( l<2 )
    Warning( "Failed to write last \\n\\n to mail" );

  close( writeHandle );

  int retVal = 0;
  int wStatus;
  if( waitpid( child, &wStatus, WNOHANG )==-1 )
    {
    Notice( "waitpid returned -1 (error.  errno=%d/%s).", errno, strerror( errno ));
    retVal = -1;
    }

  if( WIFEXITED( wStatus ) )
    {
    /* Notice( "child %d exited.", (int)child); */
    retVal = 0;
    }

  if( retVal != 0 )
    {
    kill( child, SIGHUP );
    sleep( 1 );
    kill( child, SIGKILL );
    }

  if( retVal )
    retVal += -100;

  return retVal;
  }
