/*
 * fsv_reader.cpp
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

#define VERSION_STRING "1.0.0"
string fileString = "";

#define MIN(A, B) A<B?A:B

/** number of microseconds since 00:00:00 january 1, 1970 UTC */
typedef long long switch_time_t;
typedef struct file_header {
    int32_t version;
    char video_codec_name[32];
    char video_fmtp[128];
    uint32_t audio_rate;
    uint32_t audio_ptime;
    switch_time_t created;
} FILE_HEADER;

bool Parse(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	printf("############## fsv_reader ############## \n");
	printf("# Version : %s \n", VERSION_STRING);
	printf("# Build date : %s %s \n", __DATE__, __TIME__ );
	srand(time(0));

	/* Ignore SIGPIPE */
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, 0);

	Parse(argc, argv);

	if( fileString.length() > 0 ) {
		 FILE *file = fopen(fileString.c_str(), "r");

		 unsigned int ret = 0;

		 // read header
		 FILE_HEADER header;
		 ret = fread(&header, 1, sizeof(header), file);
		 if( ret == sizeof(header) ) {
			 printf("################ fsv header ################# \n");
			 printf("# version : %d \n", header.version);
			 printf("# video_codec_name : %s \n", header.video_codec_name);
			 printf("# video_fmtp : %s \n", header.video_fmtp);
			 printf("# audio_rate : %u \n", header.audio_rate);
			 printf("# audio_ptime : %u \n", header.audio_ptime);
			 printf("# created : %lld \n", header.created);
			 printf("################ fsv header end ################# \n");

			 int byte = 0;
			 char temp[4096];
			 bool bBreak = false;
			 unsigned int index = 0;

			 while ( true ) {
				 // read frame header
				 bBreak = false;
				 ret = fread(&byte, 1, sizeof(byte), file);
				 if( ret < sizeof(byte) ) {
					 if ( feof(file) == 0 ) {
						 // 不正常退出
						 printf("# read frame error \n");
						 break;
					 } else {
						 // 文件尾巴退出
						 printf("# no more frame \n");
						 break;
					 }
				 }

				 bool video = byte >> 31;
				 unsigned int len =  0x7FFF & byte;

				 if( video ) {
					 printf("################ frame[%u] ################# \n", index);
					 printf("# type : %s \n", video?"video":"audio");
					 printf("# len : %u \n", len);
				 }

				 // read frame data
				 unsigned int total = 0;
				 unsigned int once = MIN(sizeof(temp), len);
				 while( true ) {
					 ret = fread(temp, 1, once, file);
					 total += ret;
					 if( ret < once || total >= len ) {
						 if ( feof(file) == 0 ) {
							 // 不正常退出
							 break;
						 } else {
							 // 文件尾巴退出
							 break;
						 }
					 }

				 }

				 if( video ) {
					 printf("################ frame[%u] end ################# \n", index);
				 }

				 index++;

				 if( bBreak ) {
					 break;
				 }
			 }
		 } else {
			 printf("# read fsv header fail \n");
		 }

	} else {
		printf("# Usage : ./fsv_reader [ -f <fsv file> ] \n");
	}

	return EXIT_SUCCESS;
}

bool Parse(int argc, char *argv[]) {
	string key, value;
	for( int i = 1; (i + 1) < argc; i+=2 ) {
		key = argv[i];
		value = argv[i+1];

		if( key.compare("-f") == 0 ) {
			fileString = value;
		}
	}

	return true;
}
