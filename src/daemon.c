#include "utils.h"

void MySegFaultHandler( int sig )
  {
  void* array[50];
  size_t nCalls = backtrace( array, sizeof(array)-1 );
  char** functions = backtrace_symbols( array, nCalls );
  for( int i=0; i<nCalls; ++i )
    {
    Warning( "SEGV: %s", functions[i] );
    }
  Error( "SegFault - final" );
  }

void LaunchDaemon()
  {
  pid_t pid;

  /* Fork off the parent process */
  pid = fork();

  /* An error occurred */
  if( pid < 0 )
    {
    printf("Failed to fork child process\n");
    exit(EXIT_FAILURE);
    }

  /* Success: Let the parent terminate */
  if( pid > 0 )
    {
    printf("Forked child process %d\n", pid );
    exit(EXIT_SUCCESS);
    }

  /* On success: The child process becomes session leader */
  if( setsid() < 0 )
    {
    printf("Failed to setsid in child process\n");
    exit(EXIT_FAILURE);
    }

  /* Catch, ignore and handle signals */
  if( signal(SIGSEGV, MySegFaultHandler )==SIG_ERR )
    {
    printf("Failed to trap (ignore) SIGSEGV\n");
    exit(EXIT_FAILURE);
    }

  if( signal(SIGCHLD, SIG_IGN )==SIG_ERR )
    {
    printf("Failed to trap (ignore) SIGCHLD\n");
    exit(EXIT_FAILURE);
    }

  if( signal(SIGHUP, SignalHandler )==SIG_ERR )
    {
    printf("Failed to trap SIGHUP\n");
    exit(EXIT_FAILURE);
    }

  if( signal(SIGUSR1, SignalHandler )==SIG_ERR )
    {
    printf("Failed to trap SIGUSR1\n");
    exit(EXIT_FAILURE);
    }

  /* Fork off for the second time*/
  pid = fork();

  /* An error occurred */
  if( pid < 0 )
    {
    printf("Failed to fork second child process\n");
    exit(EXIT_FAILURE);
    }

  /* Success: Let the parent terminate */
  if( pid > 0 )
    {
    printf("Second parent forked - final child is %d\n", pid );
    exit(EXIT_SUCCESS);
    }

  /* Set new file permissions */
  umask(0);

  /* Change the working directory to the root directory */
  /* or another appropriated directory */
  if( chdir("/tmp")!=0 )
    {
    Warning("Failed to chdir(/tmp)");
    }

  /* Close all open file descriptors */
  for( int x = 2; x>=0; x-- )
    {
    close( x );
    }

  /* Open the log file */
  openlog( "monitor-thermostat-web", LOG_PID, LOG_DAEMON );
  inDaemon = 1;
  }

