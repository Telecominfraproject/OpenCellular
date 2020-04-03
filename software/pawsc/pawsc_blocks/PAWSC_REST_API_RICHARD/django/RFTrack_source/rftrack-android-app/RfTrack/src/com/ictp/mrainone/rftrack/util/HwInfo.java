// ----------------------------------------------------------------------
// module from:
// https://gist.github.com/AbijeetP/5764507
// la classe e' static, per utilizzarla basta usare HwInfo.<metodo>
// integrare con questi file:
// marco\Documenti\doc_android\system_feature_test\SystemFeaturesTest.java
// marco\Documenti\doc_android\gps\check_gps\javapapers.com-Android Location using GPS Network Provider\AndroidLocationGPS_NW\src\com\javapapers\android\androidgps\AndroidLocationActivity.java

// attenzione:
// nel file manifest.xml abilitare le permissions !!!
// vedere:
// http://developer.android.com/reference/android/Manifest.permission.html
//
// per verifica network:
// 	<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
// per location manager:
// 	<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION"/>
//	<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION"/>
// per sd:
//  <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
//	<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
//
// 
// ----------------------------------------------------------------------

package com.ictp.mrainone.rftrack.util;

import android.content.Context;
import android.content.pm.PackageManager;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

public class HwInfo {
    static Context context;
	static LocationManager locationManager;
	public static void setContext(Context context)
	{
		HwInfo.context = context;
	}
	
	public static void setLocationManager(LocationManager locationManager2) {
		HwInfo.locationManager = locationManager2;		
	}
	
	/***
	 * Checks if a device has GPS and returns TRUE if it does and the GPS is enabled else returns FALSE.
	 * @return FALSE or TRUE, depending on the presence and state of the GPS.
	 */
	public static boolean hasGPSandIsEnabled()
	{		
		PackageManager packageManager = context.getPackageManager();
		if (packageManager.hasSystemFeature(PackageManager.FEATURE_LOCATION_GPS))
		{
			// It's present, check if it's enabled.
			if(locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)){
				return true;
			}
			else {
				return false;
			}
		}
		else
		{
			// Not present, not enabled.
			return false;
		}
	}
	
	/***
	 * Checks if a device has network connectivity - both for WIFI and MOBILE and then returns true if it does.
	 * @return TRUE or FALSE depending on whether network connectivity is available.
	 */
	public static boolean hasNetwork()
	{
	    boolean haveConnectedWifi = false;
	    boolean haveConnectedMobile = false;

	    ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
	    NetworkInfo[] netInfo = cm.getAllNetworkInfo();
	    for (NetworkInfo ni : netInfo) {
	        if (ni.getTypeName().equalsIgnoreCase("WIFI"))
	            if (ni.isConnected())
	                haveConnectedWifi = true;
	        if (ni.getTypeName().equalsIgnoreCase("MOBILE"))
	            if (ni.isConnected())
	                haveConnectedMobile = true;
	    }
	    return haveConnectedWifi || haveConnectedMobile;
	}

	/***
	 * Get Location Provider, depending on what's available NETWORK or GPS. Returns null if both of them are disabled.
	 * @return
	 */
	public static String getLocationProvider()
	{
		if(hasGPSandIsEnabled())
		{
			return LocationManager.GPS_PROVIDER;
		}
		else if(hasNetwork())
		{
			return LocationManager.NETWORK_PROVIDER;
		}
		else {
			return null;
		}
	}
	public static boolean hasSdCard()
	{
		return android.os.Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED);
		
	}
	
}
