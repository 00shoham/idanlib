#include "utils.h"
#include "tiny-aes.h"

extern int seededRand;

int EncryptAES256( uint8_t* plaintext,
                   size_t plaintextLen,
                   uint8_t* key,
                   size_t keyLen,
                   uint8_t** outputBuffer,
                   size_t* outputSize )
  {
  if( plaintext==NULL )
    return -1;
  if( key==NULL )
    return -2;
  if( keyLen<5 )
    return -3;
  if( outputBuffer==NULL )
    return -4;
  if( outputSize==NULL )
    return -5;

  size_t targetPlaintextLen = plaintextLen;
  while( targetPlaintextLen % AES_BLOCKLEN != 0 )
    ++targetPlaintextLen;
  if( targetPlaintextLen==0 )
    targetPlaintextLen = AES_BLOCKLEN;

  /* put iv in first AES_BLOCKLEN bytes */
  uint8_t* myPlaintext = (uint8_t*)SafeCalloc( targetPlaintextLen + AES_BLOCKLEN, sizeof( uint8_t ), "resized AES input" );
  if( plaintextLen>0 )
    memcpy( myPlaintext+AES_BLOCKLEN, plaintext, targetPlaintextLen );

  if( keyLen > AES_KEYLEN )
    keyLen = AES_KEYLEN;
  uint8_t* myKey = NULL;
  if( keyLen < AES_KEYLEN )
    {
    myKey = (uint8_t *)SafeCalloc( AES_KEYLEN, sizeof(uint8_t), "resized AES key" );
    memcpy( myKey, key, keyLen );
    }
  else
    myKey = key;

  struct AES_ctx ctx = { 0 };

  if( seededRand==0 )
    {
    srand( time(NULL) );
    seededRand = 1;
    }

  for( int i=0; i<AES_BLOCKLEN; ++i )
    myPlaintext[i] = rand() % 256;
  AES_init_ctx_iv( &ctx, myKey, myPlaintext );
  AES_CBC_encrypt_buffer(&ctx, myPlaintext + AES_BLOCKLEN, targetPlaintextLen);

  *outputBuffer = myPlaintext;
  *outputSize = targetPlaintextLen + AES_BLOCKLEN;

  if( keyLen < AES_KEYLEN )
    free( myKey );

  return 0;
  }

int DecryptAES256( uint8_t* ciphertext,
                   size_t ciphertextLen,
                   uint8_t* key,
                   size_t keyLen,
                   uint8_t** outputBuffer,
                   size_t* outputSize )
  {
  if( ciphertext==NULL )
    return -1;
  if( ciphertextLen< (AES_BLOCKLEN*2) )
    return -2;
  if( key==NULL )
    return -3;
  if( keyLen<5 )
    return -4;
  if( outputBuffer==NULL )
    return -5;
  if( outputSize==NULL )
    return -6;
  if( ciphertextLen % AES_BLOCKLEN !=0 )
    return -7;

  uint8_t* targetBuffer = (uint8_t*)SafeCalloc( ciphertextLen - AES_BLOCKLEN, sizeof(uint8_t), "AES decryption buffer" );
  memcpy( targetBuffer, ciphertext + AES_BLOCKLEN, ciphertextLen - AES_BLOCKLEN );

  if( keyLen > AES_KEYLEN )
    keyLen = AES_KEYLEN;
  uint8_t* myKey = NULL;
  if( keyLen < AES_KEYLEN )
    {
    myKey = (uint8_t *)SafeCalloc( AES_KEYLEN, sizeof(uint8_t), "resized AES key" );
    memcpy( myKey, key, keyLen );
    }
  else
    myKey = key;

  struct AES_ctx ctx = { 0 };

  AES_init_ctx_iv( &ctx, myKey, ciphertext );
  AES_CBC_decrypt_buffer(&ctx, targetBuffer, ciphertextLen - AES_BLOCKLEN );

  *outputBuffer = targetBuffer;
  *outputSize = ciphertextLen - AES_BLOCKLEN ;

  if( keyLen < AES_KEYLEN )
    free( myKey );

  return 0;
  }

int EncryptAES256Base64Encode( uint8_t* plaintext,
                               size_t plaintextLen,
                               uint8_t* key,
                               size_t keyLen,
                               uint8_t** outputBuffer,
                               size_t* outputSize )
  {
  uint8_t* ciphertext = NULL;
  size_t   cipherlen = 0;
  int err = EncryptAES256( plaintext, plaintextLen, key, keyLen,
                           &ciphertext, &cipherlen );

  if( err )
    return err;

  int base64len = 0;
  char* base64 = EncodeToBase64( ciphertext, cipherlen, &base64len );
  free( ciphertext );

  *outputBuffer = (uint8_t*)base64;
  *outputSize = (size_t)base64len;

  return 0;
  }

int Base64DecodeDecryptAES256( char* base64cipher,
                               int base64Len,
                               uint8_t* key,
                               size_t keyLen,
                               uint8_t** outputBuffer,
                               size_t* outputSize )
  {
  int rawLen = 0;
  uint8_t* rawEncrypted = DecodeFromBase64( base64cipher, base64Len, &rawLen );
  if( rawLen == 0 )
    return -999;
  if( rawLen < 0 )
    return rawLen;

  size_t   plainlen = 0;
  uint8_t* plaintext = NULL;
  int err = DecryptAES256( rawEncrypted, rawLen, key, keyLen,
                           &plaintext, &plainlen );
  if( err )
    return err;

  *outputBuffer = (uint8_t*)plaintext;
  *outputSize = (size_t)plainlen;

  return 0;
  }

