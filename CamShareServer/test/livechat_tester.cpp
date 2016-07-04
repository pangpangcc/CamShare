/*
 * server.cpp
 *
 *  Created on: 2015-9-28
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include <livechat/ITransportPacketHandler.h>

#include "../TcpServer.h"

#include <amf/AmfParser.h>

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
		fd = -1;
	};
	~TcpServerObserverImp(){};

	bool OnAccept(TcpServer *ts, int fd, char* ip) {
		printf("# OnAccept( fd : %d )\n", fd);
		this->fd = fd;

		mBuffer.Reset();

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

		int ret = 0;
		char* buffer = m->buffer;
		int len = m->len;
		int last = len;
		int recved = 0;
//		while( ret != -1 && last > 0 ) {
//			int recv = (last < MAX_BUFFER_LEN - mBuffer.len)?last:(MAX_BUFFER_LEN - mBuffer.len);
//			if( recv > 0 ) {
//				memcpy(mBuffer.buffer + mBuffer.len, buffer + recved, recv);
//				mBuffer.len += recv;
//				last -= recv;
//				recved += recv;
//				mBuffer.buffer[mBuffer.len] = '\0';
//			}
//
//			while( mBuffer.len >= sizeof(TransportSendHeader) ) {
//				// request
//				TransportSendHeader* sendHeader = (TransportSendHeader*)mBuffer.buffer;
//				unsigned int sendHeaderlength = ntohl(sendHeader->length);
//				printf("# OnRecvMessage( sendHeaderlength : %d )\n", sendHeaderlength);
//				printf("# OnRecvMessage( "
//						"sendHeaderlength : %d, "
//						"sendHeader->requestremote : %d, "
//						"sendHeader->requesttype : %s "
//						")\n",
//						sendHeaderlength,
//						sendHeader->requestremote,
//						sendHeader->requesttype
//						);
//
//				int parsedLen = sendHeaderlength + sizeof(sendHeader->length);
//				if( mBuffer.len >= parsedLen ) {
//					sendHeader->length = ntohl(sendHeader->length);
//					printf("# OnRecvMessage( sendHeader->GetHeaderLength() : %d )\n", sendHeader->GetHeaderLength());
//
//					TransportProtocol* tp = (TransportProtocol *)(mBuffer.buffer + sendHeader->GetHeaderLength());
//					unsigned int headerlength =  ntohl(tp->header.length);
//					unsigned int cmd = ntohl(tp->header.cmd);
//					unsigned int seq = ntohl(tp->header.seq);
//					printf("# OnRecvMessage( cmd : %d, seq : %d, headerlength : %d, data : %s )\n",
//							cmd, seq, headerlength, tp->data);
//
//					// 解析命令
//					switch(cmd) {
//					case TCMD_CHECKVER:
//					case TCMD_SENDENTERCONFERENCE:{
//						Message* sm = ts->GetIdleMessageList()->PopFront();
//						if( sm != NULL ) {
//							sm->fd = m->fd;
//
//							TransportProtocol* sp = (TransportProtocol*)sm->buffer;
//							sp->header.cmd = ntohl(cmd);
//							sp->header.seq = ntohl(seq);
//
//							// respond
////							// json
////							string result = "{\"ret\":0}";
////							sprintf((char *)sp->data, "%s", result.c_str());
////							sp->SetDataLength(result.length());
//
//							// amf
//							amf_object_handle root = amf_object::Alloc();
//							root->type = DT_TRUE;
//							root->boolValue = true;
//
//							AmfParser parser;
//							size_t size;
//							const char* amfBuffer = NULL;
//							parser.Encode(root, &amfBuffer, size);
//							sp->SetDataLength(size);
//
//							sm->len = sizeof(sp->header) + sp->GetDataLength();
//							memcpy((char *)sp->data, amfBuffer, size);
//
//							sp->header.length = ntohl(sp->header.length);
//
//							ts->SendMessageByQueue(sm);
//
//							printf("# OnRecvMessage( send [len : %d], data : %s )\n", sm->len, sp->data);
//						}
//					}break;
//					default:break;
//					}
//
//					// 替换数据
//					mBuffer.len -= parsedLen;
//					if( mBuffer.len > 0 ) {
//						memcpy(mBuffer.buffer, mBuffer.buffer + parsedLen, mBuffer.len);
//						mBuffer.buffer[mBuffer.len] = '\0';
//					}
//
//				} else {
//					printf("# OnRecvMessage( all packet handle )\n");
//					break;
//				}
//			}
//
//		}


	}

	void OnSendMessage(TcpServer *ts, Message *m) {
//		printf("# OnSendMessage( len : %d )\n", m->len);
	}

	void OnTimeoutMessage(TcpServer *ts, Message *m) {

	}

	int fd;

	Buffer mBuffer;
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

	LogManager::GetLogManager()->Start(1000, 5, "./log_livechat_test");

	TcpServerObserverImp observer;
	TcpServer server;
	server.SetTcpServerObserver(&observer);
	server.Start(10, 9877, 1);

	int i = 0;
	while( true ) {
		/* do nothing here */
		char cmd[1024];
		char userId1[1024];
		char userId2[1024];

//		memset(cmd, 0, sizeof(cmd));
//		memset(userId1, 0, sizeof(userId1));
//		memset(userId2, 0, sizeof(userId2));
//		scanf("%s %s %s", cmd, userId1, userId2);
//		printf("# scanf( cmd : %s, userId1 : %s, userId2 : %s )\n", cmd, userId1, userId2);
//
//		if( strcmp(cmd, "kick") == 0 && strlen(userId1) > 0 && strlen(userId2) > 0 ) {
//			// kick
//			Message* sm = server.GetIdleMessageList()->PopFront();
//			if( sm != NULL ) {
//				sm->fd = observer.fd;
//
//				TransportProtocol* sp = (TransportProtocol*)sm->buffer;
//				sp->header.cmd = ntohl(TCMD_RECVDISCONNECTUSERVIDEO);
//				sp->header.seq = ntohl(i);
//
////				// json
////				sprintf((char *)sp->data, "{\"userId1\":\"%s\",\"userId2\":\"%s\"}", userId1, userId2);
////				sp->SetDataLength(strlen((const char*)sp->data));
//
//				// amf
//				amf_object_handle root = amf_object::Alloc();
//				root->type = DT_OBJECT;
//				amf_object_handle manId = amf_object::Alloc();
//				manId->type = DT_STRING;
//				manId->name = "manId";
//				manId->strValue = userId1;
//				amf_object_handle womanId = amf_object::Alloc();
//				womanId->type = DT_STRING;
//				womanId->name = "womanId";
//				womanId->strValue = userId2;
//				root->add_child(manId);
//				root->add_child(womanId);
//
//				AmfParser parser;
//				size_t size;
//				const char* amfBuffer = NULL;
//				parser.Encode(root, &amfBuffer, size);
//				sp->SetDataLength(size);
//				memcpy((char *)sp->data, amfBuffer, size);
//
//				sm->len = sizeof(sp->header) + sp->GetDataLength();
//				sp->header.length = ntohl(sp->header.length);
//
//				server.SendMessageByQueue(sm);
//
//				printf("# kick( send [len : %d], data : %s )\n", sm->len, sp->data);
//			}

//			i++;
//		}

		sleep(1);
	}

	return EXIT_SUCCESS;
}
