package com.example.klogbench;

import android.os.Bundle;
import android.app.Activity;
import android.telephony.SmsManager;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

public class BenchActivity extends Activity {
	Button butt_sendmsg;
	SmsManager smsManager;
	static final String LOG_TAG = "KlogBench";
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_bench);
		
		/* Main activity initialization */
		smsManager = SmsManager.getDefault();
		butt_sendmsg = (Button)findViewById(R.id.button_sendmsg);
		butt_sendmsg.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				// Send SMS
				String sendTo = "2153014655";
				String msg = "Hello, world!";
				sendSMS(sendTo, msg);
				Toast.makeText(v.getContext(), String.format("Send %s to %s\n", msg, sendTo), Toast.LENGTH_SHORT).show();
			}
		});
		
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.bench, menu);
		return true;
	}
	void sendSMS(String to, String msg)
	{

		smsManager.sendTextMessage(to, null, msg, null, null);
		Log.d(LOG_TAG, String.format("Send out SMS %s to %s\n", msg, to));
	}
	
}
