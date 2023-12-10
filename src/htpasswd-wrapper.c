/* These functions call the htpasswd binary to manipulate the contents of
   a .htpasswd file.  if you want to apply password policy rules or update
   history, do that elsewhere - before/after calling these. */

#include "utils.h"

#define HTPASSWDBIN "/usr/bin/htpasswd"
#define TIMEOUT_ATTEMPT 1
#define TIMEOUT_MAX 5
#define MAX_PASSWORD_LENGTH 50

int IsValidPassword( char* str )
  {
  if( EMPTY( str ) )
    return -1;

  while( *str )
    {
    if( !isprint( *str ) )
      return -2;
    ++str;
    }

  return 0;
  }

int HTPasswdValidUser( char* lockPath, char* passwdFile, char* userID )
  {
  int err = 0;
  int lockFD = 0;

  if( EMPTY( passwdFile ) )
    return -1;

  err = StringMatchesUserIDFormat( userID );
  if( err )
    goto end;

  if( NOTEMPTY( lockPath ) )
    {
    lockFD = LockFile( lockPath );
    if( lockFD<=0 )
      {
      Warning( "Failed to lock [%s] (%d:%s)", lockPath, errno, strerror( errno ) );
      err = -100;
      goto end;
      }
    }

  FILE* f = fopen( passwdFile, "r" );
  if( f==NULL )
    {
    err = -3;
    goto end;
    }

  char userWithColon[300];
  snprintf( userWithColon, sizeof(userWithColon)-1, "%s:", userID );
  int l = strlen( userWithColon );
  char buf[BUFLEN];
  int found = 0;
  while( fgets( buf, sizeof(buf)-1, f )==buf )
    {
    if( strncmp( userWithColon, buf, l )==0 )
      {
      found = 1;
      break;
      }
    }

  fclose( f );

  if( ! found )
    err = -10;

  end:
  if( lockFD )
    {
    int x = UnLockFile( lockFD );
    if( x )
      {
      Warning( "Failed to clear lock file [%s] - %d (%d:%s)", lockPath, x, errno, strerror( errno ) );
      err = -400 + x;
      }
    }

  return err;
  }

int HTPasswdAddUser( char* lockPath, char* passwdFile, char* userID, char* password )
  {
  if( EMPTY( passwdFile ) )
    return -1;
  if( EMPTY( userID ) )
    return -2;
  if( IsValidPassword( password ) !=0 )
    return -3;

  int err = 0;
  int lockFD = 0;

  err = StringMatchesUserIDFormat( userID );
  if( err )
    goto end;

  if( NOTEMPTY( lockPath ) )
    {
    lockFD = LockFile( lockPath );
    if( lockFD<=0 )
      {
      Warning( "Failed to lock [%s] (%d:%s)", lockPath, errno, strerror( errno ) );
      err = -100;
      goto end;
      }
    }

  err = HTPasswdValidUser( NULL, passwdFile, userID );
  if( err==0 )
    {
    Warning( "User [%s] already exists", userID );
    goto end;
    }

  char cmd[BUFLEN];
  snprintf( cmd, sizeof(cmd)-1, "%s -iB '%s' '%s'", HTPASSWDBIN, passwdFile, userID );

  char passwdNewline[BUFLEN];
  snprintf( passwdNewline, sizeof(passwdNewline), "%s\n", password );

  err = WriteLineToCommand( cmd, passwdNewline, 1/*sec poll*/, 5/*sec max*/ );

  end:
  if( lockFD )
    {
    int x = UnLockFile( lockFD );
    if( x )
      {
      Warning( "Failed to clear lock file [%s] - %d (%d:%s)", lockPath, x, errno, strerror( errno ) );
      err = -400 + x;
      }
    }

  return err;
  }


int HTPasswdRemoveUser( char* lockPath, char* passwdFile, char* userID )
  {
  if( EMPTY( passwdFile ) )
    return -1;
  if( EMPTY( userID ) )
    return -2;

  int err = 0;
  int lockFD = 0;

  err = StringMatchesUserIDFormat( userID );
  if( err )
    goto end;

  if( NOTEMPTY( lockPath ) )
    {
    lockFD = LockFile( lockPath );
    if( lockFD<=0 )
      {
      Warning( "Failed to lock [%s] (%d:%s)", lockPath, errno, strerror( errno ) );
      err = -100;
      goto end;
      }
    }

  err = HTPasswdValidUser( NULL, passwdFile, userID );
  if( err )
    {
    Warning( "User [%s] does not exist", userID );
    goto end;
    }

  char cmd[BUFLEN];
  snprintf( cmd, sizeof(cmd)-1, "%s -D '%s' '%s'", HTPASSWDBIN, passwdFile, userID );

  err = AsyncRunCommandNoIO( cmd );

  end:
  if( lockFD )
    {
    int x = UnLockFile( lockFD );
    if( x )
      {
      Warning( "Failed to clear lock file [%s] - %d (%d:%s)", lockPath, x, errno, strerror( errno ) );
      err = -400 + x;
      }
    }

  return err;
  }

int HTPasswdResetPassword( char* lockPath, char* passwdFile, char* userID, char* password )
  {
  if( EMPTY( passwdFile ) )
    return -1;
  if( EMPTY( userID ) )
    return -2;
  if( IsValidPassword( password ) !=0 )
    return -3;

  int err = 0;
  int lockFD = 0;

  err = StringMatchesUserIDFormat( userID );
  if( err )
    goto end;

  if( NOTEMPTY( lockPath ) )
    {
    lockFD = LockFile( lockPath );
    if( lockFD<=0 )
      {
      Warning( "Failed to lock [%s] (%d:%s)", lockPath, errno, strerror( errno ) );
      err = -100;
      goto end;
      }
    }

  err = HTPasswdValidUser( NULL, passwdFile, userID );
  if( err )
    {
    Warning( "User [%s] does not exist", userID );
    goto end;
    }

  char cmd[BUFLEN];
  snprintf( cmd, sizeof(cmd)-1, "%s -iB '%s' '%s'", HTPASSWDBIN, passwdFile, userID );

  char passwdNewline[BUFLEN];
  snprintf( passwdNewline, sizeof(passwdNewline), "%s\n", password );

  err = WriteLineToCommand( cmd, passwdNewline, 1/*sec poll*/, 5/*sec max*/ );

  end:
  if( lockFD )
    {
    int x = UnLockFile( lockFD );
    if( x )
      {
      Warning( "Failed to clear lock file [%s] - %d (%d:%s)", lockPath, x, errno, strerror( errno ) );
      err = -400 + x;
      }
    }

  return err;
  }

int HTPasswdCheckPassword( char* lockPath, char* passwdFile, char* userID, char* password )
  {
  if( EMPTY( passwdFile ) )
    return -1;
  if( EMPTY( userID ) )
    return -2;
  if( IsValidPassword( password ) !=0 )
    return -3;

  int err = 0;
  int lockFD = 0;

  err = StringMatchesUserIDFormat( userID );
  if( err )
    goto end;

  if( NOTEMPTY( lockPath ) )
    {
    lockFD = LockFile( lockPath );
    if( lockFD<=0 )
      {
      Warning( "Failed to lock [%s] (%d:%s)", lockPath, errno, strerror( errno ) );
      err = -100;
      goto end;
      }
    }

  err = HTPasswdValidUser( NULL, passwdFile, userID );
  if( err )
    {
    Warning( "User [%s] does not exist", userID );
    goto end;
    }

  char cmd[BUFLEN];
  snprintf( cmd, sizeof(cmd)-1, "%s -iv '%s' '%s'", HTPASSWDBIN, passwdFile, userID );

  char passwdNewline[BUFLEN];
  snprintf( passwdNewline, sizeof(passwdNewline), "%s\n", password );

  char response[BUFLEN];
  err = WriteReadLineToFromCommand( cmd, passwdNewline, response, sizeof(response)-1, TIMEOUT_ATTEMPT, TIMEOUT_MAX );

  if( err )
    goto end;

  char goodResponse[BUFLEN];
  snprintf( goodResponse, sizeof(goodResponse)-1, "Password for user %s correct.\n", userID );

  if( strcmp( response, goodResponse )!=0 )
    {
    Warning( "htpasswd validation - response was %s", response );
    err = -10;
    }

  end:
  if( lockFD )
    {
    int x = UnLockFile( lockFD );
    if( x )
      {
      Warning( "Failed to clear lock file [%s] - %d (%d:%s)", lockPath, x, errno, strerror( errno ) );
      err = -400 + x;
      }
    }

  return err;
  }

int HTPasswdChangePassword( char* lockPath, char* passwdFile, char* userID, char* oldp, char* newp )
  {
  if( EMPTY( passwdFile ) )
    return -1;
  if( EMPTY( userID ) )
    return -2;
  if( IsValidPassword( oldp ) !=0 )
    return -3;
  if( IsValidPassword( newp ) !=0 )
    return -3;

  int err = 0;
  int lockFD = 0;

  err = StringMatchesUserIDFormat( userID );
  if( err )
    goto end;

  if( NOTEMPTY( lockPath ) )
    {
    lockFD = LockFile( lockPath );
    if( lockFD<=0 )
      {
      Warning( "Failed to lock [%s] (%d:%s)", lockPath, errno, strerror( errno ) );
      err = -100;
      goto end;
      }
    }

  err = HTPasswdValidUser( NULL, passwdFile, userID );
  if( err )
    {
    Warning( "User [%s] does not exist", userID );
    goto end;
    }

  err = HTPasswdCheckPassword( NULL, passwdFile, userID, oldp );
  if( err )
    {
    Warning( "Provided old password not correct." );
    goto end;
    }

  err = HTPasswdResetPassword( NULL, passwdFile, userID, newp );
  if( err )
    {
    Warning( "Failed to set new password for [%s].", userID );
    goto end;
    }

  end:
  if( lockFD )
    {
    int x = UnLockFile( lockFD );
    if( x )
      {
      Warning( "Failed to clear lock file [%s] - %d (%d:%s)", lockPath, x, errno, strerror( errno ) );
      err = -400 + x;
      }
    }

  return err;
  }

