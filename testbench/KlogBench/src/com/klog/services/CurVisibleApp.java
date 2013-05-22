package com.klog.services;

import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import android.app.ActivityManager;
import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
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
			pollRunningTask = new TimerTask() {
				@Override
				public void run() {
					// TODO Auto-generated method stub
					getRunningTask();
				}};
			pollRunningTimer = new Timer();
			pollRunningTimer.schedule(pollRunningTask, 5*1000, 5*1000);
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
		Log.d(LOG_TAG, "Current activity:: " + componentInfo.getClassName() + " in package: " + componentInfo.getPackageName());
	}
}
