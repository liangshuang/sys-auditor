package com.klog.services;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Reader;
import java.io.Writer;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketAddress;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import com.klog.services.KlogEntryProtos.KlogEntry;

import android.R;
import android.app.ActivityManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Looper;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import android.widget.Toast;


public class AppAgent extends Service {
	private static final String LOG_TAG = "klog.appagent"; 
	public static final String SERVER_IP = "10.0.2.2";
	//public static final String SERVER_IP = "129.32.94.230";
	public static final String KEY_POLL = "service.key.poll";
	public static final int CMD_START_POLL = 0;
	public static final int CMD_STOP_POLL = 1;
	
	private static final int WARN_SMS = 1;
	private static final int WARN_CALL = 2;
	private static final int WARN_PREMIUM_SMS = 3;
	
	TimerTask pollRunningTask;
	Timer pollRunningTimer;
	
	ServerSocket tcpSerSock;
	AppAgentThread appServer;
	
	private NotificationManager ntfMgr = null;  
	private static final int WARN_NOTIFY_ID = 1001;
	private int numNotify = 0;
	public AppAgent() {

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
/*			pollRunningTask = new TimerTask() {
				@Override
				public void run() {
					getRunningTaskID();
				}};
			pollRunningTimer = new Timer();
			pollRunningTimer.schedule(pollRunningTask, 5*1000, 5*1000);*/
			
			appServer = new AppAgentThread();
			appServer.start();
			
/*			KlogAgentThread klogAgent = new KlogAgentThread();
			klogAgent.start();*/
			
/*			try {
				Process klogProcess = Runtime.getRuntime().exec("klogagent -u 1001");
				klogProcess.waitFor();
				BufferedReader br = new BufferedReader(new InputStreamReader(klogProcess.getInputStream()));
				String outline;
				while((outline = br.readLine()) != null)
					Log.d(LOG_TAG, outline);
				
			} catch (IOException e1) {
				// TODO Auto-generated catch block
				Log.d(LOG_TAG, "KlogAgent IOException");
				e1.printStackTrace();
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				Log.d(LOG_TAG, "KlogAgent InterruptedException");
				e.printStackTrace();
			}
			Log.d(LOG_TAG, "Started Klog Agent");*/
			
			break;
		case CMD_STOP_POLL:
/*			pollRunningTimer.cancel();
			pollRunningTimer.purge();*/
			break;
		}
		
		super.onStartCommand(intent, flags, startId);
		return START_REDELIVER_INTENT;
		
	}

	int getRunningTaskID() {
		ActivityManager am = (ActivityManager) this.getSystemService(ACTIVITY_SERVICE);
		// Get current running tasks info
		List<ActivityManager.RunningTaskInfo> taskInfo = am.getRunningTasks(1);
		ComponentName componentInfo = taskInfo.get(0).topActivity;
		int uid = getUidByPackageName(componentInfo.getPackageName());
		Log.d(LOG_TAG, "Current activity:: " + componentInfo.getClassName() + " in package: " + componentInfo.getPackageName()
				+ " UID: " + uid);
		return uid;
	}
	/*
	 * Get application UID by package name
	 */
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
	 * App agent thread to communicate with server
	 */
	class AppAgentThread extends Thread {
		@Override
		public void run() {
		
			Log.d(LOG_TAG, "App agent thread is running!");
			Socket agentSock = new Socket();
			/* connect to server */
			try {
				SocketAddress serverAddr = new InetSocketAddress(InetAddress.getByName(SERVER_IP), 8888);
				agentSock.connect(serverAddr);
			} catch (IOException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
			Log.d(LOG_TAG, "Connected to server");
			ntfMgr = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
			
			
			
			/* Waiting request from server */
			while(true) {
				try {
					//Reader sockReader = new InputStreamReader(agentSock.getInputStream());
					DataInputStream sockIStream = new DataInputStream(agentSock.getInputStream());
					DataOutputStream sockOStream = new DataOutputStream(agentSock.getOutputStream());
					byte[] msgBuf = new byte[128];
					String dst_addr;
					String msg;
					//int reqCode = sockReader.read();
					int reqCode = sockIStream.readInt();
					if(reqCode == -1) {
						sockIStream.close();
						agentSock.close();				
						break;
					}
					// Answer request or response to warning
					switch(reqCode) {
					case 0:
						int uid = getRunningTaskID();
						sockOStream.writeInt(uid);
						sockOStream.flush();
						Log.d(LOG_TAG, "Send back UID " + uid);
						break;
					case WARN_SMS:
					case WARN_PREMIUM_SMS:
						Log.d(LOG_TAG, "Sending SMS in background");
						sockIStream.read(msgBuf);
						
						dst_addr = new String(msgBuf, "UTF8");
/*						sockIStream.read(msgBuf);
						msg = msgBuf.toString();*/
						Log.d(LOG_TAG, "To "+dst_addr + "|"+msgBuf[0] +msgBuf[1]);
						String title = (reqCode == WARN_PREMIUM_SMS) ? "Warning: Premium SMS" : "Warning: SMS";
						notifyLogEvent(title, "To "+dst_addr);
						break;
					case WARN_CALL:
						Log.d(LOG_TAG, "Calling in background");
						sockIStream.read(msgBuf);
						dst_addr = msgBuf.toString();
						notifyLogEvent("Warning: Phone Call", "To "+dst_addr);
						break;
					default:
						Log.d(LOG_TAG, "Request " + reqCode);
					}
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}

		}
	}
	class KlogAgentThread extends Thread {
		@Override
		public void run() {
			

/*			byte[] klogbuf = new byte[284];
			try {
				FileInputStream fin = new FileInputStream("/sys/kernel/debug/klogger");
				fin.read(klogbuf, 0, 1);
				Log.d(LOG_TAG, "Read: " + klogbuf);
				KlogEntry kentry = KlogEntry.parseFrom(klogbuf);
				Log.d(LOG_TAG, "UID: " + kentry.getUid());
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				Log.d(LOG_TAG, "FileNotFoundException");
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				Log.d(LOG_TAG, "IOException");
				e.printStackTrace();
				
			}*/
		}
	}
	public void notifyLogEvent(String title, String info) {
		Intent resultIntent = new Intent(this, AppAgent.class);
		PendingIntent resultPendingIntent = PendingIntent.getActivity(getBaseContext(), 0, 
				resultIntent, Intent.FLAG_ACTIVITY_NEW_TASK);
		
		NotificationCompat.Builder mbuilder = new NotificationCompat.Builder(this)
			.setSmallIcon(R.drawable.ic_notification_overlay)
			.setContentTitle(title)
			.setContentInfo(info)
			.setContentIntent(resultPendingIntent)
			.setDefaults(Notification.DEFAULT_SOUND)
			.setAutoCancel(true)
			.setNumber(++numNotify);
		
		ntfMgr.notify(WARN_NOTIFY_ID, mbuilder.build());
		//Toast.makeText(this, info, Toast.LENGTH_SHORT);
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
