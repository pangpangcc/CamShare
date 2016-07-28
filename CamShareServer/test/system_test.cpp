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

#include <common/KThread.h>

#define MAX_PATH_LENGTH	512
#define VERSION_STRING "1.0.0"

int Test();
bool GetStandardTime(char* buffer, const char* srcTime);

class TestRunnable : public KRunnable {
public:
	TestRunnable() {
	}
	virtual ~TestRunnable() {
	}
protected:
	void onRun() {
		while(true) {
			int result = Test();
			if( 0 != result ) {
				printf("# result : %d, Fail \n", result);
				break;
			}
			usleep(1000 * 1000);
		}

	}
};

int main(int argc, char *argv[]) {
	printf("############## system test ############## \n");
	printf("# Version : %s \n", VERSION_STRING);
	printf("# Build date : %s %s \n", __DATE__, __TIME__ );
	srand(time(0));

	/* Ignore SIGPIPE */
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, 0);

	for(int i = 0; i < 10; i++) {
		TestRunnable* runnable = new TestRunnable();
		KThread* thread = new KThread();
		thread->start(runnable);
	}

	while(true) {
		usleep(1000 * 1000);
	}

	return EXIT_SUCCESS;
}

int Test() {
	char startStandardTime[32] = {0};
	GetStandardTime(startStandardTime, "20160722153011");
	char cmd[MAX_PATH_LENGTH] = {0};
	snprintf(cmd, sizeof(cmd), "%s '%s' '%s' '%s' '%s' '%s' '%s'",
			"/root/test/close_shell.sh",
			"/root/test/video_h264/P833388_1_20160722153011.h264",
			"/root/test/video_mp4/",
			"pic_h264",
			"P833388",
			"1",
			startStandardTime
			);
	// run shell
	int result = system(cmd);

	return result;
}

// 获取标准时间格式
bool GetStandardTime(char* buffer, const char* srcTime)
{
	bool result = false;
	if (NULL != srcTime && strlen(srcTime) == 14)
	{
		int i = 0;
		int j = 0;
		for (; srcTime[i] != 0; i++, j++)
		{
			if (i == 4
				|| i == 6)
			{
				// 日期分隔
				buffer[j++] = '-';
			}
			else if (i == 8)
			{
				// 日期与时间之间的分隔
				buffer[j++] = ' ';
			}
			else if (i == 10
					|| i == 12)
			{
				// 时间分隔
				buffer[j++] = ':';
			}
			buffer[j] = srcTime[i];
		}
		buffer[j] = 0;

		result = true;
	}
	return result;
}
