/*
 * ws_client.cpp
 *
 *  Created on: 2015-9-28
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>

#include <string>
using namespace std;

#include "WSClient.h"
#include <common/KThread.h>
#include <common/LogManager.h>

#define VERSION_STRING "1.0.0"

//char ip[128] = {"172.16.172.129"};
char ip[128] = {"192.168.88.152"};
int iTotal = 1;
int iReconnect = 120;
bool bPlay = false;
bool bNoVideo = false;

bool Parse(int argc, char *argv[]);

#define MAX_CLIENT 1000
bool testReconnect = true;
#define RECONN_MAX_TIME_S (10*1000*1000)
#define RECONN_CHECK_INTERVAL (100*1000)

bool gStart = false;
void SignalFunc(int sign_no);

WSClient client[MAX_CLIENT];
KThread* clientThreads[MAX_CLIENT];
KMutex clientRandTimeMutext[MAX_CLIENT];
int clientRandTime[MAX_CLIENT];

class ClientRunnable : public KRunnable {
public:
	ClientRunnable(WSClient *container) {
		mContainer = container;
	}
	virtual ~ClientRunnable() {
		mContainer = NULL;
	}
protected:
	void onRun() {
		while( gStart ) {
			clientRandTimeMutext[mContainer->GetIndex()].lock();
			if( clientRandTime[mContainer->GetIndex()] <= 0 ) {
				if( iReconnect ) {
					clientRandTime[mContainer->GetIndex()] = rand() % iReconnect;
				}
			}
			clientRandTimeMutext[mContainer->GetIndex()].unlock();

			LogManager::GetLogManager()->Log(
					LOG_WARNING,
					"ws_client::ClientRunnable( "
					"[Connect], "
					"index : '%d', "
					"client : %p, "
					"clientRandTime : '%d' "
					")",
					mContainer->GetIndex(),
					mContainer,
					clientRandTime[mContainer->GetIndex()]
					);

			char user[1024];
			char dest[1024];
			sprintf(user, "MM%d", mContainer->GetIndex() + 300);
			sprintf(dest, "WW%d|||PC0|||1", mContainer->GetIndex());

			if( mContainer->Connect(ip, user, dest) ) {
				while( mContainer->RecvWSPacket() ) {
					// 读取数据
				}

				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"ClientRunnable::onRun( "
						"[Disconnect], "
						"index : '%d' "
						")",
						mContainer->GetIndex()
						);

			}

			// 连接断开
			mContainer->Close();

			// 休息后重连
			sleep(1);
		}

	}

private:
	WSClient *mContainer;
};

class WSClientListenerImp : public WSClientListener {
public:
	void OnConnect(WSClient* client) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"WSClientListenerImp::OnConnect( "
				"index : '%d', "
				"client : %p "
				")",
				client->GetIndex(),
				client
				);
	}

	void OnDisconnect(WSClient* client) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"WSClientListenerImp::OnDisconnect( "
				"index : '%d', "
				"client : %p "
				")",
				client->GetIndex(),
				client
				);
	}
};
WSClientListenerImp gWSClientListenerImp;

int main(int argc, char *argv[]) {
	printf("############## ws client ############## \n");
	printf("# Version : %s \n", VERSION_STRING);
	printf("# Build date : %s %s \n", __DATE__, __TIME__ );
	srand(time(0));

	Parse(argc, argv);
	printf("# ip : %s \n", ip);
	printf("# total : %d \n", iTotal);
	printf("# reconnect time : %d \n", iReconnect);
	iReconnect *= 1000 * 1000;

	/* Ignore */
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, 0);

	/* Handle */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SignalFunc;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);

//	sigaction(SIGHUP, &sa, 0);
	sigaction(SIGINT, &sa, 0); // Ctrl-C
	sigaction(SIGQUIT, &sa, 0);
	sigaction(SIGILL, &sa, 0);
	sigaction(SIGABRT, &sa, 0);
	sigaction(SIGFPE, &sa, 0);
	sigaction(SIGBUS, &sa, 0);
	sigaction(SIGSEGV, &sa, 0);
	sigaction(SIGSYS, &sa, 0);
	sigaction(SIGTERM, &sa, 0);
	sigaction(SIGXCPU, &sa, 0);
	sigaction(SIGXFSZ, &sa, 0);

	LogManager::GetLogManager()->Start(LOG_WARNING, "log");
	LogManager::GetLogManager()->SetDebugMode(true);

	LogManager::GetLogManager()->Log(
			LOG_WARNING,
			"############## ws client ############## "
			")"
			);

	gStart = true;
	for(int i = 0; i < iTotal; i++) {
		client[i].SetWSClientListener(&gWSClientListenerImp);
		client[i].SetIndex(i);
		ClientRunnable* runnable = new ClientRunnable(&(client[i]));
		clientThreads[i] = new KThread();
		clientThreads[i]->start(runnable);

		usleep(100 * 1000);
	}

	// test reconnect
	if (testReconnect) {
		sleep(30);
		while (gStart) {
			for (int i = 0; i < iTotal && gStart; i++) {
				if( client[i].IsRunning() ) {
					clientRandTimeMutext[i].lock();
					if (clientRandTime[i] > 0 && gStart) {
						clientRandTime[i] -= RECONN_CHECK_INTERVAL;
					} else {
						// 只关闭socket, 等待接收线程停止, 再重连
						client[i].Shutdown();
					}
					clientRandTimeMutext[i].unlock();
				}
			}
			usleep(RECONN_CHECK_INTERVAL);
		}
	}

	for(int i = 0; i < iTotal; i++) {
		KRunnable* runnable = clientThreads[i]->stop();
		if( runnable ) {
			delete runnable;
		}
	}

	printf("# All threads exit \n");

	return EXIT_SUCCESS;
}

void SignalFunc(int sign_no) {
	LogManager::GetLogManager()->Log(LOG_ERR_SYS, "main( Get signal : %d )", sign_no);
	LogManager::GetLogManager()->LogFlushMem2File();

	gStart = false;
	switch(sign_no) {
	default:{
		for(int i = 0; i < iTotal; i++) {
			client[i].Shutdown();
		}

		for(int i = 0; i < iTotal; i++) {
			KRunnable* runnable = clientThreads[i]->stop();
			if( runnable ) {
				delete runnable;
			}
		}

		printf("# All threads exit \n");

		exit(EXIT_SUCCESS);
	}break;
	}
}


bool Parse(int argc, char *argv[]) {
	string key;
	string value;

	for( int i = 1; (i + 1) < argc; i+=2 ) {
		key = argv[i];
		value = argv[i+1];

		if( key.compare("-h") == 0 ) {
			memset(ip, 0, sizeof(ip));
			memcpy(ip, value.c_str(), value.length());
		} else if( key.compare("-n") == 0 ) {
			iTotal = atoi(value.c_str());
		} else if( key.compare("-reconnect") == 0 ) {
			iReconnect = atoi(value.c_str());
		}

	}

	return true;
}
