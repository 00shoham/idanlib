#include "utils.h"

void *SafeCalloc( size_t nmemb, size_t size, char* msg )
  {
  void* ptr = calloc( nmemb, size );
  if( ptr==NULL )
    {
    Error("Failed to allocate %d x %d (%s)", nmemb, size, msg );
    }
  return ptr;
  }

void FreeIfAllocated( char** ptr )
  {
  if( (*ptr)==NULL )
    return;
  FREE( *ptr );
  *ptr = NULL;
  }

