package com.ictp.mrainone.rftrack.rfexplorer;

import com.ictp.mrainone.rftrack.rfexplorer.RFEfreq;
import com.ictp.mrainone.rftrack.rfexplorer.RFEdBm;

//-----------------------------------------------------------
// Configuration parameters of the measures
//
public class RFEConfiguration
{
	// public int Start_Freq;		// 7 ASCII digits, decimal, KHZ, Value of frequency span start (lower)
	// public int End_Freq;		// 7 ASCII digits, decimal, KHZ, Value of frequency span end (higher)
	public int Amp_Top;			// 4 ASCII digits, decimal, dBm, Highest value of amplitude for GUI
	public int Amp_Bottom;		// 4 ASCII digits, decimal, dBm, Lowest value of amplitude for GUI
	
	// default configuration parameters
	public void SetDefault()
	{
		RFEfreq.SetDefault();	// frequency defaults
		// Start_Freq = 400000;	// 7 ASCII digits, decimal, KHZ, Value of frequency span start (lower)
		// End_Freq = 500000;		// 7 ASCII digits, decimal, KHZ, Value of frequency span end (higher)
		RFEdBm.SetDefault();	// set dBm limits default
		
		Amp_Top = RFEdBm.toInt(RFEdBm.HighdBm);			// Highest value of amplitude for GUI
		Amp_Bottom = RFEdBm.toInt(RFEdBm.LowdBm);		// Lowest value of amplitude for GUI
	}

	// class costructor
	public RFEConfiguration()
	{
		SetDefault();
	}

	/****
	// Returns the frequency corresponding to the index idx
	public float GetFrequency(int idx, int sweep_steps)
	{
		float ris = (float)Start_Freq;
		if(idx>0)
		{
			ris = ris + idx * ((float)(End_Freq - Start_Freq)/(float)sweep_steps);
		}
		ris = (ris / 1000.0f);            // freq. in MHz
		return(ris);
	}
	*****/
	
	// Form the string to be displayed on the set parameters.
	// Display the start/end frequency values in MHz
	public String strInfoParam()
	{
		String result = "Ranges:    ";

		// show frequency
		result= result + String.format("Frequency (MHz): " +
									"[" +
									"% 4d ... % 4d" +
									"]",
									RFEfreq.GetStart() / 1000,			// freq. in Mhz
									RFEfreq.GetEnd() / 1000);			// freq. in Mhz
		// show DB signal amplitude
		result= result + "   ";
		result= result + String.format("Amplitude (dBm): " +
									"[" +
									"% 3d ... % 3d" +
									"]",
									(Amp_Bottom),
									(Amp_Top) );
		
		return result;
	}
	
	
	// Prepare the string to display the parameters.
	// Values of start and end frequency in MHz
	public String strShowParam()
	{
		int len = 7 + 7 + 4 + 4 + 3; 		// Total length of the string, including separators
		StringBuffer dest = new StringBuffer(len);
		
		// For clarity, first appears Amp_Bottom then Amp_Top
		dest.append(String.format("% 4d, % 4d, % 3d, % 3d",
								  RFEfreq.GetStart() / 1000,			// freq. in Mhz
								  RFEfreq.GetEnd() / 1000,				// freq. in Mhz
								  (Amp_Bottom),
								  (Amp_Top)
								  )
								  );
		return dest.toString();
	}
	
	
	// Form the parameter string to send the message through the USB port
	public String strGetParam()
	{
		int len = 7 + 7 + 4 + 4 + 3;		// Total length of the string, including separators
		StringBuffer dest = new StringBuffer(len);
		dest.append(String.format("%07d,%07d,-%03d,-%03d",
								  RFEfreq.GetStart(), 
								  RFEfreq.GetEnd(), 
								  Math.abs(Amp_Top), 
								  Math.abs(Amp_Bottom)));
		return dest.toString();
	}

	// Recognize the configuration parameters RrTrack provided in the string.
	// modify 10/07: the freq. limits are in Mhz
	// MOdify 08/02/2015: the frequency limit is 6500 (4850-6100 frequency range for RFExplorer 6G model)
	public void strSetParam(String s)
	{
		// Integer: MAX, MIN VALUE
		// int maximum value (2^31-1), int minimum value -2^31.
		int tmp;
		int fld = 0;			// field index
		int Start_Freq = RFEfreq.GetStart();	// KHZ, Value of frequency span start (lower)
		int End_Freq = RFEfreq.GetEnd();		// KHZ, Value of frequency span end (higher)

		// Array containing the string subdivided in parts
		String[] items = s.split(",");
		
		for(fld = 0; fld<items.length;fld++)
		{
			// Convert the word to a numeric value
			// Note: trim clear the spaces before and after the numerical part.
			// ParseInt wants a string formed only by numeric characters
			tmp = Integer.parseInt(items[fld].trim());
			switch (fld)
			{
			case 0:             			// Start_Freq
				// Incorrect frequency value is negative
				if (tmp < 0)
					tmp = -tmp;
				// Do not accept the value if the frequency is greater than or equal to 6500 (in MHz),
				if (tmp >= 6500)
					break;
				Start_Freq = tmp * 1000;	// Save the freq. in KHz
				break;
			case 1:             // End_Freq
				// Incorrect frequency value is negative
				if (tmp < 0)
					tmp = -tmp;
				// Do not accept the value if the frequency is greater than or equal to 6500 (in MHz),
				if (tmp >= 6500)
					break;
				End_Freq = tmp * 1000;		// Save the freq. in KHz
				break;
				// modify 15/07:
				// For clarity, has changed the order of recognition,
				// Is read before Amp_Bottom (par. 3) then Amp_top (par. 2).
				// Remember that in the message serial order is reversed
			case 2:             			// Amp_Bottom
				// Do not accept an absolute value that is greater or equal to 1000
				if(Math.abs(tmp)>=1000)
					break;
				// If the value of the amplification is positive, changes sign
				if (tmp > 0)
					tmp = -tmp;
				Amp_Bottom = tmp;
				break;
			case 3:             // Amp_Top
				// Do not accept an absolute value that is greater or equal to 1000
				if(Math.abs(tmp)>=1000)
					break;
				// If the value of the amplification is positive, changes sign
				if (tmp > 0)
					tmp = -tmp;
				Amp_Top = tmp;
				break;
			}
		}
		RFEfreq.Set(Start_Freq, End_Freq);
		/***
		// If End_Freq <Start_Freq, exchange values
		if (End_Freq < Start_Freq)
		{
			tmp = End_Freq;
			End_Freq = Start_Freq;
			Start_Freq = tmp;
		}
		***/
		// If Amp_Top <Amp_Bottom, exchange values
		if (Amp_Top < Amp_Bottom)
		{
			tmp = Amp_Top;
			Amp_Top = Amp_Bottom;
			Amp_Bottom = tmp;
		}
		RFEdBm.HighdBm = RFEdBm.todBm(Amp_Top);
		RFEdBm.LowdBm = RFEdBm.todBm(Amp_Bottom);
	}

	// set the parameters
	public void SetParam(
		int freqStart,
		int freqEnd,
		int ampBottom,
		int ampTop
		)
	{
		// Integer: MAX, MIN VALUE
		// int maximum value (2^31-1), int minimum value -2^31.
		int tmp;
		int Start_Freq = RFEfreq.GetStart();	// KHZ, Value of frequency span start (lower)
		int End_Freq = RFEfreq.GetEnd();		// KHZ, Value of frequency span end (higher)

		tmp = freqStart;
		// Incorrect frequency value is negative
		if (tmp < 0)
			tmp = -tmp;
		// Do not accept the value if the frequency is greater than or equal to 6500 (in MHz),
		if (tmp >= 6500000)
			tmp = 6500000;
		Start_Freq = tmp;	// Save the freq. in KHz
		
		tmp = freqEnd;
		// Incorrect frequency value is negative
		if (tmp < 0)
			tmp = -tmp;
		// Do not accept the value if the frequency is greater than or equal to 6500 (in MHz),
		if (tmp >= 6500000)
			tmp = 6500000;
		End_Freq = tmp;	// Save the freq. in KHz

		tmp = ampBottom;
		// If the value of the amplification is positive, changes sign
		if (tmp > 0)
			tmp = -tmp;
		if(tmp<=-1000)
			tmp = -1000;
		Amp_Bottom = tmp;

		tmp = ampTop;
		// If the value of the amplification is positive, changes sign
		if (tmp > 0)
			tmp = -tmp;
		if(tmp<=-1000)
			tmp = -1000;
		Amp_Top = tmp;
		
		RFEfreq.Set(Start_Freq, End_Freq);
		if (Amp_Top < Amp_Bottom)
		{
			tmp = Amp_Top;
			Amp_Top = Amp_Bottom;
			Amp_Bottom = tmp;
		}
		RFEdBm.HighdBm = RFEdBm.todBm(Amp_Top);
		RFEdBm.LowdBm = RFEdBm.todBm(Amp_Bottom);
	}
	
}					// end of public class RFEConfiguration
