//
// RFExplorer frequency range utility
//

package com.ictp.mrainone.rftrack.rfexplorer;

import java.text.DecimalFormat;

public class RFEfreq {

	//--------------------------------
	// RFExplorer Models
		// name of models
	public static final String[] RFExplorerName = { 
			"WSUB1G",			// 0 base
			"6G",               // 1 base
			"2.4G",             // 2 base
			"3G Combo",         // 3 combo
			"3G-24G Combo",     // 4 combo
			"ISM Combo",        // 5 combo
			"6G Combo",         // 6 combo
			"WiFi Combo"        // 7 combo
	};

	public static int idxModel;
	
	// frequency limits for base models
	public static final String[] BaseFreqList = { 
		"240-960",		// 0 WSUB1G,    
		"4850-6100",	// 1 6G,  
		"2350-2550"		// 2 2.4G   
		};

	public static final String[] ComboFreqList = { 
		"240-960",		// 0 3G Combo,    
		"15-2700",		// 1 3G Combo,    
		"2350-2550",	// 2 3G-24G Combo,
		"15-2700",		// 3 3G-24G Combo,
		"240-960",		// 4 ISM Combo,   
		"2350-2550",	// 5 ISM Combo,   
		"240-960",		// 6 6G Combo,    
		"2350-2550",	// 7 6G Combo,    
		"4850-6100",	// 8 WiFi Combo,  
		"2350-2550"		// 9 WiFi Combo   
		};
		
	//--------------------------------
	// default values
	/***
	Original values:
	static final int defFreqStart = 400000;				// def. KHZ, frequency span start (lower)
	static final int defFreqEnd = 500000;				// def. KHZ, frequency span end (higher)
	***/
	// modify 11/09: insert frequency baseline model WSUB1G
	//----------------------------------------
	// modify 14/09:
	// problem on settings limits frequency.
	// If we set the model limits, there is a problem on precalibration.
	// The operation goes in loop.
	// To avoid this, the limits must reduced of 60000Khz
	static final int fInc = 60000;		// value of increment
	//----------------------------------------
	//
	static final int defFreqStart = 240000 + fInc;				// def. KHZ, frequency span start (lower)
	static final int defFreqEnd = 960000 - fInc;				// def. KHZ, frequency span end (higher)
	
	static final int defNStep = 112;					// default, n. of step
	
	//--------------------------------
	// in base to model return the string of freq limits
	public static String StrFreqLimits(int nmodel, int idx)
	{
		int pos;

		if(nmodel<0)
		{
			nmodel = 0;
		}
		else if(nmodel>7)
		{
			nmodel = 0;
		}
		if(nmodel < 3)
		{
			// base models
			return(BaseFreqList[nmodel]);
		}
		// combo models
		if((idx<0)||(idx>1))
			idx = 0;
		pos = (nmodel - 3)*2 + idx;
		return(ComboFreqList[pos]);
	}

	public static void SetFreqLimits(int nmodel, int idx)
	{
		int pos;

		if(nmodel<0)
		{
			nmodel = 0;
		}
		else if(nmodel>7)
		{
			nmodel = 0;
		}
		if((idx<0) || (idx>1))
			idx = 0;
		pos = nmodel;
		if(pos >=3)
		{
			pos = 3 + (pos - 3) * 2 + idx;
		}
		switch(pos)
		{
			// base
			case 0: 	Set(240000	+ fInc	,960000	 - fInc		); break;		// 0 WSUB1G	
			case 1: 	Set(4850000	+ fInc	,6100000 - fInc		); break;		// 1 6G   
			case 2: 	Set(2350000	+ fInc	,2550000 - fInc		); break;		// 2 2.4G     
			// combo
			case 3: 	Set(240000	+ fInc	,960000	 - fInc		); break;		// 0 3G Combo  
			case 4: 	Set(15000	+ fInc	,2700000 - fInc		); break;		// 1 3G Combo    
			case 5: 	Set(2350000	+ fInc	,2550000 - fInc		); break;		// 2 3G 24G Combo
			case 6: 	Set(15000	+ fInc	,2700000 - fInc		); break;		// 3 3G	24G Combo
			case 7: 	Set(240000	+ fInc	,960000	 - fInc		); break;		// 4 ISM Combo  
			case 8: 	Set(2350000	+ fInc	,2550000 - fInc		); break;		// 5 ISM Combo   
			case 9: 	Set(240000	+ fInc	,960000	 - fInc		); break;		// 6 6G Combo    
			case 10: 	Set(2350000	+ fInc	,2550000 - fInc		); break;		// 7 6G Combo    
			case 11: 	Set(4850000	+ fInc	,6100000 - fInc		); break;		// 8 WiFi Combo  
			case 12: 	Set(2350000	+ fInc	,2550000 - fInc		); break;		// 9 WiFi Combo   
		}
	}
	
	//--------------------------------

	static int Start_Freq = defFreqStart;				// KHZ, Value of frequency span start (lower)
	static int End_Freq = defFreqEnd;					// KHZ, Value of frequency span end (higher)
	static int n_steps = defNStep;		// n. di step in cui e' suddiviso il range di frequenze
	
	// default configuration parameters
	public static void SetDefault()
	{
		Start_Freq = defFreqStart;	// 7 ASCII digits, decimal, KHZ, Value of frequency span start (lower)
		End_Freq = defFreqEnd;		// 7 ASCII digits, decimal, KHZ, Value of frequency span end (higher)
		n_steps = defNStep;			// n. di step in cui e' suddiviso il range di frequenze
	}
	
	public static void Set(
        int startFreq,		// KHZ, Value of frequency span start (lower)
        int endFreq			// KHZ, Value of frequency span end (higher)
	)
	{
        if(startFreq<endFreq)
		{
			Start_Freq = startFreq;		// KHZ, Value of frequency span start (lower)
			End_Freq = endFreq;			// KHZ, Value of frequency span end (higher)
		}
		else
		{
			Start_Freq = endFreq;		// KHZ, Value of frequency span start (lower)
			End_Freq = startFreq;		// KHZ, Value of frequency span end (higher)
		}
	}

	// read the start frequency
	public static int GetStart()
	{
		return(Start_Freq);
	}
	
	// read the end frequency
	public static int GetEnd()
	{
		return(End_Freq);
	}

	// imposta il numero di step della suddivisione del range di frequenze
	public static void SetSteps(int steps)
	{
		if(steps<1)
			steps = 1;
		n_steps = steps;
	}
	public static int GetSteps()
	{
		return(n_steps);
	}
	// return true if the steps are same
	public static boolean areSameSteps(int steps)
	{
		if(steps == n_steps)
			return true;
		return(false);
	}
	
	// Returns the frequency in Mhz corresponding to the index idx
	public static float FreqMhz(int idx)
	{
		float ris = (float)Start_Freq;
		if(idx>0)
		{
			ris = ris + idx * ((float)(End_Freq - Start_Freq)/(float)n_steps);
		}
		ris = (ris / 1000.0f);            // freq. in MHz
		return(ris);
	}

	// Encode a row in csv format with the values of frequency
	public static String CsvString(DecimalFormat Formatter, String Separator) 
	{
        // int length = array.length;
		String result = "";
		float freq;

		// http://www.coderanch.com/t/385190/java/java/Setting-decimalFormatSymbols
        // DecimalFormat FormatterFreq = new DecimalFormat("####.000", CsvDecimalFormatSym);
        // DecimalFormat FormatterFreq = new DecimalFormat("* ##0.000", CsvDecimalFormatSym);
		
		result= result + String.format("% 4d",(int)n_steps);
		for (int i = 0; i < n_steps; i++) 
		{
			result= result + Separator;
			freq = FreqMhz(i);
			result= result + Formatter.format(freq);
		}
		return result;
	}
	
}