/**
 * Copyright (c) 2016 The Camshare project authors. All Rights Reserved.
 */
package com.qpidnetwork.camshare;

// The following four imports are needed saveBitmapToJPEG which
// is for debug only
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.SurfaceHolder.Callback;

public class VideoRenderer implements Callback, VideoRendererInterface {

    private final static String TAG = "CamshareClientJava";

    // the bitmap used for drawing.
    private Bitmap bitmap = null;
    private ByteBuffer byteBuffer = null;
    private SurfaceHolder surfaceHolder;
    // Rect of the source bitmap to draw
    private Rect srcRect = new Rect();
    // Rect of the destination canvas to draw to
    private Rect dstRect = new Rect();
    
    public boolean debug = false;
    
	/**
	 * 日志路径
	 */
	private String logFilePath = "";
	
    public VideoRenderer(SurfaceView view) {
        surfaceHolder = view.getHolder();
        if(surfaceHolder == null)
            return;
        surfaceHolder.addCallback(this);
        CreateByteBuffer(220, 180);
        
        if(debug){
        	File path = Environment.getExternalStorageDirectory();
        	String filePath = path.getAbsolutePath();
        	setLogDirPath(filePath + "/camshare/record");
        }
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
    	double Scale = 1;
        if( dstHeight > dstWidth ) {
            dstRect.right = (dstRect.left + dstWidth);
            Scale = (1.0 * dstWidth / (srcRect.right - srcRect.left));
            dstRect.bottom = (int)((dstRect.top + Scale * (srcRect.bottom - srcRect.top)));
        }
        
        Log.i(TAG, "VideoRenderer::changeDestRect" +
                " dstWidth:" + dstWidth + " dstHeight:" + dstHeight +
                " srcRect.left:" + srcRect.left +
                " srcRect.top:" + srcRect.top +
                " srcRect.right:" + srcRect.right +
                " srcRect.bottom:" + srcRect.bottom +
                " dstRect.left:" + dstRect.left +
                " dstRect.top:" + dstRect.top +
                " dstRect.right:" + dstRect.right +
                " dstRect.bottom:" + dstRect.bottom +
                " Scale: " + Scale);
    }

    public void surfaceChanged(SurfaceHolder holder, int format,
            int in_width, int in_height) {
        Log.i(TAG, "VideoRenderer::surfaceChanged");
        changeDestRect(in_width, in_height);
    }

    public void surfaceCreated(SurfaceHolder holder) {
    	Log.i(TAG, "VideoRenderer::surfaceCreated");
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
        Log.i(TAG, "VideoRendererer::surfaceDestroyed");
        bitmap = null;
        byteBuffer = null;
    }

    public Bitmap CreateBitmap(int width, int height) {
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
        srcRect.left = 0;
        srcRect.top = 0;
        srcRect.bottom = height;
        srcRect.right = width;
        
        dstRect.top = 0;
        dstRect.left = 0;
        
        changeDestRect(width, height);
        return bitmap;
    }

    synchronized public ByteBuffer CreateByteBuffer(int width, int height) {
        Log.i(TAG, "CreateByteBuffer " + width + ":" + height);
//        if (bitmap == null) {
            bitmap = CreateBitmap(width, height);
            byteBuffer = ByteBuffer.allocateDirect(width * height * 5);
//        }
        return byteBuffer;
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
        	saveBitmapToJPEG(srcRect.right - srcRect.left, srcRect.bottom - srcRect.top);
        }
        
        Canvas canvas = surfaceHolder.lockCanvas();
        if(canvas != null) {
            canvas.drawBitmap(bitmap, srcRect, dstRect, null);
            surfaceHolder.unlockCanvasAndPost(canvas);
        }
    }

	@Override
	public void DrawFrame(byte[] data, int size, int timestamp) {
		// TODO Auto-generated method stub
		byteBuffer.put(data, 0, size);
		
		DrawByteBuffer();
	}

	@Override
	public void ChangeRendererSize(int width, int height) {
		// TODO Auto-generated method stub
		 Log.i(TAG, "ChangeRendererSize " + width + ":" + height);
		CreateByteBuffer(width, height);
	}

	
	public void DrawFrame2(int[] data, int width, int height) {
		// TODO Auto-generated method stub
		bitmap = Bitmap.createBitmap(data, width, height, Bitmap.Config.RGB_565);
		DrawBitmap();
	}
	
}
