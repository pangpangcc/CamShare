#include <com_qpidnetwork_camshare_CamshareClient.h>

#include <common/CommonFunc.h>
#include <JniGobalFunc.h>

#include "CamshareClient.h"

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
				"client : %p, "
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
};
CamshareClientListenerImp gCamshareClientListener;

JNIEXPORT void JNICALL Java_com_qpidnetwork_camshare_CamshareClient_GobalInit
  (JNIEnv *env, jclass cls) {

	H264Decoder::GobalInit();
	RemoveDir("/sdcard/camshare");
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
	KLog::SetLogDirectory(JString2String(env, jfilePath));
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
