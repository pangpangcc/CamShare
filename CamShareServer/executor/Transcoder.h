/*
 * Transcoder.h
 *
 *  Created on: 2017年1月17日
 *      Author: max
 */

#ifndef EXECUTOR_TRANSCODER_H_
#define EXECUTOR_TRANSCODER_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
};
#endif

#include <string>
using namespace std;

class Transcoder {
public:
	Transcoder();
	virtual ~Transcoder();

	bool TranscodeH2642JPEG(const string& h264File, const string& jpegFile);

private:
	bool CreateJPEG(AVFrame* pFrame, int width, int height, const string& jpegFile);
};

#endif /* EXECUTOR_TRANSCODER_H_ */
