package com.qpidnetwork.camshare;

import android.util.Log;

public class CamshareClient implements CamshareClientJniCallback {
	static {
		System.loadLibrary("camshareclient");
		Log.i("CamshareClient", "CamshareClient Load Library");
		GobalInit();
	}
	
	/**
	 * 全局初始化
	 */
	private native static void GobalInit();
	
	/**
	 * C实例指针
	 */
	private long client = -1;
	
	/**
	 * 渲染器, 用于画图
	 */
	private VideoRendererInterface renderer;
	
	/**
	 * 监听器
	 */
	private CamshareClientCallback callback;
	
	/**
	 * 设置日志文件夹路径
	 * @param logFilePath	路径
	 */
	static public native void SetLogDirPath(String filePath);
	
	/**
	 * 初始化实例
	 * @param renderer	渲染器, 用于画图
	 * @param callback	回调处理
	 * @return 成功失败
	 */
	public boolean Create(VideoRendererInterface renderer, CamshareClientCallback callback) {
		this.renderer = renderer;
		this.callback = callback;
		client = Create(this);
		return client != -1;
	}
	/**
	 * 创建实例
	 * @param callback	回调处理
	 * @return	实例指针
	 */
	private native long Create(CamshareClientJniCallback callback);
	
	/**
	 * 销毁实例
	 */
	public void Destroy() {
		if( client != -1 ) {
			Destroy(client);
		}
	}
	/**
	 * 销毁实例
	 * @param client	实例指针
	 */
	private native void Destroy(long client);
	
	/**
	 * 设置录制H264视频文件夹路径
	 * @param logFilePath	路径
	 */
	public void SetRecordFilePath(String filePath) {
		SetRecordFilePath(this.client, filePath);
	}
	private native void SetRecordFilePath(long client, String filePath);
	
    /**
     * 国家
     */
    public enum UserType {
    	Man,
    	Lady
    };
    
    /**
     * 连接并登录
     * @param user
     * @param domain
     * @param siteId
     * @param sid
     * @param userType
     * @return
     */
	public boolean Login(String user, String domain, String siteId, String sid, UserType userType) {
		Log.i("CamshareClientJava", String.format("Login( user : %s, domain : %s, siteId : %s, sid : %s, userType : %d )", user, domain, siteId, sid, userType.ordinal()));
		return Login(client, user, domain, siteId, sid, userType.ordinal());
	}
	private native boolean Login(long client, String user, String domain, String siteId, String sid, int userType);

	/**
	 * 拨号进入会议室
	 * @param dest
	 * @param serverId
	 * @return 成功/失败
	 */
	public boolean MakeCall(String dest, String serverId) {
		Log.i("CamshareClientJava", String.format("MakeCall( dest : %s, serverId : %s )", dest, serverId));
		return MakeCall(client, dest, serverId);
	}
	private native boolean MakeCall(long client, String dest, String serverId);
	
	/**
	 * 挂断会话
	 * @return 成功/失败
	 */
	public boolean Hangup() {
		Log.i("CamshareClientJava", String.format("Hangup()"));
		return Hangup(client);
	}
	private native boolean Hangup(long client);
	
	/**
	 * 断开连接
	 */
	public void Stop() {
		Log.i("CamshareClientJava", String.format("Stop()"));
		Stop(client);
	}
	private native void Stop(long client);
	
	@Override
	public void onLogin(boolean success) {
		// TODO Auto-generated method stub
		Log.i("CamshareClientJava", String.format("onLogin( success : %s )", success?"true":"false"));
		this.callback.onLogin(this, success);
	}

	@Override
	public void onMakeCall(boolean success) {
		// TODO Auto-generated method stub
		Log.i("CamshareClientJava", String.format("onMakeCall( success : %s )", success?"true":"false"));
		this.callback.onMakeCall(this, success);
	}
	
	@Override
	public void onHangup(String cause) {
		// TODO Auto-generated method stub
		Log.i("CamshareClientJava", String.format("onHangup( cause : %s )", cause));
		this.callback.onHangup(this, cause);
	}
	
	@Override
	public void onRecvVideo(byte[] data, int size, int timestamp) {
		// TODO Auto-generated method stub
		renderer.DrawFrame(data, size, timestamp);
	}
	
	@Override
	public void onRecvVideoSizeChange(int width, int height) {
		Log.i("CamshareClientJava", String.format("onRecvVideoSizeChange( width : %d, height : %d )", width, height));
		this.callback.onRecvVideoSizeChange(width, height);
	}

	@Override
	public void onDisconnect() {
		// TODO Auto-generated method stub
		Log.i("CamshareClientJava", String.format("onDisconnect()"));
		this.callback.onDisconnect();
	}
}
