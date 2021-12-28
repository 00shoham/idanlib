#ifndef _INCLUDE_BASE
#define _INCLUDE_BASE

#include "utils.h"

#define CTOF(TEMP) ((TEMP)*9.0/5.0+32.0)
#define FTOC(TEMP) ((TEMP - 32.0)*5.0/9.0)
#define ROUND5(N) (round( (N)*2.0 )/2.0)
#define SANE_TEMPERATURE(TT) ((TT)>-50.0 && (TT)<50.0)
#define INVALID_TEMPERATURE -99.0

#endif
