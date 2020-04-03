// coda FIFO per messaggi ricevuti dal RFExplorer
// vedere:
// http://opensourceforgeeks.blogspot.it/2013/12/fifo-based-queue-implementation-in-java.html

package com.ictp.mrainone.rftrack.rfexplorer;

import java.util.LinkedList;
import java.util.Queue;

import com.ictp.mrainone.rftrack.rfexplorer.RFEdata;

public class RFEdataFifo {

	Queue<RFEdata> myQueue;			// coda di dati provenienti da RFE

	// costruttore
	public RFEdataFifo()
	{
		myQueue = new LinkedList<RFEdata>();
	}
	
	// Returns true if this collection contains no elements.
	public boolean isEmpty()
	{
		return(myQueue.isEmpty());
	}
	
	// Inserts the specified element into this queue if it is possible 
	// to do so immediately without violating capacity restrictions, 
	// returning true upon success and throwing an llegalStateException 
	// if no space is currently available.
	boolean add(RFEdata e)
	{
		return(myQueue.add(e));
	}
	
	// Retrieves and removes the head of this queue, 
	// or returns null if this queue is empty.
	public RFEdata poll()
	{
		return(myQueue.poll());
	}
	
	// Retrieves, but does not remove, the head of this queue, 
	// or returns null if this queue is empty.
	public RFEdata peek()
	{
		return(myQueue.peek());
	}
}
