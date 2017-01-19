/*
 * transcoder_tester.cpp
 *
 *  Created on: 2017年1月17日
 *      Author: max
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <string>
using namespace std;

#include <executor/Transcoder.h>

int main(int argc, char *argv[]) {
	printf("############## transcoder tester ############## \n");
	srand(time(0));

	Transcoder tester;
	tester.TranscodeH2642JPEG("C348348.h264", "C348348.jpg");
}
