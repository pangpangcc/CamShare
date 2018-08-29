/**
 * Copyright (c) 2016 The Camshare project authors. All Rights Reserved.
 */
package com.qpidnetwork.camshare;

// The following four imports are needed saveBitmapToJPEG which
// is for debug only
import java.io.ByteArrayOutputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;

public class VideoRenderer implements Callback, VideoRendererInterface {

    private final static String TAG = VideoRenderer.class.getName();

    // the bitmap used for drawing.
    private Bitmap bitmap = null;
    private ByteBuffer byteBuffer = null;
    private SurfaceHolder surfaceHolder;
    public boolean debug = false;
    
    private int viewWidth = 0;
    private int viewHeight = 0;
    private int videoStreamWidth = 0;
    private int videoStreamHeight = 0;
    private int byteBufferSize = 0;
    
    private boolean isStartRender = false;
    
	/**
	 * 日志路径
	 */
	private String logFilePath = "";
	
	public VideoRenderer(){
		
	}
	
	/**
	 * 绑定界面元素
	 * @param view
	 * @param viewWidth
	 * @param viewHeight
	 */
	public void bindView(SurfaceView view, int viewWidth, int viewHeight){
		surfaceHolder = view.getHolder();
        this.viewWidth = viewWidth;
        this.viewHeight = viewHeight;
        if(surfaceHolder == null)
            return;
        surfaceHolder.addCallback(this);
        isStartRender = true;
	}
	
	/**
	 * 解除界面绑定，停止渲染
	 */
	public void unbindView(){
		this.viewWidth = 0;
		this.viewHeight = 0;
		isStartRender = false;
		if(surfaceHolder != null){
			surfaceHolder.removeCallback(this);
		}
	}
	
	/**
	 * 设置是否Debug模式
	 * @param debug
	 */
	public void setDebug(boolean debug){
		this.debug = debug;
	}
	
	/**
	 * 清除缓存
	 */
	public void clean(){
		videoStreamWidth = 0;
		videoStreamHeight = 0;
		byteBuffer = null;
		bitmap = null;
	}

	/**
	 * 设置日志路径
	 * @param logFilePath	日志路径
	 */
	public void setLogDirPath(String logFilePath) {
		this.logFilePath = logFilePath;
	}
	
    // surfaceChanged and surfaceCreated share this function
    private void changeDestRect(int dstWidth, int dstHeight) {
    	viewWidth = dstWidth;
    	viewHeight = dstHeight;
    }

    public void surfaceChanged(SurfaceHolder holder, int format,
            int in_width, int in_height) {
        Log.i(TAG, "VideoRenderer::surfaceChanged");
        changeDestRect(in_width, in_height);
    }

    public void surfaceCreated(SurfaceHolder holder) {
    	Log.i(TAG, "VideoRenderer::surfaceCreated");
    	isStartRender = true;
        Canvas canvas = surfaceHolder.lockCanvas();
        if(canvas != null) {
            Rect dst = surfaceHolder.getSurfaceFrame();
            if(dst != null) {
                changeDestRect(dst.right - dst.left, dst.bottom - dst.top);
            }
            surfaceHolder.unlockCanvasAndPost(canvas);
        }
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
    	isStartRender = false;
    }
    
    private ByteBuffer createOrResetBuffer(int width, int height){
    	Log.i(TAG, "createOrResetBuffer " + width + ":" + height);
    	if(byteBuffer != null){
    		byteBuffer.clear();
    	}
    	byteBufferSize = width * height * 3;
    	byteBuffer = ByteBuffer.allocateDirect(byteBufferSize);
    	bitmap = CreateBitmap(width, height);
    	return byteBuffer;
    	
    }
    
    private Bitmap CreateBitmap(int width, int height) {
        Log.i(TAG, "CreateByteBitmap " + width + ":" + height);
        if (bitmap == null) {
            try {
                android.os.Process.setThreadPriority(
                    android.os.Process.THREAD_PRIORITY_DISPLAY);
            }
            catch (Exception e) {
            }
        }
        bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565);
        return bitmap;
    }

    // It saves bitmap data to a JPEG picture, this function is for debug only.
    private void saveBitmapToJPEG(int width, int height) {
        ByteArrayOutputStream byteOutStream = new ByteArrayOutputStream();
        bitmap.compress(Bitmap.CompressFormat.JPEG, 100, byteOutStream);

        try{
            FileOutputStream output = new FileOutputStream(String.format(
                "%s/render_%d.jpg", logFilePath, System.currentTimeMillis()));
            output.write(byteOutStream.toByteArray());
            output.flush();
            output.close();
        }
        catch (FileNotFoundException e) {
        }
        catch (IOException e) {
        }
    }

    public void DrawByteBuffer() {
        if(byteBuffer == null)
            return;
        byteBuffer.rewind();
        bitmap.copyPixelsFromBuffer(byteBuffer);
        DrawBitmap();
        byteBuffer.clear();
    }

    public void DrawBitmap() {
        if(bitmap == null)
            return;

        if( debug ) {
        	// The follow line is for debug only
        	saveBitmapToJPEG(videoStreamWidth, videoStreamHeight);
        }
        Canvas canvas = surfaceHolder.lockCanvas();
        if(canvas != null) {
        	if(viewHeight > 0 && viewWidth >0
        			&& videoStreamHeight>0 && videoStreamWidth > 0){
            	Matrix matrix = new Matrix();
            	float scale = 0.0f;
            	float widthScale = ((float)viewWidth)/videoStreamWidth;
            	float heightScale = ((float)viewHeight)/videoStreamHeight;
            	scale = widthScale > heightScale?widthScale:heightScale;
            	float dx = -(videoStreamWidth*scale - viewWidth)/2;
            	float dy = -(videoStreamHeight*scale - viewHeight)/2;
            	Log.i(TAG, "DrawBitmap scale: " + scale + " dx:" + dx + " dy:" + dy);
            	matrix.postScale(scale, scale);
            	matrix.postTranslate(dx, dy);
                canvas.drawBitmap(bitmap, matrix, null);
        	}
        	surfaceHolder.unlockCanvasAndPost(canvas);
        }
    }

	@Override
	public void DrawFrame(byte[] data, int size, int timestamp) {
		// TODO Auto-generated method stub
		Log.i(TAG, "DrawFrame size: " + size + " timestamp:" + timestamp + " byteBufferSize:" + byteBufferSize);
		if(isStartRender){
			if(byteBuffer != null &&
					size < byteBufferSize){
				byteBuffer.put(data, 0, size);
				DrawByteBuffer();
			}else{
				Log.i(TAG, "DrawFrame Error byteBufferSize: " + byteBufferSize + "  FrameSize:" + size);
			}
		}
	}

	@Override
	public void onRecvVideoSizeChange(int width, int height) {
		// TODO Auto-generated method stub
		Log.i(TAG, "onRecvVideoSizeChange width: " + width + " height:" + height);
		if(videoStreamWidth != width
				|| videoStreamHeight != height){
			videoStreamWidth = width;
			videoStreamHeight = height;
			createOrResetBuffer(width, height);
		}
	}

//	@Override
//	public void DrawFrame2(int[] data, int width, int height) {
//		Log.i(TAG, "DrawFrame2 width: " + width + " height:" + height);
//		if(videoStreamWidth != width
//				|| videoStreamHeight != height){
//			videoStreamWidth = width;
//			videoStreamHeight = height;
//		}
//		if(isStartRender){
//			bitmap = Bitmap.createBitmap(data, width, height, Bitmap.Config.RGB_565);
//			DrawBitmap();
//		}
//	}
}
