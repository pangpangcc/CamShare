/**
  * Author	: Samson
  * Date	: 2016-01-26
  * Description	: for check momory leak
  * File	: CheckMomoryLeak.h
  */

#pragma once

#ifdef _CHECK_MOMORY_LEAK

void * operator new(unsigned int size);
void * operator new(unsigned int size, const char *file, int line);

void operator delete(void * p);
void operator delete[](void * p);
void operator delete(void * p, const char *file, int line);
void operator delete[](void * p, const char *file, int line);

#define new new(__FILE__, __LINE__)
//#define delete delete(__FILE__, __LINE__)

#endif

void OutputAllocInfo();
