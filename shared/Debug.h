#pragma once
#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <string>
#include <cmath>


using namespace std;

#define NONE    0x00
#define CRIT    0x01
#define EMR     0x03
#define ERR     0x07
#define INFO    0x0F
#define DBG     0x1F

#define DEBUG_LEVEL ERR

#define DBG_T1 mArgParser.dbgStart
#define DBG_T2 mArgParser.dbgEnd

#define TIMESTR_ARG(T) ( \
	T<0?  "": \
	( \
		to_string(int(trunc(T/3600))) + \
		to_string(int(trunc((T - trunc(T/3600))/60))) + ":" + \
		to_string(T - trunc(T/3600)*3600 - trunc((T - trunc(T / 3600)) / 60)*60) \
	).c_str() \
)

//#define WHERESTR "[FILE : %s, FUNC : %s, LINE : %d, TIME: %s]: "
//#define WHEREARG __FILE__,__func__,__LINE__


#define DEBUG(C, f, T, X, ...)  { \
/* \
	if (X <= ERR && X  != 0 ) \
		fprintf(f, "*E* " __VA_ARGS__); \
	else \
*/ \
	if (C)\
		fprintf(f, __VA_ARGS__); \
	fflush(f); \
}



#define DEBUG_PRINTF(log, c, f, T, X, _fmt, ...)  \
	if(log || (DEBUG_LEVEL & X) == X) \
		if (DEBUG_LEVEL == DBG) { \
			if (T < 0) \
				DEBUG(c, f, T, X, "[FUNC : %s]: " _fmt, __func__ , __VA_ARGS__) \
			else \
				DEBUG(c, f, T, X, "[FUNC: %s, TIME: %s]: " _fmt, __func__ , TIMESTR_ARG(T), __VA_ARGS__) \
		} else { \
			if (T < 0) \
				DEBUG(c, f, T, X, _fmt, __VA_ARGS__) \
			else \
				DEBUG(c, f, T, X, "%s: " _fmt , TIMESTR_ARG(T), __VA_ARGS__) \
		}


#define DEBUG_PRINT(T, X, _fmt, ...) DEBUG_PRINTF(true, (T > DBG_T1 && T < DBG_T2), stdout, T, X, _fmt, __VA_ARGS__)

#define DBG_PRINT(X, _fmt, ...) DEBUG_PRINTF(false, true, stdout, -1, X, _fmt, __VA_ARGS__)




#endif
