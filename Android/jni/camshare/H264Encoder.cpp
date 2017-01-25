/*
 * H264Encoder.cpp
 *
 *  Created on: 2016年12月08日
 *      Author: alex
 */

#include <H264Encoder.h>
#include <common/KLog.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

#include <common/Arithmetic.h>

H264Encoder::H264Encoder() {
	// TODO Auto-generated constructor stub
	mCodec = NULL;
	mContext = NULL;
	mGotPicture = 0;
	mLen = 0;
	mFilePath = "";

	// 采集的宽
	mCaptureWidth = 320;
	// 采集的高
	mCaptureHeight = 240;
	// 采集格式
	mCaptureFormat = AV_PIX_FMT_NV21;

	// 编码的宽
	mEncodeWidth = 176;
	// 编码的高
	mEncodeHeight = 144;
	// 编码格式
	mEncodeId = AV_CODEC_ID_H264;

	// 采集裁剪后的y大小
	my_length = mCaptureWidth * mCaptureHeight;
	// 采集裁剪后U或V的大小
	muv_length = mCaptureWidth * mCaptureHeight/4;
    // 采集频率
	mFramerate = 6;
}

H264Encoder::~H264Encoder() {
	// TODO Auto-generated destructor stub
}

void H264Encoder::GobalInit() {
	avcodec_register_all();
}

bool H264Encoder::Create() {
	FileLog("CamshareClient",
			"H264Encoder::Create(start"
			")"
			);
	mCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!mCodec) {
		// codec not found
		FileLog("CamshareClientError",
				"H264Encoder::Create( "
				"this : %p, "
				"fail, "
				"codec not found "
				")",
				this
				);

		return false;
	}

	mContext = avcodec_alloc_context3(mCodec);
	if (!mContext){
		// codec not found
		FileLog("CamshareClientError",
				"H264Encoder::Create( "
				"this : %p, "
				"fail, "
				"mContext not found "
				")",
				this
				);

		return false;
	}

	mContext->codec_type = AVMEDIA_TYPE_VIDEO;
	mContext->bit_rate = 64000;
	mContext->rc_max_rate = 64000;
	mContext->rc_min_rate = 64000;
	mContext->width    = mEncodeWidth;
	mContext->height   = mEncodeHeight;

	 mContext->bit_rate_tolerance = 64000;
	 mContext->rc_buffer_size = 64000;
	 mContext->rc_initial_buffer_occupancy = mContext->rc_buffer_size*3/4;
	 mContext->rc_buffer_aggressivity = (float)1.0;
	 mContext->rc_initial_cplx = (float)0.5;

	mContext->time_base.num = 1;
	mContext->time_base.den = mFramerate;
	mContext->gop_size      = mFramerate;
	mContext->max_b_frames  = 0;
	mContext->pix_fmt       = AV_PIX_FMT_YUV420P;
	mContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	mContext->keyint_min    = mFramerate;
	//mContext->qmin = 10;
	//mContext->qmax = 51;
	mContext->level = 21;
	//mContext->refs = 4;


	AVDictionary *opts = NULL;
	//if (mEncodeId == AV_CODEC_ID_H264){
		// 编译速度
		//av_opt_set(mContext->priv_data, "preset", "ultrafast", 0);
		av_opt_set(mContext->priv_data, "preset", "superfast", 0);
		// 零延时
	    av_opt_set(mContext->priv_data,"tune", "zerolatency", 0);

	  // av_opt_set(mContext->priv_data, "profile", "baseline", 0);

	   av_dict_set(&opts, "profile", "baseline", 0);
	   //av_dict_set(&opts, "rotate", "90", 0);
	//}

	if (avcodec_open2(mContext, mCodec, &opts) < 0){
		// codec not found
		FileLog("CamshareClientError",
				"H264Encoder::Create( "
				"this : %p, "
				"fail, "
				"open avcodec is fail"
				")",
				this
				);
		return false;
	}

	FileLog("CamshareClient",
			"H264Encoder::Create(end"
			")"
			);
	return true;
}

void H264Encoder::Destroy() {
	FileLog("CamshareClient",
			"H264Encoder::Destroy(start"
			"this:%p"
			")",
			this
			);

	if( mContext ) {
		avcodec_close(mContext);
		av_free(mContext);
		mContext = NULL;
	}

	mCodec = NULL;
	FileLog("CamshareClient",
			"H264Encoder::Destroy(end"
			"this:%p"
			")",
			this
			);

}

bool H264Encoder::EncodeFrame(
		const unsigned char* srcData,
		unsigned int dataLen,
		unsigned char* desData,
		int& desDataLen,
		bool& isKeyFrame,
		int width,
		int height
		) {

	FileLog("CamshareClient",
			"H264Encoder::EncodeFrame(start"
			"width:%d,"
			"height:%d,"
			"mCaptureWidth:%d,"
			"mCaptureHeight:%d,"
			"mContext->width:%d,"
			"mContext->height:%d"
			")",
			width,
			height,
			mCaptureWidth,
			mCaptureHeight,
			mContext->width,
			mContext->height
			);

	if(width <= 0 || height <= 0)
	{
		return false;
	}
	if(width != mCaptureWidth || height != mCaptureHeight)
	{
		FileLog("CamshareClient",
				"H264Encoder::EncodeFrame("
				"mCaptureWidth or mCaptureHeight set"
				"width:%d,"
				"height:%d,"
				"mCaptureWidth:%d,"
				"mCaptureHeight:%d,"
				"mContext->width:%d,"
				"mContext->height:%d"
				")",
				width,
				height,
				mCaptureWidth,
				mCaptureHeight,
				mContext->width,
				mContext->height
				);
		SetCaptureVideoSize(width, height);
	}

	FileLog("CamshareClient",
			"H264Encoder::EncodeFrame(1"
			")"
			);
	bool bFlag = false;
	int ret;
	int enc_got_frame = 0;
	// new一个采集一帧视频的容器
	AVFrame * mFrameYUV = av_frame_alloc();
	// new一个一帧视频的大小
	uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, mCaptureWidth, mCaptureHeight));
	FileLog("CamshareClient",
			"H264Encoder::EncodeFrame(2"
			")"
			);
	// 将out_buffer放进pFrameYUV里面
	avpicture_fill((AVPicture *)mFrameYUV, out_buffer, PIX_FMT_YUV420P, mCaptureWidth, mCaptureHeight);
	FileLog("CamshareClient",
			"H264Encoder::EncodeFrame(3"
			")"
			);

	 // 安卓摄像头数据为NV21格式，此处将其转换为YUV420P格式
	 //jbyte* in = (jbyte*)(*env)->GetByteArrayElements(env, yuv, 0);

	 uint8_t * in = ( uint8_t *)srcData;
	 memcpy(mFrameYUV->data[0], in, my_length);
	 memcpy(mFrameYUV->data[1], in + my_length, muv_length);
	 memcpy(mFrameYUV->data[2], in + my_length + muv_length, muv_length);
//	 for(int i = 0; i<muv_length; i++)
//	 {
//	 	 *(mFrameYUV->data[2] + i) = *(in + my_length + i*2);
//	 	 *(mFrameYUV->data[1] + i) = *(in + my_length + i*2 + 1);
//	 }

	 mFrameYUV->format = AV_PIX_FMT_YUV420P;
	 mFrameYUV->width  = mCaptureWidth;
	 mFrameYUV->height = mCaptureHeight;
//	 mFrameYUV->linesize[0] = mCaptureWidth;
//	 mFrameYUV->linesize[1] = mCaptureWidth/2;
//	 mFrameYUV->linesize[2] = mCaptureWidth/2;
		AVFrame *rgbFrame = av_frame_alloc();
//		int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, mContext->width, mContext->height);
//		uint8_t* buffer = (uint8_t *)av_malloc(numBytes);
//		FileLog("CamshareClient1",
//				"H264Encoder::EncodeFrame(avpicture_fill"
//				")"
//
//				);
//		avpicture_fill((AVPicture *)rgbFrame, (uint8_t *)buffer, AV_PIX_FMT_YUV420P,
//				mContext->width, mContext->height);
//
		FileLog("CamshareClient",
				"H264Encoder::EncodeFrame(4"
				")"
				);
		uint8_t *buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, mContext->width, mContext->height));
		FileLog("CamshareClient",
				"H264Encoder::EncodeFrame(5"
				")"
				);
		// 将out_buffer放进pFrameYUV里面
		avpicture_fill((AVPicture *)rgbFrame, buffer, PIX_FMT_YUV420P, mContext->width, mContext->height);
		FileLog("CamshareClient",
				"H264Encoder::EncodeFrame(6"
				")"
				);

		struct SwsContext *img_convert_ctx = sws_getContext(mCaptureWidth, mCaptureHeight, AV_PIX_FMT_YUV420P,
				mContext->width, mContext->height, AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
		FileLog("CamshareClient",
				"H264Encoder::EncodeFrame(7"
				")"
				);

		if (img_convert_ctx == NULL){
			FileLog("CamshareClient",
					"H264Encoder::EncodeFrame(sws_scale"
					"img_convert_ctx is NULL"
					")"

					);
			return false;
		}
		FileLog("CamshareClient",
				"H264Encoder::EncodeFrame(8"
				")"
				);
		sws_scale(img_convert_ctx,
				mFrameYUV->data,
				mFrameYUV->linesize, 0, mCaptureHeight, rgbFrame->data,
				rgbFrame->linesize);
		FileLog("CamshareClient",
				"H264Encoder::EncodeFrame(9"
				")"
				);
		sws_freeContext(img_convert_ctx);
		FileLog("CamshareClient",
				"H264Encoder::EncodeFrame(10"
				")"
				);
		img_convert_ctx = NULL;


//	 // 编码储存容器
	 AVPacket mEnc_pkt;
	 mEnc_pkt.data = NULL;
	 mEnc_pkt.size = 0;
	 av_init_packet(&mEnc_pkt);
		FileLog("CamshareClient",
				"H264Encoder::EncodeFrame(11"
				")"
				);
	ret = avcodec_encode_video2(mContext, &mEnc_pkt, rgbFrame, &enc_got_frame);
	FileLog("CamshareClient",
			"H264Encoder::EncodeFrame(12"
			")"
			);
	 if (enc_got_frame == 1){

//		 // 帧的类型
		 AVPictureType pictType = mContext->coded_frame->pict_type;
		 // 是否是关键帧 ， I帧和key_frame为关键帧
		 //bool isKey_frame       = mContext->coded_frame->key_frame;

		 //AVPictureType pictType = mFrameYUV->pict_type;
//		 // 是否是关键帧 ， I帧和key_frame为关键帧
		 bool isKey_frame       = false;
		 if ((mEnc_pkt.data[4] & 0x1f) == 5 || (mEnc_pkt.data[3] & 0x1f) == 5)
		 {
			 isKey_frame = true;
		 }
		 int datalen = 0;

			FileLog("CamshareClient",
					"Jni::EncodeFrame"
					"mEnc_pkt.size:%d,"
					"mEnc_pkt[0]:%x,"
					"mEnc_pkt[1]:%x,"
					"mEnc_pkt[2]:%x,"
					"mEnc_pkt[3]:%x,"
					"mEnc_pkt[4]:%x,"
					"ppsAndpspLen:%d,"
					"pictType:%d,"
					"framepictType:%d"
					")",
					mEnc_pkt.size,
					mEnc_pkt.data[0],
					mEnc_pkt.data[1],
					mEnc_pkt.data[2],
					mEnc_pkt.data[3],
					mEnc_pkt.data[4],
					mContext->extradata_size,
					pictType,
					mFrameYUV->pict_type
					);

		 if (pictType == AV_PICTURE_TYPE_I && isKey_frame)
		 {
			 isKeyFrame = true;
			 //char data2[2000] = {0};
			 memcpy(desData, mContext->extradata, mContext->extradata_size);
			 datalen = mContext->extradata_size;

		 }
		 else
		 {
			 isKeyFrame = false;
		 }

			FileLog("CamshareClient",
					"H264Encoder::EncodeFrame(13"
					")"
					);
		 memcpy(desData + datalen, mEnc_pkt.data, mEnc_pkt.size);
			FileLog("CamshareClient",
					"H264Encoder::EncodeFrame(14"
					")"
					);
		 desDataLen = mEnc_pkt.size + datalen;

//			// 录制文件
//			if( mFilePath.length() > 0 ) {
//				FILE* file = fopen(mFilePath.c_str(), "a+b");
//			    if( file != NULL ) {
//			    	int iLen = -1;
//			    	fseek(file, 0L, SEEK_END);
//					iLen = fwrite(desData, 1, desDataLen, file);
//			    	fclose(file);
//			    }
//			}

		bFlag = true;
		av_free_packet(&mEnc_pkt);
	 }
	 else
	 {
			FileLog("CamshareClientError",
					"H264Encoder::EncodeFrame("
					")"

					);
	 }

	FileLog("CamshareClient",
			"H264Encoder::EncodeFrame(end"
			")"
			);
	av_free(mFrameYUV);
	 av_free(out_buffer);
	 mFrameYUV = NULL;
	 av_free(rgbFrame);
	 av_free(buffer);
	 rgbFrame = NULL;
	return bFlag;
}

void H264Encoder::SetRecordFile(const string& filePath) {
	mFilePath = filePath;
	FileLog("CamshareClient",
			"H264Encoder::SetRecordFile( start"
			"this : %p, "
			"filePath : %s "
			")",
			this,
			filePath.c_str()
			);
}

// 设置采集参数
void H264Encoder::SetCaptureParam(int width, int height, int format)
{
	// 采集的宽
	mCaptureWidth = width;
	// 采集的高
	mCaptureHeight = height;
	// 采集格式
	mCaptureFormat = format;

	// 默认采集高宽和编码高宽一样，编码格式为h264；这些以后都可以改
	// 编码的宽
	//mEncodeWidth = width;
	// 编码的高
	//mEncodeHeight = height;
	// 编码格式
	mEncodeId = AV_CODEC_ID_H264;

	my_length = width * height;
	muv_length = width * height/4;

}


// 设置发送视频高宽
bool H264Encoder::SetVideoSize(int width, int heigt)
{
	bool result = false;
	// 编码的宽
	mEncodeWidth = width;
	// 编码的高
	mEncodeHeight = heigt;

	if (mContext)
	{
		mContext->width    = mEncodeWidth;
		mContext->height   = mEncodeHeight;
	}
	result = true;
	return result;
}

bool H264Encoder::yuv420torgb565(unsigned char* yuv, int width, int heigt, unsigned char* rgb, int& frameLen)
{

	bool result = false;

	AVFrame * FrameYUV = av_frame_alloc();
	// new一个一帧视频的大小
	uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, width, heigt));
	// 将out_buffer放进pFrameYUV里面
	avpicture_fill((AVPicture *)FrameYUV, out_buffer, PIX_FMT_YUV420P, width, heigt);

	 // 安卓摄像头数据为NV21格式，此处将其转换为YUV420P格式
	 //jbyte* in = (jbyte*)(*env)->GetByteArrayElements(env, yuv, 0);

	 uint8_t * in = ( uint8_t *)yuv;
	 int ylen = width * heigt;
	 int ulen =  width * heigt/4;
	 memcpy(FrameYUV->data[0], in, ylen);
	 memcpy(FrameYUV->data[1], in + ylen, ulen);
	 memcpy(FrameYUV->data[2], in + ylen + ulen, ulen);
//	 for(int i = 0; i<muv_length; i++)
//	 {
//	 	 *(mFrameYUV->data[2] + i) = *(in + my_length + i*2);
//	 	 *(mFrameYUV->data[1] + i) = *(in + my_length + i*2 + 1);
//	 }

	 FrameYUV->format = AV_PIX_FMT_YUV420P;
	 FrameYUV->width  = width;
	 FrameYUV->height = heigt;

	AVFrame *rgbFrame = av_frame_alloc();
	int numBytes = avpicture_get_size(PIX_FMT_RGB565, width, heigt);
	uint8_t* buffer = (uint8_t *)av_malloc(numBytes);
	avpicture_fill((AVPicture *)rgbFrame, (uint8_t *)buffer, PIX_FMT_RGB565,
			width, heigt);
	struct SwsContext *img_convert_ctx = sws_getContext(width, heigt, AV_PIX_FMT_YUV420P,
			width, heigt, PIX_FMT_RGB565, SWS_POINT, NULL, NULL, NULL);

	if (img_convert_ctx == NULL){
		av_free(FrameYUV);
		 av_free(rgbFrame);
		return false;
	}

	sws_scale(img_convert_ctx,
			(const uint8_t* const *)FrameYUV->data,
			FrameYUV->linesize, 0, heigt, rgbFrame->data,
			rgbFrame->linesize);

	frameLen = numBytes;

	memcpy(rgb, rgbFrame->data[0], frameLen);



	av_free(FrameYUV);
	 av_free(rgbFrame);
	sws_freeContext(img_convert_ctx);
	img_convert_ctx = NULL;

	result = true;
	return result;
}


// 设置采集高宽
bool H264Encoder::SetCaptureVideoSize(int width, int heigt)
{
	bool result = false;
	// 采集的宽
	mCaptureWidth = width;
	// 采集的高
	mCaptureHeight = heigt;

	// 编码的宽
	//mEncodeWidth = width;
	// 编码的高
	//mEncodeHeight = heigt;

	my_length = width * heigt;
	muv_length = width * heigt/4;

//	if (mContext)
//	{
//		mContext->width    = mEncodeWidth;
//		mContext->height   = mEncodeHeight;
//	}
	result = true;
	return result;
}

// 设置采集率
bool H264Encoder::SetCaptureVideoFramerate(int rate)
{
	bool result = false;
	// 采集率
	mFramerate = rate;
	if (mContext)
	{
		mContext->time_base.num = 1;
		mContext->time_base.den = mFramerate;
		mContext->gop_size      = mFramerate;
		mContext->keyint_min    = mFramerate;
	}
	result = true;
	return result;
}
