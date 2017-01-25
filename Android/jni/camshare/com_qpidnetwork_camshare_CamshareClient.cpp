#include <com_qpidnetwork_camshare_CamshareClient.h>

#include <common/CommonFunc.h>
#include <JniGobalFunc.h>

#include "CamshareClient.h"

//// 旋转0度，裁剪，NV21转YUV420P
//static unsigned char* detailPic0(unsigned char* data, int imageWidth, int imageHeight, int newImageW, int newImageH) {
//	unsigned char* yuv_temp =new unsigned char[newImageW*newImageH*3/2];
//    int deleteW = (imageWidth - newImageW) / 2;
//    int deleteH = (imageHeight - newImageH) / 2;
//    //处理y 旋转加裁剪
//    int i, j;
//    int index = 0;
//    for (j = deleteH; j < imageHeight- deleteH; j++) {
//        for (i = deleteW; i < imageWidth- deleteW; i++)
//            yuv_temp[index++]= data[j * imageWidth + i];
//    }
//
//    //处理u
//    index= newImageW * newImageH;
//
//    for (i = imageHeight + deleteH / 2;i < imageHeight / 2 * 3 - deleteH / 2; i++)
//        for (j = deleteW + 1; j< imageWidth - deleteW; j += 2)
//            yuv_temp[index++]= data[i * imageWidth + j];
//
//    //处理v 旋转裁剪加格式转换
//    for (i = imageHeight + deleteH / 2;i < imageHeight / 2 * 3 - deleteH / 2; i++)
//        for (j = deleteW; j < imageWidth - deleteW; j += 2)
//            yuv_temp[index++]= data[i * imageWidth + j];
//    return yuv_temp;
//}
//
////旋转0度，裁剪，NV21转YUV420P（竖着录制后摄像头）
//static unsigned char* detailPic90(unsigned char* data, int imageWidth, int imageHeight, int newImageW, int newImageH) {
//
//	unsigned char* yuv_temp =new unsigned char[newImageW*newImageH*3/2];
//    int deleteW = (imageWidth - newImageH) / 2;
//    int deleteH = (imageHeight - newImageW) / 2;
//
//    int i, j;
//
//    for (i = 0; i < newImageH; i++){
//        for (j = 0; j < newImageW; j++){
//            yuv_temp[(newImageH- i) * newImageW - 1 - j] = data[imageWidth * (deleteH + j) + imageWidth - deleteW
//                    -i];
//        }
//    }
//
//    int index = newImageW * newImageH;
//    for (i = deleteW + 1; i< imageWidth - deleteW; i += 2)
//        for (j = imageHeight / 2 * 3 -deleteH / 2; j > imageHeight + deleteH / 2; j--)
//            yuv_temp[index++]= data[(j - 1) * imageWidth + i];
//
//    for (i = deleteW; i < imageWidth- deleteW; i += 2)
//        for (j = imageHeight / 2 * 3 -deleteH / 2; j > imageHeight + deleteH / 2; j--)
//            yuv_temp[index++]= data[(j - 1) * imageWidth + i];
//    return yuv_temp;
//}
//
////旋转0度，裁剪，NV21转YUV420P
//static unsigned char* detailPic180(unsigned char* data, int imageWidth, int imageHeight, int newImageW, int newImageH) {
//	unsigned char* yuv_temp =new unsigned char[newImageW*newImageH*3/2];
//	int deleteW = (imageWidth - newImageW) / 2;
//	int deleteH = (imageHeight - newImageH) / 2;
//	//处理y 旋转加裁剪
//	int i, j;
//	int index = newImageW * newImageH;
//	for (j = deleteH; j < imageHeight- deleteH; j++) {
//		for (i = deleteW; i < imageWidth- deleteW; i++)
//			yuv_temp[--index]= data[j * imageWidth + i];
//	}
//
//	//处理u
//	index= newImageW * newImageH * 5 / 4;
//
//	for (i = imageHeight + deleteH / 2;i < imageHeight / 2 * 3 - deleteH / 2; i++)
//		for (j = deleteW + 1; j< imageWidth - deleteW; j += 2)
//			yuv_temp[--index]= data[i * imageWidth + j];
//
//	//处理v 旋转裁剪加格式转换
//	index= newImageW * newImageH * 3 / 2;
//	for (i = imageHeight + deleteH / 2;i < imageHeight / 2 * 3 - deleteH / 2; i++)
//		for (j = deleteW; j < imageWidth- deleteW; j += 2)
//			yuv_temp[--index]= data[i * imageWidth + j];
//
//	return yuv_temp;
//}
//
////旋转0度，裁剪，NV21转YUV420P（竖着录制前摄像头）
//static unsigned char* detailPic270(unsigned char* data, int imageWidth, int imageHeight, int newImageW, int newImageH) {
//	unsigned char* yuv_temp =new unsigned char[newImageW*newImageH*3/2];
//    int deleteW = (imageWidth - newImageH) / 2;
//    int deleteH = (imageHeight - newImageW) / 2;
//    int i, j;
//    //处理y 旋转加裁剪
//    for (i = 0; i < newImageH; i++){
//        for (j = 0; j < newImageW; j++){
//            yuv_temp[i* newImageW + j] = data[imageWidth * (deleteH + j) + imageWidth - deleteW - i];
//        }
//    }
//
//    //处理u 旋转裁剪加格式转换
//    int index = newImageW * newImageH;
//    for (i = imageWidth - deleteW - 1;i > deleteW; i -= 2)
//        for (j = imageHeight + deleteH / 2;j < imageHeight / 2 * 3 - deleteH / 2; j++)
//            yuv_temp[index++]= data[(j) * imageWidth + i];
//
//    //处理v 旋转裁剪加格式转换
//
//    for (i = imageWidth - deleteW - 2;i >= deleteW; i -= 2)
//        for (j = imageHeight + deleteH / 2;j < imageHeight / 2 * 3 - deleteH / 2; j++)
//            yuv_temp[index++]= data[(j) * imageWidth + i];
//
//    return yuv_temp;
//}

static void decodeYUV420SP(unsigned char* rgb, const unsigned char* yuv420sp, int width, int height) {
	int frameSize = width * height;
	for (int j = 0, yp = 0; j < height; j++) {
		int uvp = frameSize + (j >> 1) * width/2, u = 0, v = 0;
		int vup = frameSize + frameSize/4 + (j >> 1) * width/2;
		for (int i = 0; i < width; i++, yp++) {
			int y = (0xff & ((int) yuv420sp[yp]));
			if (y < 0) y = 0;
			if ((i & 1) == 0) {

				//v = (0xff & yuv420sp[uvp++]) - 128;
				//u = (0xff & yuv420sp[uvp++]) - 128;

				u = (0xff & yuv420sp[uvp++]) - 128;
				v = (0xff & yuv420sp[vup++]) - 128;
			}
//			int y1192 = 1192 * y;
//			int r = (y1192 + 1634 * v);
//			int g = (y1192 - 833 * v - 400 * u);
//			int b = (y1192 + 2066 * u);
//			if (r < 0) r = 0; else if (r > 262143) r = 262143;
//			if (g < 0) g = 0; else if (g > 262143) g = 262143;
//			if (b < 0) b = 0; else if (b > 262143) b = 262143;
//			//rgb[yp] = 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00) | ((b >> 10) & 0xff);

			// 这是rgb的方法
//			int r = y + 1.4075*v;
//			int g = y - 0.3455*u - 0.7169*v;
//			int b = y + 1.779*u;
            // 这是rgb优化的方法
			int r = y + v + (v*103>>8);
			int g = y - (u*88>>8) - (v*183>>8);
			int b = y + u + (u*198>>8);

			r = r>255?255:(r<0?0:r);
			g = g>255?255:(g<0?0:g);
			b = b>255?255:(b<0?0:b);


			rgb[yp*2] = (((g & 0x1C) << 3) | (b >> 3));
			rgb[yp*2 + 1] = ((r & 0xF8) | (g>>5));
		}
	}
}

class CamshareClientListenerImp : public CamshareClientListener {
	void OnLogin(CamshareClient *client, bool success) {
		FileLog("CamshareClient",
				"Jni::OnLogin( "
				"client : %p, "
				"success : %s "
				")",
				client,
				success?"true":"false"
				);
		JNIEnv* env;
		jint iRet = JNI_ERR;
		gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
		if( env == NULL ) {
			iRet = gJavaVM->AttachCurrentThread((JNIEnv **)&env, NULL);
		}

		/* real callback java */
		gCallbackMap.Lock();
		CallbackMap::iterator itr = gCallbackMap.Find((jlong)client);
		if( itr != gCallbackMap.End() ) {
			jobject jCallback = itr->second;
			if( jCallback != NULL ) {
				jclass jCallbackCls = env->GetObjectClass(jCallback);
				if (jCallbackCls != NULL ) {
					string signure = "(Z)V";
					jmethodID jMethodID = env->GetMethodID(jCallbackCls, "onLogin",
							signure.c_str());

					if( jMethodID ) {
						env->CallVoidMethod(jCallback, jMethodID, success);
					}
				}
			}
		}
		gCallbackMap.Unlock();

		if( iRet == JNI_OK ) {
			gJavaVM->DetachCurrentThread();
		}
	}

	void OnMakeCall(CamshareClient *client, bool success) {
		FileLog("CamshareClient",
				"Jni::OnMakeCall( "
				"client : %p, "
				"success : %s "
				")",
				client,
				success?"true":"false"
				);
		JNIEnv* env;
		jint iRet = JNI_ERR;
		gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
		if( env == NULL ) {
			iRet = gJavaVM->AttachCurrentThread((JNIEnv **)&env, NULL);
		}

		/* real callback java */
		gCallbackMap.Lock();
		CallbackMap::iterator itr = gCallbackMap.Find((jlong)client);
		if( itr != gCallbackMap.End() ) {
			jobject jCallback = itr->second;
			if( jCallback != NULL ) {
				jclass jCallbackCls = env->GetObjectClass(jCallback);
				if (jCallbackCls != NULL ) {
					string signure = "(Z)V";
					jmethodID jMethodID = env->GetMethodID(jCallbackCls, "onMakeCall",
							signure.c_str());

					if( jMethodID ) {
						env->CallVoidMethod(jCallback, jMethodID, success);
					}
				}
			}
		}
		gCallbackMap.Unlock();

		if( iRet == JNI_OK ) {
			gJavaVM->DetachCurrentThread();
		}
		FileLog("CamshareClient",
				"Jni::OnMakeCall( end"
				")"
				);
	}

	void OnHangup(CamshareClient *client, const string& cause) {
		FileLog("CamshareClient",
				"Jni::OnHangup( "
				"client : %p, "
				"cause : %s "
				")",
				client,
				cause.c_str()
				);
		JNIEnv* env;
		jint iRet = JNI_ERR;
		gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
		if( env == NULL ) {
			iRet = gJavaVM->AttachCurrentThread((JNIEnv **)&env, NULL);
		}

		/* real callback java */
		gCallbackMap.Lock();
		CallbackMap::iterator itr = gCallbackMap.Find((jlong)client);
		if( itr != gCallbackMap.End() ) {
			jobject jCallback = itr->second;
			if( jCallback != NULL ) {
				jclass jCallbackCls = env->GetObjectClass(jCallback);
				if (jCallbackCls != NULL ) {
					string signure = "(Ljava/lang/String;)V";
					jmethodID jMethodID = env->GetMethodID(jCallbackCls, "onHangup",
							signure.c_str());

					if( jMethodID ) {
						jstring jcause = env->NewStringUTF(cause.c_str());
						env->CallVoidMethod(jCallback, jMethodID, jcause);
						env->DeleteLocalRef(jcause);
					}
				}
			}
		}
		gCallbackMap.Unlock();

		if( iRet == JNI_OK ) {
			gJavaVM->DetachCurrentThread();
		}
	}

	void OnDisconnect(CamshareClient* client) {
		FileLog("CamshareClient",
				"Jni::OnDisconnect( "
				"client : %p "
				")",
				client
				);

		JNIEnv* env;
		jint iRet = JNI_ERR;
		gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
		if( env == NULL ) {
			iRet = gJavaVM->AttachCurrentThread((JNIEnv **)&env, NULL);
		}

		/* real callback java */
		gCallbackMap.Lock();
		CallbackMap::iterator itr = gCallbackMap.Find((jlong)client);
		if( itr != gCallbackMap.End() ) {
			jobject jCallback = itr->second;
			if( jCallback != NULL ) {
				jclass jCallbackCls = env->GetObjectClass(jCallback);
				if (jCallbackCls != NULL ) {
					string signure = "()V";
					jmethodID jMethodID = env->GetMethodID(jCallbackCls, "onDisconnect",
							signure.c_str());

					if( jMethodID ) {
						env->CallVoidMethod(jCallback, jMethodID);
					}
				}
			}
		}
		gCallbackMap.Unlock();

		if( iRet == JNI_OK ) {
			gJavaVM->DetachCurrentThread();
		}
	}

	void OnRecvVideo(CamshareClient* client, const char* data, unsigned int size, unsigned int timestamp) {
		FileLog("CamshareClient",
				"Jni::OnRecvVideo( "
				"client : %p, "
				"size : %u, "
				"timestamp : %u "
				")",
				client,
				size,
				timestamp
				);

		JNIEnv* env;
		jint iRet = JNI_ERR;
		gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
		if( env == NULL ) {
			iRet = gJavaVM->AttachCurrentThread((JNIEnv **)&env, NULL);
		}

		/* real callback java */
		gCallbackMap.Lock();
		CallbackMap::iterator itr = gCallbackMap.Find((jlong)client);
		if( itr != gCallbackMap.End() ) {
			jobject jCallback = itr->second;
			if( jCallback != NULL ) {
				jclass jCallbackCls = env->GetObjectClass(jCallback);
				if (jCallbackCls != NULL ) {
					string signure = "([BII)V";
					jmethodID jMethodID = env->GetMethodID(jCallbackCls, "onRecvVideo",
							signure.c_str());

					if( jMethodID ) {
						jbyteArray jData = env->NewByteArray(size);
						env->SetByteArrayRegion(jData, 0, size, (jbyte*)data);
						env->CallVoidMethod(jCallback, jMethodID, jData, size, timestamp);
						env->DeleteLocalRef(jData);
					}
				}
			}
		}
		gCallbackMap.Unlock();

		if( iRet == JNI_OK ) {
			gJavaVM->DetachCurrentThread();
		}

		FileLog("CamshareClient",
				"Jni::OnRecvVideo(end "
				"client : %p, "
				"this : %p "
				")",
				client,
				this
				);
	}

	void OnRecvVideoSizeChange(CamshareClient* client, unsigned int width, unsigned int height) {
		FileLog("CamshareClient",
				"Jni::OnRecvVideoSizeChange( "
				"client : %p, "
				"width : %u, "
				"height : %u "
				")",
				client,
				width,
				height
				);

		JNIEnv* env;
		jint iRet = JNI_ERR;
		gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
		if( env == NULL ) {
			iRet = gJavaVM->AttachCurrentThread((JNIEnv **)&env, NULL);
		}

		/* real callback java */
		gCallbackMap.Lock();
		CallbackMap::iterator itr = gCallbackMap.Find((jlong)client);
		if( itr != gCallbackMap.End() ) {
			jobject jCallback = itr->second;
			if( jCallback != NULL ) {
				jclass jCallbackCls = env->GetObjectClass(jCallback);
				if (jCallbackCls != NULL ) {
					string signure = "(II)V";
					jmethodID jMethodID = env->GetMethodID(jCallbackCls, "onRecvVideoSizeChange",
							signure.c_str());

					if( jMethodID ) {
						env->CallVoidMethod(jCallback, jMethodID, width, height);
					}
				}
			}
		}
		gCallbackMap.Unlock();

		if( iRet == JNI_OK ) {
			gJavaVM->DetachCurrentThread();
		}
	}

	void OnCallVideo(CamshareClient* client, const char* data, unsigned int width, unsigned int height, unsigned int time) {
		FileLog("CamshareClient",
				"Jni::OnCallVideo(start"
				"width:%d,"
				"height:%d,"
				"time:%d"
				")",
				width,
				height,
				time
				);
		JNIEnv* env;
		jint iRet = JNI_ERR;
		gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
		if( env == NULL ) {
			iRet = gJavaVM->AttachCurrentThread((JNIEnv **)&env, NULL);
		}

		int newsize = width * height * 2;
		unsigned char* rgb = new unsigned char[newsize];
		decodeYUV420SP(rgb, (const unsigned char*)data, width, height);

		/* real callback java */
		gCallbackMap.Lock();
		CallbackMap::iterator itr = gCallbackMap.Find((jlong)client);
		if( itr != gCallbackMap.End() ) {
			jobject jCallback = itr->second;
			if( jCallback != NULL ) {
				jclass jCallbackCls = env->GetObjectClass(jCallback);
				if (jCallbackCls != NULL ) {
					string signure = "([BIIII)V";
					jmethodID jMethodID = env->GetMethodID(jCallbackCls, "onCallVideo",
							signure.c_str());

					if( jMethodID ) {
						FileLog("CamshareClient",
								"Jni::OnCallVideo("
								"start CallVoidMethod"
								")"
								);
						jbyteArray jData = env->NewByteArray(newsize);
						env->SetByteArrayRegion(jData, 0, newsize, (jbyte*)rgb);
						env->CallVoidMethod(jCallback, jMethodID, jData, width, height, newsize, time);
						env->DeleteLocalRef(jData);
						FileLog("CamshareClient",
								"Jni::OnCallVideo("
								"end CallVoidMethod"
								")"
								);
					}
				}
			}
		}

		gCallbackMap.Unlock();
		delete[] rgb;
		rgb = NULL;
		if( iRet == JNI_OK ) {
			gJavaVM->DetachCurrentThread();
		}

		FileLog("CamshareClient",
				"Jni::OnCallVideo(end"
				"width:%d,"
				"height:%d,"
				"time:%d,"
				"size:%d"
				")",
				width,
				height,
				time,
				newsize
				);
	}

};
CamshareClientListenerImp gCamshareClientListener;

JNIEXPORT void JNICALL Java_com_qpidnetwork_camshare_CamshareClient_GobalInit
  (JNIEnv *env, jclass cls) {

	H264Decoder::GobalInit();
}

JNIEXPORT jlong JNICALL Java_com_qpidnetwork_camshare_CamshareClient_Create
  (JNIEnv *env, jobject thiz, jobject callback) {
	CamshareClient* client = new CamshareClient();
	client->SetCamshareClientListener(&gCamshareClientListener);

	jobject jcallback = env->NewGlobalRef(callback);
	gCallbackMap.Insert((jlong)client, jcallback);

	return (long long)client;
}

JNIEXPORT void JNICALL Java_com_qpidnetwork_camshare_CamshareClient_Destroy
  (JNIEnv *env, jobject thiz, jlong jclient) {
	CamshareClient* client = (CamshareClient *)jclient;

	jobject jcallback = gCallbackMap.Erase((jlong)client);
	if( jcallback ) {
		env->DeleteGlobalRef(jcallback);
	}

	delete client;
}

JNIEXPORT void JNICALL Java_com_qpidnetwork_camshare_CamshareClient_SetLogDirPath
  (JNIEnv *env, jobject thiz, jstring jfilePath) {
	string path = JString2String(env, jfilePath);
	MakeDir(path);
	KLog::SetLogDirectory(path);
}

JNIEXPORT void JNICALL Java_com_qpidnetwork_camshare_CamshareClient_SetRecordFilePath
  (JNIEnv *env, jobject thiz, jlong jclient, jstring jfilePath) {
	CamshareClient* client = (CamshareClient *)jclient;
	client->SetRecordFilePath(JString2String(env, jfilePath));
}

JNIEXPORT jboolean JNICALL Java_com_qpidnetwork_camshare_CamshareClient_Login
  (JNIEnv *env, jobject thiz, jlong jclient, jstring juser, jstring jdomain, jstring jsite, jstring jsid, jint userType) {
	bool bFlag = false;
	CamshareClient* client = (CamshareClient *)jclient;

	if( userType >= UserType_Man && userType < UserType_Count ) {
		bFlag = client->Start(
				JString2String(env, jdomain),
				JString2String(env, juser),
				"",
				JString2String(env, jsite),
				JString2String(env, jsid),
				(UserType)userType
				);
	}

	return bFlag;
}

JNIEXPORT jboolean JNICALL Java_com_qpidnetwork_camshare_CamshareClient_MakeCall
  (JNIEnv *env, jobject thiz, jlong jclient, jstring jdest, jstring jserverId) {
	bool bFlag = false;
	CamshareClient* client = (CamshareClient *)jclient;

	bFlag = client->MakeCall(
			JString2String(env, jdest),
			JString2String(env, jserverId)
			);

	return bFlag;
}

/*
 * Class:     com_qpidnetwork_camshare_CamshareClient
 * Method:    Hangup
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_com_qpidnetwork_camshare_CamshareClient_Hangup
  (JNIEnv *env, jobject thiz, jlong jclient) {
	bool bFlag = false;
	CamshareClient* client = (CamshareClient *)jclient;

	bFlag = client->Hangup();

	return bFlag;
}

/*
 * Class:     com_qpidnetwork_camshare_CamshareClient
 * Method:    Stop
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_qpidnetwork_camshare_CamshareClient_Stop
  (JNIEnv *env, jobject thiz, jlong jclient) {
	CamshareClient* client = (CamshareClient *)jclient;
	client->Stop(true);
}

/*
 * Class:		com_qpidnetwork_camshare_CamshareClient
 * Method:  	SetVideoSize
 * Signature:	(JII)Z
 */
JNIEXPORT jboolean JNICALL Java_com_qpidnetwork_camshare_CamshareClient_SetSendVideoSize
  (JNIEnv *env, jobject thiz, jlong client, jint width, jint height)
{
	FileLog("CamshareClient",
			"Jni::SetVideoSize(start "
			"width:%d,"
			"heigh:%d"
			")",
			width,
			height
			);
	bool bFlag = false;
	CamshareClient* pclient = (CamshareClient *)client;

	bFlag = pclient->SetVideoSize(width, height);


	FileLog("CamshareClient",
			"Jni::SetVideoSize(end "
			")"

			);
	return bFlag;
}

/*
 * Class:		com_qpidnetwork_camshare_CamshareClient
 * Method:  	SetVideoRate
 * Signature:	(JI)Z
 */
JNIEXPORT jboolean JNICALL Java_com_qpidnetwork_camshare_CamshareClient_SetVideoRate
  (JNIEnv *env, jobject thiz, jlong client, jint rate){
	FileLog("CamshareClient",
			"Jni::SetVideoRate(start "
			"rate:%d"
			")",
			rate
			);
	bool bFlag = false;
	CamshareClient* pclient = (CamshareClient *)client;

	bFlag = pclient->setVideRate(rate);

	FileLog("CamshareClient",
			"Jni::SetVideoRate(end "
			")"
			);

	return bFlag;
}

/*
 * Class:		com_qpidnetwork_camshare_CamshareClient
 * Method:  	ChooseVideoFormate
 * Signature:	(J)[I
 */
JNIEXPORT jint JNICALL Java_com_qpidnetwork_camshare_CamshareClient_ChooseVideoFormate
  (JNIEnv *env, jobject thiz, jlong client, jintArray formates)
{
	FileLog("CamshareClient",
			"Jni::ChooseVideoFormate begin("
			")"
			);
	int result = -1;
	CamshareClient* pclient = (CamshareClient *)client;
	jsize len = env->GetArrayLength(formates);
	if (len <= 0){
		FileLog("CamshareClient",
				"Jni::ChooseVideoFormate"
				"Not video Formate"
				")"
				);
		return result;
	}
	// 获取java传递下来数组的长度
	jint* array = env->GetIntArrayElements(formates, 0);
	if (array == NULL){
		FileLog("CamshareClient",
				"Jni::ChooseVideoFormate"
				"fail not Formate"
				")"
				);
		return result;
	}

	result = pclient->ChooseVideoFormate(array, len, 0);

	FileLog("CamshareClient",
			"Jni::ChooseVideoFormate end("
			"videoFormate:%d"
			")",
			result
			);
	return result;
}

/*
 * Class:		com_qpidnetwork_camshare_CamshareClient
 * Method:  	SendVideoData
 * Signature:	(J[BIIII)Z
 */
JNIEXPORT jboolean JNICALL Java_com_qpidnetwork_camshare_CamshareClient_SendVideoData(JNIEnv *env, jobject thiz, jlong client, jbyteArray data, jint size, jlong timesp, jint width, jint heigh, jint direction){
	FileLog("CamshareClient",
			"Jni::SendVideoData(start "
			"size:%d,"
			"timesp:%lld,"
			"width:%d,"
			"heigh:%d,"
			"direction:%d"
			")",
			size,
			timesp,
			width,
			heigh,
			direction
			);
	bool bFlag = false;
	CamshareClient* pclient = (CamshareClient *)client;
	int len = env->GetArrayLength(data);
	unsigned char* pData = new unsigned char[len];
	env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte*>(pData));
	bFlag = pclient->InsertVideoData(pData, size, timesp, width, heigh, direction);
	delete[] pData;
	pData = NULL;

	FileLog("CamshareClient",
			"Jni::SendVideoData(end "
			"bFlag:%d,"
			")",
			bFlag
			);
	return bFlag;
}
