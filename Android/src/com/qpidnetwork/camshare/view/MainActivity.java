package com.qpidnetwork.camshare.view;

import java.io.File;
import java.io.IOException;
import java.util.List;

import android.app.Activity;
import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import com.qpidnetwork.camshare.CamshareClient;
import com.qpidnetwork.camshare.CamshareClient.UserType;
import com.qpidnetwork.camshare.CamshareClientCallback;
import com.qpidnetwork.camshare.CaptureCapabilityAndroid;
import com.qpidnetwork.camshare.R;
import com.qpidnetwork.camshare.VideoRenderer;
import com.qpidnetwork.tool.CrashHandlerJni;




public class MainActivity extends Activity implements CamshareClientCallback, PreviewCallback, Callback {
	private VideoRenderer videoRenderer = null;
	private SurfaceView surfaceView = null;
	private CamshareClient camshareClient = null;

	private SurfaceView videosendView = null;
	private Camera mCamera = null;
	private SurfaceHolder localPreview = null;
	   // 採集數據的格式
    private int PIXEL_FORMAT = ImageFormat.NV21;
    PixelFormat pixelFormat = new PixelFormat();
    // 采集数目个数
    private final int numCaptureBuffers = 3;
    // 摄像头方位 （-1： 没有； 其他就是前摄像头了，暂时不做后摄像头）
    private int mCamIdx = -1;
    
    private long timesp = 0;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		File path = Environment.getExternalStorageDirectory();
		String filePath = path.getAbsolutePath();
		Log.i("CamshareClientJava", "filePath : " + filePath);

        Runtime runtime = Runtime.getRuntime();
		try {
			runtime.exec("rm -rf " + filePath + "/camshare");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
        
		CrashHandlerJni.SetCrashLogDirectory(filePath + "/camshare/crash");
		CamshareClient.SetLogDirPath(filePath + "/camshare/log");
		
        surfaceView = (SurfaceView) this.findViewById(R.id.surfaceView); 
        videoRenderer = new VideoRenderer(surfaceView);
        //videoRenderer = new VideoRenderer(null);
        camshareClient = new CamshareClient();
        camshareClient.Create(videoRenderer, this);
        camshareClient.SetRecordFilePath(filePath + "/camshare/record");
       
        mCamIdx = FindFrontCamera();

        videosendView = (SurfaceView) this.findViewById(R.id.surfaceViewSend);
        localPreview = videosendView.getHolder();
        localPreview.addCallback(this);
        localPreview.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
       // mCamera = Camera.open();
        // 採集數據的格式
        
        Button loginButton = (Button)this.findViewById(R.id.button1);
        loginButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
//				camshareClient.Login("MM1", "192.168.88.152", "1", "SESSION123456", UserType.Man);
				camshareClient.Login("MM211", "172.25.10.31", "1", "SESSION123456", UserType.Man);
//				camshareClient.Login("MM2", "52.196.96.7", "1", "SESSION123456", UserType.Man);
				//camshareClient.Login("WW1", "52.196.96.7", "1", "SESSION123456", UserType.Lady);
			}
		});
        
        Button makeCallButton = (Button)this.findViewById(R.id.button2);
        makeCallButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				camshareClient.MakeCall("WW1", "PC0");
//				camshareClient.Login("WW1", "52.196.96.7", "1", "SESSION123456", UserType.Man);
//				StartCapture(320,240,10);
//				camshareClient.Login("WW1", "192.168.88.152", "1", "SESSION123456", UserType.Man);
				//camshareClient.StartCapture();
			}
		});
        
        Button hangupButton = (Button)this.findViewById(R.id.button3);
        hangupButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				camshareClient.Hangup();
//				StopCapture();
				//camshareClient.StopCapture();
			}
		});
        
        Button disconnectButton = (Button)this.findViewById(R.id.button4);
        disconnectButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				camshareClient.Stop();
//				StopCapture();
				
			}
		});
	}

	@Override
	public void onLogin(CamshareClient client, boolean success) {
		// TODO Auto-generated method stub
		if( success ) {
			//camshareClient.MakeCall("WW1", "PC0");
			camshareClient.MakeCall("WW1", "PC0");
		}

	}

	@Override
	public void onMakeCall(CamshareClient client, boolean success) {
		// TODO Auto-generated method stub
	}

	@Override
	public void onHangup(CamshareClient client, String cause) {
		// TODO Auto-generated method stub

	}

	@Override
	public void onRecvVideoSizeChange(int width, int height) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onDisconnect() {
		// TODO Auto-generated method stub
		
	}
	
  public int StartCapture(int width, int height, int frameRate) {	
  int res = tryStartCapture(width, height, frameRate);
	  
  return res;
}

  private int tryStartCapture(int width, int height, int frameRate) {

	  if (mCamera != null)
	  {
		  StopCapture();
	  }
      if (mCamIdx > -1){
      	mCamera = Camera.open(mCamIdx);
      }
	  
      if (mCamera == null) {
          //Log.e(TAG, "Camera not initialized %d" + id);
          return -1;
      }

      // 采集的格式
      PixelFormat.getPixelFormatInfo(PIXEL_FORMAT, pixelFormat);

      // 获取摄像头的参数（这个可以判断要设置的参数是否符合）
      Camera.Parameters parameters = mCamera.getParameters();
      //parameters.setPictureSize(currentCapability.width,
       //      currentCapability.height);
      
      List<Size> supportedPreviewSizes = parameters.getSupportedPreviewSizes();
      Size size = getBestSupportedSize(supportedPreviewSizes, width, height);
      if (size.width < width || size.height < height){
    	  return -1;
      }
      
      CaptureCapabilityAndroid currentCapability =
      new CaptureCapabilityAndroid();
      // 采集的宽
      currentCapability.width = width;
      // 采集的高
      currentCapability.height =  height;
      // 采集的最大帧率
      currentCapability.maxFPS = frameRate;
      
       // 设置采集的宽和高
      parameters.setPreviewSize(currentCapability.width,
              currentCapability.height);
      // 设置采集的格式
      parameters.setPreviewFormat(PIXEL_FORMAT);
      // 设置采集的帧率 
      parameters.setPreviewFrameRate(currentCapability.maxFPS);
      // parameters.set("rotation", "180");

      try {
          mCamera.setPreviewDisplay(localPreview);
          mCamera.setDisplayOrientation(90);
    	  mCamera.setParameters(parameters);
      } catch (RuntimeException e) {
         // Log.e(TAG, "setParameters failed", e);
          return -1;
      } catch (IOException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}

      // 设置一帧的大小，和存储个数。
      int bufSize = width * height * pixelFormat.bitsPerPixel / 8;
      byte[] buffer = null;
      for (int i = 0; i < numCaptureBuffers; i++) {
          buffer = new byte[bufSize];
          mCamera.addCallbackBuffer(buffer);
      }
      mCamera.setPreviewCallbackWithBuffer(this);
      
      // 开始采集
      mCamera.startPreview();


      return 0;
  }
  
  public int StopCapture() {
     // Log.d(TAG, "StopCapture");
      try {

    	  mCamera.stopPreview();
    	  mCamera.setPreviewCallbackWithBuffer(null);
    	  mCamera.release();
    	  mCamera = null;
      } catch (RuntimeException e) {
          return -1;
      }

     // isCaptureStarted = false;
      return 0;
  }

@Override
public void surfaceCreated(SurfaceHolder holder) {
	// TODO Auto-generated method stub
	//mCamera = Camera.open();
    if (mCamera != null) {
    	try {
			mCamera.setPreviewDisplay(localPreview);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }
}

@Override
public void surfaceChanged(SurfaceHolder holder, int format, int width,
		int height) {

}

@Override
public void surfaceDestroyed(SurfaceHolder holder) {
	// TODO Auto-generated method stub
	// TODO Auto-generated method stub


			
	mCamera.setPreviewCallback(null);


}

// 摄像头采集视频数据回调
@Override
public void onPreviewFrame(byte[] data, Camera camera) {
	// TODO Auto-generated method stub

	Log.i("Jni::RtmpClient::SendVideoData","onPreviewFrame : ");

    // 摄像头方向
	int direction = 0;
	CameraInfo info = new CameraInfo();
	mCamera.getCameraInfo(mCamIdx, info);
//	if (info.facing == CameraInfo.CAMERA_FACING_FRONT){
//		direction = 360 - info.orientation;
//	}
//	else{
//		direction = info.orientation;
//	}
	direction = info.orientation;
	
	// 摄像头的宽和高
	int width	= camera.getParameters().getPreviewSize().width;
	int height	= camera.getParameters().getPreviewSize().height;
		
	// 系统时间
	timesp = System.currentTimeMillis();
		
	camshareClient.SendVideoData(data, data.length, timesp, width, height, direction);
	mCamera.addCallbackBuffer(data);
	//}
}

private void swicth() {
	// TODO Auto-generated method stub
	
}

// 查找前摄像头，返回前摄像头的id号，没有就返回-1
private int FindFrontCamera(){
	int cameraCount = 0;
	CameraInfo cameraInfo = new Camera.CameraInfo();
	cameraCount = Camera.getNumberOfCameras();
	
	for(int camIdx = 0; camIdx < cameraCount; camIdx++){
		Camera.getCameraInfo(camIdx, cameraInfo);
		if (cameraInfo.facing == CameraInfo.CAMERA_FACING_FRONT){
		//if (cameraInfo.facing == CameraInfo.CAMERA_FACING_BACK){
			return camIdx;
		}
	}
	
	return  -1;
	
}

private Size getBestSupportedSize(List<Size> sizes, int width, int height){
	Size bestSize = sizes.get(0);
	int largestArea = width * height;
	for (Size s : sizes){
		int area = s.width * s.height;
		if (area >= largestArea){
			if (s.width >= width && s.height >= height){
				bestSize = s;
				largestArea = area;
				break;
			}

		}
	}
	return bestSize;
}

}
