package com.qpidnetwork.camshare;

public interface CamshareClientCallback {
	public void onLogin(CamshareClient client, boolean success);
	public void onMakeCall(CamshareClient client, boolean success);
	public void onHangup(CamshareClient client, String cause);
	public void onRecvVideoSizeChange(int width, int height);
	public void onDisconnect();
}
