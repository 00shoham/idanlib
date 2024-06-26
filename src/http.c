#include "utils.h"

/* #undef DEBUG */
#define DEBUG 1

char hexDigits[] = "0123456789abcdef";

int GetSocketFromHostAndPort( char* serverName, int portNum,
                              struct addrinfo** aiPtr )
  {
  struct addrinfo hints;
  struct addrinfo *addr = NULL;

  memset( &hints, 0, sizeof(hints) );
  hints.ai_family = AF_INET; 
  hints.ai_socktype = SOCK_STREAM; 
  hints.ai_protocol = 0 /* IP - 5 is TCP */;
  hints.ai_flags = AI_CANONNAME | AI_ALL | AI_ADDRCONFIG;

  char portNumString[10];
  snprintf( portNumString, sizeof(portNumString)-1, "%d", portNum );

  int err = getaddrinfo( serverName, portNumString, &hints, &addr );
  if( err!=0 )
    {
    Warning( "Failed to locate %s:%s - %d:%s", serverName, portNumString, 
             errno, strerror( errno ) );
    return err;
    }

  int sockFD = socket( AF_INET, SOCK_STREAM, addr->ai_protocol );
  if( sockFD<=0 )
    {
    Warning( "Failed to create socket - %d:%s",
             errno, strerror( errno ) );
    freeaddrinfo( addr );
    addr = NULL;
    return -100 + sockFD;
    }

  err = connect( sockFD, addr->ai_addr, addr->ai_addrlen );
  if( err<0 )
    {
    Warning( "Failed to connect socket on %s:%d - %d:%s",
             serverName, portNum, errno, strerror( errno ) );
    close( sockFD );
    freeaddrinfo( addr );
    addr = NULL;
    return -200 + err;
    }

  *aiPtr = addr;
  return sockFD;
  }

int TCPPortSendReceive( char* serverName, int portNum,
                        char* httpPostText,
                        char* responseBuffer, int responseBufLen )
  {
  if( EMPTY( serverName ) )
    {
    Warning( "Cannot send command to empty server" );
    return -1;
    }
  if( portNum<1 || portNum>65535 )
    {
    Warning( "Cannot send command to invalid port number (%d)", portNum );
    return -2;
    }
  if( EMPTY( httpPostText ) )
    {
    Warning( "Cannot send empty command to web service" );
    return -3;
    }

  struct addrinfo* addr = NULL;
  int sockFD = GetSocketFromHostAndPort( serverName, portNum, &addr );
  if( sockFD<=0 )
    {
    if( addr!=NULL )
      {
      freeaddrinfo( addr );
      addr = NULL;
      }
    return sockFD;
    }

  size_t l = strlen( httpPostText );
  /* size_t debugL = l; */
  while( l>0 )
    {
    size_t n = write( sockFD, httpPostText, l );
    l -= n;
    httpPostText += n;
    }
  /* Notice( "Sent %d bytes to TCP port", (int)debugL ); */

  char* ptr = responseBuffer;
  *ptr = 0;
  l = responseBufLen - 1;
  size_t n = 0;
  for(;;)
    {
    /* Notice( "Select from socket on %s:%d", serverName, portNum ); */
    fd_set readSet;
    struct timeval timeout;
    FD_ZERO( &readSet );
    FD_SET( sockFD, &readSet );
    timeout.tv_sec = 5; /* generous timeout of 5s */
    timeout.tv_usec = 0;
    int result = select( FD_SETSIZE, &readSet, NULL, NULL, &timeout );

    /* Notice( "Select returned %d", result ); */
    if( result>0 )
      {
      n = read( sockFD, ptr, l );
      if( n>0 )
        {
        /* Notice( "Read %d bytes from TCP port", (int)debugL ); */
        ptr += n;
        *ptr = 0;
        /* Notice( "Read so far: [%s]", responseBuffer ); */
        l -= n;
        }
      else
        {
        /* Notice( "Read 0 bytes from %s:%d - closed?", serverName, portNum ); */
        break;
        }
      }
    else if( result==0 )
      { /* timeout */
      /* Notice( "Read from %s:%d timed out", serverName, portNum ); */
      break;
      }
    else /* closed? */
      {
      break;
      }
    }

  /* Notice( "Exited read loop" ); */

  if( l>0 )
    {
    *ptr = 0;
    }

  shutdown( sockFD, SHUT_RDWR );
  close( sockFD );
  freeaddrinfo( addr );
  addr = NULL;

  /* Notice( "Returning" ); */
  return 0;
  }

int CountTextLines( char* rawResponse )
  {
  if( EMPTY( rawResponse ) )
    {
    return 0;
    }
  int n = 1;
  char* ptr = NULL;
  enum state st = ST_TEXT;
  for( ptr = rawResponse; *ptr!=0; ++ptr )
    {
    int c = *ptr;
    switch( st )
      {
      case ST_TEXT:
        if( c=='\r' )
          {
          ++n;
          st = ST_GOTCR;
          }
        else if( c=='\n' )
          {
          ++n;
          st = ST_GOTNL;
          }
        break;
      case ST_GOTCR:
        if( c=='\n' )
          {
          st = ST_GOTNL;
          }
        break;
      case ST_GOTNL:
        st = ST_TEXT;
        break;
      }
    }

  return n;
  }

void ChopResponseIntoLines( char* buffer, char** lines, int n )
  {
  if( EMPTY( buffer ) )
    return;
  if( lines==NULL || n<=0 )
    return;

  char* ptr = NULL;
  char* linePtr = buffer;
  enum state st = ST_TEXT;
  int lineNo = 0;
  for( ptr = buffer; *ptr!=0; ++ptr )
    {
    int c = *ptr;
    switch( st )
      {
      case ST_TEXT:
        if( c=='\r' )
          {
          int len = ptr - linePtr;
          lines[lineNo] = malloc( len+1 );
          if( len )
            memcpy( lines[lineNo], linePtr, len );
          lines[lineNo][len] = 0;
          ++lineNo;
          if( lineNo==n )
            {
            /* seems to be normal.. Warning( "Line overflow (a) in ChopResponseIntoLines" ); */
            return;
            }
          st = ST_GOTCR;
          }
        else if( c=='\n' )
          {
          int len = ptr - linePtr;
          lines[lineNo] = malloc( len+1 );
          if( len )
            memcpy( lines[lineNo], linePtr, len );
          lines[lineNo][len] = 0;
          ++lineNo;
          if( lineNo==n )
            {
            Warning( "Line overflow (b) in ChopResponseIntoLines" );
            return;
            }
          st = ST_GOTNL;
          }
        break;
      case ST_GOTCR:
        if( c=='\n' )
          {
          st = ST_GOTNL;
          }
        break;
      case ST_GOTNL:
        st = ST_TEXT;
        linePtr = ptr;
        if( c=='\r' )
          {
          int len = ptr - linePtr;
          lines[lineNo] = malloc( len+1 );
          if( len )
            memcpy( lines[lineNo], linePtr, len );
          lines[lineNo][len] = 0;
          ++lineNo;
          if( lineNo==n )
            {
            /* seems to be normal Warning( "Line overflow (a) in ChopResponseIntoLines" ); */
            return;
            }
          st = ST_GOTCR;
          }
        else if( c=='\n' )
          {
          int len = ptr - linePtr;
          lines[lineNo] = malloc( len+1 );
          if( len )
            memcpy( lines[lineNo], linePtr, len );
          lines[lineNo][len] = 0;
          ++lineNo;
          if( lineNo==n )
            {
            Warning( "Line overflow (b) in ChopResponseIntoLines" );
            return;
            }
          st = ST_GOTNL;
          }
        break;
      }
    }

  if( lines[n-1]==NULL && lineNo==n-1 )
    {
    int len = ptr - linePtr;
    lines[lineNo] = malloc( len+1 );
    if( len )
      memcpy( lines[lineNo], linePtr, len );
    lines[lineNo][len] = 0;
    ++lineNo;
    }

#ifdef DEBUG
  Notice( "ChopResponseIntoLines - lineNo=%d, n=%d, last=%p", lineNo, n, lines[n-1] );
#endif
  }

int HTTPSendReceive( char* serverName, int portNum,
                     char* post, char* response, int responseLen )
  {
#ifdef DEBUG
  Notice( "HTTPSendReceive(%s,%d,%s,-,%d)", serverName, portNum, post, responseLen );
#endif
  char rawResponse[BUFLEN];
  int err = TCPPortSendReceive( serverName, portNum,
                                post, rawResponse, sizeof(rawResponse)-1 );
  if( err<0 )
    {
    return err;
    }
#ifdef DEBUG
    Notice( "... HTTPSendReceive RawResponse [%s]", rawResponse );
#endif

  int n = CountTextLines( rawResponse );
#ifdef DEBUG
    Notice( "... HTTPSendReceive %d lines", n );
#endif
  if( n<=0 )
    {
    return n;
    }
  char** lines = calloc( n, sizeof(char*) );
  ChopResponseIntoLines( rawResponse, lines, n );
  int chunked = 0;
  for( int i=0; i<n; ++i )
    {
#ifdef DEBUG
    Notice( "Response [%d] = [%s]\n", i, lines[i] );
#endif
    if( strstr( lines[i], "Transfer-Encoding: chunked" )!=NULL )
      {
      chunked = 1;
      break;
      }
    }
  /* printf( "%s\n", chunked ? "chunked" : "simple" ); */

  int nHeaders = 0;
  for( int i=0; i<n; ++i )
    {
    char* ptr = lines[i];
    int c = *ptr;
    if( strncmp( ptr, "HTTP/", 5 )==0 )
      {
      /* header */
      }
    else if( c>='A' && c<='Z' && strstr( ptr, ": " )!=NULL )
      {
      /* header */
      }
    else
      {
      nHeaders = i;
      break;
      }
    }
#ifdef DEBUG
  Notice("%d headers\n", nHeaders );
#endif

  char* meat = NULL;
  if( chunked )
    {
    meat = lines[nHeaders+2];
    }
  else
    {
    meat = lines[nHeaders+1];
    }

  if( meat!=NULL )
    {
    int l = strlen( meat );
    if( l >= responseLen )
      {
      l = responseLen - 1;
      }
    memcpy( response, meat, l );
    response[l] = 0;
    }

  FreeArrayOfStrings( lines, n );
  return 0;
  }

char* Encode( int nBytes, unsigned char* bytes )
  {
  if( nBytes<1 || nBytes>MAX_ENCODE || bytes==NULL )
    {
    Error("Cannot encode %d bytes at %p", nBytes, NULLPROTECT( bytes ) );
    }

  char* buf = (char*)calloc( 2*nBytes+1, sizeof(char) );
  char* ptr = buf;
  for( int i=0; i<nBytes; ++i )
    {
    int c = bytes[i];
    int high = (c & 0xf0)>>4;
    int low = (c & 0x0f);
    *(ptr++) = hexDigits[ high ];
    *(ptr++) = hexDigits[ low ];
    }
  *ptr = 0;
  return buf;
  }

unsigned char* Decode( char* string )
  {
  if( EMPTY( string ) )
    {
    return NULL;
    }

  int n = strlen( string );
  unsigned char* buf = (unsigned char*)calloc( n/2+1, sizeof(char*) );
  unsigned char* dst = buf;
  char* src = NULL;
  char* ptr = string;
  while( *ptr )
    {
    int high = *ptr;
    src = strchr( hexDigits, high );
    if( src==NULL )
      {
      Error("Cannot decode string with [%c] char (%s).", high, string );
      }
    high = (src - hexDigits);
    high = high << 4;

    ++ptr;

    int low = *ptr;
    src = strchr( hexDigits, low );
    if( src==NULL )
      {
      Error("Cannot decode string with [%c] char (%s).", low, string );
      }
    low = (src - hexDigits);

    ++ptr;

    int c = high | low;
    *(dst++) = c;
    }

  *dst = 0;

  return buf;
  }

int GetArgumentFromQueryString( char** bufPtr, char* keyword, char* regExp )
  {
  if( bufPtr==NULL )
    {
    return -1;
    }

  char* query = getenv("QUERY_STRING");
  if( EMPTY( query ) )
    {
    return -2;
    }

  char* ptr = strstr( query, keyword );
  if( EMPTY( ptr ) )
    {
    return -3;
    }

  ptr += strlen( keyword );
  if( EMPTY( ptr ) )
    {
    return -4;
    }

  if( *ptr!='=' )
    {
    return -5;
    }
  ++ptr;

  char* p = NULL;
  for( p = ptr; *p!=0 && *p!='&' && *p!='#'; ++p )
    {
    }
  int length = (p-ptr);
  if( length<1 )
    {
    return -6;
    }

  *bufPtr = calloc( length+10, sizeof(char) );
  strncpy( *bufPtr, ptr, length );

  if( NOTEMPTY( regExp ) )
    {
    if( StringMatchesRegex( regExp, *bufPtr )==0 )
      {
      return 0;
      }
    else
      {
      FREE( *bufPtr );
      *bufPtr = NULL;
      return -7;
      }
   }

  return 0;
  }

#define GETAPACHEUSER\
  "/bin/bash -c 'cat /etc/apache2/* 2>/dev/null | grep APACHE_RUN_USER= | sed \"s/.*=//\"'"

char* GetWebUser()
  {
  char buf[BUFLEN];
  int err = ReadLineFromCommand( GETAPACHEUSER, buf, sizeof(buf)-1, 1, 5 );
  if( err )
    Error( "Failed to find apache2 username from cmd [%s] -- %d:%d:%s",
           GETAPACHEUSER, err, errno, strerror( errno ) );
  return strdup( strtok( buf, "\r\n" ) );
  }

#define GETAPACHEGROUP\
  "/bin/bash -c 'cat /etc/apache2/* 2>/dev/null | grep APACHE_RUN_GROUP= | sed \"s/.*=//\"'"

char* GetWebGroup()
  {
  char buf[BUFLEN];
  int err = ReadLineFromCommand( GETAPACHEGROUP, buf, sizeof(buf)-1, 1, 5 );
  if( err )
    Error( "Failed to find apache2 groupname from cmd [%s] -- %d:%d:%s",
           GETAPACHEGROUP, err, errno, strerror( errno ) );
  return strdup( strtok( buf, "\r\n" ) );
  }

char* RemoveURLEncoding( char* src, char* dst )
  {
  if( EMPTY( src ) )
    {
    return src;
    }

  char* sptr = NULL;
  char* dptr = dst;
  for( sptr=src; (*sptr)!=0; ++sptr )
    {
    if( (*sptr)=='%'
        && strchr( HEXDIGITS, *(sptr+1) )!=NULL
        && strchr( HEXDIGITS, *(sptr+2) )!=NULL )
      {
      int n = 0;
      char buf[3];
      buf[0] = *(sptr+1);
      buf[1] = *(sptr+2);
      buf[2] = 0;
      if( sscanf( buf, "%x", &n )==1 )
        {
        *dptr = n;
        ++sptr;
        ++sptr;
        }
      else
        {
        *dptr = *sptr;
        }
      }
    else
      {
      *dptr = *sptr;
      }
    ++dptr;
    }
  *dptr = 0;

  return dst;
  }

_TAG_VALUE* ParseQueryString( _TAG_VALUE* list, char* string )
  {
#ifdef DEBUG
  Notice("ParseQueryString( %p, %s )", list, string );
#endif

  /* no assignment */
  if( strstr( string, "=" )==NULL )
    return list;

  char* ptr = NULL;
  char* nextString = NULL;
  for( ptr=string; *ptr!=0; ++ptr )
    {
    if( *ptr=='&' )
      {
      *ptr = 0;
      nextString = ptr+1;
      break;
      }
    }

  char* value = NULL;
  char* tag = string;
  for( ptr=string; *ptr!=0; ++ptr )
    {
    if( *ptr=='=' )
      {
      *ptr = 0;
      value = ptr+1;
      break;
      }
    }

  if( tag!=NULL && value!=NULL )
    {
    if( strstr( value, "%" )!=NULL )
      { /* potentially remove URL encoding */
      char* buf = malloc( strlen(value)+2 );
      char* dst = RemoveURLEncoding( value, buf );
      if( dst!=NULL )
        {
        list = NewTagValue( tag, dst, list, 0 );
        }
      free( buf );
      }
    else
      { /* no URL encoding involved */
      list = NewTagValue( tag, value, list, 0 );
      }
    }

  if( nextString!=NULL )
    {
    return ParseQueryString( list, nextString );
    }
  else
    {
    return list;
    }
  }

int gotHeader = 0;
void CGIHeader( char* contentType,
                long contentLength,
                char* pageTitle,
                int nCSS, char** css,
                int nJS, char** js
                )
  {
  if( gotHeader )
    return;
  gotHeader = 1;

  inCGI = 1;

  if( NOTEMPTY( contentType ) )
    {
    printf( "Content-Type: %s\r\n", contentType );
    printedContentType = 1;

    if( contentLength>0 )
      {
      printf( "Content-Length: %ld\r\n", contentLength );
      }
    fputs( "\r\n", stdout );

    /* Not HTML.  The stuff below is HTML specific. */
    return;
    }

  fputs( "Content-Type: text/html; Charset=US-ASCII\r\n\r\n", stdout );
  printedContentType = 1;

  fputs( "<!doctype html>\n", stdout);
  fputs( "<html lang=\"en\">\n", stdout);
  fputs( "  <head>\n", stdout);
  if( NOTEMPTY( pageTitle ) )
    {
    printf( "    <title>%s</title>\n", pageTitle );
    }

  if( css!=NULL && nCSS>0 && nCSS<100 )
    {
    for( int i=0; i<nCSS; i++ )
      {
      char* styleSheet = css[i];
      if( NOTEMPTY( styleSheet ) )
        {
        printf( "    <link rel=\"stylesheet\" href=\"%s\"/>\n", styleSheet );
        }
      }
    }

  if( js!=NULL && nJS>0 && nJS<100 )
    {
    for( int i=0; i<nJS; i++ )
      {
      char* jsFile = js[i];
      if( NOTEMPTY( jsFile ) )
        {
        printf( "    <script src=\"%s\"></script>\n", jsFile );
        }
      }
    }

  fputs( "  </head>\n", stdout);
  fputs( "  <body>\n", stdout);
  }

void CGIFooter()
  {
  fputs( "  </body>\n", stdout);
  fputs( "</html>\n", stdout);
  }

/* ParseHeaderSubVariables()
 * Parses a string which represents header "sub-variables" in a CGI POST, as shown
 * in the example above, and populate _TAG_VALUE structs with what we found.
 * You pass in a buffer which contains the already-read string to process.
 * You also pass in a pointer to a list of already known sub-variables and
 * the function will return an expanded linked list (returns a new head).
 */
_TAG_VALUE* ParseHeaderSubVariables( _TAG_VALUE* list, char* buf )
  {
  char* firstEquals = NULL;
  char* firstSemi = NULL;
  char* ptr;
  _TAG_VALUE* myList = list;

  if( *buf==0 || *buf==LF || *buf==CR )
    {
    return list;
    }

  for( ptr=buf; *ptr!=0; ++ptr )
    {
    int c = *ptr;
    if( firstEquals==NULL && c=='=' )
      {
      firstEquals = ptr;
      }
    if( firstSemi==NULL && (c==';'||c==CR) )
      {
      firstSemi = ptr;
      }
    if( firstEquals!=NULL && firstSemi!=NULL )
      {
      break;
      }
    }

  if( firstEquals==NULL || firstSemi==NULL || (firstEquals+1)>=firstSemi
      || ( (*(firstEquals+1))!='"' && !isalpha(*(firstEquals+1)) ) )
    {
    Warning("HTTP header extra data should look like name=\"value\"; but is [%s]", buf );
    return list;
    }

  *firstEquals = 0;
  *firstSemi = 0;

  myList = NewTagValue( buf+1, StripQuotes(firstEquals+1), list, 0 );

  /* recurse as we may have more sub-variables in the input buffer */
  if( *(firstSemi+1)!=0 )
    {
    myList = ParseHeaderSubVariables( myList, firstSemi+1 );
    }

  return myList;
  }

/* examples of headers we have to parse:
 * Content-Disposition: form-data; name="filename"; filename="auto-cert.txt"
 * Content-Type: text/plain
 */

/* ParseHeaderLine()
 * Parses a string which represents a CGI POST header line such as above.
 * Populate _TAG_VALUE structs with what we found.
 * You pass in a buffer which contains the already-read string to process.
 * You also pass in a pointer to a list of already known variables and
 * the function will return an expanded linked list (returns a new head).
 */
_TAG_VALUE* ParseHeaderLine( _TAG_VALUE* workingHeaders, char* buf )
  {
  char* firstColon = NULL;
  char* firstSemi = NULL;
  char* ptr;

  for( ptr=buf; *ptr!=0; ++ptr )
    {
    int c = *ptr;
    if( firstColon==NULL && c==':' )
      {
      firstColon = ptr;
      }
    if( firstSemi==NULL && (c==';'||c==CR) )
      {
      firstSemi = ptr;
      }
    if( firstColon!=NULL && firstSemi!=NULL )
      {
      break;
      }
    }

  if( firstColon==NULL || firstSemi==NULL || (firstColon+2)>=firstSemi )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("HTTP header should have at least [a: b;] but is only [%s]", buf );
    }

  *firstColon = 0;
  *firstSemi = 0;
  workingHeaders = NewTagValue( buf, firstColon+2, workingHeaders, 0 );
  if( *(firstSemi+1)!=0 )
    { /* there is stuff after the semicolon or CR */
    workingHeaders->subHeaders = ParseHeaderSubVariables( NULL, firstSemi+1 );
    }

  return workingHeaders;
  }

/* HeadersContainTag()
 * return non-NULL if a linked list of tag-value pairs contains a given tag
 */
_TAG_VALUE* HeadersContainTag( _TAG_VALUE* list, char* tag )
  {
  while( list!=NULL )
    {
    if( list->tag!=NULL && strcmp( list->tag, tag )==0 )
      {
      return list;
      }
    list=list->next;
    }

  return NULL;
  }

/* return 0 if list contains a tag and sub-tag */
int HeadersContainTagAndSubTag( _TAG_VALUE* list, char* tag, char* subTag )
  {
  _TAG_VALUE* parent = NULL;
  parent = HeadersContainTag( list, tag );
  if( parent!=NULL )
    {
    _TAG_VALUE* child = NULL;
    child = HeadersContainTag( parent->subHeaders, subTag );
    if( child!=NULL )
      {
      return 0;
      }
    }

  return -1;
  }

/* ParseValue()
 * Extract any trailing CR chars and stick a value=[%s] entry in a
 * linked list.  Used to process the value line of an HTTP form variable.
 * Note that old linked list head is passed in, new head is returned.
 */
_TAG_VALUE* ParseValue( char* buf, _TAG_VALUE* workingHeaders )
  {
  for( char *ptr=buf; *ptr!=0; ++ptr )
    {
    if( *ptr == CR )
      {
      *ptr = 0;
      break;
      }
    }

  workingHeaders = NewTagValue( "value", buf, workingHeaders, 0 );

  return workingHeaders;
  }

/* size is for http header; path is full path to the file (not
   just the folder where the file exists);
   filename is what the client should name the file */
void DownloadFile( long filesize, char* path, char* fileName )
  {
  FILE* f = fopen( path, "r" );
  if( f==NULL )
    {
    CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
    Error("Cannot open [%s] for download", NULLPROTECT( path ) );
    }

  printf( "Content-Type: application/octet-stream\r\n");
  printedContentType = 1;
  printf( "Content-Disposition: attachment; filename=\"%s\"\r\n",
          fileName==NULL ? "no-name-file" : fileName );
  printf( "Content-Length: %ld\r\n", filesize );
  printf( "X-Pad: avoid browser bug\r\n");
  printf( "\r\n" );

  char buf[BUFLEN];
  int n = 0;
  unsigned long total = 0;
  while( (n=fread( buf, sizeof(char), sizeof(buf), f ))>0 )
    {
    int m = fwrite( buf, sizeof(char), n, stdout );
    if( m!=n )
      {
      Warning("Failed to write file @ %'lu", total);
      break;
      }
    total += m;
    }
  }

void DownloadChunkedStream( int fd, char* fileName )
  {
  printf( "Content-Type: application/octet-stream\r\n");
  printedContentType = 1;
  printf( "Content-Disposition: attachment; filename=\"%s\"\r\n",
          fileName==NULL ? "no-name-file" : fileName );

  printf( "Transfer-Encoding: chunked\r\n" );
  printf( "X-Pad: avoid browser bug\r\n");
  printf( "\r\n" );

  char buf[BUFLEN];
  size_t n = 0;
  unsigned long total = 0;
  while( (n=read( fd, buf, sizeof(buf)-1))>0 )
    {
    printf( "%lx\r\n", (long)n );
    int m = fwrite( buf, sizeof(char), n, stdout );
    if( m!=n )
      {
      Warning("Failed to write file @ %'lu", total);
      break;
      }
    (void)fwrite( "\r\n", sizeof(char), 2, stdout );
    total += m;
    }
  (void)fwrite( "0\r\n\r\n", sizeof(char), 5, stdout );
  }

char* ExtractUserIDOrDie( enum callMethod cm, char* envVarName )
  {
  char* userVar = DEFAULT_USER_ENV_VAR;
  if( NOTEMPTY( envVarName ) )
    userVar = envVarName;

  if( EMPTY( userVar ) )
    {
    if( cm==cm_ui )
      {
      printf("Content-Type: text/html\r\n\r\n");
      printedContentType = 1;
      printf( "<html><body><b>Configuration problem: what variable carries the user ID?</b></body></html>\n" );
      exit(0);
      }
    else
      APIError( "API basics", -1, "Configuration problem: what variable carries the user ID?" );
    }

  char* userName = getenv( userVar );
  if( EMPTY( userName ) )
    {
    if( cm==cm_ui )
      {
      printf("Content-Type: text/html\r\n\r\n");
      printedContentType = 1;
      printf( "<html><body><b>Cannot discern user name from variable %s</b></body></html>\n", userVar );
      exit(0);
      }
    else
      {
      APIError( "API basics", -2, "Cannot discern user name from HTTP variable (%s)", userVar );
      }
    }

  return userName;
  }

int StringMatchesUserIDFormat( char* userID )
  {
  if( EMPTY( userID ) )
    return -1;

  /* more restrictive than .htpasswd actually requires -- better safe than sorry */
  char* ptr = NULL;
  for( ptr = userID; *ptr!=0; ++ptr )
    {
    int c = *ptr;
    if( isalnum(c) )
      continue;
    if( c=='_' || c=='-' || c==' ' || c=='/' )
      continue;
    return -2;
    }

  if( ptr-userID >= 250 )
    return -3;

  return 0;
  }

#ifdef DEBUG
/* myfgets()
 * wrapper to fgets used for debugging.  do not use in production code.
 */
char* myfgets( char* buf, int buflen, FILE* f )
  {
  char* s = fgets( buf, buflen, f );
  if( s==NULL )
    {
    /* Notice("fgets returned NULL"); */
    }
  else
    {
    if( debugOutput!=NULL )
      {
      fputs( buf, debugOutput );
      fflush( debugOutput );
      }
    }

  return s;
  }

/* myfgetc()
 * wrapper to fgetc used for debugging.  do not use in production code.
 */
int myfgetc( FILE* f )
  {
  int c = fgetc( f );
  if( debugOutput!=NULL )
    {
    fputc( c, debugOutput );
    fflush( debugOutput );
    }

  return c;
  }

size_t myfread( void *ptr, size_t size, size_t nmemb, FILE *stream)
  {
  size_t n = fread( ptr, size, nmemb, stream );
  if( n>0 && debugOutput!=NULL )
    {
    (void)fwrite( ptr, size, n, debugOutput );
    fflush( debugOutput );
    }
  /* LogMessage("fread() --> %d items", n); */

  return n;
  }

#else
  #define myfgets fgets
  #define myfgetc fgetc
  #define myfread fread
#endif

/* CopySingleVariable()
 * Move a _TAG_VALUE list, which represents the current CGI header variable
 * (which could be multiple lines, with sub-arguments), into the headers
 * list of a _CGI_HEADER struct.
 * Recommended to set the pointer to NULL after calling this.
 */
void CopySingleVariable( _CGI_HEADER* header, _TAG_VALUE* workingHeaders )
  {
  if( workingHeaders==NULL )
    return;

  char* tag = NULL;
  char* value = NULL;

  for( _TAG_VALUE* ptr=workingHeaders; ptr!=NULL; ptr=ptr->next )
    {
    if( ptr->tag!=NULL && *ptr->tag!=0 && strcmp(ptr->tag,"value")==0
        && ptr->value!=NULL && *ptr->value!=0 )
      {
      value = ptr->value;
      }
    }

  for( _TAG_VALUE* ptr=workingHeaders; ptr!=NULL; ptr=ptr->next )
    {
    if( ptr->tag!=NULL && *ptr->tag!=0 && strcmp(ptr->tag,"Content-Disposition")==0
        && ptr->value!=NULL && *ptr->value!=0 && strcmp(ptr->value,"form-data")==0 )
      {
      for( _TAG_VALUE* sptr=ptr->subHeaders; sptr!=NULL; sptr=sptr->next )
        {
        if( sptr->tag!=NULL && *sptr->tag!=0 && strcmp(sptr->tag,"name")==0
            && sptr->value!=NULL && *sptr->value!=0 )
          {
          tag = sptr->value;
          }
        }
      }
    }

  if( tag!=NULL && value!=NULL )
    {
    header->headers = NewTagValue( tag, value, header->headers, 0 );
    }
  }

/* ParsePostData()
 * Read stdin from a CGI POST and (a) populate a data structure with
 * form variables.  What we read is dropped into a _CGI_HEADER
 * struct whose address is passed in as an argument.
 * Don't forget to free the linked lists in there when done.
 */
int ParsePostData( FILE* stream,
                   _CGI_HEADER *header,
                   int (*funcPtr)( _CGI_HEADER * ) )
  {
#ifdef DEBUG
  debugOutput = logFileHandle;
  if( debugOutput!=NULL )
    {
    fprintf( debugOutput, "----start----\n");
    fflush( debugOutput );
    }
#endif

  enum postState state = ps_FIRSTHEADER;

  header->separatorString = NULL;
  header->files = NULL;
  header->headers = NULL;

  _TAG_VALUE* workingHeaders = NULL;

  while( !feof( stream ) )
    {
    /* shared among different states */
    char buf[BUFLEN];

    if( state==ps_FIRSTHEADER )
      {
      /* LogMessage("ps_FIRSTHEADER"); */
      if( myfgets(buf, BUFLEN, stream)!=buf )
        {
        /* this is normal if nothing was posted.. */
        /* Warning("Failed to load line of text from CGI stream (1)\n"); */
        return -1;
        }
      header->separatorString = strdup( StripEOL( buf ));

      if( *(header->separatorString) != '='
          && *(header->separatorString) != '-'
          && strstr( header->separatorString, "=" ) != NULL )
        {
#ifdef DEBUG
        Notice( "Perhaps we have var=value assignment in [%s]?",
                header->separatorString );
        Notice( "header->headers is initially %p", header->headers  );
#endif
        header->headers = ParseQueryString( header->headers , header->separatorString );
#ifdef DEBUG
        Notice( "header->headers is now %p", header->headers  );
#endif
        }
      else
        {
        state=ps_HEADERLINE;
        }
      }

    else if( state==ps_HEADERLINE )
      {
      /* LogMessage("ps_HEADERLINE"); */
      if( myfgets(buf, BUFLEN, stream)!=buf )
        {
        if( ! feof( stream ) )
          {
          Warning("Failed to load line of text from CGI stream (2)\n");
          }
        break;
        }

      if( buf[0]==CR && buf[1]==LF && buf[2]==0 )
        {
        /* next line is contents or start of data */
        if( HeadersContainTagAndSubTag( workingHeaders, "Content-Disposition", "filename" ) )
          { /* data stream */
          state = ps_DATA;
          }
        else
          { /* variable value */
          state = ps_VARIABLE;
          }
        }
      else
        {
        if( buf[0]>='A' && buf[0]<='Z' )
          {
          workingHeaders = ParseHeaderLine( workingHeaders, buf );
          }
        else
          {
          if( buf[0]=='-' && buf[1]=='-' && buf[2]=='\r' && buf[3]=='\n' )
            {
            /* end of MIME segments */
            break;
            }
          else
            {
            CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
            Error("Header line should start with [A-Z]: [%s]", buf );
            }
          }
        }
      }

    else if( state==ps_VARIABLE )
      {
      /* LogMessage("ps_VARIABLE"); */
      if( myfgets(buf, BUFLEN, stream)!=buf )
        {
        CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
        Error("Failed to load line of text from CGI stream (2)\n");
        }

      workingHeaders = ParseValue( buf, workingHeaders );
      /* printf("WorkingHeaders:\n"); */
      /* PrintTagValue("", workingHeaders); */
      CopySingleVariable( header, workingHeaders );
      FreeTagValue( workingHeaders );
      workingHeaders = NULL;

      state = ps_SEPARATOR;
      }

    else if( state==ps_DATA )
      {
      CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
      Error("Uploading files not supported by this CGI.");
      }

    else if( state==ps_SEPARATOR )
      {
      /* LogMessage("ps_SEPARATOR"); */

      if( EMPTY( header->separatorString ) )
        {
        CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
        Error("Parse error: no defined separator");
        }

      if( myfgets(buf, BUFLEN, stream)!=buf )
        {
        CGIHeader( NULL, 0, NULL, 0, NULL, 0, NULL);
        Error("Failed to load line of text from CGI stream (5)\n");
        }

      if( strncmp( buf, header->separatorString, strlen(header->separatorString ) )==0 )
        {
        FreeTagValue( workingHeaders );
        workingHeaders = NULL;
        state=ps_HEADERLINE;
        }
      else
        {
        /* perhaps we have a multi-line form input (<textarea>)? */
        if( header!=NULL && header->headers!=NULL && header->headers->value!=NULL )
          {
          /*
          Notice( "Appending it to: [%s] (old value = [%s])",
                  NULLPROTECT( header->headers->tag ),
                  NULLPROTECT( header->headers->value ) );
          */
          char** v = &(header->headers->value);

          int l = strlen( *v ) + strlen( buf ) + 5;
          char* tmp = calloc( l, sizeof( char ) );
          strcpy( tmp, *v );
          if( strchr( tmp, '\r' )==0 )
            {
            strcat( tmp, "\n" );
            }
          strcat( tmp, buf );
          free( *v );
          *v = tmp;
          }
        else
          { /* or not? */
          Warning("Parse problem: expected separator but got [%s]", buf );
          workingHeaders = AppendValue( buf, workingHeaders );
          }
        }
      }
    }

#ifdef DEBUG
  Notice("Broke out of ParsePost - separatorString=%s", NULLPROTECT( header->separatorString) );
#endif

  return 0;
  }

void ClearCookie( char* cookie )
  { 
  printf( "Set-Cookie: %s=; Max-Age=-1\n", cookie );
  }

char* GetCookieFromEnvironment( char* cookie )
  {
  if( EMPTY( cookie ) )
    {
    Warning( "Cannot GetCookieFromEnvironment() without specifying a cookie" );
    return NULL;
    }

  int l = strlen( HTTP_COOKIE_PREFIX );
  char* cookies = NULL;
  for( int i=0; environ[i]!=NULL; ++i )
    {
    char* env = environ[i];
    if( *env!=0 && strncmp( env, HTTP_COOKIE_PREFIX, l )==0 && env[l]=='=' )
      {
      cookies = env + l + 1;
      break;
      }
    }

  if( cookies==NULL )
    {
    /* no HTTP cookies were provided by the web server */
    return NULL;
    }

  char* ptr = NULL;
  l = strlen( cookie );
  char* myCookies = strdup( cookies );
  for( char* token = strtok_r( myCookies, ";", &ptr ); token!=NULL; token = strtok_r( NULL, ";", &ptr ) )
    {
    while( (*token)==' ' )
      ++token;

    if( strncasecmp( token, cookie, l )==0 && *(token+l)=='=' )
      {
      char* thisCookie = strdup(token + l + 1); 
      free( myCookies );
      return thisCookie;
      }
    }
  free( myCookies );

  return NULL;
  }

extern char* sha1hexdigits;

char* URLEncode( char* raw )
  {
  if( EMPTY( raw ) )
    return raw;

  int l = strlen( raw );
  char* encoded = (char*)SafeCalloc( l*3+1, sizeof(char), "URL encoded string" );
  char* dst = encoded;

  for( char* src = raw; *src!=0; ++src )
    {
    int c = *src;
    if( c == ' ' )
      *(dst++) = '+';
    else if( isalnum( c ) )
      *(dst++) = c;
    else
      {
      *(dst++) = '%';
      int h = (c & 0xf0) >> 4;
      int l = (c & 0x0f);
      *(dst++) = sha1hexdigits[h];
      *(dst++) = sha1hexdigits[l];
      }
    }
  *(dst) = 0;

  return encoded;
  }

char* URLDecode( char* encoded )
  {
  if( EMPTY( encoded ) )
    return encoded;

  int l = strlen( encoded );
  char* decoded = (char*)SafeCalloc( l, sizeof(char), "URL decoded string" );
  char* src = encoded;
  char* dst = decoded;
  for( ; *src!=0; ++src )
    {
    if( *src=='+' )
      {
      *(dst++) = ' ';
      }
    else if( *src=='%'
             && isxdigit( *(src+1) )
             && isxdigit( *(src+2) ) )
      {
      int ch = *(++src);
      int cl = *(++src);
      int nh = HexDigitNumber( ch );
      int nl = HexDigitNumber( cl );
      int byte = nh << 4 | nl;
      *(dst++) = byte;
      }
    else
      *(dst++) = *src;
    }

  *dst = 0;
  return decoded;
  }

int IsURLEncoded( char* string )
  {
  if( EMPTY( string ) )
    return -1;

  int nEncodedChars = 0;
  for( char* ptr=string; *ptr!=0; ++ptr )
    {
    if( *ptr=='%' )
      {
      if( isxdigit( *(ptr+1) ) && isxdigit( *(ptr+2) ) )
        {
        ++nEncodedChars;
        ptr += 2;
        }
      else
        return -2;
      }
    else if( *ptr=='+' )
      ++nEncodedChars;
    }

  if( nEncodedChars )
    return 0;

  return -3;
  }

char* MyRelativeRequestURL( char* reqVarName )
  {
  char* reqVar = EMPTY( reqVarName ) ? DEFAULT_REQUEST_URI_ENV_VAR : reqVarName;

  char* uri = getenv( reqVar );
  if( EMPTY( uri ) )
    Error( "Failed to get request URI from env var %s" , reqVar );

  char url[BUFLEN];
  snprintf( url, sizeof(url)-1, "%s%s",
            uri[0]=='/' ? "" : "/", uri );

  return strdup( url );
  }

char* FullRequestURL( char* hostVarName, char* reqVarName )
  {
  char* hostVar = EMPTY( hostVarName ) ? DEFAULT_HTTP_HOST_ENV_VAR : hostVarName;
  char* reqVar = EMPTY( reqVarName ) ? DEFAULT_REQUEST_URI_ENV_VAR : reqVarName;

  char* host = getenv( hostVar );
  if( EMPTY( host ) )
    Error( "Failed to get HTTP hostname from env var %s" , hostVar );

  char* uri = getenv( reqVar );
  if( EMPTY( uri ) )
    Error( "Failed to get request URI from env var %s" , reqVar );

  char url[BUFLEN];
  snprintf( url, sizeof(url)-1, "https://%s%s%s",
            host, uri[0]=='/' ? "" : "/", uri );

  return strdup( url );
  }

void RedirectToUrl( char* url, char* cssPath )
  {
  if( EMPTY( url ) )
    Error( "Cannot redirect to NULL URL" );

  int decoded = 0;
  char* passUrl = url;

  char* question = strchr( url, '?' );
  if( question!=NULL )
    { /* don't decode the bit after the ? - it could be a url-encoded argument */
    *question = 0;
    if( IsURLEncoded( url )==0 )
      {
      char* firstBit = URLDecode( url );
      char bigURL[BUFLEN];
      snprintf( bigURL, sizeof(bigURL)-1, "%s?%s", firstBit, question+1 );
      free( firstBit );
      passUrl = strdup( bigURL );
      decoded = 1;
      }
    /* put the question mark back */
    *question = '?';
    }
  else
    { /* simple URL - decode the whole thing if necessary */
    if( IsURLEncoded( url )==0 )
      {
      passUrl = URLDecode( url );
      decoded = 1;
      }
    }

  printf( "Status: 302\n" );
  printf( "Location: %s\n", passUrl );
  printf( "Content-Type: text/html\r\n\r\n" );
  printedContentType = 1;
  printf( "<html>\n" );
  printf( "  <head>\n" );
  printf( "    <title>Redirect</title>\n" );
  if( EMPTY( cssPath ) )
    printf( "    <link rel=\"stylesheet\" href=\"/auth2cookie/ui.css\"/>\n" );
  else
    printf( "    <link rel=\"stylesheet\" href=\"%s\"/>\n", cssPath );
  printf( "  </head>\n" );
  printf( "  <body>\n" );
  printf( "    <h1>Redirect</h1>\n" );
  printf( "    <p><a href=\"%s\">Click if not automatically redirected.</a></p>\n", url );
  printf( "  </body>\n" );
  printf( "</html>\n" );
  printf( "\n" );

  if( decoded )
    free( passUrl );
  }

/* http://shoham.ca */
int SplitUrlIntoParts( char* url, char** protocol, char** host, char** path )
  {
  if( EMPTY( url ) || protocol==NULL || host==NULL || path==NULL )
    {
    Warning( "SplitUrl() - bad args" );
    return -1;
    }

  char* copy = strdup( url );
  char* ptr = copy;

  char* separator = strstr( ptr, "://" );
  if( separator==NULL )
    { /* we assume that things like shoham.ca/somefile.html which might be interpreted as
         hostname/path in a strict sense are the file somefile.html in folder shoham.ca */
    *protocol = 0;
    *host = 0;
    *path = strdup( ptr );
    }
  else
    {
    *separator = 0;
    *protocol = strdup( ptr );
    *separator = ':';
    ptr = separator + 3;

    char* slash = strchr( ptr, '/' );
    if( slash==NULL || slash==ptr )
      {
      *host = strdup( ptr );
      *path = 0;
      }
    else
      {
      *slash = 0;
      *host = strdup( ptr );
      *slash = '/';
      *path = strdup( slash );
      }
    }

  free( copy );
  return 0;
  }

int CompareTwoUrls( char* pattern, char* example )
  {
  if( ( EMPTY( pattern ) || strcmp( pattern, "/" )==0) 
      && ( EMPTY( example ) || strcmp( example, "/")==0 ) )
    return 0;  /* two nothings match */

  if( EMPTY( pattern ) || EMPTY( example ) )
    return -1; /* nothing does not match something */

  char* protocolA = NULL;
  char* hostA = NULL;
  char* pathA = NULL;

  char* protocolB = NULL;
  char* hostB = NULL;
  char* pathB = NULL;

  int errA = SplitUrlIntoParts( pattern, &protocolA, &hostA, &pathA );
  if( errA )
    return -100 + errA;

  int errB = SplitUrlIntoParts( example, &protocolB, &hostB, &pathB );
  if( errB )
    return -200 + errB;

  /*
  printf( "Pattern %s --> %s :: %s :: %s\n", NULLPROTECT( pattern ), NULLPROTECT( protocolA ), NULLPROTECT( hostA ), NULLPROTECT( pathA ) );
  printf( "Example %s --> %s :: %s :: %s\n", NULLPROTECT( example ), NULLPROTECT( protocolB ), NULLPROTECT( hostB ), NULLPROTECT( pathB ) );
  */

  if( NOTEMPTY( protocolA ) && NOTEMPTY( protocolB ) )
    { /* if one is empty and the other isn't - probably fine */
    int compareProtocol = CompareStringToPattern( protocolA, protocolB, 0 );
    if( compareProtocol )
      return compareProtocol;
    }

  if( NOTEMPTY( hostA ) )
    {
    int compareHost = CompareStringToPattern( hostA, hostB, 0 );
    if( compareHost )
      return compareHost;
    } /* if hostA is empty, we don't care if hostB isn't */

  if( NOTEMPTY( pathA ) )
    {
    if( strcmp( pathA, "/" )==0 && EMPTY( pathB ) ) /* special case */
      return 0;
    int comparePath = CompareStringToPattern( pathA, pathB, 0 );
    if( comparePath )
      {
      return comparePath;
      }
    }

  return 0;
  }

