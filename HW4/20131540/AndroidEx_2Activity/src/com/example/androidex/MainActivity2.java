package com.example.androidex;

import java.util.Random;

import com.example.androidex.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;



public class MainActivity2 extends Activity implements View.OnClickListener{
	private static final String log = "log";
	
	static int row = 0, col = 0;
	static int[] dr = { -1, 1, 0, 0 };
	static int[] dc = { 0, 0, -1, 1 };
	int[] numList;
	LinearLayout lin;
	int width, height;
	MainActivity2 mainActivity;
	DisplayMetrics dis;
	
	TextView tvTimer;
	private IMyTimerService binder;
	private Intent intent;
	private boolean timerRunning;
	
	private ServiceConnection connection = new ServiceConnection(){
	

		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			// TODO Auto-generated method stub
			
			//get binder from service
			binder = IMyTimerService.Stub.asInterface(service);
			
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			// TODO Auto-generated method stub
			
		}
	};
	
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main2);
		
		mainActivity = this;
		dis = getApplicationContext().getResources().getDisplayMetrics();
		width = dis.widthPixels;
		height = dis.heightPixels - 200;
		
		lin = (LinearLayout)findViewById(R.id.container);		
		Button btn = (Button)findViewById(R.id.button_makeButtons);
		tvTimer = (TextView)findViewById(R.id.textView_Timer);
	
		
		OnClickListener ltn=new OnClickListener(){
			
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				
				//if already start
				if(lin.getChildAt(3) != null){
					//clear buttons 
					clearButtons();
					//unbind service
					tvTimer.setText("00:00");
					unbindService(connection);
					timerRunning = false;
				}
				
				EditText input = (EditText)findViewById(R.id.editText_input);
				String str = input.getText().toString();
				String[] substr = str.split(" ");
				
				
				intent = new Intent(mainActivity, MyTimerService.class);
				bindService(intent, connection, BIND_AUTO_CREATE);
				timerRunning = true;
				new Thread(new GetTimerThread()).start();
				
				//Validation Check
				if(substr.length != 2)	return;
				
				try{
					row = Integer.parseInt(substr[0]);
					col = Integer.parseInt(substr[1]);
				}
				catch(NumberFormatException e){
					return;
				}
				
				Log.v(log, "row : " + row + "col : " + col);
				if(row > 5 || col > 5 || (row < 2 && col < 2)){
					row = 0;	col = 0;
					return;
				}
				
				//make random number list
				Random r = new Random();
				//boolean check;
				int len = row * col;
				//int randomVal;
				numList = new int[len];
				/*
				for(int i = 0; i < len; i++){
					randomVal = r.nextInt(len);
					check = true;
					for(int j = 0; j < i; j++){
						if(numList[j] == randomVal){
							i--;
							check = false;
						}
					}
					if(check){
						numList[i] = randomVal;
						Log.v(log, Integer.toString(numList[i]));
					}
				}
				*/
				
				for(int i = 0; i < len - 1; i++)	numList[i] = i + 1;
				numList[len - 1] = 0;
				
				int zr = (len - 1) / col, zc = (len - 1) % col;
				for(int i = 0; i < 100; i++){
					int dir = r.nextInt(4);
					int tr = zr + dr[dir], tc = zc + dc[dir];
					if(tr < 0 || tc < 0 || tr >=row || tc >= col)	continue;
					numList[zr * col + zc] = numList[tr * col + tc];
					numList[tr * col + tc] = 0;
					zr = tr; zc = tc;
					
				}
				
				for(int i = 0; i < len; i++)	Log.v(log, numList[i] + " ");
				if(checkFinishAtFirst()){
					unbindService(connection);
					timerRunning = false;
					onClick(v);
				}
				
				refreshButtons();	
			}
			
		};
		
		btn.setOnClickListener(ltn);
		
		
	}

		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub
			Button numBtn = (Button)v;
			int num = Integer.parseInt((String)numBtn.getText());
			
			int len = row * col, idx = 0, r, c;
			
			//find idx
			for(int i = 0; i < len; i++){
				if(numList[i] == num){
					idx = i;
					break;
				}
			}
			r = idx / col;	c = idx % col;
			
			//check up, down, left, right button
			for(int dir = 0; dir < 4; dir++){
				int tr = r + dr[dir], tc = c + dc[dir];
				
				if(tr < 0 || tc < 0 || tr >=row || tc >= col)	continue;
				
				int target = tr * col + tc;
				if(numList[target] == 0){
					//swap
					numList[idx] = 0;
					numList[target] = num;
					
					//refresh & check finish
					clearButtons();
					refreshButtons();
					checkFinish();
					
					break;
				}
				
			}
					
		}
	
		//Refresh Buttons Layout with Numlist
		public void refreshButtons(){
			
			Log.v(log, row +" " + col + " "+ width + " " + height);
			
			for(int i = 0; i < row; i++){
				LinearLayout subLin = new LinearLayout(this);
				subLin.setLayoutParams(new LayoutParams(width, height / row));
				subLin.setTag("Buttons");
				
				for(int j = 0; j < col; j++){
					Button newBtn = new Button(this);
					newBtn.setLayoutParams(new LayoutParams(width / col, height / row));
					newBtn.setText( String.valueOf(numList[i * col + j]) );
					
					if(numList[i * col + j] > 0)
						newBtn.setOnClickListener(this);
					else
						newBtn.setBackgroundColor(Color.BLACK);
						
					subLin.addView(newBtn);
				}
				lin.addView(subLin);
			}
		}
		
		//Remove Buttons Layout
		public void clearButtons(){
			for(int i = 0; i < row; i++){
				LinearLayout childLin = (LinearLayout)lin.getChildAt(3);
				childLin.removeAllViewsInLayout();
				lin.removeView(childLin);
			}
		}
		
		//Check finish when generate random number list
		public boolean checkFinishAtFirst(){
			for(int i = 0, num = 1; i < row * col - 1; i++, num++){
				if(numList[i] != num)	return false;
			}
			return true;
		}
		
		//Check finish when user click valid number button
		public boolean checkFinish(){
			for(int i = 0, num = 1; i < row * col - 1; i++, num++){
				if(numList[i] != num)	return false;
			}
			//finish
			AlertDialog adlg = null;
			AlertDialog.Builder dialog = new AlertDialog.Builder(this);
			
			dialog.setMessage("Complete!")
				.setTitle("Puzzle")
				.setPositiveButton("OK", new DialogInterface.OnClickListener() {
					
					@Override
					public void onClick(DialogInterface dialog, int which) {
						// TODO Auto-generated method stub
						mainActivity.finish();
						dialog.dismiss();
					}
				})
				.setCancelable(false);
				
			adlg = dialog.create();
			adlg.show();
			
			//tvTimer.setText("00:00");
			unbindService(connection);
			timerRunning = false;
			return true;
		}
		
		//Get Timer info & Update TextView
		private class GetTimerThread implements Runnable{
			private Handler handler = new Handler();
			private int min = 0 , sec = 0;
			@Override
			public void run() {
				// TODO Auto-generated method stub
				
				while(timerRunning){
					if(binder == null){
						//Log.v(log, "min : " + Integer.toString(min) + "sec : " + Integer.toString(sec));
						continue;
					}
					
					handler.post(new Runnable(){

						@Override
						public void run() {
							// TODO Auto-generated method stub
							try{
								min = binder.getMin();
								sec = binder.getSec();
								//Log.v(log, "[ACTIVITY] min : " + Integer.toString(min) + "  sec : " + Integer.toString(sec));
								tvTimer.setText(min / 10 + "" + min % 10 + ":" + sec / 10 + "" + sec % 10);
							}
							
							catch(RemoteException e){
								e.printStackTrace();
							}
							
						}
						
					});
					
					try{
						Thread.sleep(500);
					}catch(InterruptedException e){
						e.printStackTrace();
					}
				}
			}
			
			
		}

		
		

}
