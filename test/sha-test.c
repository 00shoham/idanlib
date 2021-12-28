#include "base.h"

#define PWCHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !@#$%^&*()-_=+[{]}\\|;:'\",<.>/?`~"

#define MINPWLEN 5
#define MAXPWLEN 50

#define LOCKPATH "/tmp/.history.lock"
#define HISTFILE "/tmp/password.history"
#define USERID   "bob"

int RandomPasswordChar()
  {
  int nChars = strlen( PWCHARS );
  int x = rand() % nChars;
  return (int)(PWCHARS[x]);
  }

int RandomPasswordLength()
  {
  return MINPWLEN + rand() % (MAXPWLEN-MINPWLEN);
  }

void GenerateRandomPassword( char* buf, size_t chars )
  {
  int i = 0;
  for( i=0; i<chars; ++i )
    buf[i] = RandomPasswordChar();
  buf[i] = 0;
  }

int main()
  {
  srand( time(NULL) );
  char pw[BUFLEN];
  char* hash = NULL;
  char* salt = NULL;

  for( int pwNum=0; pwNum<5; ++pwNum )
    {
    printf( "Trial %d\n", pwNum );

    int err = 0;
    int pwLen = RandomPasswordLength();
    GenerateRandomPassword( pw, pwLen );
    printf( "  PW = [%s]\n", pw );

    /*
    err = AddPasswordToHistory( LOCKPATH, HISTFILE, USERID, pw );
    if( err )
      printf( "  Error creating history record - %d\n", err );
    else
      printf( "  Appended history record to %s\n", HISTFILE );
    */

    char* record = NULL;
    err = GeneratePasswordHistoryRecord( "someuser", pw, &record );
    if( err )
      Error( "Generating password history record failed (%d)", err );
    printf( "  HISTORY = [%s]\n", record );

    time_t tWhen = 0;
    char* userID = NULL;
    err = ParseHistoryRecord( record, &userID, &tWhen, &salt, &hash );
    if( err )
      Error( "Parsing password history record failed (%d)", err );

    printf( "  SALT = [%s]\n", salt );
    printf( "  HASH = [%s]\n", hash );

    int match = DoesPasswordMatchHash( pw, salt, hash );
    printf( "  Match = %d\n", match );

    printf( "\n" );
    free( salt );
    free( hash );
    }

  return 0;
  }
