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

// java.io: Provides for system input and output through data streams, serialization and the file system.
import java.io.*;

import com.ictp.mrainone.rftrack.util.sdmemory;

//-----------------------------------------------------------
// sd external memory class
//
public class TxtFile
{
	boolean fOpen;				// True when the data file is open to save the measures

    // SD Card Storage
    sdmemory sdCard;			// sd card external memory
    File directory;				// directory containing the file
	String FileName;			// file name
	String FileExt;				// extension of the file containing the data

	String DataDirectory;       // Directory that contains the files of acquisitions
	
	// file that stores the data
    File file;					// File: An "abstract" representation of a file system entity identified by a pathname.
    FileOutputStream fos;		// output stream that writes bytes to the data file
    OutputStreamWriter osw;		// bridge from character streams to byte streams: Characters written to it are encoded into bytes using a specified charset.

	// constructor
	public TxtFile(sdmemory sd_card, String data_dir, String name, String ext)
	{
		sdCard = sd_card;	// sd card external memory
		FileName = name;
		FileExt = ext;
        fOpen = false;		// file Not Available
	}
	
	public void OpenFile()
    {
        if (sdCard.isMounted())
        {
            try
            {
                // SD Card Storage
                // sdCard = Environment.getExternalStorageDirectory();
                directory = new File(sdCard.getAbsolutePath() + DataDirectory);
                directory.mkdirs();
                file = new File(directory, FileName + "." + FileExt);
                fos = new FileOutputStream(file);
                osw = new OutputStreamWriter(fos);
                
                fOpen = true;			// success: file opened

            }
            catch (IOException e)
            {
                e.printStackTrace();
            }
        }
        else
        {
            fOpen = false;				// SD Card Not Available
        }
    }

	// write the string to the file
    public void WriteData(String dataToSave)
    {
        if(fOpen)
        {
            try
            {
                osw.write(dataToSave);
                osw.flush();
            }
            catch(IOException ie)
            {
                ie.printStackTrace();
            }
        }
    }

    public void CloseFile()
    {
        try
        {
            osw.close();
        }
        catch(IOException ie)
        {
            ie.printStackTrace();
        }
    }
}