package com.klog.services;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import android.app.ActivityManager;
import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

public class CurVisibleApp extends Service {
	private static final String LOG_TAG = "service.CurVisibleApp"; 
	public static final String KEY_POLL = "service.key.poll";
	public static final int CMD_START_POLL = 0;
	public static final int CMD_STOP_POLL = 1;
	
	TimerTask pollRunningTask;
	Timer pollRunningTimer;
	
	ServerSocket tcpSerSock;
	AppServerThread appServer;
	public CurVisibleApp() {

	}

	@Override
	public IBinder onBind(Intent intent) {
		// TODO: Return the communication channel to the service.
		throw new UnsupportedOperationException("Not yet implemented");
	}
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		Bundle bundle = intent.getExtras();
		int cmd = bundle.getInt(KEY_POLL);
		Log.d(LOG_TAG, String.format("Invoke startCommand %d in service CurVisibleApp", cmd));
		switch(cmd) {
		case CMD_START_POLL:
			// Setup and start poll timer
			pollRunningTask = new TimerTask() {
				@Override
				public void run() {
					getRunningTask();
				}};
			pollRunningTimer = new Timer();
			pollRunningTimer.schedule(pollRunningTask, 5*1000, 5*1000);
			// Listen for server request and alert
			try {
				tcpSerSock = new ServerSocket(8099);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			
			appServer = new AppServerThread();
			appServer.start();
			break;
		case CMD_STOP_POLL:
			pollRunningTimer.cancel();
			pollRunningTimer.purge();
			break;
		}
		
		
		
		super.onStartCommand(intent, flags, startId);
		return START_REDELIVER_INTENT;
		
	}

	void getRunningTask() {
		ActivityManager am = (ActivityManager) this.getSystemService(ACTIVITY_SERVICE);
		// Get current running tasks info
		List<ActivityManager.RunningTaskInfo> taskInfo = am.getRunningTasks(1);
		ComponentName componentInfo = taskInfo.get(0).topActivity;
		int uid = getUidByPackageName(componentInfo.getPackageName());
		Log.d(LOG_TAG, "Current activity:: " + componentInfo.getClassName() + " in package: " + componentInfo.getPackageName()
				+ " UID: " + uid);
	}
	int getUidByPackageName(String packageName) {
		int uid = -1;
		PackageManager pm = getPackageManager();
		List<ApplicationInfo> packages = pm.getInstalledApplications(PackageManager.GET_META_DATA);
		for(ApplicationInfo packageInfo : packages) {
			if(packageInfo.packageName.equals(packageName)) {
				uid = packageInfo.uid;
				break;
			}
		}
		return uid;
	}
	/*
	 * Socket server thread to communicate with server
	 */
	class AppServerThread extends Thread {
		@Override
		public void run() {
			Log.d(LOG_TAG, "thread is running!");
			while(true) {
				try {
					Socket c = tcpSerSock.accept();
					// Receive data from server
					Log.d(LOG_TAG, "Waiting for data!");
					Reader sockReader = new InputStreamReader(c.getInputStream());
					int data = sockReader.read();
					Log.d(LOG_TAG, "Received " + data);
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
	}
	@Override
	public void onDestroy() {
		try {
			tcpSerSock.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		if(pollRunningTimer != null) {
			pollRunningTimer.cancel();
			pollRunningTimer.purge();
		}
	}
}
