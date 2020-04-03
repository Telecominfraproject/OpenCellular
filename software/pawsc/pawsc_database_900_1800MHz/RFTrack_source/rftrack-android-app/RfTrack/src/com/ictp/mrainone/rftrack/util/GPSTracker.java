// read doc:
// http://www.androidhive.info/2012/07/android-gps-location-manager-tutorial/

package com.ictp.mrainone.rftrack.util;

// import java.lang.*;

import android.app.AlertDialog;
import android.app.Service;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.location.GpsStatus;
import android.location.GpsStatus.Listener;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.IBinder;
import android.provider.Settings;
import android.util.Log;

// per la gestione db
import com.ictp.mrainone.rftrack.helper.DatabaseHelper;

public class GPSTracker extends Service implements LocationListener {

	private final Context mContext;

	//-----------------------------------
	// Database Helper per db sqlite
	DatabaseHelper db;
	// variabili utilizzate per l'inserzione nella tabella sqlite
	long old_campaign_id;		// id previous campaign
	long campaign_id;			// id actual campaign
	
	long location_id = 0;		// id db actual location
	
	// flag for GPS status
	boolean isGPSEnabled = false;

	// flag for network status
	boolean isNetworkEnabled = false;

	// flag for GPS status
	boolean canGetLocation = false;

	Location location; 		// location
	// boolean fFixGps = true;	// true if gps has location fixed
	boolean fFixGps = false;	// true if gps has location fixed
    String StrGpsStatus;

	// old latitude and longitude position
	boolean fPosIsChanged;	// true if (latitude, longitude) is changed
	double oldLatitude; 	// old Latitude
	double oldLongitude; 	// old Longitude
	
	// position data
	double Latitude; 		// Latitude
	double Longitude; 		// Longitude
	double Altitude;		// altitude if available, in meters above the WGS 84 reference ellipsoid.
	float Speed;			// speed if it is available, in meters/second over ground.	
	float Accuracy;			// estimated accuracy of this location, in meters.
	long locTime;			// UTC time of this fix, in milliseconds since January 1, 1970.

	// if the position returned is (0.0d, 0.0d), these parameters are inserted manually.
	// In this manner, we force to save a correct position, if we know it.
	double LatitudePos0; 	// Latitude
	double LongitudePos0;	// Longitude
	double AltitudePos0;	// altitude if available, in meters above the WGS 84 reference ellipsoid.
	
	// The minimum distance to change Updates in meters
	private static final long MIN_DISTANCE_CHANGE_FOR_UPDATES = 10; // 10 meters

	// The minimum time between updates in milliseconds
	// ori
	// private static final long MIN_TIME_BW_UPDATES = 1000 * 60 * 1; // 1 minute
	// mr: mod. 22/07
	private static final long MIN_TIME_BW_UPDATES = 1000 * 1; // 1 second

	// Declaring a Location Manager
	protected LocationManager locationManager;


	// http://stackoverflow.com/questions/15453576/android-check-if-gps-is-searching-has-fix-or-is-not-in-use
	// see:
	// http://developer.android.com/reference/android/location/GpsStatus.html
	public Listener mGPSStatusListener = new GpsStatus.Listener()
	{    
		public void onGpsStatusChanged(int event) 
		{   
			// 4 events managed:
			// GPS_EVENT_FIRST_FIX: Event sent when the GPS system has received its first fix since starting.
			// GPS_EVENT_SATELLITE_STATUS: Event sent periodically to report GPS satellite status.
			// GPS_EVENT_STARTED: Event sent when the GPS system has started.
			// GPS_EVENT_STOPPED: Event sent when the GPS system has stopped.
			switch(event) 
			{
			case GpsStatus.GPS_EVENT_STARTED:
				// Event sent when the GPS system has started.
				StrGpsStatus = "GPS Searching";
				break;
			case GpsStatus.GPS_EVENT_STOPPED:
				// Event sent when the GPS system has stopped.
				fFixGps = false;	// true if gps has location fixed
				StrGpsStatus = "GPS Stopped";
				break;
			case GpsStatus.GPS_EVENT_FIRST_FIX:
				// Event sent when the GPS system has received its first fix since starting.
				// GPS_EVENT_FIRST_FIX Event is called when GPS is locked            
				fFixGps = true;	// true if gps has location fixed
				StrGpsStatus = "GPS Fix";
				Location gpslocation = locationManager
							.getLastKnownLocation(LocationManager.GPS_PROVIDER);

				if(gpslocation != null)
				{       
					// StrGpsStatus = "GPS Info";
					// StrGpsStatus = "GPS Info:"+gpslocation.getLatitude()+":"+gpslocation.getLongitude());

					/*
						* Removing the GPS status listener once GPS is locked  
						*/
					locationManager.removeGpsStatusListener(mGPSStatusListener);                
				}               

				break;
			case GpsStatus.GPS_EVENT_SATELLITE_STATUS:
				// Event sent periodically to report GPS satellite status.
				StrGpsStatus = "GPS_EVENT_SATELLITE_STATUS";
				break;                  
			}
		}
	};	
	
	public GPSTracker(Context context, DatabaseHelper dbHelper, long start_campaign_id) {
		this.mContext = context;
		
		//------------------------
		// per la gestione db sqlite
		db = dbHelper;
		//------------------------
		// to force save data, change a value of campaign_id
		campaign_id = 0L;
		// set data corresponding to the location with coordinates (0.0, 0.0)
		SetLocation_0(0.0d, 0.0d, 0.0d);

		clearLocationData();
		
		// getLocation();
		// disabilitato 01/08
		// setCampaignId(start_campaign_id);
	}

	public void setdbHelper(DatabaseHelper dbHelper)
	{
		//------------------------
		// per la gestione db sqlite
		db = dbHelper;
		//------------------------
	}
	
	// see: http://www.tutorialspoint.com/java/lang/double_compare.htm
	// compare the (latitude, longitude) with previous values
	private boolean checkPositionIsChanged() {
		int retval;
		
		fPosIsChanged = false;
		retval = Double.compare(oldLatitude, Latitude);
		if(retval!= 0)
		{
			fPosIsChanged = true;
			return(true);
		}
		retval = Double.compare(oldLongitude, Longitude);
		if(retval!= 0)
		{
			return(true);
		}
		return(false);
	}

	// compare the coordinates. Return true if are the same
	public boolean compareCoordinates(
			double Lat_Pos, 	// Latitude
			double Lon_Pos		// Longitude
			)
	{
		int retval;
		
		retval = Double.compare(Latitude, Lat_Pos);
		if(retval!= 0)
		{
			return(false);
		}
		retval = Double.compare(Longitude, Lon_Pos);
		if(retval!= 0)
		{
			return(false);
		}
		// the values are the same
		return(true);
	}
	
	// set data for location with coordinates (0.0, 0.0)
	public void SetLocation_0(
			double Lat_Pos0, 	// Latitude
			double Lon_Pos0,	// Longitude
			double Alt_Pos0		// altitude in meters
			)
	{
		LatitudePos0 = Lat_Pos0; 	// Latitude
		LongitudePos0 = Lon_Pos0;	// Longitude
		AltitudePos0 = Alt_Pos0;	// altitude if available, in meters above the WGS 84 reference ellipsoid.
	}
	
	// force the actual (Latitude, Longitude, Altitude) with the correspondibg data of Location_0
	public void forceCoordinateLocation_0() {
		Latitude = LatitudePos0;  	// Latitude
		Longitude = LongitudePos0;	// Longitude
		Altitude = AltitudePos0;	// altitude if available, in meters above the WGS 84 reference ellipsoid.
	}
	
	private boolean clearLocationData() {
		// save old position
		oldLatitude = Latitude; 	// latitude, in degrees.
		oldLongitude = Longitude;	// longitude, in degrees.
		
		/***
		ori:
		Latitude = 0.0d;	// latitude, in degrees.
		Longitude = 0.0d;	// longitude, in degrees.
		Altitude = 0.0d;	// altitude if available, in meters above the WGS 84 reference ellipsoid.
		***/
		forceCoordinateLocation_0();	// set the data corresponding at Location_0
		
		Speed = 0.0f;		// speed if it is available, in meters/second over ground.	
		Accuracy = 0.0f;	// estimated accuracy of this location, in meters.
		locTime = 0L;		// UTC time of this fix, in milliseconds since January 1, 1970.
	
		return(checkPositionIsChanged());
	}
	
	private boolean readLocationData() {
		if (location != null) {
			// save old position
			oldLatitude = Latitude; 	// latitude, in degrees.
			oldLongitude = Longitude;	// longitude, in degrees.
		
			Latitude = location.getLatitude();
			Longitude = location.getLongitude();
			Speed = location.getSpeed();
			Altitude = location.getAltitude();
			Accuracy = location.getAccuracy();
			locTime = location.getTime();
			
			// 27/07: check if the coordinates are (0.0, 0.0).
			// In this case, use the data of Position_0
			if(compareCoordinates(0.0d, 0.0d))
			{
				forceCoordinateLocation_0();	// set the data corresponding at Location_0
			}

			return(checkPositionIsChanged());
		}
		
		return(clearLocationData());
	}
	
	// return an array with the coordinata of the actual position
	public double[] getLocationCoordinates() {
		double[] coord = new double[3];
		
		coord[0] = getLatitude();
		coord[1] = getLongitude();
		coord[2] = getAltitude();
		
		if(compareCoordinates(0.0d, 0.0d))
		{
			coord[0] = LatitudePos0;
			coord[1] = LongitudePos0;
			coord[2] = AltitudePos0;
		}
		
		return(coord);
	}

	// return an array with the coordinata of the last position read
	public double[] getLastLocationCoordinates() {
		double[] coord = new double[3];
		
		coord[0] = Latitude;
		coord[1] = Longitude;
		coord[2] = Altitude;
		
		if(compareCoordinates(0.0d, 0.0d))
		{
			coord[0] = LatitudePos0;
			coord[1] = LongitudePos0;
			coord[2] = AltitudePos0;
		}
		
		return(coord);
	}
	
	public boolean hasGPSandIsEnabled()
	{
		boolean gpsEnabled = false;
		
		try {
			locationManager = (LocationManager) mContext
					.getSystemService(LOCATION_SERVICE);

			// getting GPS status
			gpsEnabled = locationManager
					.isProviderEnabled(LocationManager.GPS_PROVIDER);
		
		} catch (Exception e) {
			e.printStackTrace();
		}

		return gpsEnabled;
	}
	
	public boolean hasNetwork()
	{
		boolean netEnabled = false;

		try {
			locationManager = (LocationManager) mContext
					.getSystemService(LOCATION_SERVICE);

			// getting network status
			netEnabled = locationManager
					.isProviderEnabled(LocationManager.NETWORK_PROVIDER);
		} catch (Exception e) {
			e.printStackTrace();
		}
		return netEnabled;
	}

	public String getLocationProvider()
	{
		if(hasGPSandIsEnabled())
		{
			return locationManager.GPS_PROVIDER;
		}
		else if(hasNetwork())
		{
			return locationManager.NETWORK_PROVIDER;
		}
		else {
			return "No location provider enabled";
		}
	}
	
	public Location getLocation() {
		try {
			
			boolean fPosChanged = false; 	// true if the position is changed
			
			// fFixGps = true;					// true has fix gps
			
			locationManager = (LocationManager) mContext
					.getSystemService(LOCATION_SERVICE);

			// getting GPS status
			isGPSEnabled = locationManager
					.isProviderEnabled(LocationManager.GPS_PROVIDER);

			// getting network status
			isNetworkEnabled = locationManager
					.isProviderEnabled(LocationManager.NETWORK_PROVIDER);

			if (!isGPSEnabled && !isNetworkEnabled) {
				// no network provider is enabled
				// fFixGps = false;					// false no fix gps
				fPosChanged = clearLocationData();
			} 
			else {
				this.canGetLocation = true;
				if (isGPSEnabled) {
					// priority for GPS:
					// GPS Enabled, get lat/long using GPS Services
					locationManager.requestLocationUpdates(
							LocationManager.GPS_PROVIDER,
							MIN_TIME_BW_UPDATES,
							MIN_DISTANCE_CHANGE_FOR_UPDATES, this);
					Log.d("GPS Enabled", "GPS Enabled");
					if (locationManager != null) {
						//---------------------------------
						// http://stackoverflow.com/questions/15453576/android-check-if-gps-is-searching-has-fix-or-is-not-in-use
						// Register GPSStatus listener for events
						// Each time if you call GPS function, GPSStatus listener is registered
						locationManager.addGpsStatusListener(mGPSStatusListener); 
						//---------------------------------
						
						location = locationManager
								.getLastKnownLocation(LocationManager.GPS_PROVIDER);
						
						fPosChanged = readLocationData();
					}
					/***
					else
					{
						fFixGps = false;					// false no fix gps
					}
					***/
				}
				if (isNetworkEnabled) {
					// network enabled
					// Check if you have a previous location with GPS
					if (location == null) {
						// no location with GPS. Use network
						locationManager.requestLocationUpdates(
								LocationManager.NETWORK_PROVIDER,
								MIN_TIME_BW_UPDATES,
								MIN_DISTANCE_CHANGE_FOR_UPDATES, this);
						Log.d("Network", "Network");
						if (locationManager != null) {
							//---------------------------------
							// http://stackoverflow.com/questions/15453576/android-check-if-gps-is-searching-has-fix-or-is-not-in-use
							// Register GPSStatus listener for events
							// Each time if you call GPS function, GPSStatus listener is registered
							locationManager.addGpsStatusListener(mGPSStatusListener); 
							//---------------------------------

							location = locationManager
									.getLastKnownLocation(LocationManager.NETWORK_PROVIDER);
							
							fPosChanged = readLocationData();
						}
						/***
						else
						{
							fFixGps = false;					// false no fix gps
						}
						***/
					}
				}
			}
			
			//-------------------------------------
			// check if the position is changed.
			// If true, save the new position in db
			if(fPosChanged)
			{
				// save the position in db
			}
			//-------------------------------------

		} catch (Exception e) {
			e.printStackTrace();
		}

		return location;
	}
	
	
	/**
	 * Stop using GPS listener
	 * Calling this function will stop using GPS in your app
	 * */
	public void stopUsingGPS(){
		if(locationManager != null){
			locationManager.removeUpdates(GPSTracker.this);
		}		
	}

	/**
	 * Function to get flag fix gps
	 * */
	public boolean getFixGps(){
		return fFixGps;
	}
	
	/**
	 * Function to get Latitude
	 * */
	public double getLatitude(){
		Latitude = 0.0;		// latitude, in degrees.
		if(location != null){
			Latitude = location.getLatitude();
		}
		
		// return Latitude
		return Latitude;
	}
	
	/**
	 * Function to get Longitude
	 * */
	public double getLongitude(){
		Longitude = 0.0;	// longitude, in degrees.
		if(location != null){
			Longitude = location.getLongitude();
		}
		
		// return Longitude
		return Longitude;
	}

	/**
	 * Function to get Altitude
	 * */
	public double getAltitude(){
		Altitude = 0.0;	// altitude if available, in meters above the WGS 84 reference ellipsoid.
		if(location != null){
			Altitude = location.getAltitude();
		}
		
		// return Altitude
		return Altitude;
	}

	/**
	 * Function to get Speed
	 * */
	public float getSpeed(){
		Speed = 0.0f;		// speed if it is available, in meters/second over ground.
		if(location != null){
			Speed = location.getSpeed();
		}
		
		// return Speed
		return Speed;
	}

	/**
	 * Function to get Accuracy
	 * */
	public float getAccuracy(){
		Accuracy = 0.0f;		// Accuracy if it is available, in meters/second over ground.
		if(location != null){
			Accuracy = location.getAccuracy();
		}
		
		// return Accuracy
		return Accuracy;
	}

	/**
	 * Function to get Time
	 * */
	public long getTime(){
		locTime = 0;		// UTC time of this fix, in milliseconds since January 1, 1970.
		if(location != null){
			locTime = location.getTime();
		}
		
		// return Time
		return locTime;
	}
	
	/**
	 * Function to check GPS/wifi enabled
	 * @return boolean
	 * */
	public boolean canGetLocation() {
		return this.canGetLocation;
	}
	
	/**
	 * Function to show settings alert dialog
	 * On pressing Settings button will lauch Settings Options
	 * */
	public void showSettingsAlert(){
		AlertDialog.Builder alertDialog = new AlertDialog.Builder(mContext);
   	 
        // Setting Dialog Title
        alertDialog.setTitle("GPS is settings");
 
        // Setting Dialog Message
        alertDialog.setMessage("GPS is not enabled. Do you want to go to settings menu?");
 
        // On pressing Settings button
        alertDialog.setPositiveButton("Settings", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog,int which) {
            	Intent intent = new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS);
            	mContext.startActivity(intent);
            }
        });
 
        // on pressing cancel button
        alertDialog.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
            dialog.cancel();
            }
        });
 
        // Showing Alert Message
        alertDialog.show();
	}

	@Override
	public void onLocationChanged(Location location) {
	}

	@Override
	public void onProviderDisabled(String provider) {
	}

	@Override
	public void onProviderEnabled(String provider) {
	}

	@Override
	public void onStatusChanged(String provider, int status, Bundle extras) {
	}

	@Override
	public IBinder onBind(Intent arg0) {
		return null;
	}

	//-----------------------------------------------
	// gestione salvataggio location in Db sqlite
	//-----------------------------------------------
	
	public void setCampaignId(long id) {
		old_campaign_id = campaign_id;
		campaign_id = id;
		if(old_campaign_id != campaign_id)
		{
			// force to save the location in db
			readLocationSvDb(true);
		}
	}
	public long getCampaignId() {
		return(campaign_id);
	}
	
	public long getLocationId() {
		return(location_id);
	}
	
	public long readLocationSvDb(boolean fForce) {

		getLocation();
		
		if(fForce || fPosIsChanged)
		{
			// the position is changed or forced saving in db
			if(db != null)
			{
				location_id = db.createLocation(
						campaign_id, 
						locTime,		// UTC time of this fix, in milliseconds since January 1, 1970.
						Latitude,		// latitude, in degrees.
						Longitude,		// longitude, in degrees.
						Altitude,		// altitude if available, in meters above the WGS 84 reference ellipsoid.
						Speed,			// speed if it is available, in meters/second over ground.	
						Accuracy);		// estimated accuracy of this location, in meters.
			}
		}
		return(location_id);
	}
}
