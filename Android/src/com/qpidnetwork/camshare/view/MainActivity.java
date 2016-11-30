package com.qpidnetwork.camshare.view;

import com.qpidnetwork.camshare.CamshareClient;
import com.qpidnetwork.camshare.CamshareClient.UserType;
import com.qpidnetwork.camshare.CamshareClientCallback;
import com.qpidnetwork.camshare.R;
import com.qpidnetwork.camshare.VideoRenderer;

import android.app.Activity;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class MainActivity extends Activity implements CamshareClientCallback {
	private VideoRenderer videoRenderer = null;
	private SurfaceView surfaceView = null;
	private CamshareClient camshareClient = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		CamshareClient.SetLogDirPath("/sdcard/camshare");
		
        surfaceView = (SurfaceView) this.findViewById(R.id.surfaceView); 
        videoRenderer = new VideoRenderer(surfaceView);
        camshareClient = new CamshareClient();
        camshareClient.Create(videoRenderer, this);
        camshareClient.SetRecordFilePath("/sdcard/camshare");
        
        Button loginButton = (Button)this.findViewById(R.id.button1);
        loginButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				camshareClient.Login("MM1", "192.168.88.152", "1", "SESSION123456", UserType.Man);
//				camshareClient.Login("MM1", "172.25.10.32", "1", "SESSION123456", UserType.Man);
			}
		});
        
        
        Button makeCallButton = (Button)this.findViewById(R.id.button2);
        makeCallButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				camshareClient.MakeCall("WW1", "PC0");
			}
		});
        
        Button hangupButton = (Button)this.findViewById(R.id.button3);
        hangupButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				camshareClient.Hangup();
			}
		});
        
        Button disconnectButton = (Button)this.findViewById(R.id.button4);
        disconnectButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				camshareClient.Stop();
			}
		});
	}

	@Override
	public void onLogin(CamshareClient client, boolean success) {
		// TODO Auto-generated method stub
		if( success ) {
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

}
