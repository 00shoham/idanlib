#ifndef _BASE64_INC
#define _BASE64_INC

int ValidBase64String( const char *ascii, int len );
char* EncodeToBase64( const void* binaryData, int len, int *outputLength );
unsigned char* DecodeFromBase64( const char* ascii, int len, int *outputLength );

#endif
