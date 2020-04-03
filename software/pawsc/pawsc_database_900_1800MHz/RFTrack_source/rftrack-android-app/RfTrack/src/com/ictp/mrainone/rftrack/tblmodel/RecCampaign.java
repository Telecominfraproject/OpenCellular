// record of table Campaign

package com.ictp.mrainone.rftrack.tblmodel;

public class RecCampaign {

	int id;
	String name;
	String note;
	int antenna;
	int status;
	String created_at;

	// constructors
	public RecCampaign() {
	}

	public RecCampaign(String name, String note, int antenna, int status) {
		this.name = name;
		this.note = note;
		this.antenna = antenna;
		this.status = status;
	}

	public RecCampaign(int id, String name, String note, int antenna, int status) {
		this.id = id;
		this.name = name;
		this.note = note;
		this.antenna = antenna;
		this.status = status;
	}

	// setters
	public void setId(int id) {
		this.id = id;
	}

	public void setName(String name) {
		this.name = name;
	}

	public void setNote(String note) {
		this.note = note;
	}

	public void setAntenna(int antenna) {
		this.antenna = antenna;
	}
	
	public void setStatus(int status) {
		this.status = status;
	}
	
	public void setCreatedAt(String created_at){
		this.created_at = created_at;
	}

	// getters
	public long getId() {
		return this.id;
	}

	public String getName() {
		return this.name;
	}

	public String getNote() {
		return this.note;
	}

	public int getAntenna() {
		return this.antenna;
	}
	
	public int getStatus() {
		return this.status;
	}
}
