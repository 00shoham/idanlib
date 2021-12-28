#ifndef _INCLUDE_JSON
#define _INCLUDE_JSON

#define OPENBR '{'
#define CLOSEBR '}'

_TAG_VALUE* ParseJSON( char* string );
int CompareJSON( char* jsonA, char* jsonB,
                 _TAG_VALUE* tagsToDelete );
int ListToJSON( _TAG_VALUE* list, char* buf, int bufLen );
int NestedListToJSON( char* arrayName, _TAG_VALUE* list, char* buf, int bufLen );
int JSONToHTTPPost( char* relURL, char* json, char* buf, int bufLen );

#endif
