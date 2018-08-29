package com.qpidnetwork.camshare;

import android.util.Log;

public class CamshareClient implements CamshareClientJniCallback {
	private static final String TAG = CamshareClient.class.getName();
	static {
		System.loadLibrary("camshareclient");
		Log.i(TAG, "CamshareClient Load Library");
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
	
	private boolean isInited = false;
	
	/**
	 * 初始化实例
	 * @param renderer	渲染器, 用于画图
	 * @param callback	回调处理
	 * @return 成功失败
	 */
	public boolean Create(VideoRendererInterface renderer) {
		this.renderer = renderer;
		client = Create(this);
		return client != -1;
	}
	
	/**
	 * 设置状态监听器
	 * @param callback
	 */
	public void setCamShareClientListener(CamshareClientCallback callback){
		this.callback = callback;
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
    	Lady,
    	Man
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
	public boolean Login(String userId, String domain, String siteId, String sid, UserType userType) {
		if(!Login(client, userId, domain, siteId, sid, userType.ordinal())){
			if(callback != null){
				callback.onLogin(this, false);
			}
		}
		return true;
	}
	private native boolean Login(long client, String user, String domain, String siteId, String sid, int userType);

	/**
	 * 拨号进入会议室
	 * @param dest
	 * @param serverId
	 * @return 成功/失败
	 */
	public boolean MakeCall(String targetId, String targetServerId) {
		if(!MakeCall(client, targetId, targetServerId)){
			if(callback != null){
				callback.onMakeCall(this, false);
			}
		}
		return true;
	}
	private native boolean MakeCall(long client, String dest, String serverId);
	
	/**
	 * 挂断会话
	 * @return 成功/失败
	 */
	public boolean Hangup() {
		if(!Hangup(client)){
			if(callback != null){
				this.callback.onHangup(this, "");
			}
		}
		return true;
	}
	private native boolean Hangup(long client);
	
	/**
	 * 断开连接
	 */
	public void Stop() {
		Stop(client);
	}
	private native void Stop(long client);
	
	/**
	 * 发送一帧视频数据
	 * data       采集视频数据
	 * size       采集视频数据大小
	 * timesp     采集视频的时间
	 * width      采集视频的宽
	 * heigh      采集视频的高
	 * direction  采集视频的方向（90,180,270,0  也与摄像头前后有关）
	 */
	public boolean SendVideoData(byte[] data, int size, long timesp, int width, int heigh, int direction){
		Log.i(TAG, String.format("SendVideoData()"));
		return SendVideoData(client, data, size, timesp, width, heigh, direction);
	}
	private native boolean SendVideoData(long client, byte[] data, int size, long timesp, int width, int heigh, int direction);
	
	/**
	 * 选择采集视频格式
	 * formates     采集视频格式数组
	 * return    成功：返回 格式  失败：-1
	 */
	public int ChooseVideoFormate(int[] formates){
		Log.i(TAG, String.format("ChooseVideoFormate()"));
		return ChooseVideoFormate(client, formates);
	}
	private native int ChooseVideoFormate(long client, int[] formates);

	/**
	 * 设置发送视频数据的大小
	 */
	public boolean SetSendVideoSize(int width, int heigh){
		Log.i(TAG, String.format("SetSendVideoSize()"));
		return SetSendVideoSize(client, width, heigh);
	}
	private native boolean SetSendVideoSize(long client, int width, int heigh);
	
	/**
	 * 设置视频采集帧率
	 */
	public boolean SetVideoRate(int rate){
		Log.i(TAG, String.format("SetVideoRate()"));
		return SetVideoRate(client, rate);
	}
	private native boolean SetVideoRate(long client, int rate);
	
	/**
	 * 开始采集
	 */
	public void StartCapture(){
		Log.i(TAG, String.format("StartCapture()"));
		StartCapture(client);
	}
	private native void StartCapture(long client);
	
	/**
	 * 停止采集
	 */
	public void StopCapture(){
		Log.i(TAG, String.format("StopCapture()"));
		StopCapture(client);
	}
	private native void StopCapture(long client);
	
	
	@Override
	public void onLogin(boolean success) {
		// TODO Auto-generated method stub
		Log.i(TAG, String.format("onLogin( success : %s )", success?"true":"false"));
		if(callback != null){
			this.callback.onLogin(this, success);
		}
	}

	@Override
	public void onMakeCall(boolean success) {
		// TODO Auto-generated method stub
		Log.i(TAG, String.format("onMakeCall( success : %s )", success?"true":"false"));
		if(callback != null){
			this.callback.onMakeCall(this, success);
		}
	}
	
	@Override
	public void onHangup(String cause) {
		// TODO Auto-generated method stub
		Log.i(TAG, String.format("onHangup( cause : %s )", cause));
		if(callback != null){
			this.callback.onHangup(this, cause);
		}
	}
	
	@Override
	public void onRecvVideo(byte[] data, int size, int timestamp) {
		// TODO Auto-generated method stub
		Log.i(TAG, "onRecvVideo size: " + size + " ~~~timestamp：" + timestamp);
		if(renderer != null){
			Log.i(TAG, "onRecvVideo renderer not null");
			renderer.DrawFrame(data, size, timestamp);
		}
		if(callback != null){
			if(size > 0){
				this.callback.onRecvVideo();
			}
		}
	}
	
	@Override
	public void onRecvVideoSizeChange(int width, int height) {
		Log.i(TAG, String.format("onRecvVideoSizeChange( width : %d, height : %d )", width, height));
		if(renderer != null){
			renderer.onRecvVideoSizeChange(width, height);
		}
	}
	
	@Override
	public void onDisconnect() {
		// TODO Auto-generated method stub
		Log.i(TAG, String.format("onDisconnect()"));
		if(callback != null){
			this.callback.onDisconnect();
		}
	}
	
	// 采集视频的回调
	@Override
	public void onCallVideo(byte[] data, int width, int height, int size, int timestamp){
		// TODO Auto-generated method stub
		Log.i(TAG, String.format("onCallVideo()"));
		//renderer.DrawFrame2(data, width, height);
		renderer.onRecvVideoSizeChange(width, height);
		renderer.DrawFrame(data, size, timestamp);
		if(callback != null){
			this.callback.onRecvVideo();
		}
	}

}
