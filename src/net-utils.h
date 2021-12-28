#ifndef _INCLUDE_NET_UTILS
#define _INCLUDE_NET_UTILS

typedef struct ipaddr
  {
  unsigned char bytes[4];
  int bits;
  } IPADDR;

#define ISBYTE(I) ((I)>=0 && (I)<256)

#define RE_IPADDR "[0-9][0-9]?[0-9]?\\.[0-9][0-9]?[0-9]?\\.[0-9][0-9]?[0-9]?\\.[0-9][0-9]?[0-9]?"

int PingAddress( char* netAddress );
int IPAddressFromCommand( char* buf, int bufLen, char* command );
int StringToIp( IPADDR* dst, char* src );
void ExpandIP( int* array, IPADDR* addr );
int IPinSubnet( IPADDR* ip, IPADDR* subnet );

#endif
