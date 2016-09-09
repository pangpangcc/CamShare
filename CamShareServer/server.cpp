/*
 * server.cpp
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

#include <string>
#include <map>
using namespace std;

#include "CamShareMiddleware.h"

string sConf = "";  // 配置文件

bool Parse(int argc, char *argv[]);
void SignalFunc(int sign_no);

int main(int argc, char *argv[]) {
	printf("############## CamShare Middleware ############## \n");
	printf("# Version : %s \n", VERSION_STRING);
	printf("# Build date : %s %s \n", __DATE__, __TIME__);
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

	Parse(argc, argv);

	bool bFlag = false;
	CamShareMiddleware server;
	if( sConf.length() > 0 ) {
		bFlag = server.Run(sConf);
	} else {
		printf("# Usage : ./camshare-middleware [ -f <config file> ] \n");
		bFlag = server.Run("/etc/camshare-middleware.config");
	}

	while( bFlag && server.IsRunning() ) {
		/* do nothing here */
		sleep(5);
	}

	return EXIT_SUCCESS;
}

bool Parse(int argc, char *argv[]) {
	string key, value;
	for( int i = 1; (i + 1) < argc; i+=2 ) {
		key = argv[i];
		value = argv[i+1];

		if( key.compare("-f") == 0 ) {
			sConf = value;
		}
	}

	return true;
}

void SignalFunc(int sign_no) {
	LogManager::GetLogManager()->Log(LOG_ERR_SYS, "main( Get signal : %d )", sign_no);
	LogManager::GetLogManager()->LogFlushMem2File();

	switch(sign_no) {
	default:{
		exit(EXIT_SUCCESS);
	}break;
	}
}
