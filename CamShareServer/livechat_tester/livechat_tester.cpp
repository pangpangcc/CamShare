/*
 * server.cpp
 *
 *  Created on: 2015-9-28
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include <livechat/ITransportPacketHandler.h>
#include "../TcpServer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <string>
#include <map>
using namespace std;

#define VERSION_STRING "1.0.0"

string sConf = "";  // 配置文件

class TcpServerObserverImp : public TcpServerObserver {
public:
	TcpServerObserverImp(){
	};
	~TcpServerObserverImp(){};

	bool OnAccept(TcpServer *ts, int fd, char* ip) {
		printf("# OnAccept( fd : %d )\n", fd);
		return true;
	}

	void OnDisconnect(TcpServer *ts, int fd) {
		printf("# OnDisconnect( fd : %d )\n", fd);
	}

	void OnClose(TcpServer *ts, int fd) {
		printf("# OnClose( fd : %d )\n", fd);
	}

	void OnRecvMessage(TcpServer *ts, Message *m) {
		printf("# OnRecvMessage( len : %d )\n", m->len);

		if( m->len >= sizeof(TransportSendHeader) + sizeof(TransportHeader) ) {
			// request
			TransportSendProtocol* rp = (TransportSendProtocol*)m->buffer;
			rp->sendHeader.length = ntohl(rp->sendHeader.length);
			rp->header.length =  ntohl(rp->header.length);
			rp->header.cmd = ntohl(rp->header.cmd);
			rp->header.seq = ntohl(rp->header.seq);

			printf("# OnRecvMessage( cmd : %d, seq : %d )\n", rp->header.cmd, rp->header.seq);

			switch(rp->header.cmd) {
			case TCMD_CHECKVER:
			case TCMD_SENDENTERCONFERENCE:{
				// respond
				string result = "{\"ret\":0}";
				Message* sm = ts->GetIdleMessageList()->PopFront();
				if( sm != NULL ) {
					sm->fd = m->fd;

					TransportProtocol* sp = (TransportProtocol*)sm->buffer;
					sp->header.cmd = ntohl(rp->header.cmd);
					sp->header.seq = ntohl(rp->header.seq);
					sp->SetDataLength(result.length());

					sprintf((char *)sp->data, "%s", result.c_str());
					sm->len = sizeof(sp->header) + sp->GetDataLength();

					sp->header.length = ntohl(sp->header.length);

					ts->SendMessageByQueue(sm);

					printf("# OnRecvMessage( send len : %d )\n", sm->len);
				}
			}break;
			default:break;
			}

		}
	}

	void OnSendMessage(TcpServer *ts, Message *m) {
		printf("# OnSendMessage( len : %d )\n", m->len);
	}

	void OnTimeoutMessage(TcpServer *ts, Message *m) {

	}

};

int main(int argc, char *argv[]) {
	printf("############## livechat tester ############## \n");
	printf("# Version : %s \n", VERSION_STRING);
	printf("# Build date : %s %s \n", __DATE__, __TIME__ );
	srand(time(0));

	/* Ignore SIGPIPE */
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, 0);

//	LogManager::GetLogManager()->Start(1000, 5, "./log_livechat_test");

	TcpServerObserverImp observer;
	TcpServer server;
	server.SetTcpServerObserver(&observer);
	server.Start(10, 9877, 1);

	while( true ) {
		/* do nothing here */
		sleep(10);
	}

	return EXIT_SUCCESS;
}
