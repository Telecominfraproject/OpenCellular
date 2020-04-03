// record of table Campaign

package com.ictp.mrainone.rftrack.tblmodel;

public class RecConfig {

	int id;
	int start_freq;
	int end_freq;
	int amp_top;
	int amp_bottom;
	int nsteps;
	String created_at;

	// constructors
	public RecConfig() {
	}

	// setters
	public void setId(int id) {
		this.id = id;
	}
	public void setStartFreq(int start_freq) {
		this.start_freq = start_freq;
	}
	public void setEndFreq(int end_freq) {
		this.end_freq = end_freq;
	}
	public void setAmpTop(int amp_top) {
		this.amp_top = amp_top;
	}
	public void setAmpBottom(int amp_bottom) {
		this.amp_bottom = amp_bottom;
	}
	public void setNSteps(int nsteps) {
		this.nsteps = nsteps;
	}
	public void setCreatedAt(String created_at){
		this.created_at = created_at;
	}

	// getters
	public long getId() {
		return this.id;
	}
	public int getStartFreq() {
		return this.start_freq;
	}
	public int getEndFreq() {
		return this.end_freq;
	}
	public int getAmpTop() {
		return this.amp_top;
	}
	public int getAmpBottom() {
		return this.amp_bottom;
	}
	public int getNSteps() {
		return this.nsteps;
	}
	public String getCreatedAt(){
		return this.created_at;
	}
}
