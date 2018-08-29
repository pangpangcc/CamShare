package com.qpidnetwork.camshare;

public interface VideoRendererInterface {
    public void DrawFrame(byte[] data, int size, int timestamp);
    public void onRecvVideoSizeChange(int width, int height);
}
