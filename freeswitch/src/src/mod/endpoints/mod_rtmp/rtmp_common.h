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

// get rtmp app(e.g., the str is "phone?param1=1&param2=1", then the buffer is "phone")
// return the buffer string length
inline int GetRtmpApp(const char* str, char* buffer, int bufferSize)
{
	int result = 0;
	if (NULL != str	&& str[0] != '\0'
		&& NULL != buffer && bufferSize > 0)
	{
		const char* appSeparator = strstr(str, "?");
		int dataLen = (int)(appSeparator - str);
		if (NULL != appSeparator && bufferSize > dataLen)
		{
			memcpy(buffer, str, dataLen);
			buffer[dataLen] = '\0';
			result = dataLen;
		}
	}
	return result;
}

// get rtmp app parameter(e.g., "client?param1=1&param2=1")
// return the number of parsed item
typedef struct _tagRtmpStreamParam {
	char key[64];
	char value[64];
} RtmpStreamParamItem;
inline int GetRtmpStreamParam(const char* str, RtmpStreamParamItem* items, int itemSize)
{
	int result = 0;
	if (NULL != str && str[0] != '\0'
		&& NULL != items && itemSize > 0)
	{
		do {
			const char* data = NULL;
			const char* appSeparator = NULL;

			appSeparator = strstr(str, "?");
			if (NULL == appSeparator) {
				break;
			}

			data = appSeparator + 1;
			if ('\0' == data[0]) {
				break;
			}

			// parsing str(like "client?param1=value1&param2=value2") to RtmpStreamParamItem(key, value)
			{
				int index = 0;
				const char* itemStart = data;
				const char* itemOffset = itemStart;
				const char* keyStart = NULL;
				const char* keyEnd = NULL;
				const char* valueStart = NULL;
				const char* valueEnd = NULL;
				int valueSize = 0;
				while (index < itemSize
						&& NULL != itemStart && '\0' != itemStart[0])
				{
					const char* itemEnd = strstr(itemStart, "&");
					if (NULL == itemEnd) {
						itemEnd = itemStart + strlen(itemStart);
						itemOffset = itemEnd;
					}
					else {
						itemOffset = itemEnd + 1;
					}

					keyStart = itemStart;
					keyEnd = strstr(keyStart, "=");
					if (NULL != keyEnd)
					{
						int keySize = (int)(keyEnd - keyStart);
						if (sizeof(items[index].key) > keySize) {
							memcpy(items[index].key, keyStart, keySize);
							items[index].key[keySize] = '\0';

							valueStart = keyEnd + 1;
							valueEnd = itemEnd;
							valueSize = (int)(valueEnd - valueStart);
							if (sizeof(items[index].value) > valueSize) {
								memcpy(items[index].value, valueStart, valueSize);
								items[index].value[valueSize] = '\0';

								index++;
							}
						}
					}
					itemStart = itemOffset;
				}
				result = index;
			}
		} while (0);
	}
	return result;
}

#endif /* SRC_MOD_ENDPOINTS_MOD_RTMP_RTMP_COMMON_H_ */
