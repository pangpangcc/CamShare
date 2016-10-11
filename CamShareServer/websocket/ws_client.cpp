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
#include "../LogManager.h"

#define VERSION_STRING "1.0.0"

#define SERVER_IP "127.0.0.1"
//#define SERVER_IP "192.168.88.152"
#define MAX_CLIENT 100
bool testReconnect = true;
#define RECONN_MAX_TIME_MS (20*1000*1000)
#define RECONN_CHECK_INTERVAL (200*1000)


bool gStart = false;
void SignalFunc(int sign_no);

WSClient client[MAX_CLIENT];
KThread* clientThreads[MAX_CLIENT];


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
			char temp[1024];
			sprintf(temp, "MM%d", mContainer->GetIndex());

			if( mContainer->Connect(SERVER_IP, temp, "WW1|||PC0|||1") ) {
				while( mContainer->RecvWSPacket() ) {
					// 读取数据
				}

				LogManager::GetLogManager()->Log(
						LOG_WARNING,
						"ClientRunnable::onRun( "
						"tid : %d, "
						"[Disconnect], "
						"index : '%d' "
						")",
						(int)syscall(SYS_gettid),
						mContainer->GetIndex()
						);

			}

			// 连接断开
			client->Close();

			int randtime = rand() % RECONN_MAX_TIME_MS;
			while (randtime > 0 && gStart) {
				int waittime = (randtime > RECONN_CHECK_INTERVAL ? RECONN_CHECK_INTERVAL : randtime);
				randtime -= waittime;
				usleep(waittime);
			}
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
				"tid : %d, "
				"index : '%d', "
				"client : %p "
				")",
				(int)syscall(SYS_gettid),
				client->GetIndex(),
				client
				);
	}

	void OnDisconnect(WSClient* client) {
		LogManager::GetLogManager()->Log(
				LOG_WARNING,
				"WSClientListenerImp::OnDisconnect( "
				"tid : %d, "
				"index : '%d', "
				"client : %p "
				")",
				(int)syscall(SYS_gettid),
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

	gStart = true;
	for(int i = 0; i < MAX_CLIENT; i++) {
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
                srand(time(NULL));
		while (gStart) {
			for (int i = 0; i < MAX_CLIENT && gStart; i++) {
				int randtime = rand() % RECONN_MAX_TIME_MS;
				while (randtime > 0 && gStart) {
					int waittime = (randtime > RECONN_CHECK_INTERVAL ? RECONN_CHECK_INTERVAL : randtime);
					randtime -= waittime;
					usleep(waittime);
				}
				client[i].Close();
			}
		}
	}

	for(int i = 0; i < MAX_CLIENT; i++) {
		clientThreads[i]->stop();
	}

	printf("# All threads exit \n");
//	while( true ) {
//		sleep(1);
//	}

	return EXIT_SUCCESS;
}

void SignalFunc(int sign_no) {
	LogManager::GetLogManager()->Log(LOG_ERR_SYS, "main( Get signal : %d )", sign_no);
	LogManager::GetLogManager()->LogFlushMem2File();

	gStart = false;
	switch(sign_no) {
	default:{
		for(int i = 0; i < MAX_CLIENT; i++) {
			client[i].Shutdown();
		}

		for(int i = 0; i < MAX_CLIENT; i++) {
			clientThreads[i]->stop();
		}

		printf("# All threads exit \n");

		exit(EXIT_SUCCESS);
	}break;
	}
}
