package com.qpidnetwork.camshare;

public interface CamshareClientJniCallback {
	public void onLogin(boolean success);
	public void onMakeCall(boolean success);
	public void onHangup(String cause);
	public void onRecvVideo(byte[] data, int size, int timestamp);
	public void onRecvVideoSizeChange(int width, int height);
	public void onDisconnect();
}
