#include "utils.h"

/*
 * base64.c
 * shamelessly copied from NibbleAndAHalf and modified.  Thank You!!
 *
 * Original was created by William Sherif on 2022-02-01.
 * Copyright (c) 2022 William Sherif. All rights reserved.
 */

/*

  https://github.com/superwills/NibbleAndAHalf
  base64.h -- Fast base64 encoding and decoding.
  version 1.0.1, Feb 1, 2022 812a

  Copyright (C) 2013 William Sherif

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  William Sherif
  will.sherif@gmail.com

  YWxsIHlvdXIgYmFzZSBhcmUgYmVsb25nIHRvIHVz

*/

/* Checks the integrity of a base64 string to make sure it is
   made up of only characters in the base64 alphabet (array b64) */
#define isbase64ValidChr( ch ) ( ('0' <= ch && ch <= '9') || \
  ('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z') || \
  ch=='+' || ch=='/' )  /* other 2 valid chars, + ending chrs */
/* '=' is NOT considered a valid base64 chr, it's only valid at the end for padding */

#define isMultipleOf(a,x) (!((a)%x))

#define SEXTET_A(byte0) (byte0 >> 2)
#define SEXTET_B(byte0, byte1) (((0x3&byte0) << 4) | (byte1 >> 4))
#define SEXTET_C(byte1, byte2) (((0xf&byte1) << 2) | (byte2 >> 6))
#define SEXTET_D(byte2) (0x3f&byte2)

#define EQUALS '='

/* map in both directions: */
const static char* b64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const static unsigned char unb64[] =
  {
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,  62,   0,   0,   0,  63,  52,  53,
 54,  55,  56,  57,  58,  59,  60,  61,   0,   0,
  0,   0,   0,   0,   0,   0,   1,   2,   3,   4,
  5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
 25,   0,   0,   0,   0,   0,   0,  26,  27,  28,
 29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
 39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
 49,  50,  51,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,
  }; /* note - exactly 256 bytes above */

int ValidBase64String( const char *ascii, int len )
  {
  if( ascii==NULL )
    return -1;

  if( ascii[0]==0 && len==0 ) /* empty is okay */
    return 0;

  if( len<0 )
    return -2;

  if( NOTEMPTY( ascii ) && len==0 )
    return -3;

  if( EMPTY( ascii ) && len!=0 )
    return -4;

  if( len % 4 )
    return -5;

  int i=0;
  for( i=0; i < (len-2); i++ )
    {
    if( !isbase64ValidChr( ascii[i] ) )
      {
      Warning( "ERROR in ValidBase64String at chr %d [%c]. String is NOT valid base64.\n", i, ascii[i] );
      return -6;
      }
    }

  // Only last 2 can be '='
  // Check 2nd last:
  if( ascii[i]==EQUALS )
    {
    // If the 2nd last is = the last MUST be = too
    if( ascii[i+1] != EQUALS )
      {
      Warning( "ValidBase64String at chr %d.\n"
               "If the 2nd last chr is '=' then the last chr must be '=' too.\n "
               "String is NOT valid base64.", i );
      return -7;
      }
    }
  else if( !isbase64ValidChr( ascii[i] ) )  // not = or valid base64
    {
    // 2nd last was invalid and not '='
    Warning( "ValidBase64String at chr %d (2nd last chr). String is NOT valid base64.\n", i );
    return -8;
    }

  // check last
  i++;
  if( ascii[i]!=EQUALS && !isbase64ValidChr( ascii[i] ) )
    {
    Warning( "ValidBase64String at chr %d (last chr). String is NOT valid base64.\n", i );
    return -9;
    }

  return 0;
  }

char* EncodeToBase64( const void* binaryData, int len, int *outputLength )
  {
  if( binaryData==NULL )
    return NULL;
  if( len==0 )
    return NULL;
  if( outputLength==NULL )
    Error( "EncodeToBase64 with no place to return output size" );

  const unsigned char* bin = (const unsigned char*)binaryData;

  int lenMod3 = len % 3;

  int pad = ((lenMod3&1)<<1) + ((lenMod3&2)>>1); // 2 gives 1 and 1 gives 2, but 0 gives 0.

  *outputLength = 4*(len + pad)/3;
  char* base64String = (char*)SafeCalloc( *outputLength + 1, sizeof(char), "Base64 output buffer" );

  int i = 0;
  int byteNo = 0;

  for( byteNo=0; byteNo <= len-3; byteNo+=3 )
    {
    unsigned char BYTE0 = bin[byteNo];
    unsigned char BYTE1 = bin[byteNo+1];
    unsigned char BYTE2 = bin[byteNo+2];

    base64String[i++] = b64[ SEXTET_A(BYTE0) ];
    base64String[i++] = b64[ SEXTET_B(BYTE0, BYTE1) ];
    base64String[i++] = b64[ SEXTET_C(BYTE1, BYTE2) ];
    base64String[i++] = b64[ SEXTET_D(BYTE2) ];
    }

  if( pad==1 )
    {
    unsigned char BYTE0 = bin[byteNo];
    unsigned char BYTE1 = bin[byteNo+1];
    base64String[i++] = b64[ SEXTET_A(BYTE0) ];
    base64String[i++] = b64[ SEXTET_B(BYTE0, BYTE1) ];
    base64String[i++] = b64[ (0xf&BYTE1) << 2 ];
    base64String[i++] = EQUALS;
    }
  else if( pad==2 )
    {
    unsigned char BYTE0 = bin[byteNo];
    base64String[i++] = b64[ SEXTET_A(BYTE0) ];
    base64String[i++] = b64[ (0x3&BYTE0) << 4 ]; // "padded" by 0's, these 2 bits are still HI ORDER BITS.
    base64String[i++] = EQUALS;
    base64String[i++] = EQUALS;
    }

  base64String[i] = 0;
  return base64String;
  }

unsigned char* DecodeFromBase64( const char* ascii, int len, int *outputLength )
  {
  if( EMPTY( ascii ) )
    return (unsigned char*)ascii;
  if( len==0 )
    Error( "Non-empty buffer in DecodeFromBase64 but length==0" );
  if( outputLength==NULL )
    Error( "Nowhere to return output size in DecodeFromBase64" );

#ifdef SAFEBASE64
  if( ValidBase64String( ascii, len )!=0 )
    return NULL;
#endif

  const unsigned char *safeAsciiPtr = (const unsigned char*)ascii;

  int pad = 0;
  if( len > 1 )
    {
    if( safeAsciiPtr[ len-1 ]==EQUALS )
      ++pad;
    if( safeAsciiPtr[ len-2 ]==EQUALS )
      ++pad;
    }

  *outputLength = 3*(len/4) - pad;
  if( *outputLength < 0 )
    *outputLength = 0;

  unsigned char *bin = (unsigned char*)SafeCalloc( *outputLength + 10, sizeof(char), "DecodeFromBase64 output" );

  int outputPos = 0; // counter for bin
  int charNo; // counter for what base64 char we're currently decoding

  for( charNo=0; charNo <= len-4-pad; charNo += 4 )
    {
    int A=unb64[safeAsciiPtr[charNo]];
    int B=unb64[safeAsciiPtr[charNo+1]];
    int C=unb64[safeAsciiPtr[charNo+2]];
    int D=unb64[safeAsciiPtr[charNo+3]];

    bin[outputPos++] = (A<<2) | (B>>4);
    bin[outputPos++] = (B<<4) | (C>>2);
    bin[outputPos++] = (C<<6) | (D);
    }

  if( isMultipleOf(len, 4) )
    {
    if( pad==1 )
      {
      int A=unb64[safeAsciiPtr[charNo]];
      int B=unb64[safeAsciiPtr[charNo+1]];
      int C=unb64[safeAsciiPtr[charNo+2]];
      bin[outputPos++] = (A<<2) | (B>>4);
      bin[outputPos++] = (B<<4) | (C>>2);
      }
    else if( pad==2 )
      {
      int A=unb64[safeAsciiPtr[charNo]];
      int B=unb64[safeAsciiPtr[charNo+1]];
      bin[outputPos++] = (A<<2) | (B>>4);
      }
    }

  bin[outputPos] = 0;

  return bin;
  }

