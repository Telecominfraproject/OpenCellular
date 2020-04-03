// Copyright 2014 Marco Rainone
//
// This module is multi-licensed and may be used under the terms
// of any of the following licenses:
//
//  EPL, Eclipse Public License, V1.0 or later, http://www.eclipse.org/legal
//  LGPL, GNU Lesser General Public License, V2.1 or later, http://www.gnu.org/licenses/lgpl.html
//
// Please contact the author if you need another license.
// This module is provided "as is", without warranties of any kind.

package com.ictp.mrainone.rftrack.util;

import java.io.File;

// android.os: Provides basic operating system services, message passing, and inter-process communication on the device.
import android.os.Environment;			// Environment: Provides access to environment variables.
import android.util.Log;				// for debug logging
//-----------------------------------------------------------
// sd external memory class
//
public class sdmemory
{
    File sd;					// position of sd card

	// constructor
	public sdmemory()
	{
		sd = Environment.getExternalStorageDirectory();					// the primary external storage directory.
	}

    // The external storage may be unavailable.
	// That because we can mount it as USB storage and in some cases remove it from the device.
    // Therefore we should always check its availability before using it.
    // We can simply check external storage availability using the getExternalStorageState() method.

    // Checks if SD Card is available for read and write
    public boolean isMounted()
    {
        String status = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(status))
        {
            return true;
        }
        return false;
    }

    // Checks if SD Card is available to at least read
    public boolean isReadable()
    {
        String status = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(status)
                || Environment.MEDIA_MOUNTED_READ_ONLY.equals(status))
        {
            return true;
        }
        return false;
    }
    
    // Returns the absolute path of this file. 
    // An absolute path is a path that starts at a root of the file system.
    public String getAbsolutePath()
    {
        return(sd.getAbsolutePath());
    }
	
	// create a directory in sd card
	// return null if the directory does not create
	// note:
	// dirname is without the initial '/'
	// set permission: <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
	// http://stackoverflow.com/questions/21028247/how-to-create-folder-in-sdcard-in-android
	public File mkdir(String dirname)
	{
        String state = Environment.getExternalStorageState();
		File directory = null;
        Log.d("sdmem State", state);

        if (Environment.MEDIA_MOUNTED.equals(state)) {
			try {
				directory = new File(
					Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator + dirname);
				if (!directory.exists()) {
					 directory.mkdirs();		// create directory
				}
			}
			catch(Exception e){
				Log.w("creating file error", e.toString());
			}		
			return directory;
		}
		else
		{
			return null;
		}
	}
	
	// check if exist a directory
	public boolean dirExist(File dir)
	{
		return dir.exists();
	}
	
	// http://stackoverflow.com/questions/4943629/how-to-delete-a-whole-folder-and-content
	// delete a whole folder and its content
	// the static method is used for performance.
	// the code to be run, and don't want to instantiate an extra object to do so, shove it into a static method. 
	public static boolean rmdir(File dir) {
		File[] files = dir.listFiles();
		boolean success = true;
		if (files != null) {
			for (File file : files) {
				if (file.isDirectory()) {
					success &= rmdir(file);
				}
				if (!file.delete()) {
					Log.w("Failed to delete ", "DELETE FAIL");
					success = false;
				}
			}
		}
		return success;
	}
}

// creare zip in sd:
// http://stackoverflow.com/questions/6522356/how-to-zip-the-files-in-android
// unzip programmatically:
// http://stackoverflow.com/questions/3382996/how-to-unzip-files-programmatically-in-android
