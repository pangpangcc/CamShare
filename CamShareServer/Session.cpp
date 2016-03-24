/*
 * Session.cpp
 *
 *  Created on: 2015年11月15日
 *      Author: kingsley
 */

#include "Session.h"

Session::Session(Client* client, ILiveChatClient* livechat) {
	// TODO Auto-generated constructor stub
	this->client = client;
	this->livechat = livechat;
}

Session::~Session() {
	// TODO Auto-generated destructor stub
	for(RequestkMap::iterator itr = mRequestMap.Begin(); itr != mRequestMap.End(); itr++) {
		if( itr->second != NULL ) {
			delete itr->second;
		}
	}
}

void Session::AddRequest(int seq, IRequest* request) {
	mRequestMap.Lock();
	RequestkMap::iterator itr = mRequestMap.Find(seq);
	if( itr == mRequestMap.End() ) {
		mRequestMap.Insert(seq, request);
	}
	mRequestMap.Unlock();
}

IRequest* Session::EraseRequest(int seq) {
	IRequest* request = NULL;
	mRequestMap.Lock();
	RequestkMap::iterator itr = mRequestMap.Erase(seq);
	if( itr != mRequestMap.End() ) {
		request = itr->second;
	}
	mRequestMap.Unlock();
	return request;
}
