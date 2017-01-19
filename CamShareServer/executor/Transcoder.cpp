/*
 * Transcoder.cpp
 *
 *  Created on: 2017年1月17日
 *      Author: max
 */

#include "Transcoder.h"

Transcoder::Transcoder() {
	// TODO Auto-generated constructor stub
    // 注册编解码器
    av_register_all();
}

Transcoder::~Transcoder() {
	// TODO Auto-generated destructor stub
}

bool Transcoder::TranscodeH2642JPEG(const string& h264File, const string& jpegFile) {
	bool bFlag = true;

    int videoStream = -1;
    AVCodecContext *pCodecCtx = NULL;
    AVFormatContext *pFormatCtx = NULL;
    AVCodec *pCodec = NULL;
    AVFrame *pFrame, *pFrameRGB = NULL;
    struct SwsContext *pSwsCtx = NULL;

    AVPacket packet;
    int frameFinished;
    int pictureSize;
    uint8_t *outBuff = NULL;

    // 分配AVFormatContext
    pFormatCtx = avformat_alloc_context();

    // 打开视频文件
    if( avformat_open_input(&pFormatCtx, h264File.c_str(), NULL, NULL) != 0 ) {
    	printf("# Transcoder::TranscodeH2642JPEG( avformat_open_input fail ) \n");
    	bFlag = false;
    }

    if( bFlag ) {
        // 获取流信息
        if( avformat_find_stream_info(pFormatCtx, NULL) < 0 ) {
        	bFlag = false;
        }
    }

    if( bFlag ) {
        //获取视频流
        for( int i = 0; i < pFormatCtx->nb_streams; i++ ) {
            if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ) {
                videoStream = i;
                break;
            }
        }

        bFlag = (videoStream != -1);
    }

    if( bFlag ) {
        // 寻找解码器
        pCodecCtx = pFormatCtx->streams[videoStream]->codec;
        pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
        if( pCodec == NULL ) {
        	bFlag = false;
        }
    }

    if( bFlag ) {
        // 打开解码器
        if( avcodec_open2(pCodecCtx, pCodec, NULL) < 0 ) {
        	bFlag = false;
        }
    }

    if( bFlag ) {
        // 为每帧图像分配内存
        pFrame = avcodec_alloc_frame();
        pFrameRGB = avcodec_alloc_frame();
        if( (pFrame == NULL) || (pFrameRGB == NULL) ) {
        	bFlag = false;
        }
    }

    if( bFlag ) {
        // 确定图片尺寸
        pictureSize = avpicture_get_size(PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height);
        outBuff = (uint8_t*)av_malloc(pictureSize);
        if( outBuff == NULL ) {
        	bFlag = false;
        }
    }

    if( bFlag ) {
        avpicture_fill((AVPicture *)pFrameRGB, outBuff, PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height);
        //设置图像转换上下文
        pSwsCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
            pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUVJ420P,
            SWS_BICUBIC, NULL, NULL, NULL);
    }

    if( bFlag ) {
        while( av_read_frame(pFormatCtx, &packet) >= 0 ) {
            if( packet.stream_index == videoStream ) {
                avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

                if( frameFinished ) {
                    // 保存为jpeg时不需要反转图像
                	bFlag = CreateJPEG(pFrame, pCodecCtx->width, pCodecCtx->height, jpegFile);

                	// 只解一帧
                	break;
                }
            }
            av_free_packet(&packet);
        }
    }

    if( pSwsCtx ) {
    	sws_freeContext(pSwsCtx);
    }
    if( pFrame ) {
    	av_free(pFrame);
    }
    if( pFrameRGB ) {
    	av_free(pFrameRGB);
    }
    if( pCodecCtx ) {
    	avcodec_close(pCodecCtx);
    }
    if( pFormatCtx ) {
    	avformat_close_input(&pFormatCtx);
    }

    if( outBuff ) {
    	av_free(outBuff);
    }

	return bFlag;
}

bool Transcoder::CreateJPEG(AVFrame* pFrame, int width, int height, const string& jpegFile) {
	bool bFlag = true;

    AVStream* pAVStream = NULL;
    AVCodecContext* pCodecCtx = NULL;
    AVCodec* pCodec = NULL;

    // 分配AVFormatContext对象
    AVFormatContext* pFormatCtx = avformat_alloc_context();

    // 设置输出文件格式
    pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);

    // 创建并初始化一个和该url相关的AVIOContext
    if( avio_open(&pFormatCtx->pb, jpegFile.c_str(), AVIO_FLAG_READ_WRITE) < 0) {
    	bFlag = false;
    }

    if( bFlag ) {
        // 构建一个新stream
        pAVStream = avformat_new_stream(pFormatCtx, 0);
        if( pAVStream == NULL ) {
        	bFlag = false;
        }
    }

    if( bFlag ) {
        // 设置该stream的信息
        pCodecCtx = pAVStream->codec;
        pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
        pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        pCodecCtx->pix_fmt = PIX_FMT_YUVJ420P;
        pCodecCtx->width = width;
        pCodecCtx->height = height;
        pCodecCtx->time_base.num = 1;
        pCodecCtx->time_base.den = 25;

        // Begin Output some information
        av_dump_format(pFormatCtx, 0, jpegFile.c_str(), 1);
        // End Output some information
    }

    if( bFlag ) {
        // 查找解码器
        pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
        if( !pCodec ) {
        	bFlag = false;
        }
    }

    if( bFlag ) {
        // 设置pCodecCtx的解码器为pCodec
        if( avcodec_open2(pCodecCtx, pCodec, NULL) < 0 ) {
        	bFlag = false;
        }
    }

    if( bFlag ) {
    	bFlag = false;

        // Write Header
        avformat_write_header(pFormatCtx, NULL);
        int y_size = pCodecCtx->width * pCodecCtx->height;

        // 给AVPacket分配足够大的空间
        AVPacket pkt;
        av_new_packet(&pkt, y_size * 3);

        int got_picture = 0;
        int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
        if( ret >= 0 ) {
            if( got_picture == 1 ) {
            	bFlag = (av_write_frame(pFormatCtx, &pkt) >= 0);
            }
        }

        av_free_packet(&pkt);

        // Write Trailer
        av_write_trailer(pFormatCtx);
    }

    if( pAVStream ) {
        avcodec_close(pAVStream->codec);
    }
    if( pCodecCtx ) {
    	avcodec_close(pCodecCtx);
    }
    if( pFormatCtx->pb ) {
    	avio_close(pFormatCtx->pb);
    }
    if( pFormatCtx ) {
    	avformat_free_context(pFormatCtx);
    }

    return bFlag;
}
