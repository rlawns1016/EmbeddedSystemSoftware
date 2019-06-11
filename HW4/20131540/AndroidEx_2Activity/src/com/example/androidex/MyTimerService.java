package com.example.androidex;

import android.app.Service; 
import android.content.Intent; 
import android.os.Handler;
import android.os.IBinder; 
import android.os.RemoteException;
import android.util.Log;
import android.widget.Toast;


public class MyTimerService extends Service{
	private static final String log = "log";
	private boolean finish;
	private int min = 0;
	private int sec = 0;
	
	public MyTimerService(){
		
	}
	
	IMyTimerService.Stub binder = new IMyTimerService.Stub(){
		@Override
		
		public int getSec() throws RemoteException{
			return sec;
		}
		
		public int getMin() throws RemoteException{
			return min;
		}
	};
	@Override
	public void onCreate(){
		super.onCreate();
		
		//Using Thread
		finish = false;
		Thread timer = new Thread(new Timer());
		timer.start();
	}
	
	@Override
	public void onDestroy(){
		super.onDestroy();
		finish = true;
	}
	@Override
	public IBinder onBind(Intent intent) {
		// TODO Auto-generated method stub
		return binder;
	}
	
	@Override
	public boolean onUnbind(Intent intent){
		finish = true;
		return super.onUnbind(intent);
	}
	
	private class Timer implements Runnable{
		
		//private Handler handler = new Handler();
		
		@Override
		public void run() {
			// TODO Auto-generated method stub
			
			//break if finish
			while(!finish){
				//sleep 1 sec
				try{
					Thread.sleep(1000);
				}
				catch(InterruptedException e){
					e.printStackTrace();
				}
				
				sec++;
				if(sec == 60){
					min++;
					sec = 0;
				}
				
				Log.v(log, "[SERVICE] min : " + Integer.toString(min) + "  sec : " + Integer.toString(sec));
			}			
		}
		
	}

}
