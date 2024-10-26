#ifndef _INCLUDE_TAG_VALUE
#define _INCLUDE_TAG_VALUE

enum valueType { VT_INVALID, VT_STR, VT_INT, VT_DOUBLE, VT_LIST, VT_NULL };

#define INVALID_INT -999999
#define INVALID_DOUBLE -99999.0

typedef struct _tag_value
  {
  char* tag;
  char* value;
  int iValue;
  double dValue;
  enum valueType type;
  struct _tag_value *subHeaders;
  struct _tag_value *next;
  } _TAG_VALUE;

enum valueType GuessType( char* str );
char* TypeName( enum valueType t );

_TAG_VALUE* NewTagValue( char* tag, const char* value, _TAG_VALUE* list, int replaceDup );
_TAG_VALUE* AppendValue( char* buf, _TAG_VALUE* workingHeaders );
_TAG_VALUE* NewTagValueInt( char* tag, int value, _TAG_VALUE* list, int replaceDup );
_TAG_VALUE* NewTagValueDouble( char* tag, double value, _TAG_VALUE* list, int replaceDup );
_TAG_VALUE* NewTagValueGuessType( char* tag, char* value, _TAG_VALUE* list, int replaceDup );
_TAG_VALUE* NewTagValueNull( char* tag, _TAG_VALUE* list, int replaceDup );
_TAG_VALUE* NewTagValueList( char* tag, _TAG_VALUE* subList, _TAG_VALUE* list, int replaceDup );
_TAG_VALUE* CopyTagValueList( _TAG_VALUE* list );
char* GetTagValue( _TAG_VALUE* list, char* tagName );
_TAG_VALUE* FindTagValue( _TAG_VALUE* list, char* tagName );
_TAG_VALUE* FindTagValueByValue( _TAG_VALUE* list, char* value );
_TAG_VALUE* FindTagValueNoCase( _TAG_VALUE* list, char* tagName );
int GetTagValueInt( _TAG_VALUE* list, char* tagName );
double GetTagValueDouble( _TAG_VALUE* list, char* tagName );
_TAG_VALUE* GetTagValueList( _TAG_VALUE* list, char* tagName );
void PrintTagValue( int indent, _TAG_VALUE* list );
void PrintTagValueFromList( _TAG_VALUE* list, char* tag );
char* GetTagValueSafe( _TAG_VALUE* list, char* tagName, char* expr );
void FreeTagValue( _TAG_VALUE* list );
_TAG_VALUE* DeleteTagValue( _TAG_VALUE* list, char* tag );
int CompareTagValueList( _TAG_VALUE* a, _TAG_VALUE* b );
int CompareTagValueListBidirectional( _TAG_VALUE* a, _TAG_VALUE* b );

int ExpandMacros( char* src, char* dst, int dstLen, _TAG_VALUE* patterns );
int ExpandMacrosVA( char* src, char* dst, int dstLen, ... );
_TAG_VALUE* TagIntList( char* placeHolderPtr, ... );

char* AggregateMessages( _TAG_VALUE* messages );

_TAG_VALUE* AppendTagValue( _TAG_VALUE* list, _TAG_VALUE* newItem );
_TAG_VALUE* ExtractValueFromPath( _TAG_VALUE* tree, _TAG_VALUE* path );
_TAG_VALUE* ParsePath( char* textPath );

#endif

