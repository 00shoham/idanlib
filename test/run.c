#include "utils.h"

#define WORKINGDIR "/data/security"
#define BACKUPCMD "/bin/tar cf -  garage-calgary/image-2023-03-19_20-49-56.jpg garage-calgary/image-2023-03-19_20-49-55.jpg garage-calgary/image-2023-03-19_20-49-54.jpg garage-calgary/image-2023-03-19_20-49-53.jpg garage-calgary/image-2023-03-19_20-49-52.jpg garage-calgary/image-2023-03-19_20-49-51.jpg garage-calgary/image-2023-03-19_20-49-50.jpg garage-calgary/image-2023-03-19_20-49-49.jpg garage-calgary/image-2023-03-19_20-49-48.jpg garage-calgary/image-2023-03-19_20-49-47.jpg garage-calgary/image-2023-03-19_20-49-46.jpg garage-calgary/image-2023-03-19_20-49-45.jpg garage-calgary/image-2023-03-19_20-49-44.jpg garage-calgary/image-2023-03-19_20-49-37.jpg garage-calgary/image-2023-03-19_20-49-36.jpg garage-calgary/image-2023-03-19_20-49-35.jpg garage-calgary/image-2023-03-19_20-49-34.jpg garage-calgary/image-2023-03-19_20-49-33.jpg garage-calgary/image-2023-03-19_20-49-32.jpg garage-calgary/image-2023-03-19_20-49-31.jpg garage-calgary/image-2023-03-19_20-49-30.jpg garage-calgary/image-2023-03-19_20-49-29.jpg garage-calgary/image-2023-03-19_20-49-28.jpg garage-calgary/image-2023-03-19_20-49-27.jpg garage-calgary/image-2023-03-19_20-49-26.jpg garage-calgary/image-2023-03-19_20-49-24.jpg garage-calgary/image-2023-03-19_20-49-22.jpg garage-calgary/image-2023-03-19_20-49-21.jpg garage-calgary/image-2023-03-19_20-49-20.jpg garage-calgary/image-2023-03-19_20-49-19.jpg garage-calgary/image-2023-03-19_20-49-18.jpg garage-calgary/image-2023-03-19_20-49-16.jpg garage-calgary/image-2023-03-19_20-49-15.jpg garage-calgary/image-2023-03-19_20-49-14.jpg garage-calgary/image-2023-03-19_20-49-13.jpg garage-calgary/image-2023-03-19_20-49-11.jpg garage-calgary/image-2023-03-19_20-49-09.jpg garage-calgary/image-2023-03-19_20-49-08.jpg garage-calgary/image-2023-03-19_20-49-06.jpg garage-calgary/image-2023-03-19_20-49-05.jpg garage-calgary/image-2023-03-19_20-49-04.jpg garage-calgary/image-2023-03-19_20-49-03.jpg garage-calgary/image-2023-03-19_20-49-01.jpg garage-calgary/image-2023-03-19_20-48-59.jpg garage-calgary/image-2023-03-19_20-48-58.jpg garage-calgary/image-2023-03-19_20-48-57.jpg garage-calgary/image-2023-03-19_20-48-56.jpg garage-calgary/image-2023-03-19_20-48-54.jpg garage-calgary/image-2023-03-19_20-48-53.jpg garage-calgary/image-2023-03-19_20-48-52.jpg | /usr/bin/ssh -p 1022 idan@shoham.ca \"cd /data/security; tar xpf -\""

void Test()
  {
  int err = -1;
  char* cmd = BACKUPCMD;
  char oldpath[BUFLEN], *oldp=NULL;

  oldp = getcwd( oldpath, sizeof(oldpath)-1 );
  if( oldp!=oldpath )
    Error( "Failed to get old path" );

  err = chdir( WORKINGDIR );
  if( err )
    Error( "Failed to chdir to %s - %d", WORKINGDIR, err );

  err = ASyncRunShellNoIO( cmd );
  if( err )
    Error( "Failed to ASyncRunShellNoIO %s - %d", cmd, err );

  err = chdir( oldp );
  if( err )
    Error( "Failed to return to %s - %d", oldp, err );

  printf( "Running..\n" );
  }

int main( int argc, char** argv )
  {
  Test();
  return 0;
  }
