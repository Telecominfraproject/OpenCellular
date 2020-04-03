//
// RFExplorer dBm data utility
//
// In RFExplorer i dBm vengono salvati in un byte.
// https://en.wikipedia.org/wiki/DBm
// dBm (dBmW or Decibel-milliwatts), abbreviation for the power ratio in decibels (dB) 
// of the measured power referenced to one milliwatt (mW).
// Power level	Power	Notes
// 80 dBm       100 kW	Typical transmission power of FM radio station with 50-kilometre (31 mi) range
// 36 dBm       4 W Typical maximum output power for a Citizens' band radio station (27 MHz) in many countries
// 27 dBm	500 mW	Typical cellular phone transmission power
// 0 dBm        1.0 mW  Bluetooth standard (Class 3) radio, 1 m range
// −10 dBm      100 µW  Maximum received signal power of wireless network (802.11 variants)
// −73 dBm      50.12 pW "S9" signal strength, a strong signal, on the S-meter of a typical ham or shortwave radio receiver
// −100 dBm     0.1 pW  Minimum received signal power of wireless network (802.11 variants)
// −127.5 dBm   0.178 fW = 178 aW  Typical received signal power from a GPS satellite
// Lo strumento fornisce i valori in dBm con questa rappresentazione:
// 1) i dBm sono cambiati di segno
// 2) il valore viene moltiplicato per 2
// Per contenere i dBb in un byte, con questo tipo di rappresentazione il valore minimo possibile
// corrisponde al valore esadecimale 0xFF (255 corrisponde a: (-255 / 2) = -127.5
//

package com.ictp.mrainone.rftrack.rfexplorer;

import java.text.DecimalFormat;
import com.ictp.mrainone.rftrack.rfexplorer.RFEfreq;

public class RFEdBm {

	// valore minimo dBm:
	// −127.5 dBm   0.178 fW = 178 aW  Typical received signal power from a GPS satellite
	// Lo strumento fornisce i valori in dBm con questa rappresentazione:
	// 1) i dBm sono cambiati di segno
	// 2) il valore viene moltiplicato per 2
	// Per contenere i dBb in un byte, con questo tipo di rappresentazione il valore minimo possibile
	// corrisponde al valore esadecimale 0xFF (255 corrisponde a: (-255 / 2) = -127.5
	static final byte LowestdBm = (byte)(255 & 0xFF);
	
	// default values for dBm limits:
	static final byte defLowdBm = (byte)(240 & 0xFF);	// min default: -120 (stored as 240)
	static final byte defHighdBm = (byte)(100 & 0xFF);	// max default: -50 (stored as 160)
	// static final byte defHighdBm = 160;	// max default: -80 (stored as 160)
	
	// values for dbm limits
	static byte LowdBm = defLowdBm;		// min value dBm
	static byte HighdBm = defHighdBm;	// max value dBm
	
	// default configuration parameters
	public static void SetDefault()
	{
		LowdBm = defLowdBm;		// min value dBm
		HighdBm = defHighdBm;	// max value dBm
	}
	
	// Converts the dBm byte value in float
	public static int toInt(byte dBmVal)
	{
		int val;
		int b;
		
		// calculate the value of dBm: Beware of the unsigned value !!!
		b = dBmVal & 0xFF; 			// b is an integer
		b = -b;
		val = b/2;		// According to the protocol specification, the value is divided by 2
		
		return(val);
	}

	// Converts the dBm byte value in float
	public static float toFloat(byte dBmVal)
	{
		float fVal;
		int b;
		
		// calculate the value of dBm: Beware of the unsigned value !!!
		b = dBmVal & 0xFF; 			// b is an integer
		b = -b;
		fVal = ((float)b)/2.0f;		// According to the protocol specification, the value is divided by 2
		
		return(fVal);
	}
	
	// valore intero da trasformare nel formato dBm di RFExplorer
	// nota: il valore intero non fornisce valori dopo la virgola
	public static byte todBm(int value)
	{
		int val;
		val = - value;
		val = (val * 2) & 0xFF;
		return( (byte)val );
	}
	// valore reale da trasformare nel formato dBm di RFExplorer
	public static byte todBm(float value)
	{
		float val;
		int b;

		val = - value;
		val = (val * 2.0f);
		b = ( (int)Math.round(val)) & 0xFF;
		return( (byte)b );
	}
	
	// compare two dBm and return the dBm max value
	// note: the dBm has the sign changed
	public static byte max(byte dBm1, byte dBm2)
	{
		if(dBm1 < dBm2)
			return dBm1;
		return dBm2;
	}
	// compare two dBm and return the dBm max value
	// note: the dBm has the sign changed
	public static byte min(byte dBm1, byte dBm2)
	{
		if(dBm1 > dBm2)
			return dBm1;
		return dBm2;
	}
	
	// Encode a row in csv format with the values contained in a byte array
	public static String CsvString(byte[] array, DecimalFormat Formatter, String Separator) 
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
				result= result + Separator;
			}
			// Insert the value: Beware of the unsigned value !!!
			val = RFEdBm.toFloat(array[i]);
			
			// result= result + String.format("% 5.1f", val);
            // result= result + FormatterDBval.format(val).replaceAll("\\G0", " ");
            result= result + Formatter.format(val);
		}

		return result;
	}
	
}