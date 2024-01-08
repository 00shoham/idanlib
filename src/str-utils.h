#ifndef _INCLUDE_STR_UTILS
#define _INCLUDE_STR_UTILS

#define EMPTY( XX ) ( XX==NULL || *(XX)==0 )
#define NOTEMPTY( XX ) ( XX!=NULL && *(XX)!=0 )
#define NULLPROTECT( XX ) ((XX)==NULL) ? "NULL" : (char*)(XX)
#define NULLPROTECTE( XX ) ((XX)==NULL) ? "" : (char*)(XX)

char* TrimHead( char* ptr );
void TrimTail( char* ptr );
char* RemoveExtraSpaces( char* raw, int noQuotes );
void GenerateIdentifier( char* buf, int nChars );
char* StripQuotes( char* buf );
char* StripEOL( char* buf );
void ChompEOL( FILE* stream );
int StringStartsWith( const char* string, const char* prefix, int caseSensitive );
int StringEndsWith( const char* string, const char* suffix, int caseSensitive );
int AllDigits( char* str );
int AllDigitsSingleDot( char* str );
int CountInString( const char* string, const int c );
int StringIsAnIdentifier( char* str );
void LowerCase( char* dst, int dstSize, char* src );
void UpperCase( char* dst, int dstSize, char* src );
int StringMatchesRegex( char* expr, char* str );
int CompareStringToPattern( char* pattern, char* example, int caseSensitive );
char* ExtractRegexFromString( char* expr, char* str );
void FreeArrayOfStrings( char** array, int len );
char* GenerateAllocateGUID();
int CompareStrings( const void* vA, const void* vB);
void MaskNonPrintableChars( unsigned char* str );
char* EncodeNonPrintableChars( unsigned char* str, unsigned char* omitChars  );
char* StrDupIfNotNull( char* str );
int HexDigitNumber( int c );
char* EscapeString( uint8_t* rawString, size_t rawLen, char* buf, size_t buflen );
uint8_t* UnescapeString( char* src, uint8_t* dst, size_t buflen );
char* SimpleHash( char* string, int nBytes );
int IsUnicodeMarkup( char* raw );
char* UnescapeUnicodeMarkup( char* raw );
int CountOccurrences( char* pattern, char* search );
char* SearchAndReplace( char* pattern, char* search, char* replace );

#endif
