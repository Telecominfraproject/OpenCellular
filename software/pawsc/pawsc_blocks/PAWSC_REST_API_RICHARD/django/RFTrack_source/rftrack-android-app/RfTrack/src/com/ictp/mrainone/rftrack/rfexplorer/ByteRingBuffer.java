// Copyright 2014 Christian d'Heureuse, Inventec Informatik AG, Zurich, Switzerland
// www.source-code.biz, www.inventec.ch/chdh
//
// This module is multi-licensed and may be used under the terms
// of any of the following licenses:
//
//  EPL, Eclipse Public License, V1.0 or later, http://www.eclipse.org/legal
//  LGPL, GNU Lesser General Public License, V2.1 or later, http://www.gnu.org/licenses/lgpl.html
//
// Please contact the author if you need another license.
// This module is provided "as is", without warranties of any kind.

/**
* A ring buffer (circular buffer, FIFO) for bytes.
*
* <p>All methods of this class are non-blocking and not synchronized (not thread-safe).
*/

package com.ictp.mrainone.rftrack.rfexplorer;

// mr: la classe utilizza il metodo arraycopy
// http://www.tutorialspoint.com/java/lang/system_arraycopy.htm
// public static void arraycopy(Object src, int srcPos, Object dest, int destPos, int length)
// Parameters
//     src -- This is the source array.
//     srcPos -- This is the starting position in the source array.
//     dest -- This is the destination array.
//     destPos -- This is the starting position in the destination data.
//     length -- This is the number of array elements to be copied.
// Return Value
// This method does not return any value.
// Exception
//     IndexOutOfBoundsException -- if copying would cause access of data outside array bounds.
//     ArrayStoreException -- if an element in the src array could not be stored into the dest array because of a type mismatch.
//     NullPointerException -- if either src or dest is null.

public class ByteRingBuffer {

	private byte[]               rBuf;                         // ring buffer data
	private int                  rBufSize;                     // ring buffer size
	private int                  rBufPos;                      // position of first (oldest) data byte within the ring buffer
	private int                  rBufUsed;                     // number of used data bytes within the ring buffer

	/**
* Creates a ring buffer.
*/
	public ByteRingBuffer (int size) {
		if (size <= 0) {
			throw new IllegalArgumentException(); 
		}
		rBufSize = size;
		rBuf = new byte[rBufSize]; 
	}

	/**
* Resizes the ring buffer by preserving it's data.
*
* <p>If the new size is not enough to keep all used data in the ring buffer,
* the excess old data is discarded.
*/
	public void resize (int newSize) {
		if (newSize <= 0) {
			throw new IllegalArgumentException(); 
		}
		if (newSize < rBufUsed) {                               // if new buffer is too small to contain all data
			discard(rBufUsed - newSize);						// Discards the oldest len (rBufUsed - newSize) bytes within the ring buffer.
		}                       // discard oldest data
		// create new buffer and transfer data into it 
		byte[] newBuf = new byte[newSize];
		int newBufUsed = read(newBuf, 0, newSize);              // transfer data to new buffer
		rBuf = newBuf;

		rBufSize = newSize;
		rBufPos = 0;											// position of first (oldest) data byte within the ring buffer
		rBufUsed = newBufUsed; 									// number of used data bytes within the ring buffer
	}

	/**
* Returns the size of the ring buffer.
*/
	public int getSize() {
		return rBufSize;
	}

	/**
* Returns the number of free bytes within the ring buffer.
*/
	public int getFree() {
		return (rBufSize - rBufUsed); 
	}

	/**
* Returns the number of used bytes within the ring buffer.
*/
	public int getUsed() {
		return rBufUsed; 
	}

	/**
* Clears the ring buffer.
*/
	public void clear() {
		rBufPos = 0;							// position of first (oldest) data byte within the ring buffer
		rBufUsed = 0; 							// number of used data bytes within the ring buffer
	}

	/**
* Discards the oldest <code>len</code> bytes within the ring buffer.
* This has the same effect as calling <code>read(new byte[len], 0, len)</code>.
*
* @param len
*    The number of bytes to be discarded.
*/
	public void discard (int len) {
		if (len < 0) {
			throw new IllegalArgumentException(); 
		}
		int trLen = Math.min(len, rBufUsed);
		rBufPos = clip(rBufPos + trLen);		// position of first (oldest) data byte within the ring buffer
		rBufUsed -= trLen;						// number of used data bytes within the ring buffer
	}

	/**
* Writes data to the ring buffer.
*
* @return
*    The number of bytes written.
*    This is guaranteed to be <code>min(len, getFree())</code>.
*/
	public int write(byte[] buf, int pos, int len) {
		if (len < 0) {
			throw new IllegalArgumentException(); 
		}
		if (rBufUsed == 0) {
			rBufPos = 0; 						// position of first (oldest) data byte within the ring buffer
		}                                       // (speed optimization)
		int p1 = rBufPos + rBufUsed;
		if (p1 < rBufSize) {                                    // free space in two pieces
			int trLen1 = Math.min(len, rBufSize - p1);
			append(buf, pos, trLen1);
			int trLen2 = Math.min(len - trLen1, rBufPos);
			append(buf, pos + trLen1, trLen2);
			return (trLen1 + trLen2); 
		}
		else {                                                 // free space in one piece
			int trLen = Math.min(len, rBufSize - rBufUsed);
			append(buf, pos, trLen);
			return trLen; 
		}
	}

	/**
* Writes data to the ring buffer.
*
* <p>Convenience method for: <code>write(buf, 0, buf.length)</code>
*/
	public int write(byte[] buf) {
		return (write(buf, 0, buf.length)); 
	}

	// write new data in the circular buffer starting from the first free position: 
	// append data to the end of the last position occupied
	private void append(byte[] buf, int pos, int len) {
		if (len == 0) {
			return; 
		}
		if (len < 0) {
			throw new AssertionError(); 
		}
		int p = clip(rBufPos + rBufUsed);		// position of the first free element within the circular buffer
		
		// arraycopy(Object src, int srcPos, Object dest, int destPos, int length)
		System.arraycopy(buf, pos, rBuf, p, len);
		rBufUsed += len; 					// number of used data bytes within the ring buffer
	}

	/**
* Reads data from the ring buffer.
*
* @return
*    The number of bytes read.
*    This is guaranteed to be <code>min(len, getUsed())</code>.
*/
	public int read(byte[] buf, int pos, int len) {
		if (len < 0) {
			throw new IllegalArgumentException(); 
		}
		int trLen1 = Math.min(len, Math.min(rBufUsed, rBufSize - rBufPos));
		remove(buf, pos, trLen1);	// copia su buf, a partire da pos, un numero di byte pari a trLen1
		int trLen2 = Math.min(len - trLen1, rBufUsed);
		remove(buf, pos + trLen1, trLen2);		// copia su buf, a partire da (pos + trLen1), un numero di byte pari a trLen2
		return (trLen1 + trLen2); 
	}

	/**
* Reads data from the ring buffer.
*
* <p>Convenience method for: <code>read(buf, 0, buf.length)</code>
*/
	public int read(byte[] buf) {
		return (read(buf, 0, buf.length)); 
	}

	// sposta dal ring buffer data un numero di byte pari a len, caricandoli in buf a partire dalla posizione pos
	private void remove (byte[] buf, int pos, int len) {
		if (len == 0) {
			return; 
		}
		if (len < 0) {
			throw new AssertionError(); 
		}
		
		// arraycopy(Object src, int srcPos, Object dest, int destPos, int length)
		System.arraycopy(rBuf, rBufPos, buf, pos, len);
		rBufPos = clip(rBufPos + len);				// position of first (oldest) data byte within the ring buffer
		rBufUsed -= len; 							// number of used data bytes within the ring buffer
	}

	// Actual position of the element within the circular buffer. 
	// Returns the modulus of the division of p with the buffer size
	private int clip(int p) {
		return (p < rBufSize) ? p : (p - rBufSize); 
	}
}
