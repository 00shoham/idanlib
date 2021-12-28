#include "utils.h"

char* MakeFullPath( const char* folder, const char* file )
  {
  if( EMPTY( file ) )
    return NULL;

  if( EMPTY( folder ) )
    return strdup( file );

  int lFolder = strlen( folder );
  int lFile = strlen( file );

  int addSlash = 0;
  if( folder[lFolder-1]!='/' )
    addSlash = 1;

  int totalLength = lFolder + lFile + addSlash;
  char* buf = (char*)malloc( totalLength+1 );
  char* ptr = buf;
  memcpy( ptr, folder, lFolder );
  ptr += lFolder;
  if( addSlash )
    *(ptr++) = '/';
  memcpy( ptr, file, lFile );
  ptr += lFile;
  *ptr = 0;

  return buf;
  }

/* Check if a file exists and can be opened.
 * Return file size if true; 0 otherwise.
 */
int FileExists( const char* path )
  {
  FILE* f = fopen( path, "r" );
  if( f==NULL )
    {
    return -1;
    }
  fclose( f );

  return 0;
  }

int FileExists2( const char* folder, const char* fileName )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  int err = FileExists( fullPath );
  FREEIFNOTNULL( fullPath );
  return err;
  }

int FileUnlink2( const char* folder, const char* fileName )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  int err = unlink( fullPath );
  FREEIFNOTNULL( fullPath );
  return err;
  }

int FileRename2( const char* folder, const char* oldName, const char* newName )
  {
  char* oldPath = MakeFullPath( folder, oldName );
  char* newPath = MakeFullPath( folder, newName );
  int err = rename( oldName, newName );
  FREEIFNOTNULL( newPath );
  FREEIFNOTNULL( oldPath );
  return err;
  }

time_t FileDate( const char* path )
  {
  if( FileExists( path )!=0 )
    {
    return -1;
    }

  struct stat sbuf;
  if( stat( path, &sbuf )<0 )
    {
    return -2;
    }

  return sbuf.st_ctime;
  }

time_t FileDate2( const char* folder, const char* fileName )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  time_t t = FileDate( fullPath );
  FREEIFNOTNULL( fullPath );
  return t;
  }

long FileSize( const char* path )
  {
  if( FileExists( path )!=0 )
    {
    return -1;
    }

  struct stat sbuf;
  if( stat( path, &sbuf )<0 )
    {
    return -2;
    }

  return sbuf.st_size;
  }

long FileSize2( const char* folder, const char* fileName )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  long l = FileSize( fullPath );
  FREEIFNOTNULL( fullPath );
  return l;
  }

/* Check if a directory exists and can be opened.
 * Return 1 if true; 0 otherwise.
 */
int DirExists( const char* path )
  {
  DIR* d = opendir( path );
  if( d==NULL )
    {
    return -1;
    }
  closedir( d );
  return 0;
  }

int DirExists2( const char* folder, const char* fileName )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  int err = DirExists( fullPath );
  FREEIFNOTNULL( fullPath );
  return err;
  }

/* path is optional - assume current dir */
int GetOrderedDirectoryEntries( const char* path,
                                const char* prefix,
                                const char* suffix,
                                char*** entriesP,
                                int sort )
  {
  if( EMPTY( path ) )
    path = ".";

  if( entriesP==NULL )
    Error("Must specify a char*** arg entriesP to GetOrderedDirectoryEntries");

  DIR* d = opendir( path );
  if( d==NULL )
    {
    return -1;
    }

  int nEntries = 0;
  struct dirent * de;
  while( (de=readdir( d ))!=NULL )
    {
    if( NOTEMPTY( de->d_name )
        && ( suffix==NULL || StringEndsWith( de->d_name, suffix, 0 )==0 )
        && ( prefix==NULL || StringStartsWith( de->d_name, prefix, 0 )==0 ) )
      {
      ++nEntries;
      }
    }

  if( nEntries==0 )
    {
    *entriesP = NULL;
    closedir( d );
    return 0;
    }

  char** entries = NULL;
  entries = (char**)calloc( nEntries+1, sizeof( char* ) );

  rewinddir( d );
  int n = 0;
  while( (de=readdir( d ))!=NULL && n<nEntries )
    {
    if( NOTEMPTY( de->d_name )
        && ( suffix==NULL || StringEndsWith( de->d_name, suffix, 0 )==0 )
        && ( prefix==NULL || StringStartsWith( de->d_name, prefix, 0 )==0 ) )
      {
      entries[ n ] = strdup( de->d_name );
      ++n;
      }
    }
  entries[n] = NULL;

  closedir( d );

  /* race condition - n could have changed since previous list */
  nEntries = n;
  if( sort )
    {
    qsort( entries, nEntries, sizeof( char* ), CompareStrings );
    }

  *entriesP = entries;
  return nEntries;
  }

int GetOrderedDirectoryEntries2( const char* parentFolder,
                                 const char* childFolder,
                                 const char* prefix,
                                 const char* suffix,
                                 char*** entriesP,
                                 int sort )
  {
  char* fullPath = MakeFullPath( parentFolder, childFolder );
  int err = GetOrderedDirectoryEntries( fullPath,
                                        prefix, suffix,
                                        entriesP, sort );
  FREEIFNOTNULL( fullPath );
  return err;
  }

/* Extract the path name, without the filename, from a full path.
 * Return NULL on failure conditions.
 * Note that the path argument is not modified.
 */
char* GetFolderFromPath( char* path, char* folder, int folderSize )
  {
  char* src = NULL;
  char* dst = NULL;
  char* lastSlash = NULL;

  if( EMPTY( path ) ) Error( "Cannot extract folder from empty path" );
  if( folder == NULL ) Error( "Cannot write folder to a NULL buffer" );
  dst = folder;
  *dst = 0;

  for( src=path, dst=folder; *src!=0 && (dst-folder)<folderSize-1; ++src, ++dst )
    {
    *dst = *src;
    if( *dst == '/' )
      {
      lastSlash = dst;
      }
    }
  *dst = 0;
  if( lastSlash!=NULL )
    {
    *lastSlash = 0;
    }
  return folder;
  }

/* Extract the filename name, without the folder, from a full path.
 * Return NULL on failure conditions.
 */
char* GetFilenameFromPath( char* path )
  {
  if( EMPTY( path ) )
    {
    return NULL;
    }

  while( path!=NULL && *path!=0 )
    {
    char* ptr = strchr( path, '/' );
    if( ptr==NULL )
      {
      ptr = strchr( path, '\\' );
      }
    if( ptr==NULL )
      {
      return path;
      }
    else
      {
      path = ptr+1;
      }
    }

  return NULL;
  }

char* GetBaseFilenameFromPath( char* path, char* buf, int bufLen )
  {
  if( EMPTY( path ) )
    {
    return NULL;
    }

  if( buf==NULL || bufLen-1 < strlen(path) )
    {
    return NULL;
    }

  strncpy( buf, path, bufLen-1 );
  for( char* ptr=buf+strlen(buf)-1; ptr>=buf; ptr-- )
    {
    if( *ptr=='.' )
      {
      *ptr = 0;
      break;
      }
    }

  return buf;
  }

int IsFolderWritable( char* path )
  {
  if( EMPTY( path ) )
    {
    return -1; /* no path provided */
    }

  char pathWithFile[BUFLEN];
  char* ptr = pathWithFile;
  char* end = pathWithFile + sizeof( pathWithFile ) - 1;

  strncpy( ptr, path, end-ptr );
  ptr += strlen( pathWithFile );
  if( *(ptr-1) != '/' )
    {
    *ptr = '/';
    ++ptr;
    *ptr = 0;
    }

  if( end-ptr<10 )
    {
    return -2; /* not enough room in buffer */
    }

  GenerateIdentifier( ptr, 8 );

  FILE* f = fopen( ptr, "w" );
  if( f==NULL )
    {
    return -3; /* not writable! */
    }

  fclose( f );
  unlink( ptr );

  return 0;
  }

/* SanitizeFilename()
 * Strip unacceptable chars from an input filename, to create a filename
 * that is "safe."  Return an allocated string with the new filename.
 * Don't forget to free the returned filename.
 */
char* SanitizeFilename( const char* path, const char* prefix, const char* fileName, int removeSlash )
  {
  char buf[BUFLEN];
  const char* srcPtr;
  char* dstPtr;
  char* endPtr = buf+BUFLEN-2;

  strncpy( buf, path, sizeof(buf)-2 );
  dstPtr = buf + strlen(buf);

  if( dstPtr > buf )
    {
    if( *(dstPtr-1)!='/' )
      {
      *dstPtr = '/';
      ++dstPtr;
      *dstPtr = 0;
      }
    }

  if( NOTEMPTY( prefix ) )
    {
    strncpy( dstPtr, prefix, endPtr-dstPtr-2 );
    dstPtr += strlen( dstPtr );
    *(dstPtr++) = '-';
    *dstPtr = 0;
    }

  srcPtr = fileName;

  for( ; *srcPtr!=0 && dstPtr<=endPtr; ++srcPtr )
    {
    int c = *srcPtr;
    if( strchr( VALIDFILECHARS, c )!=NULL )
      {
      *dstPtr = c;
      ++dstPtr;
      *dstPtr = 0;
      }
    else
      {
      if( dstPtr>buf && *(dstPtr-1)==REPLACEMENTCHAR )
        {
        /* no need for sequences of replacement chars */
        }
      else
        {
        *dstPtr = REPLACEMENTCHAR;
        ++dstPtr;
        *dstPtr = 0;
        }
      }
    }

  /* strip any / chars */
  dstPtr = buf;
  if( removeSlash )
    {
    char* p;
    while( (p=strchr( dstPtr, '/' ))!=NULL )
      {
      dstPtr = p+1;
      }
    }

  return strdup( dstPtr );
  }

char* GetNameFromPath( char* path )
  {
  if( EMPTY( path ) )
    {
    return NULL;
    }

  for( char* ptr=path+strlen(path)-1; ptr>=path; ptr-- )
    {
    if( *ptr=='/' )
      {
      return ptr+1;
      }
    }

  return path;
  }

char** libraryExtensions = NULL;
int nExtensions = 0;

int FilterDirEnt( const struct dirent *de )
  {
  if( de==NULL )
    return 0;

  const char* fileName = de->d_name;
  if( EMPTY( fileName ) )
    {
    return 0;
    }
  else
    {
    for( int i=0; i<nExtensions; ++i )
      {
      char* ext = libraryExtensions[i];
      if( StringEndsWith( fileName, ext, 0 )==0 )
        {
        return 1;
        }
      }
    }

  return 0;
  }

void FreeDirentList( struct dirent **de, int n )
  {
  for( int i=0; i<n; ++i )
    {
    FREEIFNOTNULL( de[i] );
    }
  }

int GetAvailableSpaceOnVolume( char* path, char* buf, int bufLen )
  {
  if( EMPTY( path ) )
    {
    Warning("GetAvailableSpaceOnVolume must be given a path");
    return -1;
    }

  if( buf==NULL || bufLen<10 )
    {
    Warning("No buffer passed to GetAvailableSpaceOnVolume");
    return -1;
    }

  buf[0] = 0;

  char cmd[BUFLEN];
  int err = ExpandMacrosVA( DF_COMMAND,
                          cmd, sizeof(cmd)-1,
                          "PATH", path,
                          NULL, NULL );
  if( err!=1 )
    {
    Warning( "Should have seen one expansion in [%s] - got %d",
             DF_COMMAND, err );
    return -2;
    }

  FILE* dfH = popen( cmd, "r" );
  if( dfH==NULL )
    {
    Warning( "Failed to popen(%s)", cmd );
    return -3;
    }

  size_t n = fread( buf, sizeof(char), bufLen, dfH );
  fclose( dfH );
  if( n>0 && n<bufLen-1 )
    {
    buf[n] = 0;
    TrimTail( buf );
    return 0;
    }

  Warning( "Reading from DF command returned invalid result - %d", n );
  return -4;
  }

long GetAvailableSpaceOnVolumeBytes( char* path )
  {
  char buf[BUFLEN];
  int n = GetAvailableSpaceOnVolume( path, buf, sizeof( buf ) );
  if( n<0 )
    {
    return n;
    }

  double space = 0;
  if( sscanf( buf, "%lf", &space )==1 )
    {
    return (long)space * (long)1024;
    }
  else
    {
    Warning("DF results [%s] do not look like %%lfG", buf );
    return -10;
    }
  }

int FileCopy( const char* src, const char* dst )
  {
  if( EMPTY( src ) )
    {
    Warning("Cannot copy unspecified source file");
    return -1;
    }

  if( EMPTY( dst ) )
    {
    Warning("Cannot copy to unnamed destination file");
    return -2;
    }

  FILE* s = fopen( src, "r" );
  if( s==NULL )
    {
    Warning("Cannot read from [%s] for copy operation (%d:%s)", src, errno, strerror( errno )  );
    return -3;
    }

  FILE* d = fopen( dst, "w" );
  if( d==NULL )
    {
    Warning("Cannot write to [%s] for copy operation (%d:%s)", dst, errno, strerror( errno ) );
    fclose( s );
    return -4;
    }

  char data[BIGBUF];
  size_t n = 0;
  do
    {
    n = fread( data, sizeof(char), sizeof(data)-1, s );
    if( n>0 )
      {
      (void)fwrite( data, sizeof(char), n, d );
      }
    } while( n>0 );

  fclose( d );
  fclose( s );

  return 0;
  }

int FileCopy2( const char* srcFolder, const char* srcFile,
               const char* dstFolder, const char* dstFile )
  {
  char* srcPath = MakeFullPath( srcFolder, srcFile );
  char* dstPath = MakeFullPath( dstFolder, dstFile );
  int err = FileCopy( srcPath, dstPath );
  FREEIFNOTNULL( dstPath );
  FREEIFNOTNULL( srcPath );
  return err;
  }

long FileRead( const char* filename, unsigned char** data )
  {
  long size = FileSize( filename );

  if( size<0 )
    {
    return size;
    }

  if( data==NULL )
    {
    Warning("FileRead requires a target data pointer");
    return -3;
    }

  *data = (unsigned char*)malloc( size+1 );
  if( *data==NULL )
    {
    Warning("Cannot allocate %ld bytes in FileRead", size );
    return -4;
    }

  FILE* s = fopen( filename, "r" );
  if( s==NULL )
    {
    Warning("Cannot read from %s (%d:%s)", filename, errno, strerror( errno ) );
    return -5;
    }

  unsigned char* ptr = *data;
  long remaining = size;
  size_t n = 0;
  do
    {
    n = fread( ptr, sizeof(char), remaining, s );
    if( n>0 )
      {
      ptr += n;
      }
    } while( n>0 );

  fclose( s );

  if( ptr - (*data) != size )
    {
    Warning("Read %ld bytes but expected %ld bytes", ptr - (*data), size );
    return -6;
    }

  ptr = *data;
  *(ptr+size) = 0;

  return size;
  }

long FileRead2( const char* folder, const char* fileName,
                unsigned char** data )
  {
  char* fullPath = MakeFullPath( folder, fileName );
  long l = FileRead( fullPath, data );
  FREEIFNOTNULL( fullPath );
  return l;
  }

void EnsureDirExists( char* path )
  {
  if( EMPTY( path ) )
    {
    Error( "Cannot ensure that empty folder exists" );
    }

  if( DirExists( path )!=0 )
    {
    int err = mkdir( path, 0755 );
    if( err!=0 )
      {
      Error( "Failed to create folder [%s]", path );
      }
    }
  }

int CountFilesInFolder( char* folder, char* prefix, char* suffix,
                        time_t *earliest, time_t* latest )
  {
  int nEntries = 0;
  char* realFolder = NULL;

  if( EMPTY( folder ) )
    {
    realFolder = ".";
    }
  else
    {
    realFolder = folder;
    }

  DIR* d = opendir( realFolder );
  if( d==NULL )
    {
    Error( "Cannot open directory %s (%d:%s)", folder, errno, strerror( errno ) );
    }

  struct dirent * de;
  while( (de=readdir( d ))!=NULL )
    {
    if( NOTEMPTY( de->d_name )
        && ( suffix==NULL || StringEndsWith( de->d_name, suffix, 0 )==0 )
        && ( prefix==NULL || StringStartsWith( de->d_name, prefix, 0 )==0 ) )
      {
      ++nEntries;

      if( earliest!=NULL || latest!=NULL )
        {
        char fullPath[BUFLEN];
        char* realPath = NULL;
        if( folder==NULL )
          {
          realPath = de->d_name;
          }
        else
          {
          snprintf( fullPath, sizeof(fullPath)-1, "%s/%s", folder, de->d_name );
          realPath = fullPath;
          }

        struct stat sbuf;
        if( stat( realPath, &sbuf )==0 )
          {
          time_t t = sbuf.st_mtime;
          if( earliest!=NULL )
            {
            if( *earliest==0 || t<*earliest )
              {
              *earliest = t;
              }
            }
          if( latest!=NULL )
            {
            if( *latest==0 || t>*latest )
              {
              *latest = t;
              }
            }
          }
        }
      }
    }

  closedir( d );

  return nEntries;
  }

void GrabEndOfFile( FILE* input, char* output, int outputLen )
  {
  char* endPtr = output + outputLen - 1;
  char* ptr = output;
  char buf[BUFLEN];
  while( fgets( buf, sizeof(buf)-1, input )==buf )
    {
    strncpy( ptr, buf, endPtr-ptr );
    ptr += strlen( buf );
    *ptr = 0;
    }
  }

void TailFile( FILE* input, int nLines, char* output, int outputLen )
  {
  if( input==NULL || nLines<1 || output==NULL || outputLen<2 )
    Error( "Abuse of TailFile" );

  fseek( input, 0, SEEK_END );
  off_t pos = ftell( input );
  int gotLines = 0;
  int c = -1;

  while( pos>=0 )
    {
    if( pos==0 )
      {
      ++gotLines;
      break;
      }

    fseek( input, pos, SEEK_SET );
    c = fgetc( input );

    if( c=='\n' )
      ++gotLines;

    if( gotLines>nLines )
      break;

    --pos;
    }

  if( pos<=0 )
    {
    pos = 0;
    fseek( input, pos, SEEK_SET );
    }

  GrabEndOfFile( input, output, outputLen );
  }

int LockFile( char* fileName )
  {
  int fd = open( fileName, O_CREAT | O_WRONLY, S_IRWXU );

  struct flock fl;
  fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
  fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
  fl.l_start  = 0;        /* Offset from l_whence         */
  fl.l_len    = 1;        /* length, 0 = to EOF           */
  fl.l_pid    = getpid(); /* our PID                      */

  int err = fcntl(fd, F_SETLKW, &fl);  /* set the lock, waiting if necessary */

  if( err )
    {
    Warning( "Lockf error - %d / %d / %s", err, errno, strerror( errno ) );fflush(stdout);
    close( fd );
    return -1;
    }

  return fd;
  }

int UnLockFile( int fd )
  {
  if( fd<=0 )
    return -1;

  struct flock fl;
  fl.l_type   = F_UNLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start  = 0;
  fl.l_len    = 1;
  fl.l_pid    = getpid();

  int err1 = fcntl(fd, F_SETLKW, &fl);

  int err2 = close( fd );

  if( err1 )
    return err1;

  if( err2 )
    return err2;

  return 0;
  }

int Touch( char* path )
  {
  if( EMPTY( path ) )
    Error( "Cannot Touch(NULL)" );

  FILE* f = fopen( path, "a" );
  if( f!=NULL )
    {
    fclose( f );
    return 0;
    }
  else
    Warning( "Failed to fopen(%s,a)", path );

  return -1;
  }

int TouchEx( const char* folder, const char* fileName, const char* user, const char* group, mode_t mode )
  {
  if( EMPTY( folder )
      || EMPTY( fileName )
      || EMPTY( user )
      || EMPTY( group )
      || mode==0 )
    Error( "TouchEx(...NULL...)" );

  int err = 0;
  char* fullPath = MakeFullPath( folder, fileName );
  err = FileExists( fullPath );
  if( err!=0 )
    { /* create it */
    err = Touch( fullPath );
    if( err!=0 )
      { /* failed */
      return err;
      }
    }

  /* should exist now */
  uid_t userNum = GetUID( user );
  gid_t groupNum = GetGID( group );
  err = chown( fullPath, userNum, groupNum );
  if( err )
    {
    Warning( "chown( %s, %d, %d ) -> %d:%d:%s",
           fullPath, (int)userNum, (int)groupNum, err, errno, strerror( errno ) );
    return err;
    }

  err = chmod( fullPath, mode );
  if( err )
    {
    Warning( "chmod( %s, %d ) -> %d:%d:%s",
           fullPath, (int)mode, err, errno, strerror( errno ) );
    return err;
    }

  return 0;
  }

int RotateFile( char* path, int keepN )
  {
  int err = 0;

  /* possibly remove old rotated files first */
  char folderPart[BUFLEN];
  char* filePart;
  folderPart[0] = 0;
  (void)GetFolderFromPath( path, folderPart, sizeof(folderPart)-1 );
  filePart = GetFilenameFromPath( path );
  if( NOTEMPTY( folderPart )
      && NOTEMPTY( filePart ) )
    {
    char prefix[BUFLEN];
    snprintf( prefix, sizeof(prefix)-1, "%s-", filePart );
    char** fileNames = NULL;
    int nFiles = GetOrderedDirectoryEntries( folderPart, prefix, NULL, &fileNames, 1 );
    if( nFiles > keepN )
      {
      for( int i=0; i < (nFiles-keepN); ++i )
        {
        char toRemove[BUFLEN*2];
        snprintf( toRemove, sizeof(toRemove)-1, "%s/%s", folderPart, fileNames[i] );
        err = unlink( toRemove );
        if( err )
          Warning( "Tried to remove [%s] - got %d:%d:%s", err, errno, strerror( errno ) );
        }
      }
    if( nFiles )
      FreeArrayOfStrings( fileNames, nFiles );
    }

  char friendlyTime[BUFLEN];
  (void)DateTimeStr( friendlyTime, sizeof( friendlyTime )-1, 0, time(NULL) );
  int l = strlen( path ) + strlen( friendlyTime ) + 10;
  char* newName = (char*)SafeCalloc( l, sizeof(char), "path for file rotation" );
  strcpy( newName, path );
  strcat( newName, "-" );
  strcat( newName, friendlyTime );
  err = rename( path, newName );
  if( err )
    Warning( "Failed to rename [%s] to [%s] - %d:%d:%s",
             path, newName, err, errno, strerror( errno ) );
  FREE( newName );
  return err;
  }



