package com.example.klogbench;

import android.os.Bundle;
import android.provider.ContactsContract;
import android.app.Activity;
import android.content.ContentResolver;
import android.database.Cursor;
import android.telephony.SmsManager;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.Toast;

public class BenchActivity extends Activity {
	Button butt_sendmsg;
	Button butt_readcontact;
	SmsManager smsManager;
	static final String LOG_TAG = "KlogBench";
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_bench);
		
		/* Main activity initialization */
		smsManager = SmsManager.getDefault();
		// Send SMS Button
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
		// Read Contact Button
		butt_readcontact = (Button)findViewById(R.id.button_readcontact);
		butt_readcontact.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				ContentResolver cr = getContentResolver();
				Cursor cur = cr.query(ContactsContract.Contacts.CONTENT_URI, null, null, null, null);
				if(cur.getCount() > 0) {
					while(cur.moveToNext()) {
						String name = cur.getString(cur.getColumnIndex(ContactsContract.Contacts.DISPLAY_NAME));
						String id = cur.getString(cur.getColumnIndex(ContactsContract.Contacts._ID));
						//Toast.makeText(v.getContext(), name, Toast.LENGTH_SHORT).show();
						// Get phone number
						Cursor phoneCur = cr.query(ContactsContract.CommonDataKinds.Phone.CONTENT_URI, null, 
								ContactsContract.CommonDataKinds.Phone.CONTACT_ID+" = ?", 
								new String[]{id}, null);
								while(phoneCur.moveToNext()) {
									String number = phoneCur.getString(phoneCur.getColumnIndex(ContactsContract.CommonDataKinds.Phone.NUMBER));
									Toast.makeText(v.getContext(), name+" :"+number, Toast.LENGTH_SHORT).show();
								}
					}
				}
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
