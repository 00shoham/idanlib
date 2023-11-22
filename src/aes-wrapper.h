#ifndef _INCLUDE_AES_WRAPPER
#define _INCLUDE_AES_WRAPPER

int EncryptAES256( uint8_t* plaintext,
                   size_t plaintextLen,
                   uint8_t* key,
                   size_t keyLen,
                   uint8_t** outputBuffer,
                   size_t* outputSize );

int DecryptAES256( uint8_t* ciphertext,
                   size_t ciphertextLen,
                   uint8_t* key,
                   size_t keyLen,
                   uint8_t** outputBuffer,
                   size_t* outputSize );

int EncryptAES256Base64Encode( uint8_t* plaintext,
                               size_t plaintextLen,
                               uint8_t* key,
                               size_t keyLen,
                               uint8_t** outputBuffer,
                               size_t* outputSize );

int Base64DecodeDecryptAES256( char* base64cipher,
                               int base64Len,
                               uint8_t* key,
                               size_t keyLen,
                               uint8_t** outputBuffer,
                               size_t* outputSize );

#endif
