/*
 * Session.cpp
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#include "Session.h"
#include "LogManager.h"

class CheckTimeoutRunnable : public KRunnable {
public:
	CheckTimeoutRunnable(Session *container) {
		mContainer = container;
	}
	virtual ~CheckTimeoutRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		mContainer->CheckTimeoutRunnableHandle();
	}
private:
	Session *mContainer;
};

Session::Session(ILiveChatClient* livechat) {
	// TODO Auto-generated constructor stub
	this->livechat = livechat;

	mIsRunning = true;
	timeout = 0;

	mpCheckTimeoutRunnable = new CheckTimeoutRunnable(this);
	mpCheckTimeoutThread = new KThread(mpCheckTimeoutRunnable);
	mpCheckTimeoutThread->start();
}

Session::~Session() {
	// TODO Auto-generated destructor stub
	// 停止检测超时线程
	mIsRunning = false;

	if( mpCheckTimeoutThread ) {
		mpCheckTimeoutThread->stop();
		delete mpCheckTimeoutThread;
		mpCheckTimeoutThread = NULL;
	}

	if( mpCheckTimeoutRunnable ) {
		delete mpCheckTimeoutRunnable;
		mpCheckTimeoutRunnable = NULL;
	}

	// 释放所有请求
	mRequestMap.Lock();
	for(RequestkMap::iterator itr = mRequestMap.Begin(); itr != mRequestMap.End(); itr++) {
		IRequest* request = itr->second;
		if( request != NULL ) {
			// 标记任务完成, 执行失败
			request->FinisRequest(false);

			// 释放任务
			delete request;
			request = NULL;
		}
	}
	mRequestMap.Clear();

	for(RequestItemList::iterator itr2 = mRequestItemList.Begin(); itr2 != mRequestItemList.End();) {
		RequestItem* item = *itr2;
		delete item;
		mRequestItemList.PopValueUnSafe(itr2++);
	}

	mRequestMap.Unlock();
}

bool Session::AddRequest(const string& identify, IRequest* request) {
	bool bFlag = false;
	mRequestMap.Lock();
	RequestkMap::iterator itr = mRequestMap.Find(identify);
	if( itr == mRequestMap.End() ) {
		// 插入任务列表
		mRequestMap.Insert(identify, request);

		// 插入任务检测列表
		RequestItem* item = new RequestItem(identify, request);
		mRequestItemList.PushBack(item);

		bFlag = true;

	}
	mRequestMap.Unlock();
	return bFlag;
}

IRequest* Session::EraseRequest(const string& identify) {
	IRequest* request = NULL;
	mRequestMap.Lock();
	RequestkMap::iterator itr = mRequestMap.Find(identify);
	if( itr != mRequestMap.End() ) {
		request = itr->second;
		mRequestMap.Erase(itr);
	}
	mRequestMap.Unlock();
	return request;
}

void Session::CheckTimeoutRunnableHandle() {
	while( mIsRunning ) {
		sleep(1);

		// 不检测
		if( timeout == 0 ) {
			continue;
		}

		mRequestMap.Lock();
		for(RequestItemList::iterator itr = mRequestItemList.Begin(); itr != mRequestItemList.End();) {
			RequestItem* item = *itr;

			if( GetTickCountDifferences(item->timestamp, GetTickCount()) > timeout ) {
				LogManager::GetLogManager()->Log(
						LOG_STAT,
						"Session::CheckTimeoutRunnableHandle( "
						"[检测任务超时, 发现有任务超时], "
						"item->identify : %s, "
						"item->request : %p "
						")",
						item->identify.c_str(),
						item->request
						);

				// 移除任务列表
				RequestkMap::iterator itr2 = mRequestMap.Find(item->identify);
				if( itr2 != mRequestMap.End() ) {
					IRequest* request = itr2->second;
					if( item->request == request ) {
						LogManager::GetLogManager()->Log(
								LOG_WARNING,
								"Session::CheckTimeoutRunnableHandle( "
								"[检测任务超时, 发现有任务超时, 任务未完成, 标记执行失败], "
								"item->identify : %s, "
								"item->request : %p "
								")",
								item->identify.c_str(),
								item->request
								);

						if( request != NULL ) {
							// 标记任务完成, 执行失败
							request->FinisRequest(false);

							// 释放任务
							delete request;
							request = NULL;
						}
						mRequestMap.Erase(itr2);

					} else {
						LogManager::GetLogManager()->Log(
								LOG_STAT,
								"Session::CheckTimeoutRunnableHandle( "
								"[检测任务超时, 发现有任务超时, 任务已经被替换], "
								"item->identify : %s, "
								"item->request : %p "
								")",
								item->identify.c_str(),
								item->request
								);
					}

				} else {
					LogManager::GetLogManager()->Log(
							LOG_STAT,
							"Session::CheckTimeoutRunnableHandle( "
							"[检测任务超时, 发现有任务超时, 任务已经完成], "
							"item->identify : %s, "
							"item->request : %p "
							")",
							item->identify.c_str(),
							item->request
							);
				}

				// 移除任务检测列表
				mRequestItemList.PopValueUnSafe(itr++);

				// 释放任务检测Item
				delete item;

			} else {
				itr++;
			}

		}
		mRequestMap.Unlock();
	}

}
