#ifndef _INCLUDE_FS_UTILS
#define _INCLUDE_FS_UTILS

#define VALIDFILECHARS "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_."
#define VALIDPATHCHARS "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_./"
#define REPLACEMENTCHAR '-'

#define DF_COMMAND "/bin/df %PATH% | /bin/grep -v '^Filesystem' | /usr/bin/awk '{print $4}'"

char* MakeFullPath( const char* folder, const char* file );
int FileExists( const char* path );
int FileExists2( const char* folder, const char* fileName );
int FileUnlink2( const char* folder, const char* fileName );
int FileRename2( const char* folder, const char* oldName, const char* newName );
long FileSize( const char* path );
long FileSize2( const char* folder, const char* fileName );
time_t FileDate( const char* path );
time_t FileDate2( const char* folder, const char* fileName );
int FileCopy( const char* src, const char* dst );
int FileCopy2( const char* srcFolder, const char* srcFile,
               const char* dstFolder, const char* dstFile );
long FileRead( const char* filename, unsigned char** data );
long FileRead2( const char* folder, const char* filename,
                unsigned char** data );
char* GetFolderFromPath( char* path, char* folder, int folderSize );
char* GetFilenameFromPath( char* path );
char* GetBaseFilenameFromPath( char* path, char* buf, int bufLen );
int DirExists( const char* path );
int DirExists2( const char* folder, const char* fileName );
int IsFolderWritable( char* path );
char* GetNameFromPath( char* path );
int FilterDirEnt( const struct dirent *de );
void FreeDirentList( struct dirent **de, int n );
int GetOrderedDirectoryEntries( const char* path,
                                const char* prefix,
                                const char* suffix,
                                char*** entriesP,
                                int sort );
int GetOrderedDirectoryEntries2( const char* parentFolder,
                                 const char* childFolder,
                                 const char* prefix,
                                 const char* suffix,
                                 char*** entriesP,
                                 int sort );
void EnsureDirExists( char* path );
void GrabEndOfFile( FILE* input, char* output, int outputLen );
void TailFile( FILE* input, int nLines, char* output, int outputLen );
int LockFile( char* fileName );
int UnLockFile( int fd );
int Touch( char* path );
int TouchEx( const char* folder, const char* fileName, const char* user, const char* group, mode_t mode );
int RotateFile( char* path, int keepN );
char* SanitizeFilename( const char* path, const char* prefix, const char* fileName, int removeSlash );
int GetAvailableSpaceOnVolume( char* path, char* buf, int bufLen );
long GetAvailableSpaceOnVolumeBytes( char* path );
int CountFilesInFolder( char* folder, char* prefix, char* suffix,
                        time_t *earliest, time_t* latest );
int FileMatchesRegex( char* expr, char* fileName );

#endif

