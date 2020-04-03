
// classe che contiene un messaggio da parte di rf-explorer
//
package com.ictp.mrainone.rftrack.rfexplorer;

// import com.ictp.mrainone.rftrack.rfexplorer.RFEfreq;
import com.ictp.mrainone.rftrack.rfexplorer.RFEdBm;

public class RFEdata
{
	long id;					// n. id
	long tm;					// tempo di creazione
	
	byte[] msg;					// messaggio ricevuto con i valori dBm
	int len;					// lunghezza del messaggio
	
	//--------------------------------
	// valori minimi, massimi, medi per l'analisi
	byte min;					// valore minimo
	int med;					// valore medio

	byte max;					// valore massimo
	int idxMax;					// indice dell'elemento che contiene il valore massimo
	
	static byte globalMax;		// valore massimo globale
	static int globalIdxMax;	// indice del valore max globale
	
	//--------------------------------
	
	// costruttore
	public RFEdata(
		long Index,
		long tmAcq,			// tempo di acquisizione in msec
		byte[] data, 
		int datalen)
	{
		byte temp;						
		
		// copy dBm
		msg = new byte[datalen];			// crea il vettore
		len = datalen;
		// arraycopy(Object src, int srcPos, Object dest, int destPos, int length)
		System.arraycopy(data, 0, msg, 0, datalen);
		
		id = Index;
		// tm = System.currentTimeMillis();	// leggi il tempo di creazione
		tm = tmAcq;

		min = data[0];
		max = data[0];
		med = (int)min;
		
		if(Index == 0)
		{
			// e' la prima volta, inizializza i valori max globali
			globalMax = data[0];
			globalIdxMax = 0;
		}
		
		if(datalen == 0)
			return;
		
		// calcola i valori minimi, max, medi
		
		for(int i = 1;i<datalen;i++)
		{
			min = RFEdBm.min(min, data[i]);
			med = med + (int)data[i];

			temp = max;			// vecchio max
			max = RFEdBm.max(max, data[i]);
			if(temp != max)
			{
				idxMax = i;		// indice nuovo max
			}
		}
		med = med / datalen;
		
		// controlla se il massimo e' superiore a globalMax
		temp = globalMax;
		globalMax = RFEdBm.max(max, globalMax);
		if(temp != globalMax)
		{
			globalIdxMax = idxMax;		// indice nuovo max
		}
	}
	
	// return the msg length
	public int getMsgLen() {
		return len;
	}
	
	// per la gestione db: restituisce il valore intero del dBm, senza la parte decimale.
	// Per il tipo di applicazione, questa non e' significativa
	public int getdBmInt(int idx) {
		if((idx<0) || (idx>len))
		{
			return -255;		// restituisci un valore dbm estremamente piccola
		}
		int result = -((int)msg[idx] & 0xFF);
		return (result / 2);
	}
	
	
	// restituisce copia del messaggio
	public byte[]CopyMsg()
	{
		byte[] result = new byte[len];
		
		// arraycopy(Object src, int srcPos, Object dest, int destPos, int length)
		System.arraycopy(msg, 0, result, 0, len);
		return(result);
	}

	// restituisce copia dei dBm in intero cambiati di segno
	public int[]CopyMsgToIntArray()
	{
		int[] result = new int[len];
		
		// arraycopy(Object src, int srcPos, Object dest, int destPos, int length)
		for(int i=0;i<len;i++)
		{
			result[i] = -((int)msg[i] & 0xFF);
		}
		return(result);
	}

	// per la gestione del grafico: 
	// crea un vettore di float contenente i dBm, senza la parte decimale.
	// Per il tipo di applicazione, questa non e' significativa
	public float[]CopydBmToFloatArray()
	{
		int value;
		float[] float_array = new float[len];
		
		for(int i=0;i<len;i++)
		{
			value = -((int)msg[i] & 0xFF);
			value = (value / 2);
			// load the float_array
			float_array[i] = (float)value;
		}
		return(float_array);
	}

	// versione che copia i dati nell'array definito in float_array
	public void CopydBmToFloatArray(float[] float_array)
	{
		int value;
		
		for(int i=0;i<len;i++)
		{
			value = -((int)msg[i] & 0xFF);
			value = (value / 2);
			// load the float_array
			float_array[i] = (float)value;
		}
		return;
	}

	// per verifica occupazione white spaces: 
	// Restituisce la percentuale di campioni dBm che risultano sotto la soglia impostata
	// Per il tipo di applicazione, questa non e' significativa
	public float PercdBmBelowThreshold(float threshold)
	{
		int count;
		int value;
		float data;
		
		count = 0;
		for(int i=0;i<len;i++)
		{
			value = -((int)msg[i] & 0xFF);
			value = (value / 2);
			data = (float)value;
			if(data<=threshold)
				count++;
		}
		// calc percentuage
		data = (float)count;
		data = data * 100.0f;
		data = data / ((float)len);
		return(data);
	}

	
	// restituisce il riferimento al messaggio
	public byte[]Msg()
	{
		return(msg);
	}
	
	public long Time()
	{
		return (tm);
	}
	
	public byte Max()
	{
		return (max);
	}
	
	public byte Min()
	{
		return (min);
	}
	
	public byte Media()
	{
		return (byte)(med & 0xFF);
	}

	public long Id()
	{
		return (id);
	}

	// indice e valore max globale
	public byte GlobalMax()
	{
		return (globalMax);
	}

	public int GlobalIdxMax()
	{
		return (globalIdxMax);
	}
	
}