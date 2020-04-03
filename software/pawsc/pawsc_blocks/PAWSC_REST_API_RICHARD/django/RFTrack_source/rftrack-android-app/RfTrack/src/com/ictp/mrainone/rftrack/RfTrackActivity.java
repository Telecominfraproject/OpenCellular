/* Copyright 2014 Marco Rainone, for ICTP Wireless Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 */
// see the documentation:
// http://developer.android.com/reference/android/widget/package-summary.html

package com.ictp.mrainone.rftrack;


// java.io: Provides for system input and output through data streams, serialization and the file system.
import java.io.*;

// java.text: Provides classes and interfaces for handling text, dates, numbers, and messages in a manner independent of natural languages.
import java.text.*;
import java.util.Arrays;
// java.util: Contains the collections framework, legacy collection classes, event model, date and time facilities, internationalization,
// and miscellaneous utility classes (a string tokenizer, a random-number generator, and a bit array).
import java.util.Calendar;
import java.util.Comparator;
import java.util.Date;
import java.util.List;
import java.util.Locale;
// java.util.concurrent: Utility classes commonly useful in concurrent programming.
import java.util.concurrent.ExecutorService;		// ExecutorService: An Executor that provides methods to manage termination and methods that can produce a Future for tracking progress of one or more asynchronous tasks.
import java.util.concurrent.Executors;

import java.util.regex.Matcher;
import java.util.regex.Pattern;


// android.app:Contains high-level classes encapsulating the overall Android application model.
import android.app.Activity;			// Activity: An activity is a single, focused thing that the user can do.
import android.app.AlertDialog;			// AlertDialog: A subclass of Dialog that can display one, two or three buttons.
import android.app.ProgressDialog;
import android.os.Environment;
import android.net.Uri;

// android.content: Contains classes for accessing and publishing data on a device.
import android.content.ContentResolver;
import android.content.Context;				// Context:	Interface to global information about an application environment.
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
// android.graphics: Provides low level graphics tools such as canvases, color filters, points, and rectangles that let you handle drawing to the screen directly. 
import android.graphics.Color;

// android.view: Provides classes that expose basic user interface classes that handle screen layout and interaction with the user.
import android.view.LayoutInflater;		// LayoutInflater: Instantiates a layout XML file into its corresponding View objects.
import android.view.View;				// View: This class represents the basic building block for user interface components.
import android.view.View.MeasureSpec;
import android.view.View.OnClickListener;
import android.view.Display;
import android.view.Gravity;

// for menus:
import android.view.Menu;
import android.view.MenuItem;
import android.widget.AdapterView;
// android.widget: The widget package contains (mostly visual) UI elements to use on Application screen.
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ArrayAdapter;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ScrollView;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.TextView;
import android.widget.Toast;

// android.os: Provides basic operating system services, message passing, and inter-process communication on the device.
import android.os.Bundle;				// Bundle: A mapping from String values to various Parcelable types.
// import android.os.Environment;			// Environment: Provides access to environment variables.

import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.view.WindowManager;

// android.hardware.usb: Provides support to communicate with USB hardware peripherals that are connected to Android-powered devices.
// import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;

// android.media: provides classes that manage various media interfaces in audio and video. 
// used for the generation of a tone. See the example:
// http://matrix-examplecode.blogspot.it/2011/08/dtmf-tone-generator.html
import android.media.AudioManager;
import android.media.ToneGenerator;

// android.location: Contains the framework API classes that define Android location-based and related services (for gps)
// import android.location.Location;
// import android.location.LocationListener;
// import android.location.LocationManager;
// import android.location.LocationProvider;
import android.util.DisplayMetrics;
// android.util: Provides common utility methods such as date/time manipulation, base64 encoders and decoders, string and number conversion methods, and XML utilities.
import android.util.Log;				// for debug logging
// import android.util.TypedValue;

// See project home page: http://code.google.com/p/usb-serial-for-android/
import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManager;
import com.ictp.mrainone.rftrack.R;
import com.ictp.mrainone.rftrack.helper.DatabaseHelper;
import com.ictp.mrainone.rftrack.tblmodel.RecCampaign;
import com.ictp.mrainone.rftrack.tblmodel.RecConfig;
import com.ictp.mrainone.rftrack.rfexplorer.RFEdBm;
import com.ictp.mrainone.rftrack.rfexplorer.RFEfreq;
import com.ictp.mrainone.rftrack.rfexplorer.RxRfMsg;
import com.ictp.mrainone.rftrack.rfexplorer.RFEdata;
import com.ictp.mrainone.rftrack.rfexplorer.RFEdataFifo;
import com.ictp.mrainone.rftrack.rfexplorer.RFEConfiguration;
import com.ictp.mrainone.rftrack.rfexplorer.GraphView;
import com.ictp.mrainone.rftrack.util.GPSTracker;
import com.ictp.mrainone.rftrack.util.sdmemory;
import com.ictp.mrainone.rftrack.util.HwInfo;
import com.ictp.mrainone.rftrack.util.MyEditText;
import com.ictp.mrainone.rftrack.util.TableGenerator;

// import au.com.bytecode.opencsv.*;
// import au.com.bytecode.opencsv.CSVWriter;

import java.net.URL;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;

/**
 * Activity for the measures acquisition of the Spectrum Analyzer RFExplorer
 * www.rf-explorer.com
 *
 * @author Marco Rainone. 
 * Program developed for the ICTP Wireless laboratory
 */

// To create an activity, you must create a subclass of Activity (or an existing subclass of it).
public class RfTrackActivity extends Activity
{
	//---------------------------------------
	// stringa contenente la versione
	final String ProgramName = "RfTrack Logger";
	final String ProgramVersion = "0.80 Beta - April 14th, 2016";

	//---------------------------------------
	// menu item
	Menu myMenu;
	
	// menu items
	// change 15/07: enabled only three items
    final int MENU_STOP_ACQ = Menu.FIRST;
    final int MENU_HW_CHECK = Menu.FIRST + 1;
    final int MENU_FONT_SIZE = Menu.FIRST + 2;
    final int MENU_CLEAN_SCREEN = Menu.FIRST + 3;
    final int MENU_HELP = Menu.FIRST + 4;   
	
	
	// ====================================
	// @@@ used for debugging
	// CharSequence contentCharSequence; 		// contain entire text content
	boolean bContentFormatHex = false;
	int contentFontSize = 16;
	boolean bWriteEcho = true;
    final String[] contentFormatItems = {"Character","Hexadecimal"};
    final String[] baudRateItems = {"500000","2400"};
    final String[] fontSizeItems = {"5","6","7","8","10","12","14","16","18","20"};
    final String[] echoSettingItems = {"On","Off"};
    final String[] typeEmailItems = {"PUBLIC to Main Server","PRIVATE to Main Server","Choose the email address"};

	// posto a true se e' ritorno da OnPause
	boolean fReturnOnPause;
	
    // Interface to global information about an application environment.
	public Context global_context;
    final Context context = this;
	
	// ====================================
	// graphical objects    
	ScrollView scrollView;
	TextView readText;
	EditText writeText;

    // EditText is a thin veneer over TextView that configures itself to be editable.
    private EditText editTextMainScreen;

    // Displays text to the user and optionally allows them to edit it. A TextView is a complete text editor,
    // however the basic class is configured to not allow editing
    private TextView mTitleTextView;
    private TextView mDumpTextView;

    // Layout container for a view hierarchy that can be scrolled by the user,
    // allowing it to be larger than the physical display
    private ScrollView mScrollView;
	
    private Button btnSetRFEparameters;
	
    // Returns the simple name of the class represented by this Class as defined in the source code.
    public final String TAG = RfTrackActivity.class.getSimpleName();

	// ====================================
	// ====================================
	// Gestione database
	// ====================================

	// Database Helper
	DatabaseHelper db;
	long campaign_id;				// id actual campaign

	// ====================================
	// ====================================
	// Generazione toni
	// ====================================

	//---------------------------------------
	// for the generation of the tones
	ToneGenerator tone;

	private void initSoundTone()
	{
		//-------------------------------------
		// 18/11/2014: init tone generator. 
		// A tone is generated for each group of measurements acquired by the instrument
        tone = new ToneGenerator(AudioManager.STREAM_DTMF, 100);
		//-------------------------------------
	}
	
	// aggiorna il segnale sonoro
	private void updateSoundTone()
	{
		// sound signaling storing a group of measures
		// a beep is generated each 32 readings
		if((Idx_Letture % 10) == 0)
		{
			// TONE_SUP_INTERCEPT_ABBREV=30
			tone.startTone(30,100);
		}
	}
	
	// ====================================
	// Fine Generazione toni
	// ====================================
	// ====================================
	
    //-----------------------------------------------------------
	// Configuration parameters of the measures
    //
    RFEConfiguration RfCfg = new RFEConfiguration();      // configurazione strumento
	
	// ====================================
	// ====================================
	// Gestione della seriale
	// ====================================

    // android.hardware.usb.UsbManager:
    // This class allows you to access the state of USB and communicate with USB devices.
    // Currently only host mode is supported in the public API.
    // The system's USB service.
    private UsbManager mUsbManager;

	private void getUsbManager()
	{
		// Get mUsbManager from Android.
        mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
	}
	
    /**
     * The device currently in use, or {@code null}.
     */
    private UsbSerialDriver mSerialDevice;

	//---------------------------------------
	// serial parameters 
	//---------------------------------------
	// the RF Explorer RF spectrum analyzer uses 2 baudrates: 500000 (default) and 2400.
	// Verify and set the correct baudrate using the menu instrument.
	// Note: transmission with baud rate from 500k has been tested even 
	// in the virtual machine virtualbox used for debugging
	//
	int baudRate = 500000;					// two baudrates: 2400 or 500000
//	final int baudRate = 2400;				// two baudrates: 2400 or 500000

	// per menu selezione baudrate
	int nBaudSelected = 0; // select at 0
	int nBaudBuffKey = nBaudSelected; // add buffer value 
	
	// per menu selezione tipo di email
	int nTypeEmail = 0;						// indice email di tipo PUBLIC
	/****
	Originale:
	// gestione coda messaggi
	RFEdataFifo RxFifo = new RFEdataFifo();
    // gestione messaggio lettura dati
    RxRfMsg RxMsg = new RxRfMsg(RxFifo);
	****/
	// modifica 22/07: la creazione e' stata fatta in OnCreate
	// gestione coda messaggi
	RFEdataFifo RxFifo;
    // gestione messaggio lettura dati
    RxRfMsg RxMsg;
	
	
	long Idx_Letture;			// Number of readings actually made

	// max values
	float MaxDBFreq;			// Value of the frequency corresponding to the maximum value of the signal
	float MaxDB;				// Maximum signal value in DB in floating point
	// byte MaxDBbyte;				// Byte value of the Maximum signal value in DB 

	// Configuration of the floating point format in the .csv file
	// DecimalFormat: subclass of NumberFormat that formats decimal numbers.
	DecimalFormat FormatterNumber;	// value formatter for generic number
	DecimalFormat FormatterFreq;	// value formatter for frequency
	DecimalFormat FormatterDBval;	// value formatter for DB
	DecimalFormat FormatterDBmax;	// value formatter for max DB
	
    // An Executor that provides methods to manage termination and methods that can produce a Future for tracking progress of one or more asynchronous tasks
    public final ExecutorService mExecutor = Executors.newSingleThreadExecutor();

	
	//----------------------------------------------------------------
	// manager ricezione seriale
	//
	
    public SerialInputOutputManager rxSerialIoManager;

    public final SerialInputOutputManager.Listener mListener =
        new SerialInputOutputManager.Listener()
    {

        @Override
        public void onRunError(Exception e)
        {
            Log.d(TAG, "Runner stopped.");
        }

        @Override
        public void onNewData(final byte[] data)
        {
            // Runs the specified action on the UI thread.
            // If the current thread is the UI thread, then the action is executed immediately.
            // If the current thread is not the UI thread, the action is posted to the event queue of the UI thread.
            // action: the action to run on the UI thread
            // RfTrackActivity.this.runOnUiThread
            // RfTrackActivity.this.runOnUiThread
            runOnUiThread(new Runnable() {
					@Override public void run()
					{
                        // ori RfTrackActivity.this.updateReceivedData(data);
                        RfTrackActivity.this.updateReceivedData(data);
					}
				}
            );
        }		// end public void onNewData
    };			// end new SerialInputOutputManager.Listener
	
	static boolean fDisableReceive = false;		// se true la ricezione e' disabiltata
	
	boolean setDisableReceive(boolean flag)
	{
		fDisableReceive = flag;
		return(fDisableReceive);
	}
	boolean IsReceiveDisabled()
	{
		return fDisableReceive;
	}
	
	// modifica 19/07
    public void updateReceivedData(byte[] data)
    {
		boolean initMaxDB = false;

		// for debugging:
        // final String message = "Read " + data.length + " bytes: \n" + HexDump.dumpHexString(data) + "\n\n";
		
		//-----------------------------------
		// modifica 07/08:
		// inserito il flag per dare piu' tempo per modificare i valori di configurazione.
		// 
		if(IsReceiveDisabled())
			return;
		//-----------------------------------
		
		// i messaggi sono inseriti in coda
		// updateRxData(data);
		RxMsg.updateRx(data);
		while(RxFifo.isEmpty() == false)
		{
			// There is a message completely received in queue
			RFEdata msg = RxFifo.poll();
			
			/****
			***/					
			//-----------------------------------------------------
			// calcola il tempo in msec del messaggio ricevuto
			long delta_tm_msg = RxMsg.DeltaTmMsec(msg.Time());

			//-----------------------------------------------------
			// write the data in the CSV file
			initMaxDB = updateCsvData(delta_tm_msg, msg);
			//-----------------------------------------------------

			//-----------------------------------------
			// sound signaling storing a group of measures
			// a beep is generated each 32 readings
			updateSoundTone();
			//-----------------------------------------
			
			// update the info on screen
			
			/***
			//----------------------
			// dbg: update the info in hex
			// mDumpTextView.append(message);
			mDumpTextView.append(HexDump.dumpHexString(RxMsg.GetMsg()));
			RxMsg.clean();
			mScrollView.smoothScrollTo(0, mDumpTextView.getBottom());
			//----------------------
			***/
			
			/***
			ori
			// show the maximum values
			if(updatedBm(initMaxDB, msg) == true)
			{
				ShowInfoPosMaxdBm();
				if(initMaxDB)
					initMaxDB = false;
			}
			***/
			// modifica 04/08
			ShowInfoNMeasurements(); 	// update n. measurements
			// ShowInfoFixPosition();
			ShowInfoGps();
			// aggiorna la percentuale campioni inferiori a -100dBm
			ShowInfoPercThreshold();
			
			//---------------------------------------
			// modifica 12/04
			// aggiorna il grafico con i dati del messaggio ricevuto
			graphView.updateValues(RxMsg.getGraphDbm());
			//---------------------------------------
			
			/****
			// disabilitato il 14/04
			if(RxMsg.MaxdBmIsChanged())
			{
				MaxDB = RxMsg.getMaxdBm();
				MaxDBFreq = RxMsg.getMaxdBmFreq();
				ShowInfoPosMaxdBm();
				
				RxMsg.resetChangeMaxdBm();		// reset flag change max dBm
			
	            //---  // per debug
	            //---  // String[] mrRow = {"33121.98311", "33181.49911", "4412.43311", "113211.43823", "2222.43233", "110.29999"};
	            //---  String[] mrRow = new String[6];
				//---  double[] coord;
				//---  DecimalFormat df = new DecimalFormat("* ########0.000000");
				//---  // get the actual position
				//---  coord = gps.getLastLocationCoordinates();
				//---  
				//---  // Time: 
				//---  SimpleDateFormat dateFormat = new SimpleDateFormat(
				//---  		"HH:mm:ss.SSS", Locale.getDefault());
				//---  Date date = new Date();
				//---  mrRow[0] = dateFormat.format(date);
				//---  // Latitude: 
				//---  mrRow[1] = df.format(coord[0]);
				//---  // Longitude:
				//---  mrRow[2] = df.format(coord[1]);
				//---  // Altitude: 
				//---  mrRow[3] = df.format(coord[2]);
				//---  // Freq (MHz): 
				//---  mrRow[4] = FormatterFreq.format(MaxDBFreq);
				//---  // Max dBm:
				//---  mrRow[5] = FormatterDBval.format(MaxDB);
				//---  
				//---  mTable.addRow(mrRow);
			}
			***/
			
			RxFifo.peek();

		}
    }			// updateReceivedData
	
    private boolean stopRxSerialManager()
    {
        if (rxSerialIoManager == null)
        {
			// non c'e' un manager di ricezione seriale da chiudere
			return false;
		}
		Log.i(TAG, "Stopping rx serial io manager ..");
		rxSerialIoManager.stop();
		rxSerialIoManager = null;
		return true;
    }

    private boolean startRxSerialManager()
    {
        if (isSerialAcquired()==false)
        {
			return false;
		}
		
		// la porta seriale e' disponibile
		Log.i(TAG, "Starting rx serial io manager ..");
		rxSerialIoManager = new SerialInputOutputManager(mSerialDevice, mListener);
		mExecutor.submit(rxSerialIoManager);
        return true;
    }
	
	/**************
	// @@@ la funzione updateReceivedData e' stata suddivisa in due parti
	// esegue l'update del messaggio letto dalla seriale.
	// Ritorna true quando il messaggio e' completo
    private boolean updateRxData(byte[] data)
    {
		return(RxMsg.updateRx(data));
	}
	**************/
	
		// aggiorna il max dBm. Restituisci true se e' da aggiornare
	
	/****
	04/08 disabilitato
	// modifica 30/7: usa il valore max letto con RFEdata
    private boolean updatedBm(boolean initMaxdBm, RFEdata msgAcq)
	{
		boolean result = false;			// false if the dBm values is not updated
		
		float val;

		val = RFEdBm.toFloat(msgAcq.GlobalMax());		// max convertito in float

		// Check if the dBm is greater of the actual value
		// if((initMaxdBm == true) || (MaxDB <= val))
		if((initMaxdBm == true) || (MaxDB < val))
		{
			MaxDB = val;
			// MaxDBbyte = msgAcq.GlobalMax();			// byte max dBm
			MaxDBFreq = RFEfreq.FreqMhz(msgAcq.GlobalIdxMax());
			result = true;					// frequency value updated.
		}
		return result;
	}
	****/

	// visualizza il n. di misure
	private void ShowInfoNMeasurements()
	{
		String data = "";
		// measurement number
		data = FormatterNumber.format((double)RxMsg.getNMsgRead());
		mTable.SetText(1, 0, data);
	}

	private void ShowInfoFixPosition()
	{
		// double[] coord;
		String data = "";
		// measurement number
		if(gps.getFixGps())
		{
			data = "Pos. Fixed";
		}
		else
		{
			data = "--------";
		}
		mTable.SetText(1, 1, data);
	}
	
	private void ShowInfoGps()
	{
		double[] coord;
		DecimalFormat df = new DecimalFormat("* ###0.00000");
		String data = "";
		
		// get the actual position
		coord = gps.getLastLocationCoordinates();
		// check if gps is fixed
		if(gps.getFixGps())
		{
			data = "Pos. Fixed";
		}
		else
		{
			data = "--------";
		}
		// update fix
		mTable.SetText(1, 1, data);
		
		// Latitude: 
		data = df.format(coord[0]);
		mTable.SetText(1, 2, data);
		// Longitude:
		data = df.format(coord[1]);
		mTable.SetText(1, 3, data);
		// Altitude: 
		data = df.format(coord[2]);
		mTable.SetText(1, 4, data);
	}

	private void ShowInfoPercThreshold()
	{
		DecimalFormat df = new DecimalFormat("* ###0");
		String data = "";
		
		// Percentuage samples < 100dBm: 
		data = df.format(RxMsg.getPercBelowThreshold());
		mTable.SetText(1, 5, data);
	}
	
	// visualizza i dBm e la posizione
	private void ShowInfoPosMaxdBm()
	{
		String[] rowCol = new String[8];
		double[] coord;
		// original: DecimalFormat df = new DecimalFormat("* ########0.000000");
		
		// modify 06/04
		DecimalFormat df = new DecimalFormat("* ###0.00000");
		DecimalFormat dfFreq = new DecimalFormat("* ##0.000");
		DecimalFormat dfDb = new DecimalFormat("* ##0.0");
		
		// get the actual position
		coord = gps.getLastLocationCoordinates();
		
		// Time: 
		/***
		original with milliseconds
		SimpleDateFormat dateFormat = new SimpleDateFormat(
				"HH:mm:ss.SSS", Locale.getDefault());
		****/
		SimpleDateFormat dateFormat = new SimpleDateFormat(
				"HH:mm:ss", Locale.getDefault());
		Date date = new Date();

		// measurement number
		rowCol[0] = FormatterNumber.format((double)RxMsg.getNMsgRead());
		
		// gps fix
		if(gps.getFixGps() == true)
		{
			rowCol[1] = "Fixed !!";
		}
		else
		{
			rowCol[1] = "--------";
		}
		// Latitude: 
		rowCol[2] = df.format(coord[0]);
		// Longitude:
		rowCol[3] = df.format(coord[1]);
		// Altitude: 
		rowCol[4] = df.format(coord[2]);
		// Max dBm:
		rowCol[5] = dfDb.format(MaxDB);
		// Freq (MHz): 
		rowCol[6] = dfFreq.format(MaxDBFreq);

		rowCol[7] = dateFormat.format(date);		// time
		
		// original
		// mTable.addRow(rowCol);
		// modify 06/04:
		mTable.SetTextColumn(1, rowCol);
		
		// update the screen
		// mDumpTextView.append(message);
		// mScrollView.smoothScrollTo(0, mDumpTextView.getBottom());
	}

	/****
    private void oriShowInfoPosMaxdBm()
	{
		double[] coord;
		DecimalFormat df = new DecimalFormat("* ########0.000000");

		// get the actual position
		coord = gps.getLastLocationCoordinates();
		
		// ori:
		// String message = "[Latitude: " + df.format(gps.getLatitude()) +
        //         " - Longitude: " + df.format(gps.getLongitude()) +
		// 		"]    Freq.: " + FormatterFreq.format(MaxDBFreq) + 
		// 		" . Max dBm: " + FormatterDBval.format(MaxDB) + "\n";
		String message = "[Latitude: " + df.format(coord[0]) +
                " - Longitude: " + df.format(coord[1]) +
				"]    Freq.: " + FormatterFreq.format(MaxDBFreq) + 
				" . Max dBm: " + FormatterDBval.format(MaxDB) + "\n";
		
		// for testing
		// " . Max dBm: " + FormatterDBmax.format(MaxDB) + "[" + String.format("%04X", MaxDBbyte) + "]" + "\n";
		
		// update the screen
		mDumpTextView.append(message);
		mScrollView.smoothScrollTo(0, mDumpTextView.getBottom());
	}
	****/

	// restituisce true se la seriale e' acquisita
	private boolean isSerialAcquired()
	{
		boolean fAcquired = false;

        if (mSerialDevice != null)
        {
			fAcquired = true;
		}
		return fAcquired;
	}
	
	private boolean OpenSerial()
	{
		boolean done = true;
		
        if (isSerialAcquired() == false)
        {
			return false;
		}
		
		try
		{
			mSerialDevice.open();
		}
		catch (IOException e)
		{
			Log.e(TAG, "Error OpenSerial: " + e.getMessage(), e);
			done = false;
			// Ignore.
		}
		return done;
	}
	
	private boolean CloseSerial()
	{
		boolean done = true;
		
        if (isSerialAcquired() == false)
        {
			return false;
		}
		
		try
		{
			mSerialDevice.close();
		}
		catch (IOException e)
		{
			Log.e(TAG, "Error CloseSerial: " + e.getMessage(), e);
			done = false;
			// Ignore.
		}
		//----------------------------
		// 23/08: disabled
		// mSerialDevice = null;
		//----------------------------
		
		return done;
	}

	private boolean SetBaudSerial(int baud)
	{
		boolean done = true;
		
        if (isSerialAcquired() == false)
        {
			return false;
		}
		
		try
		{
			mSerialDevice.setParameters(baud, 8, 1, 0);
		}
		catch (IOException e)
		{
			Log.e(TAG, "Error SetBaudSerial: " + e.getMessage(), e);
			done = false;
			// Ignore.
		}
		return done;
	}

	private boolean SendCfgMessage(int timeoutMillis)
	{
		boolean done = true;

        if (isSerialAcquired() == false)
        {
			return false;
		}
		
		try
		{
			//---------------------
			// mr: invio del messaggio
			// String msg = "# C2-F:0400000,0500000,-050,-120";

			// vettore di byte contenente il messaggio di configurazione
			byte[] msgSendCfg = 
				RxMsg.MsgConfigurationData(
					RFEfreq.GetStart(),		// KHZ, Value of frequency span start (lower)
					RFEfreq.GetEnd(),		// KHZ, Value of frequency span end (higher)
					RfCfg.Amp_Top,			// dBm, Highest value of amplitude for GUI
					RfCfg.Amp_Bottom );		// dBm, Lowest value of amplitude for GUI
			mSerialDevice.write(msgSendCfg, timeoutMillis);					// send the message

			// Convert the string message to display it
			String msg = new String(msgSendCfg);			// conversione corretta byte[] to String
			// aggiorna il log
			Log.i("mr", "Msg protocollo:>" + msg);
			
		}
		catch (IOException e)
		{
			Log.e(TAG, "Error SendCfgMessage: " + e.getMessage(), e);
			done = false;
			// Ignore.
		}
		return done;
	}
	
	// ====================================
	// fine Gestione della seriale
	// ====================================
	// ====================================


	// ====================================
	// ====================================
	// inizio Gestione file csv
	// ====================================

    // change 03/11/2014:
    // designed two types of files:
    // .csv contain data acquisitions
    // .log contains information about the data acquisition

    //-----------------------------------------------------------
    // mr:
    // External Storage
    // http://www.androidaspect.com/2014/02/android-external-storage.html
    // Checking External Storage Availability

    final String AcqDataDirectory = "/RFTrk_data";       // Directory that contains the files of acquisitions

	
    // SD Card Storage
    sdmemory sdCard = new sdmemory();	// sd card external memory
    File directory;				// directory containing the file

	String FileName;			// file name 
	String FileExt;				// extension of the file containing the data
	String FileExtLog;          // extension of the file containing the log info

	// file that stores log info
    File fileLog;               // "abstract" representation of a file system entity identified by a pathname.
    FileOutputStream fosLog;    // output stream that writes bytes to the log file
    OutputStreamWriter oswLog;  // bridge from character streams to byte streams
	
	// file that stores the data
    File file;					// File: An "abstract" representation of a file system entity identified by a pathname.
    FileOutputStream fos;		// output stream that writes bytes to the data file
    OutputStreamWriter osw;		// bridge from character streams to byte streams: Characters written to it are encoded into bytes using a specified charset.
	
	//---------------------------------------
	long Csv_start_time;		// Time in milliseconds to start creating the csv file

	char DecimalSeparator;		// Decimal separator char
	String CsvSeparator;		// Separator in the line of the csv file

	DecimalFormatSymbols CsvDecimalFormatSym;		// Used to set the value of the numerical formatting
	
	// Writing a CSV file with Java using OpenCSV
	// http://snipplr.com/view/58019/writing-a-csv-file-with-java-using-opencsv/
	BufferedWriter outCsv;
	
	boolean fOpen;				// True when the data file is open to save the measures

	//=================================================
	// SD CARD UTILITIES
	//=================================================
	
	public static final String  EXTERNALSD_FILE_PATH	= Environment.getExternalStorageDirectory().getAbsolutePath();
	// directory che contiene i database
	public static final String  NameDirectoryOfDb		= "RFE_db";
	public static String  DIRDB_FULL_PATH;			// contiene il path completo della directory dei db
	
		// restituisci il path completo della directory del db
	public static String DirDbFullPath() {
		// full path name of db directory
		DIRDB_FULL_PATH = EXTERNALSD_FILE_PATH + File.separator + NameDirectoryOfDb;
		File directoryOfDb = new File(DIRDB_FULL_PATH);
		if (!directoryOfDb.exists()) {
			directoryOfDb.mkdirs();
		}
		// return the complete path
		return (directoryOfDb.getPath());
	}

	
	//------------------------------------------------------------------------
	// NOTE:
    // The external storage may be unavailable.
	// Thatbecause we can mount it as USB storage and in some cases remove it from the device.
    // Therefore we should always check its availability before using it.
    // We can simply check external storage availability using the getExternalStorageState() method.

	// Create the file name containing the date
	// See these info:
	// http://www.tutorialspoint.com/java/java_date_time.htm
	// http://stackoverflow.com/questions/2271131/display-the-current-time-and-date-in-an-android-application
	//
	public void SetFileName()
	{
        Calendar c = Calendar.getInstance();
        // System.out.println("Current time => "+c.getTime());
        SimpleDateFormat df = new SimpleDateFormat("yyMMdd_HHmmss");
        // FileName have current date/time
        FileName = df.format(c.getTime());
		
		// type of extensions used:
		FileExt =  "csv";             // Filename extension for the file with measures
		FileExtLog = "log";           // Filename extension for the file with log infos
	}
	
	public void OpenCsvFile()
    {
        if (!sdCard.isMounted())
        {
            // SD Card Not Available
            fOpen = false;
			return;
        }
		try
		{
			// SD Card Storage
			// sdCard = Environment.getExternalStorageDirectory();
            directory = new File(sdCard.getAbsolutePath() + AcqDataDirectory);
			if (!directory.exists())
				directory.mkdirs();
			
			SetFileName();								// Create a file name containing current date and time
			
			String fname = FileName + "." + FileExt;
			file = new File(directory, fname);
			fos = new FileOutputStream(file);
            osw = new OutputStreamWriter(fos);       	// osw: file with csv data
            
            
            String fnameLog = FileName + "." + FileExtLog;
            fileLog = new File(directory, fnameLog);
            fosLog = new FileOutputStream(fileLog);
            oswLog = new OutputStreamWriter(fosLog);    // oswLog: file with log data

			//-------------------------------
			// set file csv parameters
			DecimalSeparator = ',';			// use comma for decimal separator
			CsvSeparator = ";";				// csv fields separated by ';'
			
			// Set formatter for numeric format. See
			// http://www.coderanch.com/t/385190/java/java/Setting-decimalFormatSymbols
			CsvDecimalFormatSym = new DecimalFormatSymbols(Locale.US);  
			CsvDecimalFormatSym.setDecimalSeparator(DecimalSeparator);
			
			//-------------------------------
			// http://www.coderanch.com/t/385190/java/java/Setting-decimalFormatSymbols
			FormatterNumber = new DecimalFormat("* ##0", CsvDecimalFormatSym);
			FormatterFreq = new DecimalFormat("* ##0.000", CsvDecimalFormatSym);
			FormatterDBval = new DecimalFormat("* #####0.0", CsvDecimalFormatSym);
			FormatterDBmax = new DecimalFormat("* #####0.0", CsvDecimalFormatSym);
 			
			// initial time in msec
			// http://stackoverflow.com/questions/9707938/calculating-time-difference-in-milliseconds
			Csv_start_time = System.currentTimeMillis();
			
			Idx_Letture = 0L;
			// success message
			fOpen = true;
			
			// Write the initial values in the log file
			WriteCsvInitialInfo();
		}
		catch (IOException e)
		{
			e.printStackTrace();
		}
    }

	// Write the initial values in the log file
    public void WriteCsvInitialInfo()
    {
        if(!fOpen)
        {
			return;
		}
		// write the string to the file
		try
		{
			String str;
			str = String.format("Start Frequency:") +
				CsvSeparator+
				String.format("% 8d", RFEfreq.GetStart()) +
				String.format("\n");
            oswLog.write(str);
			str = String.format("End Frequency:") +
				CsvSeparator+
				String.format("% 8d", RFEfreq.GetEnd()) +
				String.format("\n");
			oswLog.write(str);
			str = String.format("Amplitude Highest value dBm:") +
				CsvSeparator+
				String.format("% 8d", RfCfg.Amp_Top) +
				String.format("\n");
			oswLog.write(str);
			str = String.format("Amplitude Lowest value dBm:") +
				CsvSeparator+
				String.format("% 8d", RfCfg.Amp_Bottom) +
				String.format("\n\n");
			oswLog.write(str);
			oswLog.flush();
		}
		catch(IOException ie)
		{
			 ie.printStackTrace();
		}
    }

	// Calculates the difference between the present time and an initial time
	public long DeltaMsec(long start_time)
	{
		long end_time = System.currentTimeMillis();
		return(end_time-start_time);
	}
	
    public void WriteCsvData(long tMsec, String values)
    {
        if(!fOpen)
        {
			return;
		}
		// write the string to the file
		try
		{
			double[] coord;
			
            // ori DecimalFormat dtimef = new DecimalFormat("* ###0.0");
			// per dbg
            DecimalFormat dtimef = new DecimalFormat("* ###0.000");
            
			// read the actual position:
			coord = gps.getLocationCoordinates();
			
			// ori
			// String tm = dtimef.format(((float)DeltaMsec(Csv_start_time))/1000.0f);
            // modifica 19/07
			String tm = dtimef.format(((float)tMsec)/1000.0f);
            osw.write(tm);
            osw.write(CsvSeparator);
			//--------------------------------
			// posizione
			DecimalFormat df = new DecimalFormat("* ########0.000000");
            // ori abilitata osw.write(df.format(Latitude));
            // osw.write(df.format(gps.getLatitude()));
            osw.write(df.format(coord[0]));
            osw.write(CsvSeparator);
            // ori
            // osw.write(df.format(Longitude));
            // osw.write(df.format(gps.getLongitude()));
            osw.write(df.format(coord[1]));
            osw.write(CsvSeparator);
			//--------------------------------
            osw.write(values);
            osw.write("\n");
            // osw.write(values + "\n");
			osw.flush();
		}
		catch(IOException ie)
		{
			 ie.printStackTrace();
		}
    }

	/****
	// Create a line with the values of of frequency
	public String dumpCsvFreq(byte[] array) 
	{
        // int length = array.length;
		String result = "";
		byte Nval;
		float freq;

		// http://www.coderanch.com/t/385190/java/java/Setting-decimalFormatSymbols
        // DecimalFormat FormatterFreq = new DecimalFormat("####.000", CsvDecimalFormatSym);
        // DecimalFormat FormatterFreq = new DecimalFormat("* ##0.000", CsvDecimalFormatSym);
		
		int offset = 0;
		Nval = array[2];			// contiene il numero di valori seguenti

		result= result + String.format("% 4d",(int)Nval);
		result= result + CsvSeparator;
		// for (int i = offset; i < offset + length; i++) 
		for (int i = 0; i < Nval; i++) 
		{
			freq = RfCfg.GetFrequency(i, Nval);
			if(i>offset)
			{
				result= result + CsvSeparator;
			}
			result= result + FormatterFreq.format(freq);
		}

		return result;
	}
	****/
	
	// Encode a row in csv format with the values contained in a byte array
	public String dumpCsvString(byte[] array) 
	{
        int length = array.length;
		String result = "";
		
		// see:
		// http://www.coderanch.com/t/385190/java/java/Setting-decimalFormatSymbols
        // DecimalFormat FloatFormatter = new DecimalFormat("####.0", CsvDecimalFormatSym);
        // DecimalFormat FormatterDBval = new DecimalFormat("* #####0.0", CsvDecimalFormatSym);
		
		// Warning:
		// in Java, all integer values are signed.
		// see http://www.javamex.com/java_equivalents/unsigned.shtml
		// the c++ code:
		// unsigned byte b = ...; b += 100;
		// In java is equivalent:
		// int b = ...; b = (b + 100) & 0xff;
		// int b;
		
		int Nval;
		float val;
		
		// int offset = 0;
		// Nval = array[2];				// has the following values
		Nval = RFEfreq.GetSteps();				// has the following values
		// if((offset + length)<(Nval +1))
		if(length < Nval)
		{
			// The message length is below the expected
			return result;
		}
		// Insert the number of values
		result= result + String.format("% 4d",(int)Nval);
		// for (int i = 3; i < 3 + Nval; i++) 
		for (int i = 0; i < Nval; i++) 
		{
			// Insert the separator
			// if(i>offset)
			{
				result= result + CsvSeparator;
			}
			// Insert the value: Beware of the unsigned value !!!
			val = RFEdBm.toFloat(array[i]);
			
			// result= result + String.format("% 5.1f", val);
            // result= result + FormatterDBval.format(val).replaceAll("\\G0", " ");
            result= result + FormatterDBval.format(val);
		}

		return result;
	}

	// aggiorna il file csv. Restituisci true se e' la prima lettura
    private boolean updateCsvData(long tMsec, RFEdata msgAcq)
    {
		boolean firstRead = false;
		// ori byte[] dataRx = RxMsg.GetMsg();
		byte[] dataRx = msgAcq.Msg();
		
		// calcola il tempo di ricezione messaggio in secondi

		// write the data in the CSV file
		if(Idx_Letture == 0L)
		{
			firstRead = true;
			// WriteCsvData(dumpCsvFreq(dataRx));
			WriteCsvData(tMsec, RFEfreq.CsvString(FormatterFreq, CsvSeparator));
		}
		// WriteCsvData(dumpCsvString(dataRx));
		WriteCsvData(tMsec, RFEdBm.CsvString(dataRx, FormatterDBval, CsvSeparator));
		Idx_Letture = Idx_Letture + 1;
		return firstRead;
	}
	
    public void CloseCsvFile()
    {
        try
        {
            osw.close();                // close the csv file
            oswLog.close();             // close the log file
        }
        catch(IOException ie)
        {
            ie.printStackTrace();
        }
    }

	
	// ====================================
	// fine Gestione file csv
	// ====================================
	// ====================================

	// ====================================
	// ====================================
	// start LOCATION MANAGEMENT
	// ====================================

	GPSTracker gps;
	
	/*******
	// 23/07: originale disabilitato
	// Al suo posto usato GPSTracker
	//
	//---------------------------------------
	// to read gps
	// vedere anche i progetti:
	// http://www.tutos-android.com/geolocalisation-android
	// per lettura Altitude, Accuracy
	// per speed:
	// http://jatin4589.blogspot.it/2013/08/get-speed-from-gps.html
	//
	// http://developer.android.com/reference/android/location/Location.html
	
    private LocationManager locationManager;
    String GpsStatus;

	double Latitude;	// latitude, in degrees.
	double Longitude;	// longitude, in degrees.
	double Altitude;	// altitude if available, in meters above the WGS 84 reference ellipsoid.
	float Speed;		// speed if it is available, in meters/second over ground.	
	float Accuracy;		// estimated accuracy of this location, in meters.
	long locTime;		// UTC time of this fix, in milliseconds since January 1, 1970.
	
	private void getLocationManager()
	{
        locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
	}
	
    private final LocationListener gpsLocationListener = new LocationListener() {

        @Override
        public void onStatusChanged(String provider, int status, Bundle extras) {
            switch (status) {
            case LocationProvider.AVAILABLE:
                GpsStatus = String.format("GPS available again");
                break;
            case LocationProvider.OUT_OF_SERVICE:
                GpsStatus = String.format("PS out of service");
                break;
            case LocationProvider.TEMPORARILY_UNAVAILABLE:
                GpsStatus = String.format("GPS temporarily unavailable");
                break;
            }
			editTextMainScreen.setText(GpsStatus);
        }

        @Override
        public void onProviderEnabled(String provider) {
            GpsStatus = String.format("GPS Provider Enabled");
			editTextMainScreen.setText(GpsStatus);
        }

        @Override
        public void onProviderDisabled(String provider) {
            GpsStatus = String.format("GPS Provider Disabled\n");
			editTextMainScreen.setText(GpsStatus);
        }

        @Override
        public void onLocationChanged(Location location) {
            // locationManager.removeUpdates(networkLocationListener);

			if (location == null)
			{
				Latitude = 0.0;		// latitude, in degrees.
				Longitude = 0.0;	// longitude, in degrees.
				Altitude = 0.0;		// altitude if available, in meters above the WGS 84 reference ellipsoid.
				Speed = 0.0f;		// speed if it is available, in meters/second over ground.	
				Accuracy = 0.0f;	// estimated accuracy of this location, in meters.
				locTime = 0;		// UTC time of this fix, in milliseconds since January 1, 1970.
				return;
			}
			
			Longitude = location.getLongitude();
			Latitude = location.getLatitude();
			// aggiunta 15/07
			Speed = location.getSpeed();
			Altitude = location.getAltitude();
			Accuracy = location.getAccuracy();
			locTime = location.getTime();
			
            GpsStatus = String.format("GPS location: "
                    + String.format("% 9.6f - ", location.getLatitude()) + ", "
                    + String.format("% 9.6f", location.getLongitude()) + "\n");
			editTextMainScreen.setText(GpsStatus);
        }
    };				// end of private final LocationListener gpsLocationListener

	*************/
	// ====================================
	// end LOCATION MANAGEMENT
	// ====================================
	// ====================================

//============================================================
// dialogs created programmatically
//============================================================

// dialogo per segnalare abilitazione ridotta funzioni
// L'app viene lanciata automaticamente se viene connesso il cavo otg collegato allo strumento.
// E' inutile fare tentativi di riconnessione, quindi si segnala che sono disabilitate 
// le funzioni start new campaign ed open campaign
//
void dialogInfoRFENotCommected(String title)
{
	Log.i(TAG, "show dialogInfoRFENotCommected");
	
	AlertDialog.Builder builder = 
	new AlertDialog.Builder(global_context);

	// builder.setTitle("Start Serial Communication");
	builder.setTitle(title);
	builder.setIcon(R.drawable.ictp_logo);
	
	builder.setMessage(
		"The app runs without connection to RFExplorer.\n" +
		"The following functions are disabled:\n" +
		"   1) Start New Campaign\n" +
		"   2) Open Campaign\n" +
		"To collect campaign data, exit and attach to the USB port\n" +
		"an OTG cable connected to the RFExplorer.");
	
	builder.setCancelable(false);
	builder.setPositiveButton("OK", 
		new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) 
			{
				mSerialDevice = UsbSerialProber.acquire(mUsbManager);
				Log.d(TAG, "mSerialDevice=" + mSerialDevice);
				if (isSerialAcquired() == false)
				{
					// serial was not acquired
					// mTitleTextView.setText("No serial device acquired.");
					// return;
					Toast.makeText(
							global_context, 
							"RFExplorer not connected!", 
							Toast.LENGTH_SHORT
							)
							.show();
				}
				else
				{
					// serial was not acquired
					// mTitleTextView.setText("No serial device acquired.");
					// return;
					Toast.makeText(
							global_context, 
							"Connected!", 
							Toast.LENGTH_SHORT
							)
							.show();
				}
				
				dialog.cancel();
			}
		}
	);
	AlertDialog alert = builder.create();
	alert.show();
}



// dialogo per inizio connessione seriale
void dialogSerialStartConnect(String title)
{
	Log.i(TAG, "show dialogSerialStartConnect");
	
	AlertDialog.Builder builder = 
	new AlertDialog.Builder(global_context);

	// builder.setTitle("Start Serial Communication");
	builder.setTitle(title);
	builder.setIcon(R.drawable.ictp_logo);
	
	builder.setMessage(
		"The RFExplorer seems not connected.\n" +
		"Please connect the instrument to the Android device\n" +
		"with an Usb Host cable.\n" +
		"then press the button to connect.\n" +
		"If necessary, disconnect and reattach the cable,\n" +
		"then press the button to connect.");
	
	builder.setCancelable(false);
	builder.setPositiveButton("Test connection...", 
		new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int which) 
			{
				mSerialDevice = UsbSerialProber.acquire(mUsbManager);
				Log.d(TAG, "mSerialDevice=" + mSerialDevice);
				if (isSerialAcquired() == false)
				{
					// serial was not acquired
					// mTitleTextView.setText("No serial device acquired.");
					// return;
					Toast.makeText(
							global_context, 
							"Not connected!", 
							Toast.LENGTH_SHORT
							)
							.show();
				}
				else
				{
					// serial was not acquired
					// mTitleTextView.setText("No serial device acquired.");
					// return;
					Toast.makeText(
							global_context, 
							"Connected!", 
							Toast.LENGTH_SHORT
							)
							.show();
				}
				
				/***
				//---------------------------------------------------------------
				// visualizza il nome della porta seriale aperta
				mTitleTextView.setText("Serial device: " + mSerialDevice);
				
				// serial acquired
				if(OpenSerial() == false)
				{
					mTitleTextView.setText("Error opening serial port.");
					CloseSerial();
					return;
				}
				// abilita il manager di ricezione
				stopRxSerialManager();
				startRxSerialManager();
				**/
				
				dialog.cancel();
			}
		}
	);
	AlertDialog alert = builder.create();
	alert.show();
}


// dialog per inizio comunicazione
void dialogSerialStartCommunicate(String title)
{
	Log.i(TAG, "show dialogSerialStartCommunicate");
	
	AlertDialog.Builder builder = 
	new AlertDialog.Builder(global_context);
	// builder.setTitle("Start Serial Communication");
	builder.setTitle(title);
	
	// builder.setMessage("Click connect...");
	
	builder.setSingleChoiceItems(
		baudRateItems, 
		nBaudSelected,
		new DialogInterface.OnClickListener() {
			
			@Override
			public void onClick(
			DialogInterface dialog, 
			int which) {
				Toast.makeText(
				global_context, 
				"Select "+baudRateItems[which], 
				Toast.LENGTH_SHORT
				)
				.show();
				
				// set to buffKey instead of selected (when cancel not save to selected)
				nBaudBuffKey = which;
			}
		}
	);
	
	builder.setCancelable(false);
	builder.setPositiveButton("Connect", 
		new DialogInterface.OnClickListener() 
		{
			@Override
			public void onClick(DialogInterface dialog, 
					int which) {
				Log.d(TAG,"Which value="+which);
				Log.d(TAG,"Selected value="+nBaudBuffKey);
				Toast.makeText(
						global_context, 
						"Select "+baudRateItems[nBaudBuffKey], 
						Toast.LENGTH_SHORT
						)
						.show();
				//set buff to selected
				nBaudSelected = nBaudBuffKey;
				if(nBaudBuffKey==0)
				{
					baudRate = 500000;					// two baudrates: 2400 or 500000
				}
				else
				{
					baudRate = 2400;					// two baudrates: 2400 or 500000
				}
				
				// ========================== SET COMMUNICATION PARAMETERS
				if(SetBaudSerial(baudRate) == false)
				{
					mTitleTextView.setText("Error setting baudrate port " + mSerialDevice);
					CloseSerial();
					return;
				}				
				// invia il messaggio di configurazione
				SendCfgMessage(1000);
				editTextMainScreen.setText(RfCfg.strInfoParam());		// show the message
				
				// cancella la textview del data logging
				mDumpTextView.setText("");

				// chiusura del file CSV precedente e apertura di uno nuovo
				// ci pensa il garbage collector a rilasciare la memoria
				CloseCsvFile();
				OpenCsvFile();			// crea un nuovo file CSV
				//-------------------------------------
				// modifica 07/08/2015
				setDisableReceive(false);			// abilita la ricezione
				//-------------------------------------
			}
		}
	);
	builder.setNegativeButton("Cancel", 
		new DialogInterface.OnClickListener() 
		{
			@Override
			public void onClick(DialogInterface dialog, 
					int which) {
				Toast.makeText(
						global_context, 
						"Click cancel", 
						Toast.LENGTH_SHORT
						)
						.show();
			}
		}
	);
	AlertDialog alert = builder.create();
	alert.show();
}
	
//============================================================
// Auxiliary functions for graphics
//============================================================

	// Set the size of the font used to display
	void SetTextFontSize()
	{
		mDumpTextView.setTextSize(contentFontSize);
		mTitleTextView.setTextSize(contentFontSize);
		editTextMainScreen.setTextSize(contentFontSize);
	}
	
//============================================================
// START OF MENU FUNCTIONS
//

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		myMenu = menu;

		// change 15/07: enabled only three items
		
		// http://www.linuxtopia.org/online_books/android/devguide/guide/topics/ui/menus.html
		// To make a single item checkable, use the setCheckable() method
		myMenu.add(0, MENU_STOP_ACQ, 0, "Stop Acquisition");
		/*****
		MENU VOICES DISABLED
		myMenu.add(0, MENU_HW_CHECK, 0, "Hardware check");
		myMenu.add(0, MENU_FONT_SIZE, 0, "Font Size");
		myMenu.add(0, MENU_CLEAN_SCREEN, 0, "Clean Screen");
		myMenu.add(0, MENU_HELP, 0, "Info");
		*****/
		
		return super.onCreateOptionsMenu(myMenu);
	}

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId())
        {
			
        case MENU_STOP_ACQ:
			{
				AlertDialog dialog = new AlertDialog.Builder(global_context).create();
				dialog.setTitle("Stop campaign");
				dialog.setMessage("Are you sure (Yes or No)");
				dialog.setCancelable(false);
				dialog.setButton(DialogInterface.BUTTON_POSITIVE, "Yes", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int buttonId) {
						//---------------------------------------------------
						// Stop acquisition and return to main menu
						dialog.dismiss();
						//-------------------------------------
						// modifica 07/08/2015
						// CloseSerial();
						setDisableReceive(true);			// disabilita la ricezione fino a quando non scegli un'opzione di menu
						//-------------------------------------
						CloseCsvFile();
						db.closeDB();
						showMainCmdMenu();		// torna al main menu
						//---------------------------------------------------
					}
				});
				dialog.setButton(DialogInterface.BUTTON_NEGATIVE, "No", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int buttonId) {
						dialog.dismiss();
					}
				});
				dialog.setIcon(android.R.drawable.ic_dialog_alert);
				dialog.show();
			}
			break;

		/**********************************
		MENU VOICES DISABLES
		
        case MENU_HW_CHECK:
        	//-----------------------------------------------------------
			// show the program version info
			{
				String InfoGps;
				String InfoNetwork;
				String InfoLocProvider;
				String InfoSd;
				
				if(gps.hasGPSandIsEnabled())
					InfoGps = "Gps enabled";
				else
					InfoGps = "Gps disabled";
				if(gps.hasNetwork())
					InfoNetwork = "network enabled";
				else
					InfoNetwork = "network disabled";
				InfoLocProvider = gps.getLocationProvider();
				if(HwInfo.hasSdCard())
					InfoSd = "Sd connected";
				else
					InfoSd = "Sd not connected";
					
			midToast( "Hardware\n\n" +
						"GPS: " + InfoGps + "\n" +
						"Net: " + InfoNetwork + "\n" +
						"Loc: " + InfoLocProvider + "\n" +
						"Secure Digital: " + InfoSd + "\n"
				, Toast.LENGTH_LONG);
			break;
			}   
			
			
        case MENU_FONT_SIZE:
        	//-----------------------------------------------------------
			new AlertDialog.Builder(global_context).setTitle("Font Size")
			.setItems(fontSizeItems, new DialogInterface.OnClickListener() 
			{
				@Override
				public void onClick(DialogInterface dialog, int which)
				{	
					contentFontSize = Integer.parseInt(fontSizeItems[which]);
					// sets the font size to display data
					SetTextFontSize();
					
				}
			}).show();
			
            break;

        case MENU_CLEAN_SCREEN:
        	//-----------------------------------------------------------
			// clear the TextView of the data logging
			mDumpTextView.setText("");
            break;

        case MENU_HELP:
        	//-----------------------------------------------------------
			// show the program version info
			{
			midToast( 
				ProgramName + "\n" +
				"Ver. " + ProgramVersion + "\n\n" +
				"Author: Marco Rainone -\n"
				, Toast.LENGTH_LONG);
			break;
			}   
		*******************************************/
		
        default:
        	break;
        }
 
        return super.onOptionsItemSelected(item);
    }
	
//
// END OF MENU FUNCTIONS
//============================================================

    //-----------------------------------------------------------
	private boolean bFlagAnswerDialog;		// posto true o false dal dialog
	
	public void showMainCmdMenu() 
	{
		// AlertDialog.Builder builder3 = new AlertDialog.Builder(AppActivity.this);
		AlertDialog.Builder builder3 = new AlertDialog.Builder(global_context);
		builder3.setTitle("RFTrack");
		builder3.setIcon(R.drawable.ictp_logo);
		if (isSerialAcquired())
		{
			final CharSequence[] items = { 
				"Start New Campaign", 
				"Open Campaign",
				"Send Campaign to Email",
				"Upload Campaign to Website",
				/***
				// 14/09: disabled. Problem RFExplorer
				"RFExplorer Frequency Limits",
				***/
				"Check GPS", 
				"Android Hardware Info", 
				"Program Info" 
				};
			builder3.setItems(items,
			// builder3.setSingleChoiceItems(items,
			new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog,
				int which) {
					// TODO Auto-generated method stub

					Toast.makeText(getApplicationContext(),
					"You clicked " + items[which],
					Toast.LENGTH_LONG).show();
					
					switch(which){
					case 0:
							showNewCampaign();
						break;
					case 1:
							showOpenCampaign();
						break;
					case 2:
							// 14/04/2016 sostituito: showSelectTypeEmail();
							ForcePublicEmail();
						break;
					case 3:
							//TODO: upload to website
							uploadToWebsite();
						break;
					/*==========
					// 14/09: disabled. Problem on RFExplorer
					case 3:
						showSelectRfExplorer();
						break;
					==========*/
					case 4:
							showGpsCheck();
						break;
					case 5:
							showHardwareCheck();
						break;
					case 6:
							showProgramInfo();
						break;
					}
				}

			});

		}
		else
		{
//			final CharSequence[] items = { 
		    final CharSequence[] items = { 
				// "Start New Campaign", 
				// "Open Campaign",
				"Send Campaign to Email",
				"Upload Campaign to Website?",
				/***
				// 14/09: disabled. Problem RFExplorer
				"RFExplorer Frequency Limits",
				***/
				"Check GPS", 
				"Android Hardware Info", 
				"Program Info" 
				};

			builder3.setItems(items,
			// builder3.setSingleChoiceItems(items,
			new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog,
				int which) {
					// TODO Auto-generated method stub

					Toast.makeText(getApplicationContext(),
					"You clicked " + items[which],
					Toast.LENGTH_LONG).show();
					
					switch(which){
					case 0:
							// 14/04/2016: sostituito: showSelectTypeEmail();
							ForcePublicEmail();
						break;
					case 1:
							//TODO: upload to website
							uploadToWebsite();
						break;
					case 2:
							showGpsCheck();
						break;
					case 3:
							showHardwareCheck();
						break;

					/*==========
					// 14/09: disabled. Problem on RFExplorer
					case 3:
						showSelectRfExplorer();
						break;
					==========*/
					case 4:
							showProgramInfo();
						break;
					}
				}

			});
		}
		// original: builder3.show();

		// mr modify 05/04
		AlertDialog alert = builder3.create();
		alert.show();
		
		/****
		// mr 05/04: disable new campaign open campaign if serial is not acquired.
		// http://stackoverflow.com/questions/33027918/android-alertdialog-how-to-disable-certain-choices-that-are-not-available
		list = alert.getListView();
		if (isSerialAcquired()==false)
		{
			list.getChildAt(0).setEnabled(false);
			list.getChildAt(1).setEnabled(false);
		}
		else
		{
			list.getChildAt(0).setEnabled(true);
			list.getChildAt(1).setEnabled(true);
		}
		list.getChildAt(2).setEnabled(true);
		list.getChildAt(3).setEnabled(true);
		list.getChildAt(4).setEnabled(true);
		list.getChildAt(5).setEnabled(true);
		// final ListAdapter adaptor = alert.getListView().getAdapter();
		// adaptor.getView(0, null, list).setEnabled(true);
		// adaptor.getView(1, null, list).setEnabled(true);
		// adaptor.getView(2, null, list).setEnabled(true);
		// adaptor.getView(3, null, list).setEnabled(true);
		// adaptor.getView(4, null, list).setEnabled(true);
		// adaptor.getView(5, null, list).setEnabled(true);
		// if (isSerialAcquired()==false)
		// {
			// adaptor.getView(0, null, list).setEnabled(false);
			// adaptor.getView(1, null, list).setEnabled(false);
		// }
		***/
	}
	
//----------------------------------------------
	// Add 11/09: select RFExplorer Model
	public void showSelectRfExplorer() 
	{
			
		AlertDialog.Builder builder = new AlertDialog.Builder(global_context);
		
		builder.setTitle("RFExplorer Models");

	builder.setSingleChoiceItems(
		RFEfreq.RFExplorerName, 
		0,
		new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog,
			int which) {
				Toast.makeText(getApplicationContext(),
				"You clicked " + RFEfreq.RFExplorerName[which],
				Toast.LENGTH_LONG).show();
				
				RFEfreq.idxModel = which;
				if(which<3)
					showFreqModBase(RFEfreq.RFExplorerName[which]);
				else
					showFreqModCombo(RFEfreq.RFExplorerName[which]);

				dialog.cancel();
			}

		});
		builder.show();
	}

// dialog per scelta frequenza su base
void showFreqModBase(String title)
{
	Log.i(TAG, "show showFreqModBase");
	
	String[] FreqBase = new String[1];
	FreqBase[0] = RFEfreq.StrFreqLimits(RFEfreq.idxModel, 0);
	
	AlertDialog.Builder builder = 
	new AlertDialog.Builder(global_context);
	builder.setTitle(title + " set frequency limits:");
	
	builder.setSingleChoiceItems(
		FreqBase, 
		0,
		new DialogInterface.OnClickListener() {
			
			@Override
			public void onClick(
			DialogInterface dialog, 
			int which) {
				Toast.makeText(
				global_context, 
				"Select "+RFEfreq.StrFreqLimits(RFEfreq.idxModel, 0), 
				Toast.LENGTH_SHORT
				)
				.show();
				
				RFEfreq.SetFreqLimits(RFEfreq.idxModel, 0);
				
				//-----------------------------------
				// modifica 12/04:
				// imposta i valori di frequenza per il grafico
				float min = ((float)RFEfreq.GetStart()) / 1000.0f;
				float max = ((float)RFEfreq.GetEnd()) / 1000.0f;
				graphView.setHorLabels(min, max);
				graphView.refreshGraph();
				//-----------------------------------
				
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		}
	);
	
	builder.setCancelable(false);
	AlertDialog alert = builder.create();
	alert.show();
}
	
// dialog per scelta frequenza su combo
void showFreqModCombo(String title)
{
	Log.i(TAG, "show showFreqModCombo");

	String[] FreqCombo = new String[2];
	FreqCombo[0] = RFEfreq.StrFreqLimits(RFEfreq.idxModel, 0);
	FreqCombo[1] = RFEfreq.StrFreqLimits(RFEfreq.idxModel, 1);
	
	AlertDialog.Builder builder = 
	new AlertDialog.Builder(global_context);
	builder.setTitle(title + " set frequency limits:");
	
	builder.setSingleChoiceItems(
		FreqCombo, 
		0,
		new DialogInterface.OnClickListener() {
			
			@Override
			public void onClick(
			DialogInterface dialog, 
			int which) {
				Toast.makeText(
				global_context, 
				"Select "+RFEfreq.StrFreqLimits(RFEfreq.idxModel, which), 
				Toast.LENGTH_SHORT
				)
				.show();
				
				RFEfreq.SetFreqLimits(RFEfreq.idxModel, which);
				
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		}
	);
	
	builder.setCancelable(false);
	AlertDialog alert = builder.create();
	alert.show();
}
	
//----------------------------------------------

	
	
	// input nome nuova campagna
	// aggiunto listview single choice
	// vedi: http://www.java2s.com/Code/Android/UI/SimpleListsinglechoice.htm
	private int TypeAntenna;
	
	public void showNewCampaign() {
		LinearLayout ll_Main = new LinearLayout(this);
		LinearLayout ll_Row1 = new LinearLayout(this);
		LinearLayout ll_Row2 = new LinearLayout(this);
		LinearLayout ll_Row3 = new LinearLayout(this);
		
		ll_Main.setOrientation(LinearLayout.VERTICAL);
		ll_Row1.setOrientation(LinearLayout.HORIZONTAL);
		ll_Row2.setOrientation(LinearLayout.HORIZONTAL);
		ll_Row3.setOrientation(LinearLayout.VERTICAL);
		
		//--------------------------------------------
		// per la selezione del tipo di antenna:
		// SimpleListsinglechoice
		// http://www.java2s.com/Code/Android/UI/SimpleListsinglechoice.htm
		final String[] data = { "omnidirectional antenna", 
				"directional antenna" };
		ListView listView = new ListView(this);
		// ListView.setAdapter(new ArrayAdapter<String>(this,android.R.layout.simple_list_item_single_choice, data));
		// this-The current activity context.
		// Second param is the resource Id for list layout row item
		// Third param is input array 
		ArrayAdapter arrayAdapter = new ArrayAdapter(this, android.R.layout.simple_list_item_single_choice, data);
		listView.setAdapter(arrayAdapter);
		
        listView.setItemsCanFocus(true);
        listView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
		TypeAntenna = 0;
		// seleziona l'indice del tipo di antenna
		listView.setItemChecked(TypeAntenna, true);
		
		listView.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) 
			{
				TypeAntenna = (int)id;
                String item = ((TextView)view).getText().toString();
				item = item + " (" + Integer.toString(TypeAntenna) + ")";
                Toast.makeText(getBaseContext(), item, Toast.LENGTH_LONG).show();
            }
        });
		//--------------------------------------------
		
		final EditText et_Campaign = new MyEditText(this);
		final EditText et_Info = new MyEditText(this);
		
		TextView tv_Campaign = new TextView(this);
		TextView tv_Info = new TextView(this);
		
		tv_Campaign.setText("Campaign Name: ");
		tv_Info.setText("Info: ");
		
		ll_Row1.addView(tv_Campaign);
		ll_Row1.addView(et_Campaign);
		ll_Row2.addView(tv_Info);
		ll_Row2.addView(et_Info);
		ll_Row3.addView(listView);
		ll_Main.addView(ll_Row1);
		ll_Main.addView(ll_Row2);
		ll_Main.addView(ll_Row3);
		
		AlertDialog.Builder alert = new AlertDialog.Builder(this);
		alert.setTitle("Start a new measurement campaign:");
		alert.setView(ll_Main);
		alert.setCancelable(false);
		
		// button next
		alert.setPositiveButton("Next", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				//------------------------------------------------
				// et_Campaign has the campaign name
				// Modify 27/08/2015:
				// http://stackoverflow.com/questions/4283351/how-to-replace-special-characters-in-a-string
				// in name remove all special char, but keep alphabetical and numerical characters.
				String strinput = et_Campaign.getText().toString();
				String campaign = strinput.replaceAll("[^a-zA-Z0-9]+","_");
				//------------------------------------------------
				//
				String info = et_Info.getText().toString();
				
				//---------------------------------
				String msg =
						"Campaign: " + campaign + "\n" +
						" Info: " + info + "\n";
				if(TypeAntenna == 0)
					msg = msg + "omnidirectional antenna";
				else
					msg = msg + "directional antenna";
				Toast.makeText(getBaseContext(), 
						msg, 
						Toast.LENGTH_SHORT).show();
				
				//--------------------------------
				db = new DatabaseHelper(getApplicationContext(), DIRDB_FULL_PATH, campaign);
				gps.setdbHelper(db);
				RxMsg.setdbHelper(db);
				//--------------------------------
				
				// start campaign
				campaign_id = db.createCampaign(campaign, info, TypeAntenna, 1);
				gps.setCampaignId(campaign_id);
				dialogSerialStartCommunicate("Start RFExplorer Communication");
			}
		});
		// button cancel
		alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
		
		AlertDialog dialog = alert.create();
		dialog.show();
	}


	// lista dei nomi di tutte le campagne
    String[] allCampaignNames;
	String NameOfCampaignChoosen;			// nome della campagna scelta
	
	// crea il vettore di stringhe con tutti i nomi delle campagne
	public void CreateListOfCampaignNames() {

		// http://stackoverflow.com/questions/11015833/android-getting-list-of-all-files-of-a-specific-type
		// http://stackoverflow.com/questions/12409539/android-how-get-a-drectory-list-ordered-by-name-o-by-date-descending
		//
		
		// lista i file contenuti nella directory
		File dir = new File(DIRDB_FULL_PATH);
		// lista dei db contenuti in DIRDB_FULL_PATH
		File[] dbList = dir.listFiles(new FilenameFilter(){  
			public boolean accept(File dir, String name)  
			{  
				return name.endsWith(".db");
			}  
		});
		// http://stackoverflow.com/questions/12409539/android-how-get-a-drectory-list-ordered-by-name-o-by-date-descending
		// crea la lista ordinata in base alla data di creazione decrescente
		if (dbList != null && dbList.length > 1) {
			Arrays.sort(dbList, new Comparator<File>() {
				@Override
				public int compare(File object1, File object2) {
					if (object1.lastModified() > object2.lastModified()) {
						return -1;
					} else if (object1.lastModified() < object2.lastModified()) {
						return +1;
					} else {
						return 0;
					}
					// ori
					// return (int) ((object1.lastModified() > object2.lastModified()) ? object1.lastModified(): object2.lastModified());
				}
			});
		};
		allCampaignNames = new String[dbList.length];  
		for(int cnt=0;cnt<dbList.length;cnt++)
		{
			String fname = dbList[cnt].getName();
			int pos = fname.lastIndexOf(".");
			if (pos != .1) {
				fname = fname.substring(0, pos);
			}
			allCampaignNames[cnt] = fname;  
		}  
	
	}
	
	// MODIFICA 01/08/2015
	// LISTA DB IN DIRECTORY
	public void showOpenCampaign() 
	{

		// crea la lista con tutti i nomi delle campagne salvate
		CreateListOfCampaignNames();
		
		//---------------------
		// vedi: http://stackoverflow.com/questions/3032342/arrayliststring-to-charsequence
		// final CharSequence[] items = CampaignNames.toArray(new CharSequence[CampaignNames.size()]);
		//---------------------
		
		// AlertDialog.Builder adCampNames = new AlertDialog.Builder(AppActivity.this);
		AlertDialog.Builder adCampNames = new AlertDialog.Builder(global_context);
		
		adCampNames.setTitle("List of measurement campaigns stored:");
		
		adCampNames.setItems(allCampaignNames,
			new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog,
				int which) {
					// TODO Auto-generated method stub

					NameOfCampaignChoosen = allCampaignNames[which];
					
					Toast.makeText(getApplicationContext(),
					"You clicked " + allCampaignNames[which],
					Toast.LENGTH_LONG).show();

					//--------------------------------
					db = new DatabaseHelper(getApplicationContext(), DIRDB_FULL_PATH, allCampaignNames[which]);
					gps.setdbHelper(db);
					RxMsg.setdbHelper(db);
					//--------------------------------
					
					// start campaign
					campaign_id = 1;			// ogni db contiene i dati di una sola campagna
					gps.setCampaignId(campaign_id);
					
					// ori
					// dialogSerialStartCommunicate("Start RFExplorer Communication");
					// modifica 07/08/2015
					showConfigOfCampaign();
				}
			});
		// button cancel
		adCampNames.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
			
		adCampNames.show();
	}

	// per la gestione scelta configurazione parametri freq., amp
	// lista di tutte le campagne
    List<RecConfig> allConfig;
    
	// mostra tutti i par. di configurazione di una campagna
	public void showConfigOfCampaign() 
	{
        allConfig = db.getAllConfig(false);		// get all config record order by decrescent
        
		String[] items = new String[allConfig.size()];
		
		// string of the config ranges
		for(int i=0;i<allConfig.size();i++)
		{
			items[i] = 
				"Freq: [" +
				String.valueOf(allConfig.get(i).getStartFreq() / 1000) +
				" , " +
				String.valueOf(allConfig.get(i).getEndFreq() / 1000) +
				"],  amp: [" +
				String.valueOf(allConfig.get(i).getAmpBottom()) +
				" , " +
				String.valueOf(allConfig.get(i).getAmpTop()) +
				"]";
		}
		
		//---------------------
		// vedi: http://stackoverflow.com/questions/3032342/arrayliststring-to-charsequence
		// final CharSequence[] items = CampaignNames.toArray(new CharSequence[CampaignNames.size()]);
		//---------------------
		
		// AlertDialog.Builder adCampNames = new AlertDialog.Builder(AppActivity.this);
		AlertDialog.Builder adCfgChoice = new AlertDialog.Builder(global_context);
		
		adCfgChoice.setTitle("Measurement campaign stored ranges (frequency, amp.):");
		
		adCfgChoice.setItems(items,
			new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog,
				int which) {
					// TODO Auto-generated method stub

					// Toast.makeText(getApplicationContext(),
					// "You clicked " + allConfig.get(which).getName(),
					// Toast.LENGTH_LONG).show();

					// set the parameters 
					//---------------------
					// Recognize the configuration parameters of the program RfTrack
					RfCfg.SetParam(
						allConfig.get(which).getStartFreq(),
						allConfig.get(which).getEndFreq(),
						allConfig.get(which).getAmpBottom(),
						allConfig.get(which).getAmpTop()
						);
					
					// start communication
					dialogSerialStartCommunicate("Start RFExplorer Communication");
				}
			});
		// button cancel
		adCfgChoice.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
			
		adCfgChoice.show();
	}
	
	
	
		// validating email id
		// http://javatechig.com/android/edittext-validation-in-android-example
		// http://javatechig.com/android/edittext-validation-in-android-example
	private boolean isValidEmail(String email) {
		String EMAIL_PATTERN = "^[_A-Za-z0-9-\\+]+(\\.[_A-Za-z0-9-]+)*@"
				+ "[A-Za-z0-9-]+(\\.[A-Za-z0-9]+)*(\\.[A-Za-z]{2,})$";

		Pattern pattern = Pattern.compile(EMAIL_PATTERN);
		Matcher matcher = pattern.matcher(email);
		return matcher.matches();
	}
	
	String userDestinationEmail;			// contiene la stringa della email di destinazione
	
	public void displayDialogInputEmail()
	{
		// the user chooses the email address
		final AlertDialog.Builder inputAlert = new AlertDialog.Builder(context);
		inputAlert.setCancelable(false);
		inputAlert.setTitle("Destination email address");
		inputAlert.setMessage("We need the destination email to send the db");
		final EditText userInput = new EditText(context);
		inputAlert.setView(userInput);
		inputAlert.setPositiveButton("Submit", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				userDestinationEmail = userInput.getText().toString();
				if (!isValidEmail(userDestinationEmail)) {
					userInput.setError("Invalid Email address");
				}
				else
				{
					// indirizzo valido
					inputAlert.setCancelable(true);
					dialog.dismiss();
					// invia l'email
					SendDbToEmail();
				}
			}
		});
		/***
		inputAlert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
			}
		});
		****/
		final AlertDialog alertDialog = inputAlert.create();
		
		alertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() { 
			public void onDismiss( DialogInterface dialog) { 
				//If userid get text not greater than 0
				//then reshow the alertdialog with alert.show();
				userDestinationEmail = userInput.getText().toString();
				if (!isValidEmail(userDestinationEmail)) {
					alertDialog.show();
				}
			}
		});

		alertDialog.show();
		
	}
	
	// vedi:
	// http://stackoverflow.com/questions/9466169/how-to-attach-files-with-sending-mail-in-android-application
	public void SendDbToEmail() {
		
		// imposta il filename da inviare
		String filename = DIRDB_FULL_PATH + File.separator + NameOfCampaignChoosen + ".db" ;
		File file=new File(filename);
		
		Intent i = new Intent(Intent.ACTION_SEND);
		i.setType("text/plain");
		// indirizzo di email
		i.putExtra(Intent.EXTRA_EMAIL  , new String[]{ userDestinationEmail });
		
		i.putExtra(Intent.EXTRA_SUBJECT, "RfTrack Android Program");
		if(nTypeEmail == 0)
		{
			i.putExtra(Intent.EXTRA_TEXT   , "PUBLIC");
		}
		else if(nTypeEmail == 1)
		{
			i.putExtra(Intent.EXTRA_TEXT   , "PRIVATE");
		}
		else
		{
			i.putExtra(Intent.EXTRA_TEXT   , "Enclosed the SqLite db with Campaign data");
		}
		i.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(file));
		
		try 
		{
			startActivity(Intent.createChooser(i, "Send mail..."));
		} 
		catch (android.content.ActivityNotFoundException ex) 
		{
			Toast.makeText(global_context, "There are no email clients installed.", Toast.LENGTH_SHORT).show();
		}
		finally
		{
			// mr: aggiunta:
			showMainCmdMenu();		// torna al main menu
		}
	}

	//----UPLOAD TO WEBSITE
public void uploadDb() {
		
		// imposta il filename da inviare
		String filename = DIRDB_FULL_PATH + File.separator + NameOfCampaignChoosen + ".db" ;
		File file=new File(filename);
		
		//String url = "http://192.168.8.100:8000/api/pawsc/upload"; //for home setup testing
		String url = "http://test2.pawsc.info/api/pawsc/upload";
		//Intent i = new Intent(Intent.ACTION_VIEW);
		Intent i = new Intent(Intent.ACTION_VIEW);
		i.setType("*/*");
		
		/*
		i.setType("text/plain");
		// indirizzo di email
		
		
		i.putExtra(Intent.EXTRA_EMAIL  , new String[]{ userDestinationEmail });
		
		i.putExtra(Intent.EXTRA_SUBJECT, "RfTrack Android Program");
		if(nTypeEmail == 0)
		{
			i.putExtra(Intent.EXTRA_TEXT   , "PUBLIC");
		}
		else if(nTypeEmail == 1)
		{
			i.putExtra(Intent.EXTRA_TEXT   , "PRIVATE");
		}
		else
		{
			i.putExtra(Intent.EXTRA_TEXT   , "Enclosed the SqLite db with Campaign data");
		} 
		*/
		
		
		i.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(file));
		i.setData(Uri.parse(url));
		
		try 
		{
			startActivity(Intent.createChooser(i, "upload file..."));
			//startActivityForResult(i, PICKFILE_REQUEST_CODE);
			//startActivity(i);
		} 
		catch (android.content.ActivityNotFoundException ex) 
		{
			Toast.makeText(global_context, "There are no email clients installed.", Toast.LENGTH_SHORT).show();
		}
		finally
		{
			// mr: aggiunta:
			showMainCmdMenu();		// torna al main menu
		}
	} //end uploadDb()

public void uploadDb2( ) {
	
	// imposta il filename da inviare
	String filename = DIRDB_FULL_PATH + File.separator + NameOfCampaignChoosen + ".db" ;
	File file=new File(filename);
	
	//String urlString = "http://test2.pawsc.info/api/pawsc/upload";
	//HttpURLConnection conn = null;
	//DataOutputStream dos = null;
	
	/*
	  try {
		
	
		URL url = new URL(urlString);
        HttpURLConnection connection = (HttpURLConnection)url.openConnection();
        connection.setRequestProperty("User-Agent", "");
        connection.setRequestMethod("POST");
        connection.setDoInput(true);
        connection.connect();
		*/
		
        //client.getParams().setParameter("http.socket.timeout", 90000); // 90 second
        //HttpPost post = new HttpPost(url);
/*
        MultipartEntity mpEntity = new MultipartEntity();
        mpEntity.addPart("image", new FileBody(new File(filepath), "image/jpeg"));
        post.setEntity(mpEntity);
        post.addHeader("server_id", String.valueOf(server_id));

        HttpResponse response = Connector.client.execute(post);
        if (response.getStatusLine().getStatusCode() != 200) { return "false"; }
        return convertStreamToString(response.getEntity().getContent());
    } catch (Exception e) {
        if (Constants.DEBUG) e.printStackTrace();
        return "false";
    }		

	
	*/
   
	/*USE: https://stackoverflow.com/questions/4966910/androidhow-to-upload-mp3-file-to-http-server#5176670
	 * 
	 * and changing the line: dos.writeBytes("Content-Disposition: form-data; name=\"uploadedfile\";filename=\"" + existingFileName + "\"" + lineEnd);

		To dos.writeBytes("Content-Disposition: form-data; name=\"myfile\";filename=\"" + existingFileName + "\"" + lineEnd); 
	 */
	HttpURLConnection conn = null;
    DataOutputStream dos = null;
    DataInputStream inStream = null;
    String existingFileName = Environment.getExternalStorageDirectory().getAbsolutePath() + "/mypic.png";//
    //String existingFileName = filename;
    String lineEnd = "\r\n";
    String twoHyphens = "--";
    String boundary = "*****";
    int bytesRead, bytesAvailable, bufferSize;
    byte[] buffer;
    int maxBufferSize = 1 * 1024 * 1024;
    String responseFromServer = "";
    String urlString = "http://192.168.8.100:8000/api/pawsc/upload";// "http://test2.pawsc.info/api/pawsc/upload";

    try {

        //------------------ CLIENT REQUEST
        FileInputStream fileInputStream = new FileInputStream(new File(existingFileName));
        // open a URL connection to the Servlet
        URL url = new URL(urlString);
        // Open a HTTP connection to the URL
        conn = (HttpURLConnection) url.openConnection();
        // Allow Inputs
        conn.setDoInput(true);
        // Allow Outputs
        conn.setDoOutput(true);
        // Don't use a cached copy.
        conn.setUseCaches(false);
        // Use a post method.
        conn.setRequestMethod("POST");
        conn.setRequestProperty("Connection", "Keep-Alive");
        conn.setRequestProperty("Content-Type", "multipart/form-data;boundary=" + boundary);
        dos = new DataOutputStream(conn.getOutputStream());
        dos.writeBytes(twoHyphens + boundary + lineEnd);
        //dos.writeBytes("Content-Disposition: form-data; name=\"uploadedfile\";filename=\"" + existingFileName + "\"" + lineEnd);
        dos.writeBytes("Content-Disposition: form-data; name=\"myfile\";filename=\"" + existingFileName + "\"" + lineEnd);
        dos.writeBytes(lineEnd);
        // create a buffer of maximum size
        bytesAvailable = fileInputStream.available();
        bufferSize = Math.min(bytesAvailable, maxBufferSize);
        buffer = new byte[bufferSize];
        // read file and write it into form...
        bytesRead = fileInputStream.read(buffer, 0, bufferSize);

        while (bytesRead > 0) {

            dos.write(buffer, 0, bufferSize);
            bytesAvailable = fileInputStream.available();
            bufferSize = Math.min(bytesAvailable, maxBufferSize);
            bytesRead = fileInputStream.read(buffer, 0, bufferSize);

        }

        // send multipart form data necesssary after file data...
        dos.writeBytes(lineEnd);
        dos.writeBytes(twoHyphens + boundary + twoHyphens + lineEnd);
        // close streams
        Log.e("Debug", "File is written");
        fileInputStream.close();
        dos.flush();
        dos.close();

    } catch (MalformedURLException ex) {
        Log.e("Debug", "error: " + ex.getMessage(), ex);
    } catch (IOException ioe) {
        Log.e("Debug", "error: " + ioe.getMessage(), ioe);
    }

    //------------------ read the SERVER RESPONSE
    try {

        inStream = new DataInputStream(conn.getInputStream());
        String str;

        while ((str = inStream.readLine()) != null) {

            Log.e("Debug", "Server Response " + str);

        }

        inStream.close();

    } catch (IOException ioex) {
        Log.e("Debug", "error: " + ioex.getMessage(), ioex);
    }//end uploadDb2()
	
}

	

	
	void uploadToWebsite()
	{
		showSelectCampaignForUpload();
	} //end UploadToWebsite()
	
	void showSelectCampaignForUpload()
	{
		// crea la lista con tutti i nomi delle campagne salvate
		CreateListOfCampaignNames();
		
		//---------------------
		// vedi: http://stackoverflow.com/questions/3032342/arrayliststring-to-charsequence
		// final CharSequence[] items = CampaignNames.toArray(new CharSequence[CampaignNames.size()]);
		//---------------------
		
		// AlertDialog.Builder adCampNames = new AlertDialog.Builder(AppActivity.this);
		AlertDialog.Builder adCampNames = new AlertDialog.Builder(global_context);
		
		adCampNames.setTitle("List of measurement campaigns stored:");
		
		adCampNames.setItems(allCampaignNames,
			new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog,
				int which) {
					// TODO Auto-generated method stub

					NameOfCampaignChoosen = allCampaignNames[which];
					
					Toast.makeText(getApplicationContext(),
					"You clicked " + allCampaignNames[which],
					Toast.LENGTH_LONG).show();

					//--------------------------------
					if(nTypeEmail == 2)
					{
						displayDialogInputEmail();
					}
					else
					{
						userDestinationEmail = "tvwsanalyzer@gmail.com";
						// invia l'email
						uploadDb();
						//uploadDb2();
					}
					//--------------------------------
				}
			});
		// button cancel
		adCampNames.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
			
			
		adCampNames.show();			
	} //end showSelectCampaignForUpload()
	
	// @@@@
	void ForcePublicEmail()
	{
		nTypeEmail = 0;				// email di tipo public
		// arrivi qui ed hai selezionato il tipo di email
		//--------------------------------
		showSelectCampaignForEmail();
		//--------------------------------
	}
	
	// 14/04/2016: disabilitato.
	// Al suo posto si usa ForcePublicEmail()
	//
	// dialogo per scelta tipo invio da effettuare via email
	void showSelectTypeEmail()
	{
		Log.i(TAG, "showSelectTypeEmail");
		
		AlertDialog.Builder builder = 
		new AlertDialog.Builder(global_context);
		builder.setTitle("what kind of email?" );
		
		// builder.setMessage("Click connect...");
		
		builder.setSingleChoiceItems(
			typeEmailItems, 
			nTypeEmail,
			new DialogInterface.OnClickListener() {
				
				@Override
				public void onClick(
				DialogInterface dialog, 
				int which) {
					Toast.makeText(
					global_context, 
					"Selected: "+typeEmailItems[which], 
					Toast.LENGTH_SHORT
					)
					.show();
					
					// set the type email
					nTypeEmail = which;
				}
			}
		);
		
		builder.setCancelable(false);
		builder.setPositiveButton("Next", 
			new DialogInterface.OnClickListener() 
			{
				@Override
				public void onClick(DialogInterface dialog, 
						int which) {
					Log.d(TAG,"Which value="+which);
					Log.d(TAG,"Selected value="+nTypeEmail);
					Toast.makeText(
							global_context, 
							"Selected: "+typeEmailItems[nTypeEmail], 
							Toast.LENGTH_SHORT
							)
							.show();
					// arrivi qui ed hai selezionato il tipo di email
					//--------------------------------
					showSelectCampaignForEmail();
					//--------------------------------
				}
			}
		);
		builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				showMainCmdMenu();		// torna al main menu
			}
		});
			
		AlertDialog alert = builder.create();
		alert.show();
	}



	// @@@@
	// LISTA DB IN DIRECTORY per inviarlo via email
	public void showSelectCampaignForEmail() 
	{

		// crea la lista con tutti i nomi delle campagne salvate
		CreateListOfCampaignNames();
		
		//---------------------
		// vedi: http://stackoverflow.com/questions/3032342/arrayliststring-to-charsequence
		// final CharSequence[] items = CampaignNames.toArray(new CharSequence[CampaignNames.size()]);
		//---------------------
		
		// AlertDialog.Builder adCampNames = new AlertDialog.Builder(AppActivity.this);
		AlertDialog.Builder adCampNames = new AlertDialog.Builder(global_context);
		
		adCampNames.setTitle("List of measurement campaigns stored:");
		
		adCampNames.setItems(allCampaignNames,
			new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog,
				int which) {
					// TODO Auto-generated method stub

					NameOfCampaignChoosen = allCampaignNames[which];
					
					Toast.makeText(getApplicationContext(),
					"You clicked " + allCampaignNames[which],
					Toast.LENGTH_LONG).show();

					//--------------------------------
					if(nTypeEmail == 2)
					{
						displayDialogInputEmail();
					}
					else
					{
						userDestinationEmail = "tvwsanalyzer@gmail.com";
						// invia l'email
						SendDbToEmail();
					}
					//--------------------------------
				}
			});
		// button cancel
		adCampNames.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
			
			
		adCampNames.show();
	}
	
	
	// lista di tutte le campagne
    List<RecCampaign> allCampaign;
    
	// apri una campagna gia' presente
	public void ORIshowOpenCampaign() 
	{
        allCampaign = db.getAllCampaign();
        
		// CharSequence[] items = new CharSequence[allCampaign.size()];
		String[] items = new String[allCampaign.size()];
		
		for(int i=0;i<allCampaign.size();i++)
		{
			items[i] = allCampaign.get(i).getName();
		}
		
		//---------------------
		// vedi: http://stackoverflow.com/questions/3032342/arrayliststring-to-charsequence
		// final CharSequence[] items = CampaignNames.toArray(new CharSequence[CampaignNames.size()]);
		//---------------------
		
		// AlertDialog.Builder adCampNames = new AlertDialog.Builder(AppActivity.this);
		AlertDialog.Builder adCampNames = new AlertDialog.Builder(global_context);
		
		adCampNames.setTitle("List of measurement campaigns stored:");
		
		adCampNames.setItems(items,
			new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog,
				int which) {
					// TODO Auto-generated method stub

					Toast.makeText(getApplicationContext(),
					"You clicked " + allCampaign.get(which).getName(),
					Toast.LENGTH_LONG).show();

					// start campaign
					campaign_id = allCampaign.get(which).getId();
					gps.setCampaignId(campaign_id);
					dialogSerialStartCommunicate("Start RFExplorer Communication");
				}
			});
		// button cancel
		adCampNames.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
			
			
		adCampNames.show();
	}
	
	// input valori posizione inseriti manualmente
	public void showManualPosition() {
		LinearLayout ll_Main = new LinearLayout(this);
		LinearLayout ll_Row1 = new LinearLayout(this);
		LinearLayout ll_Row2 = new LinearLayout(this);
		LinearLayout ll_Row3 = new LinearLayout(this);
		
		ll_Main.setOrientation(LinearLayout.VERTICAL);
		ll_Row1.setOrientation(LinearLayout.HORIZONTAL);
		ll_Row2.setOrientation(LinearLayout.HORIZONTAL);
		ll_Row3.setOrientation(LinearLayout.HORIZONTAL);
		
		final EditText et_Latitude = new MyEditText(this);
		final EditText et_Longitude = new MyEditText(this);
		final EditText et_Altitude = new MyEditText(this);
		
		TextView tv_Latitude = new TextView(this);
		TextView tv_Longitude = new TextView(this);
		TextView tv_Altitude = new TextView(this);
		
		tv_Latitude.setText("Latitude: ");
		tv_Longitude.setText("Longitude: ");
		tv_Altitude.setText("Height of instrument from ground: ");
		
		ll_Row1.addView(tv_Latitude);
		ll_Row1.addView(et_Latitude);
		ll_Row2.addView(tv_Longitude);
		ll_Row2.addView(et_Longitude);
		ll_Row3.addView(tv_Altitude);
		ll_Row3.addView(et_Altitude);
		ll_Main.addView(ll_Row1);
		ll_Main.addView(ll_Row2);
		ll_Main.addView(ll_Row3);
		
		AlertDialog.Builder alert = new AlertDialog.Builder(this);
		alert.setTitle("Insert position manually");
		alert.setView(ll_Main);
		alert.setCancelable(false);
		
		// button Ok
		alert.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				String strLatitude = et_Latitude.getText().toString();
				String strLongitude = et_Longitude.getText().toString();
				String strAltitude = et_Altitude.getText().toString();

				// long location_id;			// id db actual location
				double Latitude = Double.valueOf(strLatitude.trim()).doubleValue(); 	// Latitude
				double Longitude = Double.valueOf(strLongitude.trim()).doubleValue(); 	// Longitude
				double Altitude = Double.valueOf(strAltitude.trim()).doubleValue();	// altitude if available, in meters above the WGS 84 reference ellipsoid.
				// float Speed = 0.0f;			// speed if it is available, in meters/second over ground.	
				// float Accuracy = 0.0f;		// estimated accuracy of this location, in meters.
				// long locTime = 0;			// UTC time of this fix, in milliseconds since January 1, 1970.
				
				Toast.makeText(getBaseContext(), 
					"Latitude: " + strLatitude + 
					" Longitude: " + strLongitude + 
					" Height: " + strAltitude, 
					Toast.LENGTH_SHORT).show();
				
				// set data corresponding to the location with coordinates (0.0, 0.0)
				gps.SetLocation_0(Latitude, Longitude, Altitude);
				
				// force to save the location in db
				// 02/8 disabilitato gps.readLocationSvDb(true);
				/***
				// save data
				location_id = db.createLocation(
						campaign_id, 
						locTime,		// UTC time of this fix, in milliseconds since January 1, 1970.
						Latitude,		// latitude, in degrees.
						Longitude,		// longitude, in degrees.
						Altitude,		// altitude if available, in meters above the WGS 84 reference ellipsoid.
						Speed,			// speed if it is available, in meters/second over ground.	
						Accuracy);		// estimated accuracy of this location, in meters.
				***/
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
		// button cancel
		alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
		
		AlertDialog dialog = alert.create();
		dialog.show();
	}
	
	
	// usato per showGpsCheck()
    private Handler handler_ResultGpsCheck = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            // ringProgressDialog.dismiss();
            // tv.setText(pi_string);
            if(bFlagAnswerDialog)
            {
                ShowGpsOk();
            }
            else
            {
                showCheckManualPosition();
            }
        }
    };
	
	// ring progress dialog.
	// ritorna il valore in bFlagAnswerDialog
	// http://www.helloandroid.com/tutorials/using-threads-and-progressdialog
	public void showGpsCheck() {
	
		final ProgressDialog ringProgressDialog = ProgressDialog.show(global_context, "Please wait ...",	"Check GPS position ...", true);
		ringProgressDialog.setCancelable(true);
		new Thread(new Runnable() {
			@Override
			public void run() {
				double lon, lat;
				bFlagAnswerDialog = false;
				int i;
				
				try {
					// Here you should write your time consuming task...
					for(i=0;i<5;i++)
					{
						// Let the progress ring for 2 seconds...
						Thread.sleep(2000);
						lon = gps.getLongitude();
						if(Double.compare(lon, 0.0d)!=0)
						{
							bFlagAnswerDialog = true;
							break;
						}
						lat = gps.getLatitude();
						if(Double.compare(lat, 0.0d)!=0)
						{
							bFlagAnswerDialog = true;
							break;
						}
					}
				} catch (Exception e) {
				}
                finally{
                    ringProgressDialog.dismiss();
					handler_ResultGpsCheck.sendEmptyMessage(0);
                }
			}
		}).start();
	}
	
	public void showCheckManualPosition() {
		bFlagAnswerDialog = false;
		AlertDialog dialog = new AlertDialog.Builder(global_context).create();
		dialog.setTitle("Problem to get position");
		dialog.setMessage("Insert position data manually (Yes or No)");
		dialog.setCancelable(false);
		dialog.setButton(DialogInterface.BUTTON_POSITIVE, "Yes", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int buttonId) {
				bFlagAnswerDialog = true;
				dialog.dismiss();
				// mr: aggiunta:
				showManualPosition();		// inserisci i dati posizione
			}
		});
		dialog.setButton(DialogInterface.BUTTON_NEGATIVE, "No", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int buttonId) {
				bFlagAnswerDialog = false;
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
		dialog.setIcon(android.R.drawable.ic_dialog_alert);
		dialog.show();
	}
	
	public void ShowGpsOk() {
		AlertDialog alertDialog = new AlertDialog.Builder(global_context).create();
 
        // Setting Dialog Title
        alertDialog.setTitle("GPS Status");
 
        // Setting Dialog Message
        alertDialog.setMessage("GPS Ok");
 
        // Setting Icon to Dialog
        // alertDialog.setIcon(R.drawable.tick);
 
        // Setting OK Button
        alertDialog.setButton("OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                // Write your code here to execute after dialog closed
                // Toast.makeText(getApplicationContext(), "You clicked on OK", Toast.LENGTH_SHORT).show();
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
            }
        });
 
        // Showing Alert Message
        alertDialog.show();
	}

	public void showHardwareCheck() {
		AlertDialog dialog = new AlertDialog.Builder(global_context).create();
		dialog.setTitle("Android System Info");

		String InfoGps;
		String InfoNetwork;
		String InfoLocProvider;
		String InfoSd;
		
		if(gps.hasGPSandIsEnabled())
			InfoGps = "Gps enabled";
		else
			InfoGps = "Gps disabled";
		if(gps.hasNetwork())
			InfoNetwork = "network enabled";
		else
			InfoNetwork = "network disabled";
		InfoLocProvider = gps.getLocationProvider();
		if(HwInfo.hasSdCard())
			InfoSd = "Sd connected";
		else
			InfoSd = "Sd not connected";
			
		dialog.setMessage( 
				"Hardware\n\n" +
				"GPS: " + InfoGps + "\n" +
				"Network: " + InfoNetwork + "\n" +
				"Location prpvider used: " + InfoLocProvider + "\n" +
				"Secure Digital: " + InfoSd + "\n"
		);
		dialog.setCancelable(false);
		dialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Exit", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int buttonId) {
				bFlagAnswerDialog = false;
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
		dialog.setIcon(android.R.drawable.ic_dialog_alert);
		dialog.show();
	}

	public void showProgramInfo() {
		AlertDialog dialog = new AlertDialog.Builder(global_context).create();
		dialog.setTitle("Program Info");

		dialog.setMessage( 
			ProgramName + "\n" +
			"Ver. " + ProgramVersion + "\n" +
			"\n" +
			"Authors:\n" +
			"   Marco Zennaro - ICTP\n" +
			"   Ermanno Pietrosemoli - ICTP\n" +
			"   Marco Rainone\n" +
			"Programmer: Marco Rainone\n" +
			"\n" +
			"Web address of the reports analyzed:\n" +
			"http://wireless.ictp.it/tvws/\n" +
			"\n" +
			"Revised by:\n"+
			" David Johnson -UCT\n"+
			" Richard Maliwatu - UCT\n"
		);
		dialog.setCancelable(false);
		dialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Exit", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int buttonId) {
				bFlagAnswerDialog = false;
				dialog.dismiss();
				// mr: aggiunta:
				showMainCmdMenu();		// torna al main menu
			}
		});
		// dialog.setIcon(android.R.drawable.ic_dialog_alert);
		dialog.show();
	}
	
    //-----------------------------------------------------------
	// @@@ QUI INSERITO PER TEST TABELLA
	private ScrollView layMain;
	private TableGenerator mTable;

    //-----------------------------------------------------------
	// @@@ QUI INSERITO PER TEST GRAFICO
	private ScrollView layGraph;
	private GraphView graphView;
	double incAlfa = 0.0;
	float[] GrpValues = new float[112];
		
	void DataSet(double inc)
	{
		for(int i=0;i<112;i++)
		{
			GrpValues[i] = (float) (11.0 + 
				15.0 * (Math.cos(inc + Math.PI * (float)i /(float) 180.0)));
		}
	}
	// @@@ fine dati grafico
    //-----------------------------------------------------------

	int widthScreen;
	int heightScreen;
	
	// http://stackoverflow.com/questions/11252067/how-do-i-get-the-screensize-programmatically-in-android
	// ok, funziona
	private void getScreenResolution(Context context)
	{
		WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
		Display display = wm.getDefaultDisplay();
		DisplayMetrics metrics = new DisplayMetrics();
		display.getMetrics(metrics);
		widthScreen = metrics.widthPixels;
		heightScreen = metrics.heightPixels;

		// return "{" + width + "," + height + "}";
	}	
	
	// http://stackoverflow.com/questions/4743116/get-screen-width-and-height
	public static int getScreenWidth() {
		return Resources.getSystem().getDisplayMetrics().widthPixels;
	}

	public static int getScreenHeight() {
		return Resources.getSystem().getDisplayMetrics().heightPixels;
	}
	
    // The entire lifetime of an activity happens between the first call to onCreate(Bundle) through to a single final call to onDestroy().
    // An activity will do all setup of "global" state in onCreate(), and release all remaining resources in onDestroy().
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        // super e' un puntatore implicito alla classe madre. Punta sempre alla classe estesa da quella che stai usando.
        // the function onCreate(Bundle savedInstanceState). is called when the activity is first created, it should contain all the initialization and UI setup.
        super.onCreate(savedInstanceState);			// super is used inside a sub-class method definition to call a method defined in the super class

        // R is a class containing the definitions for all resources of a particular application package.
        // setContentView(R.layout.main); is method which dsiplays the layout definition in main.xml file in res/layout directory
        // by accessing a reference to it from the R class.
        //This method is required to display the user interface, if the activity does not include this function then it would display a blank screen
        // Activity class takes care of creating a window for you in which you can place your UI with setContentView(View)
        setContentView(R.layout.main);

		// @@@ mr
		global_context = this;
		
		fReturnOnPause = false;			// false se non c'e' ritorno da evento OnPause


		//----------------------------------------
		// https://developer.android.com/training/scheduling/wakelock.html
		// http://www.tech.theplayhub.com/how_do_i_prevent_an_android_device_from_going_to_sleep_programmatically/
		// keep the screen on 
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		
		//------------------------
		// disable rotation
		// setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
		// setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
		// This is the default value for rotation
		// original
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
		//------------------------
		
		// disable rotation
		setAutoOrientationEnabled(getContentResolver(), false);
		
		//----------------------------------------
		// per gestione db
		// db = new DatabaseHelper(getApplicationContext());
		db = null;
		DirDbFullPath();		// crea la dir dove memorizzare i db
		//----------------------------------------
		// gestione coda messaggi
		RxFifo = new RFEdataFifo();
		// gestione posizione
		gps = new GPSTracker(global_context, db, 1);
		// gestione protocollo messaggi RFExplorer
		RxMsg = new RxRfMsg(RxFifo, db, gps);

		
		//----------------------------------------
		// per info sui dispositivi hw abilitati
		HwInfo.setContext(global_context);
		
		//---------------------------------------------------------
        // per gestione gps
        // locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
		// 23/07: gest. gps disabilitata. Ori abil.
		// getLocationManager();
		//---------------------------------------------------------
		// mr: 23/07 mr gestione originale gps
		// HwInfo.setLocationManager(locationManager);
		
		// Get mUsbManager from Android.
    	// mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
		getUsbManager();
		// ------------------------------
		// mr modify 06/04
		// inserted to enable/disable menu options in base of acquisition of serial port.
		// If the serial port is not acquired, disable New Campaign, Open Campaign
        if (mSerialDevice == null)
        {
			// get link of the first compatible usb serial device in bus usb
			mSerialDevice = UsbSerialProber.acquire(mUsbManager);
			Log.d(TAG, "mSerialDevice=" + mSerialDevice);
		}
		// ------------------------------

		//------------------------------------
		// PER TABELLA
		mTable = new TableGenerator(getApplicationContext());
		layMain = (ScrollView)findViewById(R.id.table);
		// layMain = (ScrollView)findViewById(R.id.rfexploScroller);
		
		String[] mrRow = {
			"N.Readings:", 
			"GPS Pos. Fixed:", 
			"Latitude:", 
			"Longitude:", 
			"Altitude", 
			"% samples < -100dBm:"
			// "Freq (MHz):", 
			// "Time:"
			};
		
		// original mTable.addRow(mrRow);
		// mr 06/04
		mTable.addNRows(mrRow);
		
		layMain.addView(mTable.getTable());

		// ---------------------------------------
		// http://www.sherif.mobi/2013/01/how-to-get-widthheight-of-view.html
		// modifica funzionante!!!
		layMain.measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
		// int main_widht = layMain.getMeasuredWidth();
		// int main_height = layMain.getMeasuredHeight();
		// ---------------------------------------
		
		//------------------------------------
		// PER IL GRAFICO
		// getScreenResolution(global_context);
		widthScreen = getScreenWidth();
		heightScreen = getScreenHeight();
		
		
		layGraph = (ScrollView)findViewById(R.id.graphic);
		
		/**
		//-------------
		// http://stackoverflow.com/questions/2963152/android-how-to-resize-a-custom-view-programmatically
		float pixels = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 150, getResources().getDisplayMetrics());
		LinearLayout.LayoutParams params = (LinearLayout.LayoutParams) layGraph.getLayoutParams();
		params.height = (int)200;
		layGraph.setLayoutParams(params);
		//-------------
		**/
		
		String[] verlabels = new String[] { "-50", "-60", "-70", "-80", "-90", "-100" };
		String[] horlabels = new String[] { "300", "400", "500", "600", "700", "800", "900" };
		// originale demo: DataSet(0.0);
		graphView = new GraphView(
					this,
					(float)widthScreen,
					// original 14/04: (float)(widthScreen) * (float)0.38,			// height of drawing area
					// modify 14/04: aumentata dimensione
					(float)(widthScreen) * (float)0.49,			// height of drawing area
					// (float)(heightScreen - main_height - main_height/2 - main_height/5),			// height of drawing area
					// originale demo GrpValues, 
					// RxMsg.getGraphDbm(),		// dati del messaggio ricevuto
					null,					// vettore dati da visualizzare
					"dBm values",			// titolo
					horlabels, 				// horizontal labels
					verlabels, // null,		// vertical labels, 
					GraphView.BAR
					// GraphView.LINE
					);
		layGraph.addView(graphView);
		
		//-------------------------------------
		// 18/11/2014: init tone generator. 
		// A tone is generated for each group of measurements acquired by the instrument
        // tone = new ToneGenerator(AudioManager.STREAM_DTMF, 100);
		initSoundTone();

		showMainCmdMenu();
		
		//-------------------------------------
        // These elements are defined in main.xml
        // The basic unit of an Android application is an Activity.
        // An Activity displays the user interface of your application,
        // which may contain widgets like buttons, labels, text boxes, etc.
        // Typically, you define your UI using an XML file (for example, the main.xml file located in the res/layout folder),
        mTitleTextView = (TextView) findViewById(R.id.rfexploTitle);
        mDumpTextView = (TextView) findViewById(R.id.rfexploText);
        mScrollView = (ScrollView) findViewById(R.id.rfexploScroller);

        // components from main.xml

		//--------------------------------
        editTextMainScreen = (EditText) findViewById(R.id.editTextResult);
		// show parameters info
        String msg = (RfCfg.strInfoParam());
        editTextMainScreen.setText(msg);

		// mr: set the font used to display data
		SetTextFontSize();
		
		//-------------------------------------
		// button btnSetRFEparameters for change acquisition parameters
		//
        btnSetRFEparameters = (Button) findViewById(R.id.btnSetRFEparameters);
		
        btnSetRFEparameters.setOnClickListener(new OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                // get prompts.xml view
                LayoutInflater layoutInflater = LayoutInflater.from(context);

                View promptView = layoutInflater.inflate(R.layout.prompts, null);

                AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(context);

                // set prompts.xml to be the layout file of the alertdialog builder
                alertDialogBuilder.setView(promptView);

                final EditText input = (EditText) promptView.findViewById(R.id.userInput);

				//-------------------------------------
				// modifica 07/08/2015
				// disabilita la ricezione fino a quando non inserisci i parametri
				setDisableReceive(true);
				//-------------------------------------
				
                input.setText(RfCfg.strShowParam());

                // setup a dialog window
                alertDialogBuilder
                .setCancelable(false)
                .setPositiveButton("OK",
                                   new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int id)
                    {
                        // get user input and set it to result
                        editTextMainScreen.setText(input.getText());
						//---------------------
						// Recognize the string configuration parameters of the program RfTrack
						RfCfg.strSetParam(editTextMainScreen.getText().toString());

						//-------------------------------------
						// modifica 07/08/2015
						// disabilita la ricezione fino a quando non inserisci i parametri
						setDisableReceive(false);
						//-------------------------------------
						
						// ========================== SET COMMUNICATION PARAMETERS
						if(SetBaudSerial(baudRate))
						{
							// invia il messaggio di configurazione
							SendCfgMessage(1000);

							//-----------------------------------
							// modifica 12/04:
							// imposta i valori di frequenza per il grafico
							float min = ((float)RFEfreq.GetStart()) / 1000.0f;
							float max = ((float)RFEfreq.GetEnd()) / 1000.0f;
							graphView.setHorLabels(min, max);
							graphView.refreshGraph();
							//-----------------------------------
							
							editTextMainScreen.setText(RfCfg.strInfoParam());		// show the message

							// cancella la textview del data logging
							mDumpTextView.setText("");

							// chiusura del file CSV precedente e apertura di uno nuovo
							// ci pensa il garbage collector a rilasciare la memoria
							CloseCsvFile();
							OpenCsvFile();			// crea un nuovo file CSV
						}				
						
						/****************
                        try
                        {
                            // send the message

							// Byte array containing the configuration message
							byte[] msgSendCfg = 
								RxMsg.MsgConfigurationData(
									RfCfg.Start_Freq,		// KHZ, Value of frequency span start (lower)
									RfCfg.End_Freq,			// KHZ, Value of frequency span end (higher)
									RfCfg.Amp_Top,			// dBm, Highest value of amplitude for GUI
									RfCfg.Amp_Bottom );		// dBm, Lowest value of amplitude for GUI

							// ========================== SET COMMUNICATION PARAMETERS
                            mSerialDevice.setParameters(baudRate, 8, 1, 0);
                            // mSerialDevice.setParameters(500000, 8, 1, 0);
							
                            mSerialDevice.write(msgSendCfg, 1000);			// send the message
							
							// Convert the string message to display it
                            String msg = new String(msgSendCfg);			// conversione corretta byte[] to String
											
							// ok, original
                            // editTextMainScreen.setText("Tx Message: [" + msg + "]");		// show the message
							// aggiorna il log
                            Log.i("mr", "Msg protocollo:>" + msg);
							
							editTextMainScreen.setText(RfCfg.strInfoParam());		// show the message

							// cancella la textview del data logging
							mDumpTextView.setText("");

							// chiusura del file CSV precedente e apertura di uno nuovo
							// ci pensa il garbage collector a rilasciare la memoria
							CloseCsvFile();
							OpenCsvFile();			// crea un nuovo file CSV
							
                            //---------------------
                        }
                        catch (IOException e2)
                        {
                            // Ignore.
                        }
							*********************/
                    }
                })
                .setNegativeButton("Cancel",
                                   new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int id)
                    {
                        dialog.cancel();
                    }
                });

                // create an alert dialog
                AlertDialog alertD = alertDialogBuilder.create();
                alertD.show();
            }
        });	
		//
		// end btnSetRFEparameters acquisition parameters
		//-------------------------------------

    }				// end onCreate

	// http://stackoverflow.com/questions/9718317/control-default-auto-rotate-screen-in-my-application
	// enable/disable rotation
	public static void setAutoOrientationEnabled(ContentResolver resolver, boolean enabled)
	{
	  Settings.System.putInt(resolver, Settings.System.ACCELEROMETER_ROTATION, enabled ? 1 : 0);
	}
	
	// http://stackoverflow.com/questions/5148259/android-screen-orientation-change-causes-app-to-quit
	// The application closes on orientation change because you must provide with particular resources in the res folder
	// 
	// Eg:layout-land in res folder or	handle the change at runtime through programmatically

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);

		// Checks the orientation of the screen
		if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
			Toast.makeText(this, "landscape", Toast.LENGTH_SHORT).show();
		} else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT){
			Toast.makeText(this, "portrait", Toast.LENGTH_SHORT).show();
		}
	}
	
    // onPause() is where you deal with the user leaving your activity.
    // Most importantly, any changes made by the user should at this point be committed
    // (usually to the ContentProvider holding the data).
    @Override
    protected void onPause()
    {
        super.onPause();			// super is used inside a sub-class method definition to call a method defined in the super class

		//------------------------------------------
		// per gps
        // 23/07: mr gestione originale abilitata
        // locationManager.removeUpdates(gpsLocationListener);

		//------------------------------------------
		// per la seriale
        stopRxSerialManager();
		CloseSerial();
		
		fReturnOnPause = true;			// true se OnPause e' avvenuto
    }				// end onPause

    // The foreground lifetime of an activity happens between a call to onResume() until a corresponding call to onPause().
    // During this time the activity is in front of all other activities and interacting with the user.
    @Override
    protected void onResume()
    {
        super.onResume();			// super is used inside a sub-class method definition to call a method defined in the super class
		
		//----------------------------------------------------------------
		// location manager:
        // mr disabiltato 23/07 gestione originale
        // locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 3000, 0, gpsLocationListener);

		//----------------------------------------------------------------
		// serial init:
		//
		// get link of the first compatible usb serial device in bus usb
		mSerialDevice = UsbSerialProber.acquire(mUsbManager);
        Log.d(TAG, "Resumed, mSerialDevice=" + mSerialDevice);
		if (isSerialAcquired() == false)
        {
			// serial was not acquired
			/***
			ori
            mTitleTextView.setText("No serial device acquired.");
			return;
			***/
			
			// show that the app functions are reduced
			dialogInfoRFENotCommected("RFExplorer is not connected");
			/****
			// original app 0.58
			for(int i=1;i<=3;i++)
			{
				dialogSerialStartConnect("Tentative n." + String.valueOf(i));
				// dialogSerialStartConnect("Connection error");
				if (isSerialAcquired())
					break;
			}
			****/
        };
		if (isSerialAcquired() == false)
        {
			// serial was not acquired
            mTitleTextView.setText("No serial device acquired.");
			return;
		};
		//---------------------------------------------------------------
		// visualizza il nome della porta seriale aperta
		String name_serial = "";

		if(mSerialDevice != null)
		{
			name_serial = mSerialDevice.toString();
			int pos = name_serial.lastIndexOf(".");
			if (pos != -1) {
				// there is the last occorrence of '.'
				name_serial = name_serial.substring(pos+1, name_serial.length());
			}
		}
		// ori mTitleTextView.setText("Serial device: " + mSerialDevice);
		mTitleTextView.setText("Serial device: " + name_serial);
		
		// serial acquired
		if(OpenSerial() == false)
		{
			mTitleTextView.setText("Error opening serial port.");
			CloseSerial();
			return;
		}
		/**
		provato a spostarlo qui
		// abilita il manager di ricezione
        stopRxSerialManager();
        startRxSerialManager();
		**/
		
		if(fReturnOnPause == true)
		{
			// se ritorni da OnPause, invia il messaggio di configurazione immediatamente
			// per ripristinare la lettura dati
			//
			// serial opened
			// ========================== SET COMMUNICATION PARAMETERS
			if(SetBaudSerial(baudRate) == false)
			{
				mTitleTextView.setText("Error setting baudrate port " + mSerialDevice);
				CloseSerial();
				return;
			}				
			// invia il messaggio di configurazione
			SendCfgMessage(500);
			
			fReturnOnPause = false;
		}
		else
		{
			// cancella la textview del data logging
			mDumpTextView.setText("");
		}
		
		/*****************
		try
		{
			//----------------------------------------------------
			// apri il file per la scrittura
			// ori abilitato OpenCsvFile();
			// OpenFile();
			//----------------------------------------------------

			// apri la seriale e invia il messaggio
			mSerialDevice.open();

			// invia il messaggio
			try
			{
				//---------------------
				// mr: invio del messaggio
				// String msg = "# C2-F:0400000,0500000,-050,-120";
				// String msg = "# C2-F:" + (editTextMainScreen.getText().toString());

				// vettore di byte contenente il messaggio di configurazione
				byte[] msgSendCfg = 
					RxMsg.MsgConfigurationData(
						RfCfg.Start_Freq,		// KHZ, Value of frequency span start (lower)
						RfCfg.End_Freq,			// KHZ, Value of frequency span end (higher)
						RfCfg.Amp_Top,			// dBm, Highest value of amplitude for GUI
						RfCfg.Amp_Bottom );		// dBm, Lowest value of amplitude for GUI
				
				// ========================== SET COMMUNICATION PARAMETERS
				mSerialDevice.setParameters(baudRate, 8, 1, 0);
				// mSerialDevice.setParameters(500000, 8, 1, 0);
				
				mSerialDevice.write(msgSendCfg, 500);					// send the message
			}
			catch (IOException e2)
			{
				// Ignore.
			}
		}
		catch (IOException e)
		{
			Log.e(TAG, "Error setting up device: " + e.getMessage(), e);
			mTitleTextView.setText("Error opening device: " + e.getMessage());
			CloseSerial();
			return;
		}
		*******/
		//
		
		// POSIZIONE ORIGINALE:
        stopRxSerialManager();
        startRxSerialManager();
		
		//---------------------------------------------------------------
		// visualizza il nome della porta seriale aperta
		// mTitleTextView.setText("Serial device: " + mSerialDevice);

		//
		// serial init:
		//----------------------------------------------------------------
		
		// la seriale e' acquisita e inizializzata
		// apri il log csv

		// try
		{
			//----------------------------------------------------
			// apri il file csv per la scrittura
			OpenCsvFile();
			//----------------------------------------------------
		}
		/***
		catch (IOException e2)
		{
			// Ignore.
		}
		***/
    }			// end onResume


	// mr: aggiunta 26/07
	// http://stackoverflow.com/questions/11387258/onpause-and-onstop-in-activity
	@Override
    public void onStop() {
        super.onStop();
        // if (VERBOSE) Log.v(TAG, "-- ON STOP --");
		// chiudi l'attivita'
		// vedi:
		// http://blog.kibotu.net/java/android-programmatically-close-app-by-pressing-back-button-properly
		//-------------------------------
		// cleanup app, save preferences, etc.
		android.os.Process.killProcess(android.os.Process.myPid());
		// finish(); // not working properly, especially not with asynchronous tasks running
		//-------------------------------
	}
	
	// http://programmerguru.com/android-tutorial/how-to-change-the-back-button-behaviour/#code
	@Override
    public void onBackPressed() {
        //Display alert message when back button has been pressed
        backButtonHandler();
        return;
    }
 
    public void backButtonHandler() {
        AlertDialog.Builder alertDialog = new AlertDialog.Builder(global_context);
        // Setting Dialog Title
        alertDialog.setTitle("Leave application?");
        // Setting Dialog Message
        alertDialog.setMessage("Are you sure you want to leave RfTrack?");
        // Setting Icon to Dialog
        // alertDialog.setIcon(R.drawable.dialog_icon);
        // Setting Positive "Yes" Button
        alertDialog.setPositiveButton("YES",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        finish();
                    }
                });
        // Setting Negative "NO" Button
        alertDialog.setNegativeButton("NO",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        // Write your code here to invoke NO event
                        dialog.cancel();
                    }
                });
        // Showing Alert Message
        alertDialog.show();
    }
	
	//------------------------------------------------
	// utility
	//
	// j2xx.hyperterm
	//
	// call this API to show message
    void midToast(String str, int showTime)
    {
		Toast toast = Toast.makeText(global_context, str, showTime);			
		toast.setGravity(Gravity.CENTER_VERTICAL|Gravity.CENTER_HORIZONTAL , 0, 0);
		
		TextView v = (TextView) toast.getView().findViewById(android.R.id.message);
		v.setTextColor(Color.YELLOW);
		v.setTextSize(contentFontSize);
		toast.show();	
    }

}		// end class RfTrackActivity
