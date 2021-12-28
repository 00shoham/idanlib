#ifndef _INCLUDE_MEM_UTILS
#define _INCLUDE_MEM_UTILS

#define FREE(X) {if( (X)==NULL ) {Error("Trying to free NULL (%s:%d)", __FILE__, __LINE__ );} else { free(X); X=NULL; }}
#define FREEIFNOTNULL(X) {if( (X)!=NULL ) { free(X); X=NULL; }}
#define SAFESTRDUP(X) ((X)==NULL?NULL:strdup(X))

void *SafeCalloc( size_t nmemb, size_t size, char* msg);
void FreeIfAllocated( char** ptr );

#endif
