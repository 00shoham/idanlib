#ifndef _INCLUDE_SHA_WRAPPER
#define _INCLUDE_SHA_WRAPPER

void GenerateSaltAndHashPassword( char* password,
                                  char** saltPtr,
                                  char** hashPtr );
int DoesPasswordMatchHash( char* password, char* salt, char* hash );
int GeneratePasswordHistoryRecord( char* userID, char* password, char** record );
int ParseHistoryRecord( char* record,
                        char** user, time_t* when,
                        char** salt, char** hash );
void SHA1hex( char* hash, char* plainText, int ptLen );

#endif
