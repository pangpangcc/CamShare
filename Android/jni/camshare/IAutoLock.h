/*
 * IAutoLock.h
 *
 *  Created on: 2016年12月16日
 *      Author: Alex
 */

#pragma once

// 普通锁接口类
class IAutoLock {
public:
	static IAutoLock* CreateAutoLock();
	static void  ReleaseAutoLock(IAutoLock* lock);
public:
	IAutoLock(){}
	virtual ~IAutoLock(){}
public:
	virtual bool Init() = 0;
	virtual bool TryLock() = 0;
	virtual bool Lock() = 0;
	virtual bool Unlock() = 0;
};


