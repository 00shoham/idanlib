#include "sha256.h"
#include "utils.h"


#define SALT_LENGTH 4

char* HashSHA256( unsigned char* string, size_t nBytes )
  {
  unsigned char hash[SHA256_BLOCK_SIZE];
  SHA256_CTX ctx;

  sha256_init( &ctx );
  sha256_update( &ctx, string, nBytes );
  sha256_final( &ctx, hash );

  return Encode( SHA256_BLOCK_SIZE, hash );
  }

char* HashSHA256WithSalt( unsigned char* string,
                          size_t stringLength,
                          unsigned char* salt,
                          size_t saltLength )
  {
  unsigned char* buf = (unsigned char*)malloc( stringLength + saltLength + 1 );
  memcpy( buf, string, stringLength );
  memcpy( buf+stringLength, salt, saltLength );
  buf[ stringLength + saltLength ] = 0;
  char* retVal = HashSHA256( buf, stringLength + saltLength );
  free( buf );
  return retVal;
  }

int seededRand = 0;
void GenerateSalt( unsigned char* salt, size_t length )
  {
  if( seededRand==0 )
    {
    srand( time(NULL) );
    seededRand = 1;
    }

  for( int i=0; i<length; ++i )
    salt[i] = rand() % 256;
  }

void GenerateSaltAndHashPassword( char* password,
                                  char** saltPtr,
                                  char** hashPtr )
  {
  if( EMPTY( password ) )
    Error( "Cannot salt/hash empty password" );
  if( saltPtr==NULL || hashPtr==NULL )
    Error( "Cannot salt/hash with NULL pointers" );
  unsigned char salt[SALT_LENGTH];
  GenerateSalt( salt, sizeof(salt) );
  *saltPtr = Encode( sizeof(salt), salt );
  *hashPtr = HashSHA256WithSalt( (unsigned char*)password,
                                 strlen( password ),
                                 salt, sizeof( salt ) );
  /* remember to free the encoded strings */
  }

void ExtractSaltFromPasswordField( char* field, unsigned char** saltPtr )
  {
  if( EMPTY( field ) )
    Error( "Cannot extract salt from empty password field" );
  if( saltPtr==NULL )
    Error( "No return address for salt" );
  if( strlen( field ) < SALT_LENGTH*2 )
    Error( "Password field too short for salt" );

  char buf[SALT_LENGTH*2 + 1];
  strncpy( buf, field, SALT_LENGTH * 2 );
  *saltPtr = Decode( buf );
  /* remember to free it */
  }

int DoesPasswordMatchHash( char* password, char* salt, char* hash )
  {
  if( EMPTY( password ) || EMPTY( salt ) || EMPTY( hash ) )
    Error( "DoesPasswordMatchHash requires three non-NULL args" );

  unsigned char* rawSalt = Decode( salt );
  char* generatedHash = HashSHA256WithSalt( (unsigned char*)password,
                                            strlen( password ),
                                            rawSalt,
                                            SALT_LENGTH );
  free( rawSalt );

  int match = -1;
  if( strcmp( generatedHash, hash )==0 )
    match = 0;

  free( generatedHash );

  return match;
  }

int GeneratePasswordHistoryRecord( char* userID, char* password, char** record )
  {
  if( StringMatchesUserIDFormat( userID )!=0 || EMPTY( password ) )
    return -1;
  if( record==NULL )
    return -2;
  char* salt = NULL;
  char* hash = NULL;
  GenerateSaltAndHashPassword( password, &salt, &hash );
  char buf[BUFLEN];
  snprintf( buf, sizeof(buf)-1, "%s:%08lx:%s%s",
            userID, (long int)time(NULL), salt, hash );
  free( salt );
  free( hash );
  *record = strdup( buf );
  return 0;
  }

int ParseHistoryRecord( char* record,
                        char** user, time_t* when,
                        char** salt, char** hash )
  {
  if( EMPTY( record ) )
    return -1;
  if( user==NULL )
    return -2;
  if( when==NULL )
    return -3;
  if( salt==NULL )
    return -4;
  if( hash==NULL )
    return -5;

  char* ptr = NULL;
  char* userID = strtok_r( record, ":", &ptr );
  if( userID==NULL )
    return -6;
  else *user = userID;

  char* whenStr = strtok_r( NULL, ":", &ptr );
  if( whenStr==NULL )
    return -7;

  long int t;
  if( sscanf( whenStr, "%lx", &t )!=1 )
    return -8;
  *when = (time_t)t;

  char* combined = strtok_r( NULL, ":", &ptr );
  if( combined==NULL )
    return -9;

  char saltBuf[100];
  strncpy( saltBuf, combined, SALT_LENGTH*2 );
  saltBuf[SALT_LENGTH*2] = 0;
  *salt = strdup( saltBuf );

  char hashBuf[200];
  strncpy( hashBuf, combined + SALT_LENGTH*2, sizeof(hashBuf)-1 );
  *hash = strdup( hashBuf );

  return 0;
  }
