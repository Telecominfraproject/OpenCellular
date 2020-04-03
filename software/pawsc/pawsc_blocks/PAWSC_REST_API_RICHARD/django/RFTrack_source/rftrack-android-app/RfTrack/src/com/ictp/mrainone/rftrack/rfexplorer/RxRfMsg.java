
// classe per il riconoscimento del messaggio da parte di rf-explorer
//

package com.ictp.mrainone.rftrack.rfexplorer;

// per la gestione db
import com.ictp.mrainone.rftrack.helper.DatabaseHelper;

// import com.ictp.mrainone.rftrack.rfexplorer.RFEfreq;
import com.ictp.mrainone.rftrack.rfexplorer.RFEdata;
import com.ictp.mrainone.rftrack.rfexplorer.RFEdataFifo;

import com.ictp.mrainone.rftrack.util.GPSTracker;

public class RxRfMsg
{
	
	//-----------------------------------
	// Database Helper per db sqlite
	DatabaseHelper db;

	//-----------------------------------
	// gps tracker
	GPSTracker gps;
	long campaign_id;          // id actual campaign
	long location_id;			// id actual location
	//-----------------------------------
	// per config
	long config_id;				// id actual config
	
	//-----------------------------------
	// ring buffer
	// private static int rbSize = 1024;					// ring buffer size
	// private static int rbSize = 2048;					// ring buffer size
	private static int rbSize = 32000;					// ring buffer size
	private static ByteRingBuffer ringBuffer;
	//-----------------------------------
	
	// fifo utilizzato per memorizzare i messaggi
	private static RFEdataFifo RxFifo;

	// dimensione massima del messaggio da ricevere
	// - 2 byte iniziali: 0x24, 0x53 ('$S')
	// - un byte contenente in numero di valori in dBm trasmessi in seguito (normalmente 112, pari a 0x70)
	// - la sequenza di dati (teoricamente 256 valori)
	// - 2 byte finali 0x0D e 0x0A
	public static final int MaxLenMsg = 256 + 3;
	
	// n. di messaggi ricevuti in funzione dei parametri inviati
	static long nMsgRead = 0;

	//-----------------------------------
	// 12/04: per la gestione del grafico sulla pag. principale
	public float[] GraphdBm = null;
	//-----------------------------------
	
	private static long tmSendMessage;		// tempo di inizio invio messaggio in msec
	
	public enum State {

		ST_GET_CHRMSG_0x24,					// first char of message
		ST_GET_CHRMSG_0x53,					// second char
		ST_GET_CHRMSG_DATALEN,				// read the len of sequence of bytes
		ST_GET_CHRMSG_DATACHR,				// acquisition of data characters
		ST_GET_CHRMSG_0x0D,
		ST_GET_CHRMSG_0x0A;

		int constant() {
			return this.ordinal();
		}
	}
	State state;			// stato della macchina di ricezione
	
	public int lenMsg;		// lunghezza completa messaggio
	public int lenData;		// lunghezza della parte dati messaggio
	public boolean RxAll;	// true se il messaggio e' ricevuto completamente
	public byte[] msgRx = new byte[MaxLenMsg];


	//--------------------------------------
	// percentuale campioni sotto il threshold
	static float PercBelowThreshold;
	
	//--------------------------------------
	// max values
	static float MaxdBmFreq;		// Value of the frequency corresponding to the maximum value of the signal
	static float MaxdBm;			// Maximum dBm signal value in floating point
	static boolean fMaxdBmUpdated = false;	// posto a true se il max dBm e' aggiornato
	
	//--------------------------------------
	// valori passati al messaggio config
	int Start_Freq;			// KHZ, Value of frequency span start (lower)
	int End_Freq;			// KHZ, Value of frequency span end (higher)
	int Amp_Top;			// dBm, Highest value of amplitude for GUI
	int Amp_Bottom;			// dBm, Lowest value of amplitude for GUI

	//--------------------------------------
	
	// pulisci il buffer di messaggio
	public void clean()
	{
		RxAll = false;
		initStateRxMsg();			// inizializza la macchina a stati per la lettura messaggio
	}

	// costruttore
	public RxRfMsg(
		RFEdataFifo DataFifo, 
		DatabaseHelper dbHelper,
		GPSTracker gpsTrk)
	{
		RxFifo = DataFifo;
		
		ringBuffer = new ByteRingBuffer(rbSize);			// crea il ringbuffer
		//------------------------
		// aggiunta 19/07
		ringBuffer.clear();
		//------------------------
		// per la gestione db sqlite
		db = dbHelper;
		//------------------------
		// per la gestione tracking
		gps = gpsTrk;
		location_id = 1;
		campaign_id = 1;
		if(gps != null)
		{
			campaign_id = gps.getCampaignId();
			location_id = gps.getLocationId();
		}
		//------------------------
		this.clean();
		//------------------------
		GraphdBm = null;
		//------------------------
	}

	public void setdbHelper(DatabaseHelper dbHelper)
	{
		//------------------------
		// per la gestione db sqlite
		db = dbHelper;
		//------------------------
	}
	
	// forma il messaggio per la lettura dei dati
	// #<Size>C2-F:<Start_Freq>, <End_Freq>, <Amp_Top>, <Amp_Bottom>
	// <Size> =	Binary byte, Total size of the message in bytes. Size is limited to max 64 bytes.
	// <Start_Freq> = 7 ASCII digits, decimal KHZ, value of frequency span start (lower)
	// <End_Freq> =	7 ASCII digits, decimal	KHZ, value of frequency span end (higher)
	// <Amp_Top> = 4 ASCII digits, decimal dBm,	Highest value of amplitude for GUI 
	// <Amp_Bottom> = 4 ASCII digits, decimal dBm, Lowest value of amplitude for GUI 
	//
	public byte[] MsgConfigurationData(
        int startFreq,		// KHZ, Value of frequency span start (lower)
        int endFreq,		// KHZ, Value of frequency span end (higher)
        int ampTop,			// dBm, Highest value of amplitude for GUI
        int ampBottom)		// dBm, Lowest value of amplitude for GUI
	{
		Start_Freq = startFreq;
		End_Freq = endFreq;
		Amp_Top = ampTop; 
		Amp_Bottom = ampBottom;

		String msg = String.format("# C2-F:%07d,%07d,-%03d,-%03d", 
						Start_Freq, 
						End_Freq, 
						Math.abs(Amp_Top), 
						Math.abs(Amp_Bottom));
						
		// inizializza il numero di messaggi ottenuti come risposta
		nMsgRead = 0;

		// initialize la percentuale below threshold
		PercBelowThreshold=0.0f;
		
		// initialize the values of max dBm
		MaxdBm = -255.0f;
		MaxdBmFreq = 0.0f;
		fMaxdBmUpdated = false;
		
		// leggi il tempo di invio del messaggio di configurazione
		tmSendMessage = System.currentTimeMillis();
		
		// restituisci il messaggio da inviare
		return(msg.getBytes());
	}
	
	// leggi il tempo di invio messaggio di configurazione
	long GetTimeStartAcquisition()
	{
		return(tmSendMessage);
	}
	
	// restituisci il n. di msec a partire da tmSendMessage
	public long DeltaTmMsec(long end_time)
	{
		return(end_time - tmSendMessage);
	}
	
	// tempo in secondi a partire da tmSendMessage
	public float DeltaTmSeconds(long end_time)
	{
		return( ((float)DeltaTmMsec(end_time))/1000.0f );
	}
	
	// inizializza la macchina a stati per la lettura messaggio
	public void initStateRxMsg()
	{
		lenMsg = 0;
		state = State.ST_GET_CHRMSG_0x24;
	}
	
	// controlla se un byte e' presente nel buffer circolare.
	// Esegue la ricerca fino a una posizione pari a len
	private boolean FindByteMsg(Byte val, int pos, int len)
	{
		int i, rd;
		
		// continua a leggere un solo carattere fino a che non lo hai trovato
		for(i=0; i<len; i++)
		{
			rd = ringBuffer.read(msgRx, pos, 1);
			if(rd == 0)
			{
				// il buffer circolare e' vuoto oppure len e' maggiore della sua lunghezza
				return(false);
			}
			if(msgRx[pos]==val)
			{
				// byte trovato
				return(true);
			}
		}
		return(false);
	}
	
	// leggi la parte dati del messaggio
	private boolean GetMsgData()
	{
		int n_byte_rd;						// numero di byte letti
		int len = lenData - (lenMsg - 3);	// n. byte di dati ancora da ricevere

		// leggi i caratteri attualmente nel buffer.
		// Al massimo leggi un numero di caratteri pari a len
		n_byte_rd = ringBuffer.read(msgRx, lenMsg, len);
		lenMsg = lenMsg + n_byte_rd;		// aggiorna la lunghezza del messaggio
		if(n_byte_rd < len)
			return(false);
		// lettura parte dati completata
		return(true);
	}
	
	//----------------------------------------------------------------------------
	// procedure gestione valore massimo dBm
	
    private boolean updateMaxdBm(RFEdata msg)
	{
		// row of max dBm
		float val;
		boolean fUpdate;
		
		val = RFEdBm.toFloat(msg.GlobalMax());		// max convertito in float
		fUpdate = false;
		if(MaxdBm < val)
		{
			// aggiorna i dBm
			MaxdBm = val;
			MaxdBmFreq = RFEfreq.FreqMhz(msg.GlobalIdxMax());
			fUpdate = true;
		}
		else if(MaxdBm == val)
		{
			// aggiorna la frequenza
			val = RFEfreq.FreqMhz(msg.GlobalIdxMax());
			if(val != MaxdBmFreq)
			{
				MaxdBmFreq = val;
				fUpdate = true;
			}
		}
		if(fUpdate)
			fMaxdBmUpdated = true;
		
		return(fUpdate);
	}

	public boolean MaxdBmIsChanged() {
		return fMaxdBmUpdated;
	}
	
	public boolean resetChangeMaxdBm() {
		fMaxdBmUpdated = false;
		return fMaxdBmUpdated;
	}
	
	public float getMaxdBm() {
		return MaxdBm;
	}
	
	public float getMaxdBmFreq() {
		return MaxdBmFreq;
	}
	
	public long getNMsgRead() {
		return nMsgRead;
	}

	public float[] getGraphDbm() {
		return GraphdBm;
	}
	
	public float getPercBelowThreshold() {
		return PercBelowThreshold;
	}
	
	//----------------------------------------------------------------------------
	// macchina a stati per decodificare il messaggio di acquisizione dati da parte
	// dello strumento
	//

	private void DecodeRFEAcquisition(byte[] data)
	{
		// int i;
		int pos = 0;			// posizione dove analizzare il  carattere

		while(pos < data.length)
		{
			switch (state)
			{
			case ST_GET_CHRMSG_0x24:
				// read the start of message char ('$' = 0x24)
				lenMsg = 0;				// lunghezza globale messaggio
				lenData = 0;			// lunghezza parte dati
				for(;pos<data.length;pos++)
				{
					if(data[pos] == '$')
					{
						msgRx[lenMsg] = '$';
						lenMsg++;
						state = State.ST_GET_CHRMSG_0x53;
						break;
					}
				}
				if(state == State.ST_GET_CHRMSG_0x53)
				{
					pos++;
				}
				break;

			case ST_GET_CHRMSG_0x53:
				// after ('$' = 0x24), read the char ('S' = 0x53)
				if(data[pos] == (byte)0x53)
				{
					msgRx[lenMsg] = (byte)0x53;
					lenMsg++;
					pos++;
					state = State.ST_GET_CHRMSG_DATALEN;
				}
				else
				{
					// carattere non trovato
					initStateRxMsg(); 		// reinizializza la macchina a stati per  il riconoscimento
				}
				break;

			case ST_GET_CHRMSG_DATALEN:
				// leggi il terzo carattere, lunghezza della parte dati del messaggio
				msgRx[lenMsg] = data[pos];
				lenData = (int)(msgRx[lenMsg]);			// n. di byte di cui e' composto la parte data
				lenMsg++;
				pos++;
				state = State.ST_GET_CHRMSG_DATACHR;
				break;

			case ST_GET_CHRMSG_DATACHR:
				// leggi la parte dati
				int nByteToCopy = data.length - pos;
				int nByteRemained = lenData - (lenMsg - 3);			// n. byte rimanenti da copiare
				if(nByteToCopy>=nByteRemained)
					nByteToCopy = nByteRemained;
				// usa System.arraycopy per copiare i byte: 
				// public static void arraycopy(Object src, int srcPos, Object dest, int destPos, int length)
				// copia soltanto la parte data
				System.arraycopy(data, pos, msgRx, lenMsg, nByteToCopy);
				pos = pos + nByteToCopy;
				lenMsg = lenMsg + nByteToCopy;
				if(lenMsg >= (3 + lenData))
				{
					// letto tutto il messaggio
					state = State.ST_GET_CHRMSG_0x0D;
				}
				break;

			case ST_GET_CHRMSG_0x0D:
				// end of message first char: 0x0D
				if(data[pos] == (byte)0x0D)
				{
					msgRx[lenMsg] = (byte)0x0D;
					pos++;
					lenMsg++;
					state = State.ST_GET_CHRMSG_0x0A;
				}
				else
				{
					// carattere non trovato
					initStateRxMsg(); 		// reinizializza la macchina a stati per il riconoscimento
				}
				break;

			case ST_GET_CHRMSG_0x0A:
				// end of message second char: 0x0A
				if(data[pos] == (byte)0x0A)
				{
					msgRx[lenMsg] = (byte)0x0A;
					pos++;
					lenMsg++;
					// messaggio ricevuto completamente
					if(nMsgRead == 0)
					{
						// si tratta del primo messaggio dopo l'invio del comando.
						// Memorizza il numero di caratteri di cui e' composto il messaggio
						RFEfreq.SetSteps(lenData);		// lunghezza della parte dati messaggio

						// leggi l'id dell'attuale campagna
						campaign_id = gps.getCampaignId();

						// crea config
						config_id = 0;
						if(db != null)
						{
							config_id = db.createConfig(
											campaign_id, 
											Start_Freq, 
											End_Freq, 
											Amp_Top, 
											Amp_Bottom, 
											lenData);
						}
					}

					long tm = System.currentTimeMillis();	// leggi il tempo di creazione
					RFEdata msg = new RFEdata(nMsgRead, tm,
						GetMsg(), lenData);
					
					// il messaggio e' stato letto completamente
					RxFifo.add(msg);
					nMsgRead++; 		// incrementa il n. di messaggi letti

					//------------------------------------
					// modify 12/04: 
					// il messaggio e' stato ricevuto completamente,
					// aggiorna il vettore di dati per il grafico
					{
						if(GraphdBm == null)
						{
							GraphdBm = msg.CopydBmToFloatArray();
						}
						else
						{
							msg.CopydBmToFloatArray(GraphdBm);
						}
					}
					//------------------------------------
					// carica la perc. di campioni below threshold 100dBm
					PercBelowThreshold = msg.PercdBmBelowThreshold(-100.0f);
					
					boolean fUpdate = updateMaxdBm(msg);	// aggiorna il valore max dbm
					
					//---------------------------------
					// db sqlite:
					//---------------------------------
					// Aggiorna i dati nel db:
					
					// leggi la posizione attuale
					location_id = gps.readLocationSvDb(false);
					
					/***
					// 0808 disabilitato. Prima abilitato
					int[] dBm = msg.CopyMsgToIntArray();
					int len = dBm.length;
					***/
					long dBm_id = 0;
					if(db != null)
					{
						/*****
						// 04/08 portato fuori
						// row of max dBm
						float val;
						boolean fUpdate;
						
						val = RFEdBm.toFloat(msg.GlobalMax());		// max convertito in float
						fUpdate = false;
						if(MaxdBm < val)
						{
							// aggiorna i dBm
							MaxdBm = val;
							MaxdBmFreq = RFEfreq.FreqMhz(msg.GlobalIdxMax());
							fUpdate = true;
						}
						else if(MaxdBm == val)
						{
							// aggiorna la frequenza
							val = RFEfreq.FreqMhz(msg.GlobalIdxMax());
							if(val != MaxdBmFreq)
							{
								MaxdBmFreq = val;
								fUpdate = true;
							}
						}
						*****/
						
						if(fUpdate)
						{
							db.createMaxdBm(
								location_id,
								MaxdBmFreq,
								MaxdBm);
						}
						
						// row of dBm acquisition
						/***
						// originale disab. il 08/08
						dBm_id = db.createdBmData(
									location_id, 
									config_id, 
									RFEdBm.toInt(msg.Min()), 
									RFEdBm.toInt(msg.Max()), 
									RFEdBm.toInt(msg.Media()), 
									len, 
									dBm);
						***/
						dBm_id = db.createdBmData(
									location_id, 
									config_id, 
									msg);
					}
					//---------------------------------

					RxAll = true;
					// aggiunta 19/07, inserita coda di messaggi
				}
				initStateRxMsg(); 		// reinizializza la macchina a stati per il riconoscimento
				break;
			}	// end switch
		}
	}
	
	//----------------------------------------------------------------------------
	// la procedura riceve la sequenza di caratteri presenti dalla seriale
	// e li inserisce in un buffer circolare.
	// Viene controllato il flag RxAll: 
	// se e' true vuol dire che il buffer e' del messaggio e' ancora pieno 
	// perche' altre procedure stanno analizzando il messaggio stesso.
	// In questo caso, quindi la procedura si limita a riempire il buffer circolare.
	// Se RxAll e' false, procedi con l'analisi dei caratteri per formare il messaggio ricevuto
	//
	public boolean updateRx(byte[] data)
	{
		//------------------------------------
		// aggiorna il ring buffer con i dati appena ricevuti
		// ringBuffer.write(data, 0, data.length);
		//------------------------------------

		
		// while(ringBuffer.getUsed()>0)
		if(data.length>0)
		{
			DecodeRFEAcquisition(data);		// decodifica il messaggio
		}
		return(RxAll);
	}

	/////////////////////////////////////////////////////////////////////////////
	
	// restituisce true se ha ricevuto tutto il messaggio
	public boolean ChkRx()
	{
		return(RxAll);
	}

	// restituisce copia del messaggio letto
	public byte[]GetMsg()
	{
		// byte[] result = new byte[lenMsg];
		byte[] result = new byte[lenData];
		
		// arraycopy(Object src, int srcPos, Object dest, int destPos, int length)
		// originale: copia tutti i caratteri, compresi header e termine
		// System.arraycopy(msgRx, 0, result, 0, lenMsg);
		// copia soltanto la parte data
		System.arraycopy(msgRx, 3, result, 0, lenData);
		return(result);
	}
}			// end class RxRfMsg

// end public class RxRfMsg
