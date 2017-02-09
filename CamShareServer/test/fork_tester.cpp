/*
 * fork_test.cpp
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
#include <stdint.h>

#include <string>
#include <map>
using namespace std;

#include <common/KThread.h>

#define VERSION_STRING "1.0.0"

KThread* testThreads[4];

#define MIN(A, B) A<B?A:B

bool gStart = false;
void SignalFunc(int sign_no);

class TestRunnable : public KRunnable {
public:
	TestRunnable() {
	}
	virtual ~TestRunnable() {
	}
protected:
	void onRun() {
		while( gStart ) {
			system("whoami 2>&1>/dev/NULL ");
			usleep(10 * 1000);
		}
	}
};

int main(int argc, char *argv[]) {
	printf("############## fork_test ############## \n");
	printf("# Version : %s \n", VERSION_STRING);
	printf("# Build date : %s %s \n", __DATE__, __TIME__ );
//	srand(time(0));

	char temp[1204];
	memcpy(temp, '\0', 0);
	char* c = "1";
	printf("# c : %p", c);

	if( 1 ) {
		char* c = "2";
		printf("# c : %p", c);
	}
	printf("# c : %p", c);

	/* Ignore SIGPIPE */
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, 0);

//	gStart = true;
//	for(int i = 0; i < 4; i++) {
//		TestRunnable* runnable = new TestRunnable();
//		testThreads[i] = new KThread();
//		testThreads[i]->start(runnable);
//
//		usleep(100 * 1000);
//	}

//	while(gStart) {
//		TestRunnable* runnable = new TestRunnable();
//		KThread* thread = new KThread();
//		thread->start(runnable);
//		usleep(100 * 1000);
////		usleep(500 * 1000);
//	}

	while( gStart ) {
		sleep(1);
	}

	return EXIT_SUCCESS;
}

void SignalFunc(int sign_no) {
	printf("main( Get signal : %d )", sign_no);

	gStart = false;
	switch(sign_no) {
	default:{
//		for(int i = 0; i < 4; i++) {
//			KRunnable* runnable = testThreads[i]->stop();
//			if( runnable ) {
//				delete runnable;
//			}
//		}

		printf("# All threads exit \n");

		exit(EXIT_SUCCESS);
	}break;
	}
}
