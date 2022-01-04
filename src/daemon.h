#ifndef _INCLUDE_DAEMON
#define _INCLUDE_DAEMON

void MySegFaultHandler( int sig );
/* new - return child process ID if in parent */
pid_t LaunchDaemon( int closeSTDIO );

#endif
