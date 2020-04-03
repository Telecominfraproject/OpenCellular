package com.ictp.mrainone.rftrack.helper;

import android.os.Environment;

import java.io.*;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

import com.ictp.mrainone.rftrack.rfexplorer.RFEdBm;
import com.ictp.mrainone.rftrack.rfexplorer.RFEdata;
import com.ictp.mrainone.rftrack.tblmodel.*;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteStatement;
import android.util.Log;

public class DatabaseHelper extends SQLiteOpenHelper {

	// Logcat tag
	private static final String LOG = "DatabaseHelper";

	// Database Version
	private static final int DATABASE_VERSION = 1;

	// path external sd to save db
	public static final String  EXTERNALSD_FILE_PATH	= Environment.getExternalStorageDirectory().getAbsolutePath();

	// directory to save the databases
	public static final String  NameDirectoryOfDb		= "RFE_db";
	public static File directoryOfDb;					// file per la gestione directory contenente i db
	
	public static String  DIRDB_FULL_PATH;				// contiene il path completo della directory dei db
	// nota:
	// per usare la sd esterna, in AndroidManifest.xml  mettere queste righe:
		// <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
		// <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
	
	// database name
	// il nome del database
	private static String DATABASE_NAME;
	private static String FULLPATH_DATABASE_NAME;

	//---------------------------------
	// Table Names
	private static final String TABLE_CAMPAIGN = "campaign";
	private static final String TABLE_CONFIG = "config";
	private static final String TABLE_MAXDBM = "maxdbm";
	private static final String TABLE_LOCATION = "location";
	private static final String TABLE_DBMDATA = "dbmdata";
	
	//---------------------------------

	// Common column names
	private static final String KEY_ID = "id";
	private static final String KEY_CREATED_AT = "created_at";
	
	// specific column names table CAMPAIGN:
	private static final String KEY_NAME = "name";			// campaign name
	private static final String KEY_INFO = "info";			// campaing info
	private static final String KEY_ANTENNA = "antenna";	// type of antenna used in campaing
	private static final String KEY_STATUS = "status";

	// specific column names table LOCATION:
	private static final String KEY_LATITUDE = "latitude";		// latitude, in degrees.
	private static final String KEY_LONGITUDE = "longitude";	// longitude, in degrees.
	private static final String KEY_ALTITUDE = "altitude";		// altitude if available, in meters above the WGS 84 reference ellipsoid.
	private static final String KEY_SPEED = "speed";			// speed if it is available, in meters/second over ground.	
	private static final String KEY_ACCURACY = "accuracy";		// estimated accuracy of this location, in meters.
	private static final String KEY_LOCTIME = "loctime";		// UTC time of this fix, in milliseconds since January 1, 1970.
	
	// specific column names table DBMDATA:
	private static final String KEY_CONFIG_ID = "config_id";
	private static final String KEY_MIN = "min";
	private static final String KEY_MAX = "max";
	private static final String KEY_MED = "med";
	private static final String KEY_NSTEPS = "nsteps";

	// specific column names table MAXDBM:
	private static final String KEY_FREQUENCY = "freq";
	
	// specific column names table CONFIG:
	private static final String KEY_CAMPAIGN_ID = "campaign_id";
	private static final String KEY_START_FREQ = "start_freq";
	private static final String KEY_END_FREQ = "end_freq";
	private static final String KEY_AMP_TOP = "amp_top";
	private static final String KEY_AMP_BOTTOM = "amp_bottom";

	// rif gps location. Table DBMDATA, MAXDBMDATA
	private static final String KEY_LOCATION_ID = "location_id";
	
	//---------------------------------------------------------
	// Table Create Statements
	private static String CREATE_TABLE_CAMPAIGN;		
	private static String CREATE_TABLE_CONFIG;		
	private static String CREATE_TABLE_LOCATION;		
	private static String CREATE_TABLE_MAXDBM;		
	private static String CREATE_TABLE_DBMDATA;
	
	//---------------------------------------------------------
	// Insert data record Statements
	private static String SQL_INSERT_CAMPAIGN;
	private static String SQL_INSERT_CONFIG;
	private static String SQL_INSERT_LOCATION;
	private static String SQL_INSERT_MAXDBM;
	private static String SQL_INSERT_DBMDATA;

	// create a precompiled statement to insert dBm data readings
	// 07/08
	// http://www.w3schools.com/sql/sql_insert.asp
	// http://tech.vg.no/2011/04/04/speeding-up-sqlite-insert-operations/
	//
	private static SQLiteStatement stmtInsertCampaign = null;	// contains precompiled statement insert Campaign data
	private static SQLiteStatement stmtInsertConfig = null;		// contains precompiled statement insert Config data
	private static SQLiteStatement stmtInsertLocation = null;	// contains precompiled statement insert Location data
	private static SQLiteStatement stmtInsertMaxdBm = null;		// contains precompiled statement insert Max dBm data
	private static SQLiteStatement stmtInsertdBm = null;		// contains precompiled statement insert dBm data
	
	//---------------------------------------------------------
	// constructor
	//---------------------------------------------------------
	public DatabaseHelper(Context context, String NameDir, String NameFile) {
		/***
		originale, per salvataggio nella sd interna
		super(context, DATABASE_NAME, null, DATABASE_VERSION);
		****/
		
		// save the db in the external sd
		super(context, 
				NameDir + File.separator + NameFile + ".db", 
				null, 
				DATABASE_VERSION);
				
		DATABASE_NAME = NameFile + ".db";
		FULLPATH_DATABASE_NAME = NameDir;
	}

	@Override
	public void onCreate(SQLiteDatabase db) {

		//-------------------------------------
		// Set locale english
		// http://stackoverflow.com/questions/13720092/why-and-where-to-call-setlocale
		db.setLocale(new Locale("en","US"));

		//-------------------------------------
		// create the database tables
		createTblCampaign(db);
		createTblConfig(db);
		createTblLocation(db);
		createTblMaxdBm(db);
		createTbldBmData(db);
		
		//-------------------------------------
		// 08/08: create precompilated sql statements
		createSqlInsertCampaign(db);
		createSqlInsertConfig(db, 128);
		createSqlInsertLocation(db);
		createSqlInsertMaxdBm(db);
		createSqlInsertdBm(db, 128);
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		// on upgrade drop older tables
		db.execSQL("DROP TABLE IF EXISTS " + TABLE_CAMPAIGN);
		db.execSQL("DROP TABLE IF EXISTS " + TABLE_CONFIG);
		db.execSQL("DROP TABLE IF EXISTS " + TABLE_LOCATION);
		db.execSQL("DROP TABLE IF EXISTS " + CREATE_TABLE_MAXDBM);
		db.execSQL("DROP TABLE IF EXISTS " + TABLE_DBMDATA);

		// create new tables
		onCreate(db);
	}

	//---------------------------------------------------------
	// table create statements
	//---------------------------------------------------------

	private void createTblCampaign(SQLiteDatabase db) {

		// start of sql command to create table
		CREATE_TABLE_CAMPAIGN = "CREATE TABLE " + TABLE_CAMPAIGN + "("
				+ KEY_ID + " INTEGER PRIMARY KEY,";
				
		CREATE_TABLE_CAMPAIGN = CREATE_TABLE_CAMPAIGN
				+ KEY_NAME + " TEXT," 
				+ KEY_INFO + " TEXT," 
				+ KEY_ANTENNA + " INTEGER," 
				+ KEY_STATUS + " INTEGER," 
				+ KEY_CREATED_AT + " DATETIME";
		
		// end of create string
		CREATE_TABLE_CAMPAIGN = CREATE_TABLE_CAMPAIGN + ")";
		
		db.execSQL(CREATE_TABLE_CAMPAIGN);
	}

	private void createTblConfig(SQLiteDatabase db) {
		String KeyName;

		// start of sql command to create table
		CREATE_TABLE_CONFIG = "CREATE TABLE " + TABLE_CONFIG + "("
				+ KEY_ID + " INTEGER PRIMARY KEY,";
				
		CREATE_TABLE_CONFIG = CREATE_TABLE_CONFIG
				+ KEY_CREATED_AT + " DATETIME,"
				+ KEY_CAMPAIGN_ID + " INTEGER," 
				+ KEY_START_FREQ + " INTEGER," 
				+ KEY_END_FREQ + " INTEGER," 
				+ KEY_AMP_TOP + " INTEGER," 
				+ KEY_AMP_BOTTOM + " INTEGER,";
				
		// n. campioni frequenza
		CREATE_TABLE_CONFIG = CREATE_TABLE_CONFIG
				+ KEY_NSTEPS + " INTEGER";

		// campioni dBm
		for(int i = 0;i<128;i++)
		{
			CREATE_TABLE_CONFIG = CREATE_TABLE_CONFIG + ',';
			// http://stackoverflow.com/questions/275711/add-leading-zeroes-to-number-in-java
			// 0 - to pad with zeros
			// 3 - to set width to 3
			KeyName = "f" + String.format("%03d", i);
			CREATE_TABLE_CONFIG = CREATE_TABLE_CONFIG
					+ KeyName + " REAL";
		}
		
		// end of create string
		CREATE_TABLE_CONFIG = CREATE_TABLE_CONFIG + ")";
		
		db.execSQL(CREATE_TABLE_CONFIG);
	}
	
	private void createTblLocation(SQLiteDatabase db) {
		// String KeyName;

		// start of sql command to create table
		CREATE_TABLE_LOCATION = "CREATE TABLE " + TABLE_LOCATION + "("
				+ KEY_ID + " INTEGER PRIMARY KEY,";
				
		CREATE_TABLE_LOCATION = CREATE_TABLE_LOCATION
				+ KEY_CREATED_AT + " DATETIME,"
				+ KEY_CAMPAIGN_ID + " INTEGER," 
				+ KEY_LOCTIME + " INTEGER,"		// UTC time of this fix, in milliseconds since January 1, 1970.
				+ KEY_LATITUDE + " REAL,"		// latitude, in degrees.
				+ KEY_LONGITUDE + " REAL," 		// longitude, in degrees.
				+ KEY_ALTITUDE + " REAL,"		// altitude if available, in meters above the WGS 84 reference ellipsoid.
				+ KEY_SPEED + " REAL,"			// speed if it is available, in meters/second over ground.	
				+ KEY_ACCURACY + " REAL";		// estimated accuracy of this location, in meters.
		
		// end of create string
		CREATE_TABLE_LOCATION = CREATE_TABLE_LOCATION + ")";
		
		db.execSQL(CREATE_TABLE_LOCATION);
	}

	private void createTblMaxdBm(SQLiteDatabase db) {
		// String KeyName;

		// start of sql command to create table
		CREATE_TABLE_MAXDBM = "CREATE TABLE " + TABLE_MAXDBM + "("
				+ KEY_ID + " INTEGER PRIMARY KEY,";
				
		CREATE_TABLE_MAXDBM = CREATE_TABLE_MAXDBM
				+ KEY_CREATED_AT + " DATETIME,"
				+ KEY_LOCATION_ID + " INTEGER,"
				+ KEY_FREQUENCY + " REAL,"
				+ KEY_MAX + " REAL";
		
		// end of create string
		CREATE_TABLE_MAXDBM = CREATE_TABLE_MAXDBM + ")";
		
		db.execSQL(CREATE_TABLE_MAXDBM);
	}
	
	private void createTbldBmData(SQLiteDatabase db) {
		String KeyName;

		// start of sql command to create table
		CREATE_TABLE_DBMDATA = "CREATE TABLE " + TABLE_DBMDATA + "("
				+ KEY_ID + " INTEGER PRIMARY KEY,";
				
		CREATE_TABLE_DBMDATA = CREATE_TABLE_DBMDATA
				+ KEY_CREATED_AT + " DATETIME,";
				
		// indici delle tabelle contenenti le location e la configurazione
		CREATE_TABLE_DBMDATA = CREATE_TABLE_DBMDATA
				+ KEY_LOCATION_ID + " INTEGER,"
				+ KEY_CONFIG_ID + " INTEGER,";

		// valori min, max, med
		CREATE_TABLE_DBMDATA = CREATE_TABLE_DBMDATA
				+ KEY_MIN + " INTEGER," 
				+ KEY_MAX + " INTEGER," 
				+ KEY_MED + " INTEGER,";
				
		// n. campioni
		CREATE_TABLE_DBMDATA = CREATE_TABLE_DBMDATA
				+ KEY_NSTEPS + " INTEGER";

		// campioni dBm
		for(int i = 0;i<128;i++)
		{
			CREATE_TABLE_DBMDATA = CREATE_TABLE_DBMDATA + ',';
			// http://stackoverflow.com/questions/275711/add-leading-zeroes-to-number-in-java
			// 0 - to pad with zeros
			// 3 - to set width to 3
			KeyName = "v" + String.format("%03d", i);
			CREATE_TABLE_DBMDATA = CREATE_TABLE_DBMDATA
					+ KeyName + " INTEGER";
		}
		
		// end of create string
		CREATE_TABLE_DBMDATA = CREATE_TABLE_DBMDATA + ")";
		
		db.execSQL(CREATE_TABLE_DBMDATA);
	}

	// end table create statements

	//---------------------------------------------------------
	// create precompiled insert data statements
	//---------------------------------------------------------
	//
	// create a precompiled statement to insert data in table
	// example:
	// "INSERT INTO table (col1,.., coln) VALUES (?,.., ?)";
	//

	public void createSqlInsertCampaign(SQLiteDatabase db) {
		
		// start of sql command: insert table
		SQL_INSERT_CAMPAIGN = "INSERT INTO " + TABLE_CAMPAIGN + "(";
		// insert the column names
		SQL_INSERT_CAMPAIGN = SQL_INSERT_CAMPAIGN
				+ KEY_CREATED_AT + ", "
				+ KEY_NAME + ", "
				+ KEY_INFO + ", "
				+ KEY_ANTENNA + ", "
				+ KEY_STATUS;
				
		// continue the sql statement:
		SQL_INSERT_CAMPAIGN = SQL_INSERT_CAMPAIGN
				+ ") VALUES (";
		// insert the question marks
		for(int i=0;i<(5-1);i++)
		{
			SQL_INSERT_CAMPAIGN = SQL_INSERT_CAMPAIGN
					+ "?, "; 
		}
		// end of sql command: insert table
		SQL_INSERT_CAMPAIGN = SQL_INSERT_CAMPAIGN
				+ "?);"; 
		// create the compiled statement
		stmtInsertCampaign = db.compileStatement(SQL_INSERT_CAMPAIGN);
	}
	
	public void createSqlInsertConfig(SQLiteDatabase db, int nSteps) {
		String KeyName;
		
		// start of sql command: insert table
		SQL_INSERT_CONFIG = "INSERT INTO " + TABLE_CONFIG + "(";
		// insert the column names
		SQL_INSERT_CONFIG = SQL_INSERT_CONFIG
				+ KEY_CREATED_AT + ", "
				+ KEY_CAMPAIGN_ID + ", "
				+ KEY_START_FREQ + ", "
				+ KEY_END_FREQ + ", "
				+ KEY_AMP_TOP + ", "
				+ KEY_AMP_BOTTOM + ", "
				+ KEY_NSTEPS;
				
		for(int i=0;i<nSteps;i++)
		{
			KeyName = "f" + String.format("%03d", i);
			SQL_INSERT_CONFIG = SQL_INSERT_CONFIG
					+ ", " + KeyName; 
		}
		// continue the sql statement:
		SQL_INSERT_CONFIG = SQL_INSERT_CONFIG
				+ ") VALUES (";
		// insert the question marks
		for(int i=0;i<(nSteps+7-1);i++)
		{
			SQL_INSERT_CONFIG = SQL_INSERT_CONFIG
					+ "?, "; 
		}
		// end of sql command: insert table
		SQL_INSERT_CONFIG = SQL_INSERT_CONFIG
				+ "?);"; 
		// create the compiled statement
		stmtInsertConfig = db.compileStatement(SQL_INSERT_CONFIG);
	}
	
	public void createSqlInsertLocation(SQLiteDatabase db) {
		
		// start of sql command: insert table
		SQL_INSERT_LOCATION = "INSERT INTO " + TABLE_LOCATION + "(";
		// insert the column names
		SQL_INSERT_LOCATION = SQL_INSERT_LOCATION
				+ KEY_CREATED_AT + ", "
				+ KEY_CAMPAIGN_ID + ", " 
				+ KEY_LOCTIME + ", "		// UTC time of this fix, in milliseconds since January 1, 1970.
				+ KEY_LATITUDE + ", "		// latitude, in degrees.
				+ KEY_LONGITUDE + ", " 		// longitude, in degrees.
				+ KEY_ALTITUDE + ", "		// altitude if available, in meters above the WGS 84 reference ellipsoid.
				+ KEY_SPEED + ", "			// speed if it is available, in meters/second over ground.	
				+ KEY_ACCURACY;				// estimated accuracy of this location, in meters.
				
		// continue the sql statement:
		SQL_INSERT_LOCATION = SQL_INSERT_LOCATION
				+ ") VALUES (";
		// insert the question marks
		for(int i=0;i<(8-1);i++)
		{
			SQL_INSERT_LOCATION = SQL_INSERT_LOCATION
					+ "?, "; 
		}
		// end of sql command: insert table
		SQL_INSERT_LOCATION = SQL_INSERT_LOCATION
				+ "?);"; 
		// create the compiled statement
		stmtInsertLocation = db.compileStatement(SQL_INSERT_LOCATION);
	}

	public void createSqlInsertMaxdBm(SQLiteDatabase db) {
		
		// start of sql command: insert table
		SQL_INSERT_MAXDBM = "INSERT INTO " + TABLE_MAXDBM + "(";
		// insert the column names
		SQL_INSERT_MAXDBM = SQL_INSERT_MAXDBM
				+ KEY_CREATED_AT + ", "
				+ KEY_LOCATION_ID + ", "
				+ KEY_FREQUENCY + ", "
				+ KEY_MAX;
				
		// continue the sql statement:
		SQL_INSERT_MAXDBM = SQL_INSERT_MAXDBM
				+ ") VALUES (";
		// insert the question marks
		for(int i=0;i<(4-1);i++)
		{
			SQL_INSERT_MAXDBM = SQL_INSERT_MAXDBM
					+ "?, "; 
		}
		// end of sql command: insert table
		SQL_INSERT_MAXDBM = SQL_INSERT_MAXDBM
				+ "?);"; 
		// create the compiled statement
		stmtInsertMaxdBm = db.compileStatement(SQL_INSERT_MAXDBM);
	}

	public void createSqlInsertdBm(SQLiteDatabase db, int nSteps) {
		String KeyName;
		
		// start of sql command: insert table
		SQL_INSERT_DBMDATA = "INSERT INTO " + TABLE_DBMDATA + "(";
		// insert the column names
		SQL_INSERT_DBMDATA = SQL_INSERT_DBMDATA
				+ KEY_CREATED_AT + ", "
				+ KEY_LOCATION_ID + ", "
				+ KEY_CONFIG_ID + ", "
				+ KEY_MIN + ", "
				+ KEY_MAX + ", "
				+ KEY_MED + ", "
				+ KEY_NSTEPS;
				
		for(int i=0;i<nSteps;i++)
		{
			KeyName = "v" + String.format("%03d", i);
			SQL_INSERT_DBMDATA = SQL_INSERT_DBMDATA
					+ ", " + KeyName; 
		}
		// continue the sql statement:
		SQL_INSERT_DBMDATA = SQL_INSERT_DBMDATA
				+ ") VALUES (";
		// insert the question marks
		for(int i=0;i<(nSteps+7-1);i++)
		{
			SQL_INSERT_DBMDATA = SQL_INSERT_DBMDATA
					+ "?, "; 
		}
		// end of sql command: insert table
		SQL_INSERT_DBMDATA = SQL_INSERT_DBMDATA
				+ "?);"; 
		// create the compiled statement
		stmtInsertdBm = db.compileStatement(SQL_INSERT_DBMDATA);
	}

	// end create precompiled insert data statements
	
	//---------------------------------------------------------
	// insert data statements
	//---------------------------------------------------------
	//
	// To insert, is used a precompiled statement to insert data readings
	// 07/08
	// http://www.w3schools.com/sql/sql_insert.asp
	// http://tech.vg.no/2011/04/04/speeding-up-sqlite-insert-operations/
	//

	// Creating campaign
	public long createCampaign(
		String Name,
		String Info,
		int antenna,
		int status) {
		SQLiteDatabase db = this.getWritableDatabase();

		// http://developer.android.com/reference/android/database/sqlite/SQLiteProgram.html
		if(stmtInsertCampaign == null)
			createSqlInsertCampaign(db);
		
		stmtInsertCampaign.clearBindings();
		
		stmtInsertCampaign.bindString(  1, getDateTime());
		stmtInsertCampaign.bindString(  2, Name);
		stmtInsertCampaign.bindString(  3, Info);
		stmtInsertCampaign.bindLong  (  4, antenna);
		stmtInsertCampaign.bindLong  (  5, status);

		long row_id = stmtInsertCampaign.executeInsert();
		stmtInsertCampaign.clearBindings();

		return row_id;
	}
	
	// Creating a config
	public long createConfig(
			long CampaignId,
			int Start_Freq,
			int End_Freq,
			int Amp_Top,
			int Amp_Bottom,
			int nSteps) {
				
		// String KeyName;

		SQLiteDatabase db = this.getWritableDatabase();

		// http://developer.android.com/reference/android/database/sqlite/SQLiteProgram.html
		if(stmtInsertConfig == null)
			createSqlInsertConfig(db, 128);
		
		stmtInsertConfig.clearBindings();
		
		stmtInsertConfig.bindString(  1, getDateTime());
		stmtInsertConfig.bindLong  (  2, CampaignId);
		stmtInsertConfig.bindLong  (  3, Start_Freq);
		stmtInsertConfig.bindLong  (  4, End_Freq);
		stmtInsertConfig.bindLong  (  5, Amp_Top);
		stmtInsertConfig.bindLong  (  6, Amp_Bottom);
		stmtInsertConfig.bindLong  (  7, nSteps);
		for(int i=0;i<nSteps;i++)
		{
			float ris = (float)Start_Freq;
			if(i>0)
			{
				ris = ris + i * ((float)(End_Freq - Start_Freq)/(float)nSteps);
			}
			ris = (ris / 1000.0f);            // freq. in MHz
			stmtInsertConfig.bindDouble(8 + i,  ris);
		}
		for(int i=nSteps;i<128;i++)
		{
			stmtInsertConfig.bindNull(8 + i);
		}
		long row_id = stmtInsertConfig.executeInsert();
		stmtInsertConfig.clearBindings();

		return row_id;
	}

	// Creating a location
	public long createLocation(
			long CampaignId,
			long locTime,		// UTC time of this fix, in milliseconds since January 1, 1970.
			double Latitude,	// latitude, in degrees.
			double Longitude,	// longitude, in degrees.
			double Altitude,	// altitude if available, in meters above the WGS 84 reference ellipsoid.
			float Speed,		// speed if it is available, in meters/second over ground.	
			float Accuracy)		// estimated accuracy of this location, in meters.
	{
		SQLiteDatabase db = this.getWritableDatabase();

		// http://developer.android.com/reference/android/database/sqlite/SQLiteProgram.html
		if(stmtInsertLocation == null)
			createSqlInsertLocation(db);
		
		stmtInsertLocation.clearBindings();
		
		stmtInsertLocation.bindString(  1, getDateTime());
		stmtInsertLocation.bindLong  (  2, CampaignId);
		stmtInsertLocation.bindLong  (  3, locTime);
		stmtInsertLocation.bindDouble(  4, Latitude);
		stmtInsertLocation.bindDouble(  5, Longitude);
		stmtInsertLocation.bindDouble(  6, Altitude);
		stmtInsertLocation.bindDouble(  7, Speed);
		stmtInsertLocation.bindDouble(  8, Accuracy);

		long row_id = stmtInsertLocation.executeInsert();
		stmtInsertLocation.clearBindings();

		return row_id;
	}
	
	// Creating a max dBm row
	public long createMaxdBm(
			long LocationId,
			float Frequency,
			float MaxDbm) {
				
		// String KeyName;

		SQLiteDatabase db = this.getWritableDatabase();

		// http://developer.android.com/reference/android/database/sqlite/SQLiteProgram.html
		if(stmtInsertMaxdBm == null)
			createSqlInsertMaxdBm(db);
		
		stmtInsertMaxdBm.clearBindings();
		
		stmtInsertMaxdBm.bindString(  1, getDateTime());
		stmtInsertMaxdBm.bindLong  (  2, LocationId);
		stmtInsertMaxdBm.bindDouble(  3, Frequency);
		stmtInsertMaxdBm.bindDouble(  4, MaxDbm);

		long row_id = stmtInsertMaxdBm.executeInsert();
		stmtInsertMaxdBm.clearBindings();

		return row_id;
	}
	
	// Creating a db row with dBm data
	// Used precompilated statement
	public long createdBmData(
			long LocationId,
			long ConfigId,
			RFEdata msg)
	{		
			// long[] tag_ids)
				
		SQLiteDatabase db = this.getWritableDatabase();

		// http://developer.android.com/reference/android/database/sqlite/SQLiteProgram.html
		if(stmtInsertdBm == null)
			createSqlInsertdBm(db, 128);
		
		stmtInsertdBm.clearBindings();
		
		stmtInsertdBm.bindString(  1, getDateTime());
		stmtInsertdBm.bindLong  (  2, LocationId);
		stmtInsertdBm.bindLong  (  3, ConfigId);
		stmtInsertdBm.bindLong  (  4, RFEdBm.toInt(msg.Min()));
		stmtInsertdBm.bindLong  (  5, RFEdBm.toInt(msg.Max()));
		stmtInsertdBm.bindLong  (  6, RFEdBm.toInt(msg.Media()));
		stmtInsertdBm.bindLong  (  7, msg.getMsgLen());
		for(int i=0;i<msg.getMsgLen();i++)
		{
			stmtInsertdBm.bindLong  (8 + i,  msg.getdBmInt(i));
		}
		for(int i=msg.getMsgLen();i<128;i++)
		{
			stmtInsertdBm.bindNull(8 + i);
		}
		long row_id = stmtInsertdBm.executeInsert();
		stmtInsertdBm.clearBindings();

		return row_id;
	}
	
	// end insert statements

	//---------------------------------------------------------

	//
	// getting Campaign count
	//
	public int getCampaignCount() {
		String countQuery = "SELECT  * FROM " + TABLE_CAMPAIGN;
		SQLiteDatabase db = this.getReadableDatabase();
		Cursor cursor = db.rawQuery(countQuery, null);

		int count = cursor.getCount();
		cursor.close();

		// return count
		return count;
	}

	// 
	// getting all Campaign records
	// 
	public List<RecCampaign> getAllCampaign() {
		List<RecCampaign> lstRec = new ArrayList<RecCampaign>();
		String selectQuery = "SELECT  * FROM " + TABLE_CAMPAIGN;

		Log.e(LOG, selectQuery);

		SQLiteDatabase db = this.getReadableDatabase();
		Cursor c = db.rawQuery(selectQuery, null);

		// looping through all rows and adding to list
		if (c.moveToFirst()) {
			do {
				RecCampaign rc = new RecCampaign();
				rc.setId(c.getInt((c.getColumnIndex(KEY_ID))));
				rc.setName((c.getString(c.getColumnIndex(KEY_NAME))));
				rc.setNote((c.getString(c.getColumnIndex(KEY_INFO))));
				rc.setId(c.getInt((c.getColumnIndex(KEY_ANTENNA))));
				rc.setId(c.getInt((c.getColumnIndex(KEY_STATUS))));
				rc.setCreatedAt(c.getString(c.getColumnIndex(KEY_CREATED_AT)));
				// adding to RecCampaign list
				lstRec.add(rc);
			} while (c.moveToNext());
		}

		return lstRec;
	}
	
	// 
	// getting all Campaign names
	// 
	public List<String> getAllCampaignName() {
		List<String> CampaignNames = new ArrayList<String>();
		String selectQuery = "SELECT  * FROM " + TABLE_CAMPAIGN;

		Log.e(LOG, selectQuery);

		SQLiteDatabase db = this.getReadableDatabase();
		Cursor c = db.rawQuery(selectQuery, null);

		// looping through all rows and adding to list
		if (c.moveToFirst()) {
			do {
				String name;
				name = c.getString(c.getColumnIndex(KEY_NAME));
				// adding to todo list
				CampaignNames.add(name);
			} while (c.moveToNext());
		}

		return CampaignNames;
	}
	
	//---------------------------------------------------------
	
	// 
	// getting all Config records
	// 
	public List<RecConfig> getAllConfig(boolean fOrdCrescent) {
		List<RecConfig> lstRec = new ArrayList<RecConfig>();
		String selectQuery = "SELECT  * FROM " + TABLE_CONFIG;

		if(fOrdCrescent)
			selectQuery = selectQuery + " ORDER BY id ASC";
		else
			selectQuery = selectQuery + " ORDER BY id DESC";
			
		Log.e(LOG, selectQuery);

		SQLiteDatabase db = this.getReadableDatabase();
		Cursor c = db.rawQuery(selectQuery, null);

		// looping through all rows and adding to list
		if (c.moveToFirst()) {
			do {
				RecConfig rc = new RecConfig();
				rc.setId(c.getInt((c.getColumnIndex(KEY_ID))));
				rc.setStartFreq((c.getInt(c.getColumnIndex(KEY_START_FREQ))));
				rc.setEndFreq((c.getInt(c.getColumnIndex(KEY_END_FREQ))));
				rc.setAmpTop(c.getInt((c.getColumnIndex(KEY_AMP_TOP))));
				rc.setAmpBottom(c.getInt((c.getColumnIndex(KEY_AMP_BOTTOM))));
				rc.setNSteps(c.getInt((c.getColumnIndex(KEY_NSTEPS))));
				rc.setCreatedAt(c.getString(c.getColumnIndex(KEY_CREATED_AT)));
				// adding to RecCampaign list
				lstRec.add(rc);
			} while (c.moveToNext());
		}

		return lstRec;
	}
	
	// closing database
	public void closeDB() {
		SQLiteDatabase db = this.getReadableDatabase();
		if (db != null && db.isOpen())
		{
			db.close();
		}
		// 24/08: empty the SQLiteStatement
		stmtInsertCampaign = null;		// contains precompiled statement insert Campaign data
		stmtInsertConfig = null;		// contains precompiled statement insert Config data
		stmtInsertLocation = null;		// contains precompiled statement insert Location data
		stmtInsertMaxdBm = null;		// contains precompiled statement insert Max dBm data
		stmtInsertdBm = null;			// contains precompiled statement insert dBm data
	}
	
	// 
	// get datetime
	// 
	private String getDateTime() {
		// http://docs.oracle.com/javase/7/docs/api/java/text/SimpleDateFormat.html
		// es:
		// yyyy-MM-dd'T'HH:mm:ss.SSSZ"	2001-07-04T12:08:56.235-0700
		// SSS: msec
		// get date/time and msec
		SimpleDateFormat dateFormat = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.SSS", Locale.getDefault());
                // ori: "yyyy-MM-dd HH:mm:ss.SSSZ", Locale.getDefault());
		Date date = new Date();
		return dateFormat.format(date);
	}
}

//*******************************************************
// CODE DISABLED
//*******************************************************


/****
//*******************************************************
// OLD VERSIONS INSERT
// CODE OK

	// Creating campaign
	public long OriginalcreateCampaign(
		String Name,
		String Info,
		int antenna,
		int status) {
		SQLiteDatabase db = this.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_NAME, Name);
		values.put(KEY_INFO, Info);
		values.put(KEY_ANTENNA, antenna);
		values.put(KEY_STATUS, status);
		values.put(KEY_CREATED_AT, getDateTime());

		// insert row
		long row_id = db.insert(TABLE_CAMPAIGN, null, values);
		return row_id;
	}

	public long Original_createConfig(
			long CampaignId,
			int Start_Freq,
			int End_Freq,
			int Amp_Top,
			int Amp_Bottom,
			int nSteps) {
				
		String KeyName;

		SQLiteDatabase db = this.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_CREATED_AT, getDateTime());
		values.put(KEY_CAMPAIGN_ID, CampaignId);

		//---------------------------------------------------------
		values.put(KEY_START_FREQ, Start_Freq);
		values.put(KEY_END_FREQ, End_Freq);
		values.put(KEY_AMP_TOP, Amp_Top);
		values.put(KEY_AMP_BOTTOM, Amp_Bottom);
		values.put(KEY_NSTEPS, nSteps);
		for(int i=0;i<nSteps;i++)
		{
			float ris = (float)Start_Freq;
			if(i>0)
			{
				ris = ris + i * ((float)(End_Freq - Start_Freq)/(float)nSteps);
			}
			ris = (ris / 1000.0f);            // freq. in MHz
			// es: String.format("%4.3f" , x) ;
			KeyName = "f" + String.format("%03d", i);
			values.put(KeyName, ris);
		}
		//---------------------------------------------------------
		
		// insert row
		long row_id = db.insert(TABLE_CONFIG, null, values);
		return row_id;
	}
	
	public long OriginalcreateLocation(
			long CampaignId,
			long locTime,		// UTC time of this fix, in milliseconds since January 1, 1970.
			double Latitude,	// latitude, in degrees.
			double Longitude,	// longitude, in degrees.
			double Altitude,	// altitude if available, in meters above the WGS 84 reference ellipsoid.
			float Speed,		// speed if it is available, in meters/second over ground.	
			float Accuracy)		// estimated accuracy of this location, in meters.
	{
		SQLiteDatabase db = this.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_CREATED_AT, getDateTime());
		values.put(KEY_CAMPAIGN_ID, CampaignId);

		//---------------------------------------------------------
		values.put(KEY_LOCTIME, locTime);		// UTC time of this fix, in milliseconds since January 1, 1970.
		values.put(KEY_LATITUDE, Latitude);		// latitude, in degrees.
		values.put(KEY_LONGITUDE, Longitude); 	// longitude, in degrees.
		values.put(KEY_ALTITUDE, Altitude);		// altitude if available, in meters above the WGS 84 reference ellipsoid.
		values.put(KEY_SPEED, Speed);			// speed if it is available, in meters/second over ground.	
		values.put(KEY_ACCURACY, Accuracy);		// estimated accuracy of this location, in meters.
		//---------------------------------------------------------
		
		// insert row
		long row_id = db.insert(TABLE_LOCATION, null, values);
		return row_id;
	}

	public long ORIGINALcreateMaxdBm(
			long LocationId,
			float Frequency,
			float MaxDbm) {
				
		// String KeyName;

		SQLiteDatabase db = this.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_CREATED_AT, getDateTime());
		values.put(KEY_LOCATION_ID, LocationId);

		//---------------------------------------------------------
		values.put(KEY_FREQUENCY, Frequency);
		values.put(KEY_MAX, MaxDbm);
		//---------------------------------------------------------
		
		// insert row
		long row_id = db.insert(TABLE_MAXDBM, null, values);
		return row_id;
	}

	public long ORIGINALcreatedBmData(
			long LocationId,
			long ConfigId,
			int min,
			int max,
			int med,
			int nSteps,
			int[] dBm)
	{		
			// long[] tag_ids)
				
		String KeyName;

		SQLiteDatabase db = this.getWritableDatabase();

		// http://stackoverflow.com/questions/3501516/android-sqlite-database-slow-insertion
		
		ContentValues values = new ContentValues();
		// values.put(KEY_DBMDATA, todo.getNote());
		// values.put(KEY_STATUS, todo.getStatus());
		values.put(KEY_CREATED_AT, getDateTime());

		//---------------------------------------------------------
		values.put(KEY_LOCATION_ID, LocationId);
		values.put(KEY_CONFIG_ID, ConfigId);

		//---------------------------------------------------------
		// mr: per inserzione valori dBm
		values.put(KEY_MIN, min);
		values.put(KEY_MAX, max);
		values.put(KEY_MED, med);
		values.put(KEY_NSTEPS, nSteps);
		for(int i=0;i<nSteps;i++)
		{
			KeyName = "v" + String.format("%03d", i);
			values.put(KeyName, dBm[i]);
		}
		//---------------------------------------------------------
		// insert row
		long row_id = db.insert(TABLE_DBMDATA, null, values);
		return row_id;
	}
	
//*******************************************************
****/

	/************************
	// MR DISABILITATO
	
	// NOTES Table - column names
	// private static final String KEY_DBMDATA = "dbmdata";

	// TAGS Table - column names
	// private static final String KEY_TAG_NAME = "tag_name";

	// NOTE_TAGS Table - column names
	// private static final String KEY_DBMDATA_ID = "dbmdata_id";
	// private static final String KEY_TAG_ID = "tag_id";
	
	// 
	// get single todo
	// 
	public RecCampaign getdBmData(long dbmdata_id) {
		SQLiteDatabase db = this.getReadableDatabase();

		String selectQuery = "SELECT  * FROM " + TABLE_DBMDATA + " WHERE "
				+ KEY_ID + " = " + dbmdata_id;

		Log.e(LOG, selectQuery);

		Cursor c = db.rawQuery(selectQuery, null);

		if (c != null)
			c.moveToFirst();

		RecCampaign dbmdata = new RecCampaign();
		dbmdata.setId(c.getInt(c.getColumnIndex(KEY_ID)));
		dbmdata.setNote((c.getString(c.getColumnIndex(KEY_DBMDATA))));
		dbmdata.setCreatedAt(c.getString(c.getColumnIndex(KEY_CREATED_AT)));

		return dbmdata;
	}

	// 
	// getting all todos
	// 
	public List<RecCampaign> getAlldBmData() {
		List<RecCampaign> todos = new ArrayList<RecCampaign>();
		String selectQuery = "SELECT  * FROM " + TABLE_DBMDATA;

		Log.e(LOG, selectQuery);

		SQLiteDatabase db = this.getReadableDatabase();
		Cursor c = db.rawQuery(selectQuery, null);

		// looping through all rows and adding to list
		if (c.moveToFirst()) {
			do {
				RecCampaign td = new RecCampaign();
				td.setId(c.getInt((c.getColumnIndex(KEY_ID))));
				td.setNote((c.getString(c.getColumnIndex(KEY_DBMDATA))));
				td.setCreatedAt(c.getString(c.getColumnIndex(KEY_CREATED_AT)));

				// adding to todo list
				todos.add(td);
			} while (c.moveToNext());
		}

		return todos;
	}

	// 
	// getting all todos under single tag
	// 
	public List<RecCampaign> getAlldBmDataByTag(String tag_name) {
		List<RecCampaign> todos = new ArrayList<RecCampaign>();

		String selectQuery = "SELECT  * FROM " + TABLE_DBMDATA + " td, "
				+ TABLE_TAG + " tg, " + TABLE_DBMDATA_TAG + " tt WHERE tg."
				+ KEY_TAG_NAME + " = '" + tag_name + "'" + " AND tg." + KEY_ID
				+ " = " + "tt." + KEY_TAG_ID + " AND td." + KEY_ID + " = "
				+ "tt." + KEY_DBMDATA_ID;

		Log.e(LOG, selectQuery);

		SQLiteDatabase db = this.getReadableDatabase();
		Cursor c = db.rawQuery(selectQuery, null);

		// looping through all rows and adding to list
		if (c.moveToFirst()) {
			do {
				RecCampaign td = new RecCampaign();
				td.setId(c.getInt((c.getColumnIndex(KEY_ID))));
				td.setNote((c.getString(c.getColumnIndex(KEY_DBMDATA))));
				td.setCreatedAt(c.getString(c.getColumnIndex(KEY_CREATED_AT)));

				// adding to todo list
				todos.add(td);
			} while (c.moveToNext());
		}

		return todos;
	}

	//
	// getting todo count
	//
	public int getdBmDataCount() {
		String countQuery = "SELECT  * FROM " + TABLE_DBMDATA;
		SQLiteDatabase db = this.getReadableDatabase();
		Cursor cursor = db.rawQuery(countQuery, null);

		int count = cursor.getCount();
		cursor.close();

		// return count
		return count;
	}

	//
	// Updating a todo
	//
	public int updatedBmData(RecCampaign dbmdata) {
		SQLiteDatabase db = this.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_DBMDATA, dbmdata.getNote());
		values.put(KEY_STATUS, dbmdata.getStatus());

		// updating row
		return db.update(TABLE_DBMDATA, values, KEY_ID + " = ?",
				new String[] { String.valueOf(dbmdata.getId()) });
	}

	//
	// Deleting a todo
	//
	public void deletedBmData(long dbmdata_id) {
		SQLiteDatabase db = this.getWritableDatabase();
		db.delete(TABLE_DBMDATA, KEY_ID + " = ?",
				new String[] { String.valueOf(dbmdata_id) });
	}

	// FINE DISABILITATO
	************************/

	/**************************
	// MR DISABILITATO
	//
	// Creating tag
	//
	public long createTag(Tag tag) {
		SQLiteDatabase db = this.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_TAG_NAME, tag.getTagName());
		values.put(KEY_CREATED_AT, getDateTime());

		// insert row
		long tag_id = db.insert(TABLE_TAG, null, values);

		return tag_id;
	}

	
	// 
	// getting all tags
	// 
	public List<Tag> getAllTags() {
		List<Tag> tags = new ArrayList<Tag>();
		String selectQuery = "SELECT  * FROM " + TABLE_TAG;

		Log.e(LOG, selectQuery);

		SQLiteDatabase db = this.getReadableDatabase();
		Cursor c = db.rawQuery(selectQuery, null);

		// looping through all rows and adding to list
		if (c.moveToFirst()) {
			do {
				Tag t = new Tag();
				t.setId(c.getInt((c.getColumnIndex(KEY_ID))));
				t.setTagName(c.getString(c.getColumnIndex(KEY_TAG_NAME)));

				// adding to tags list
				tags.add(t);
			} while (c.moveToNext());
		}
		return tags;
	}

	// 
	// Updating a tag
	// 
	public int updateTag(Tag tag) {
		SQLiteDatabase db = this.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_TAG_NAME, tag.getTagName());

		// updating row
		return db.update(TABLE_TAG, values, KEY_ID + " = ?",
				new String[] { String.valueOf(tag.getId()) });
	}

	// 
	// Deleting a tag
	// 
	public void deleteTag(Tag tag, boolean should_delete_all_tag_todos) {
		SQLiteDatabase db = this.getWritableDatabase();

		// before deleting tag
		// check if todos under this tag should also be deleted
		if (should_delete_all_tag_todos) {
			// get all todos under this tag
			List<RecCampaign> allTagToDos = getAlldBmDataByTag(tag.getTagName());

			// delete all todos
			for (RecCampaign todo : allTagToDos) {
				// delete todo
				deletedBmData(todo.getId());
			}
		}

		// now delete the tag
		db.delete(TABLE_TAG, KEY_ID + " = ?",
				new String[] { String.valueOf(tag.getId()) });
	}

	// ------------------------ "todo_tags" table methods ----------------//

	// 
	// Creating todo_tag
	// 
	public long createTodoTag(long todo_id, long tag_id) {
		SQLiteDatabase db = this.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_DBMDATA_ID, todo_id);
		values.put(KEY_TAG_ID, tag_id);
		values.put(KEY_CREATED_AT, getDateTime());

		long id = db.insert(TABLE_DBMDATA_TAG, null, values);

		return id;
	}

	// 
	// Updating a todo tag
	// 
	public int updateNoteTag(long id, long tag_id) {
		SQLiteDatabase db = this.getWritableDatabase();

		ContentValues values = new ContentValues();
		values.put(KEY_TAG_ID, tag_id);

		// updating row
		return db.update(TABLE_DBMDATA, values, KEY_ID + " = ?",
				new String[] { String.valueOf(id) });
	}

	// 
	// Deleting a todo tag
	// 
	public void deleteToDoTag(long id) {
		SQLiteDatabase db = this.getWritableDatabase();
		db.delete(TABLE_DBMDATA, KEY_ID + " = ?",
				new String[] { String.valueOf(id) });
	}


	// MR FINE DISABILITATO
	****************************/

//
// end DatabaseHelper.java
//