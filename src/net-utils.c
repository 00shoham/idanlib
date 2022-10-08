#include "utils.h"

#define PING_COMMAND_WITH_ARG "/bin/ping -t 2 -c 2 %s"

int PingAddress( char* netAddress )
  {
  char cmd[BUFLEN];
  snprintf( cmd, BUFLEN-1, PING_COMMAND_WITH_ARG, netAddress );
  FILE* h = popen( cmd, "r" );
  if( h==NULL )
    {
    Warning( "Cannot run test-ping command against %s (%d:%s)",
             netAddress, errno, strerror( errno ) );
    return -1;
    }

  char buf[BUFLEN];
  while( fgets( buf, sizeof(buf)-1, h )==buf )
    {
    (void)StripEOL(buf);
    /* printf("Ping output: [%s]\n", buf ); */
    if( StringMatchesRegex( "^PING ", buf )==0 )
      {
      /* header */
      }
    else if( StringMatchesRegex( "bytes from.*icmp_seq.*ttl.*time", buf )==0 )
      {
      pclose( h );
      return 0;
      }
    else if( StringMatchesRegex( "Unreachable", buf )==0 )
      {
      pclose( h );
      Warning("Host %s not reachable (a)", netAddress );
      return -2;
      }
    else if( StringMatchesRegex( "100% packet loss", buf )==0 )
      {
      pclose( h );
      Warning("Host %s not reachable (b)", netAddress );
      return -3;
      }
    else if( StringMatchesRegex( " 0 received", buf )==0 )
      {
      pclose( h );
      Warning("Host %s not reachable (c)", netAddress );
      return -4;
      }
    }

  pclose( h );
  Warning("Host %s not reachable (d)", netAddress );

  return -5;
  }

int IPAddressFromCommand( char* buf, int bufLen, char* command )
  {
  if( buf==NULL )
    {
    Warning("Cannot store IP address in NULL buffer");
    return -1;
    }

  if( bufLen<16 )
    {
    Warning("Cannot store IP address in tiny buffer");
    return -2;
    }

  if( command==NULL )
    {
    Warning("Cannot extract IP address from NULL command");
    return -3;
    }

  char* tmpBuf = strdup( command );

  char* ptr = ExtractRegexFromString( RE_IPADDR, tmpBuf );
  if( ptr!=NULL )
    {
    strncpy( buf, ptr, bufLen-1 );
    FREE( tmpBuf );
    FREE( ptr );
    return 0;
    }

  FREE( tmpBuf );
  buf[0] = 0;

  return -1;
  }

int StringToIp( IPADDR* dst, char* src )
  {
  if( EMPTY( src ) || dst==NULL )
    {
    return -1;
    }

  memset( dst, 0, sizeof(IPADDR) );

  int a=0;
  int b=0;
  int c=0;
  int d=0;
  int bits=0;

  if( strchr( src, '/' )==NULL )
    {
    int n = sscanf( src, "%d.%d.%d.%d", &a, &b, &c, &d );
    if( n!=4 )
      {
      return -2;
      }
    if( !ISBYTE( a ) || !ISBYTE( b ) || !ISBYTE( c ) || !ISBYTE( d ) )
      {
      return -3;
      }
    dst->bytes[0] = a;
    dst->bytes[1] = b;
    dst->bytes[2] = c;
    dst->bytes[3] = d;
    dst->bits = 0;

    return 0;
    }

  int n = sscanf( src, "%d.%d.%d.%d/%d", &a, &b, &c, &d, &bits );
  if( n!=5 )
    {
    return -4;
    }

  if( !ISBYTE( a ) || !ISBYTE( b ) || !ISBYTE( c ) || !ISBYTE( d ) )
    {
    return -3;
    }

  if( bits<8 || bits>32 )
    {
    return -5;
    }

  dst->bytes[0] = a;
  dst->bytes[1] = b;
  dst->bytes[2] = c;
  dst->bytes[3] = d;
  dst->bits = bits;

  return 0;
  }

void ExpandIP( int* array, IPADDR* addr )
  {
  array[0] =  (addr->bytes[0] & 0x80) ? 1 : 0;
  array[1] =  (addr->bytes[0] & 0x40) ? 1 : 0;
  array[2] =  (addr->bytes[0] & 0x20) ? 1 : 0;
  array[3] =  (addr->bytes[0] & 0x10) ? 1 : 0;
  array[4] =  (addr->bytes[0] & 0x08) ? 1 : 0;
  array[5] =  (addr->bytes[0] & 0x04) ? 1 : 0;
  array[6] =  (addr->bytes[0] & 0x02) ? 1 : 0;
  array[7] =  (addr->bytes[0] & 0x01) ? 1 : 0;

  array[8] =  (addr->bytes[1] & 0x80) ? 1 : 0;
  array[9] =  (addr->bytes[1] & 0x40) ? 1 : 0;
  array[10] = (addr->bytes[1] & 0x20) ? 1 : 0;
  array[11] = (addr->bytes[1] & 0x10) ? 1 : 0;
  array[12] = (addr->bytes[1] & 0x08) ? 1 : 0;
  array[13] = (addr->bytes[1] & 0x04) ? 1 : 0;
  array[14] = (addr->bytes[1] & 0x02) ? 1 : 0;
  array[15] = (addr->bytes[1] & 0x01) ? 1 : 0;

  array[16] = (addr->bytes[2] & 0x80) ? 1 : 0;
  array[17] = (addr->bytes[2] & 0x40) ? 1 : 0;
  array[18] = (addr->bytes[2] & 0x20) ? 1 : 0;
  array[19] = (addr->bytes[2] & 0x10) ? 1 : 0;
  array[20] = (addr->bytes[2] & 0x08) ? 1 : 0;
  array[21] = (addr->bytes[2] & 0x04) ? 1 : 0;
  array[22] = (addr->bytes[2] & 0x02) ? 1 : 0;
  array[23] = (addr->bytes[2] & 0x01) ? 1 : 0;

  array[24] = (addr->bytes[3] & 0x80) ? 1 : 0;
  array[25] = (addr->bytes[3] & 0x40) ? 1 : 0;
  array[26] = (addr->bytes[3] & 0x20) ? 1 : 0;
  array[27] = (addr->bytes[3] & 0x10) ? 1 : 0;
  array[28] = (addr->bytes[3] & 0x08) ? 1 : 0;
  array[29] = (addr->bytes[3] & 0x04) ? 1 : 0;
  array[30] = (addr->bytes[3] & 0x02) ? 1 : 0;
  array[31] = (addr->bytes[3] & 0x01) ? 1 : 0;
  }

int IPinSubnet( IPADDR* ip, IPADDR* subnet )
  {
  int bigIP[32];
  int bigSUBNET[32];

  ExpandIP( bigIP, ip );
  ExpandIP( bigSUBNET, subnet );

  for( int i=0; i<subnet->bits; ++i )
    {
    if( bigIP[i] != bigSUBNET[i] )
      {
      return -1;
      }
    }

  return 0;
  }

