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
#include <curl/curl.h>

#define BIGBUF 500000
#define BUFLEN 2000

#define RE_BOOL "[01]"
#define RE_POSINT "[0-9]+"
#define RE_INTEGER "[-]?[0-9]+"
#define RE_FLOAT "[-]?[0-9.]+"
#define RE_IDENTIFIER "[0-9a-zA-Z_.-]+"

#define RE_DATE "[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]"
#define RE_TIME "[0-9][0-9]:[0-9][0-9]:[0-9][0-9]"
#define RE_TIME_NOSEC "[0-9][0-9]:[0-9][0-9]"

#define RE_EMAIL_ADDRESS "[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}"
#define RE_EMAIL_DOMAIN "[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}"


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
#include "curl-wrapper.h"
#include "lua-wrapper.h"

#endif
