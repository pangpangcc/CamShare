package com.qpidnetwork.camshare;

public interface VideoRendererInterface {
    public void DrawFrame(byte[] data, int size, int timestamp);
    public void ChangeRendererSize(int width, int height);
    public void DrawFrame2(int[] data, int width, int height);
}
