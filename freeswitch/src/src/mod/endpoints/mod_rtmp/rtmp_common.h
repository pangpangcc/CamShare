/*
 * rtmp_common.h
 *
 *  Created on: 2017年1月12日
 *      Author: max
 */

#ifndef SRC_MOD_ENDPOINTS_MOD_RTMP_RTMP_COMMON_H_
#define SRC_MOD_ENDPOINTS_MOD_RTMP_RTMP_COMMON_H_

// add by Samson 2016-05-30
#include <sys/time.h>
#include <unistd.h>

inline long long GetTickCount()
{
	long long result = 0;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	result = (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return result;
}

inline long long DiffTime(long long start, long long end)
{
    return end - start;
//	return (end > start ? end - start : (unsigned long)-1 - end + start);
}

#endif /* SRC_MOD_ENDPOINTS_MOD_RTMP_RTMP_COMMON_H_ */
