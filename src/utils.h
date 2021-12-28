#ifndef _INCLUDE_UTILS
#define _INCLUDE_UTILS

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <regex.h>
#include <ctype.h>
#include <locale.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include <errno.h>
/* #include <jpeglib.h> */
#include <math.h>
#include <pthread.h>
#include <execinfo.h>
#include <uuid/uuid.h>

#define BIGBUF 500000
#define BUFLEN 2000

#define RE_BOOL "[01]"
#define RE_POSINT "[0-9]+"
#define RE_INTEGER "[-]?[0-9]+"
#define RE_FLOAT "[0-9.]+"

#include "error.h"
#include "mem-utils.h"
#include "str-utils.h"
#include "date-utils.h"
#include "tag-value.h"
#include "proc-utils.h"
#include "fs-utils.h"
#include "net-utils.h"
#include "json-parser.h"
#include "http.h"
#include "nargv.h"
#include "globs.h"
#include "daemon.h"
#include "sha-wrapper.h"

#endif
