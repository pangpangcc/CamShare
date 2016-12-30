/*
 * WSClient.h
 *
 *  Created on: 2016年4月28日
 *      Author: max
 */

#ifndef WSCLIENT_H_
#define WSCLIENT_H_

#include <string.h>

#include <string>
#include <list>
using namespace std;

#include <common/KMutex.h>
#include <common/KSafeMap.h>

class WSClient;
class ISocketHandler;
/**
 * WebsocketClient客户端监听接口类
 */
class WSClientListener
{
public:
	WSClientListener() {};
	virtual ~WSClientListener() {}

public:
	virtual void OnConnect(WSClient* client) = 0;
	virtual void OnDisconnect(WSClient* client) = 0;
};

class WSClient {
public:
	WSClient();
	virtual ~WSClient();

	int GetIndex();
	void SetIndex(int i);

	const string& GetUser();
	bool IsConnected();
	bool IsRunning();

	void SetWSClientListener(WSClientListener *listener);

	bool Connect(const string& hostName, const string& user, const string& dest);
	void Shutdown();
	void Close();
	bool RecvWSPacket();

private:
	/**
	 * Do it when TCP connected
	 */
	bool HandShake();
	void Init();

	unsigned short port;
	string hostname;

	int mIndex;
	string mUser;
	string mDest;

	bool mbRunning;
	bool mbConnected;
	bool mbShutdown;

	ISocketHandler* m_socketHandler;
	WSClientListener* mpWSClientListener;

	/**
	 * 收包锁
	 */
	KMutex mRecvMutex;

	/**
	 * 发包锁
	 */
	KMutex mSendMutex;
};

#endif /* WSCLIENT_H_ */
