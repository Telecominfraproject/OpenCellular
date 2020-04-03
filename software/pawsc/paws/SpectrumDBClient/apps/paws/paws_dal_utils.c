/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


#include "utils/utils.h"
#include "json-parser/json_utils.h"

#include "paws_utils.h"
#include "paws_dal_utils.h"


// function prototypes (to write state)
static void db_discovery_2_json(char *k, paws_db_discovery_info_t* db_discovery, char* str_, uint32_t strsize_);
static void ul_dl_spec_2_json(char *k, ul_dl_spec_cfg_t* ul_dl_spec, char* str_, uint32_t strsize_);
static void gps_2_json(char *k, paws_gps_location_t* gps, char* str_, uint32_t strsize_);
static void paws_db_item_2_json(paws_db_item_t *db, char* str_, uint32_t strsize_);
static void profiles_2_str(char* k, spec_profile_type_t* profiles, char* str_, uint32_t strsize_);
static void spectrum_schedule_2_json(spectrum_schedule_t* sched, char* str_, uint32_t strsize_);
static void spectrum_spec_2_json(spectrum_spec_t* spec, char* str_, uint32_t strsize_);
static void spec_cfg_2_json(char *k, spec_cfg_t* cfg, char* str_, uint32_t strsize_);
static void avail_spectrum_2_json(char *k, avail_spectrum_t* avail_spec, char* str_, uint32_t strsize_);
static char* sm_state_info_2_json(sm_state_info_t* sm_state_info);

// function prototypes (to read state)
static bool json_2_db_discovery(json_value* db_disc_j, paws_db_discovery_info_t* db_disc);
static ul_dl_spec_cfg_t* json_2_ul_dl_spec(json_value* ul_dl_spec_j);
static bool json_2_gps(json_value* jval, paws_gps_location_t* gps);
static spec_profile_type_t* json_2_prof(json_value* prof_j);
static spectrum_schedule_t* json_2_spectrum_sched(json_value* spectrum_sched_j);
static spectrum_spec_t* json_2_spectrum_spec(json_value* spectrum_spec_j);
static spec_cfg_t* json_2_spec_cfg(json_value* jval);
static avail_spectrum_t* json_2_avail_spectrum(json_value* spectrum_specs_j);
static sm_state_info_t* json2_paws_sm_info(json_value* jval);




#define SM_STATE_INFO_FILE = "./state_info.json"

#define DATA_ESTIMATE_PER_DEVICE	(4000)
#define DATA_MAX_PER_DEVICE			(10000)

#define MAX_ENCODE_SM_LEN				((1/*combiner*/ + 1/*master*/ + 1/*gop*/ + MAX_SLAVE_DEVICES/*sops*/ ) * DATA_ESTIMATE_PER_DEVICE)
static char encode_str[MAX_ENCODE_SM_LEN];
static char encode_sm_str[DATA_MAX_PER_DEVICE];
static char* pstr_ = NULL;
static char tmp[250];

#define ENCODE_STRCAT(tmp, str_, strsize_)		{ uint32_t __slen = strlen(tmp);  if (__slen < (strsize_ - (pstr_ - str_))) { memcpy(pstr_, tmp, __slen); pstr_+=__slen; *pstr_='\0'; } }
#define KV_INT(tmp, k, v, str_, strsize_)		{ sprintf(tmp, "\"%s\" : %d",k,v); ENCODE_STRCAT(tmp, str_, strsize_); }
#define KV_FLOAT2(tmp, k, v, str_, strsize_)	{ sprintf(tmp, "\"%s\" : %.2f",k,v); ENCODE_STRCAT(tmp, str_, strsize_); }
#define KV_FLOAT6(tmp, k, v, str_, strsize_)	{ sprintf(tmp, "\"%s\" : %.6f",k,v); ENCODE_STRCAT(tmp, str_, strsize_); }
#define KV_BOOL(tmp, k, v, str_, strsize_)		{ if (v) {sprintf(tmp, "\"%s\" : true",k);} else {sprintf(tmp, "\"%s\" : false",k);} ENCODE_STRCAT(tmp, str_, strsize_); }
#define KV_STR(tmp, k, v, str_, strsize_)		{ if ((v) && (strlen(v) > 0)) { sprintf(tmp, "\"%s\" : \"%s\"",k,v); ENCODE_STRCAT(tmp, str_, strsize_); } }

#define ENCODE_START()			{ encode_str[0] = '\0';		pstr_ = encode_str; }
#define ENCODE_SM_START()		{ encode_sm_str[0] = '\0';  pstr_ = encode_sm_str; }



char* get_init_paws_dal_encode_str(void)
{
	ENCODE_START();
	return encode_str;
}


//#######################################################################################
void db_discovery_2_json(char *k, paws_db_discovery_info_t* db_discovery, char* str_, uint32_t strsize_)
{
	if ((!db_discovery) || (!db_discovery->wsbd_weblist) || (db_discovery->wsbd_weblist->num_items == 0))
		return;

	ENCODE_STRCAT(" \"", str_, strsize_);
	ENCODE_STRCAT(k, str_, strsize_);
	ENCODE_STRCAT("\":{", str_, strsize_);

	KV_INT(tmp, "db_discovery_duration", db_discovery->db_discovery_duration, str_, strsize_);	ENCODE_STRCAT(",", str_, strsize_);
	KV_INT(tmp, "refresh_rate", db_discovery->wsbd_weblist->refresh_rate, str_, strsize_);	ENCODE_STRCAT(",", str_, strsize_);

	ENCODE_STRCAT("\"items\":[", str_, strsize_);
	weblist_item_t* item=NULL;
	for (int i=0; i<db_discovery->wsbd_weblist->num_items; i++)
	{
		item = &db_discovery->wsbd_weblist->items[i];
		if (!item)
			continue;
		if (i > 0)
		{
			ENCODE_STRCAT(",", str_, strsize_);
		}
		ENCODE_STRCAT("{", str_, strsize_);
		KV_STR(tmp, "name", item->name, str_, strsize_);						ENCODE_STRCAT(",", str_, strsize_);
		KV_STR(tmp, "url", item->url, str_, strsize_);							ENCODE_STRCAT(",", str_, strsize_);
		KV_BOOL(tmp, "mcwsd_support", item->mcwsd_support, str_, strsize_);		ENCODE_STRCAT(",", str_, strsize_);
		KV_BOOL(tmp, "invalid", item->invalid, str_, strsize_);
		ENCODE_STRCAT("}", str_, strsize_);
	}
	ENCODE_STRCAT("]", str_, strsize_);
	ENCODE_STRCAT("}", str_, strsize_);
}



//#######################################################################################
void ul_dl_spec_2_json(char *k, ul_dl_spec_cfg_t* ul_dl_spec, char* str_, uint32_t strsize_)
{
	if ((!ul_dl_spec) || (!ul_dl_spec->dl_cfg) || (!ul_dl_spec->ul_cfg))
		return;

	ENCODE_STRCAT(" \"", str_, strsize_);
	ENCODE_STRCAT(k, str_, strsize_);
	ENCODE_STRCAT("\":{", str_, strsize_);

	spec_cfg_2_json("dl_cfg", ul_dl_spec->dl_cfg, str_, strsize_);
	ENCODE_STRCAT(",", str_, strsize_);
	spec_cfg_2_json("ul_cfg", ul_dl_spec->ul_cfg, str_, strsize_);

	ENCODE_STRCAT("}", str_, strsize_);
}


//#######################################################################################
void gps_2_json(char *k, paws_gps_location_t* gps, char* str_, uint32_t strsize_)
{
	ENCODE_STRCAT( " \"", str_, strsize_);
	ENCODE_STRCAT( k, str_, strsize_);
	ENCODE_STRCAT( "\":{", str_, strsize_);

	KV_BOOL(tmp, "fixed", gps->fixed, str_, strsize_);
	if (gps->fixed)
	{
		ENCODE_STRCAT( ",", str_, strsize_);
		KV_FLOAT6(tmp, "latitude", gps->latitude, str_, strsize_);			ENCODE_STRCAT( ",", str_, strsize_);
		KV_FLOAT6(tmp, "longitude", gps->longitude, str_, strsize_);		ENCODE_STRCAT( ",", str_, strsize_);
		KV_INT(tmp, "height", gps->height, str_, strsize_);					ENCODE_STRCAT( ",", str_, strsize_);
		KV_STR(tmp, "heightType", gps->height_type, str_, strsize_);
	}
	ENCODE_STRCAT( "}", str_, strsize_);
}


//#######################################################################################
void paws_db_item_2_json(paws_db_item_t *db, char* str_, uint32_t strsize_)
{
	ENCODE_STRCAT("{", str_, strsize_);

	KV_STR(tmp, "name", db->name, str_, strsize_);		ENCODE_STRCAT( ",", str_, strsize_);

	// db_url
	ENCODE_STRCAT( "\"db_url\" : { ", str_, strsize_);
	KV_STR(tmp, "host", db->db_url.host, str_, strsize_);	ENCODE_STRCAT( ",", str_, strsize_);
	KV_STR(tmp, "token", db->db_url.token, str_, strsize_);
	ENCODE_STRCAT( "},", str_, strsize_);

	KV_BOOL(tmp, "valid", db->valid, str_, strsize_);

	if (db->barred_utc > 0)
	{
		ENCODE_STRCAT( ",", str_, strsize_);
		ENCODE_STRCAT( "\"barred_utc\" : \"", str_, strsize_);
		strftime(tmp, sizeof(tmp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&db->barred_utc));
		ENCODE_STRCAT( tmp, str_, strsize_);
		ENCODE_STRCAT( "\"", str_, strsize_);
	}
	ENCODE_STRCAT( "}", str_, strsize_);
}




//#######################################################################################
static void profiles_2_str(char* k, spec_profile_type_t* profiles, char* str_, uint32_t strsize_)
{
	ENCODE_STRCAT( " \"", str_, strsize_);
	ENCODE_STRCAT( k, str_, strsize_);
	ENCODE_STRCAT( "\":[", str_, strsize_);

	spec_profile_type_t* prof;
	for (prof = profiles; prof; prof = prof->next)
	{
		ENCODE_STRCAT( "{", str_, strsize_);
		KV_INT(tmp, "start_hz", prof->start_hz, str_, strsize_);		ENCODE_STRCAT( ",", str_, strsize_);
		KV_INT(tmp, "end_hz", prof->end_hz, str_, strsize_);			ENCODE_STRCAT( ",", str_, strsize_);
		KV_FLOAT2(tmp, "dbm", prof->dbm, str_, strsize_);
		ENCODE_STRCAT( "}", str_, strsize_);
		if (prof->next)
			ENCODE_STRCAT( ",", str_, strsize_);
	}
	ENCODE_STRCAT( "]", str_, strsize_);
}


//#######################################################################################
static void spectrum_schedule_2_json(spectrum_schedule_t* sched, char* str_, uint32_t strsize_)
{
	if (sched)
	{
		ENCODE_STRCAT( "{", str_, strsize_);

		// event_time_range
		ENCODE_STRCAT( " \"event_time_range\" : {", str_, strsize_);
		ENCODE_STRCAT( "\"start_time\" : \"", str_, strsize_);
		strftime(tmp, sizeof(tmp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&sched->event_time_range.start_time));
		ENCODE_STRCAT( tmp, str_, strsize_);
		ENCODE_STRCAT( "\", ", str_, strsize_);
		ENCODE_STRCAT( "\"stop_time\" : \"", str_, strsize_);
		strftime(tmp, sizeof(tmp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&sched->event_time_range.stop_time));
		ENCODE_STRCAT( tmp, str_, strsize_);
		ENCODE_STRCAT( "\"},", str_, strsize_);

		// profiles
		profiles_2_str("profiles", sched->profiles, str_, strsize_);

		ENCODE_STRCAT( "}", str_, strsize_);
	}
}


//#######################################################################################
static void spectrum_spec_2_json(spectrum_spec_t* spec, char* str_, uint32_t strsize_)
{
	if (spec)
	{
		ENCODE_STRCAT( "{", str_, strsize_);
		if (spec->spectrum_schedules)
		{
			ENCODE_STRCAT( " \"spectrum_schedules\" : [", str_, strsize_);
			spectrum_schedule_t* sched;
			for (sched = spec->spectrum_schedules; sched; sched = sched->next)
			{
				spectrum_schedule_2_json(sched, str_, strsize_);
				if (sched->next)
					ENCODE_STRCAT( ",", str_, strsize_);
			}
			ENCODE_STRCAT( "],", str_, strsize_);
		}

		KV_FLOAT2(tmp, "max_contiguous_bw_hz", spec->max_contiguous_bw_hz, str_, strsize_);								ENCODE_STRCAT( ",", str_, strsize_);
		KV_FLOAT2(tmp, "max_contiguous_bw_hz_within_band", spec->max_contiguous_bw_hz_within_band, str_, strsize_);		ENCODE_STRCAT( ",", str_, strsize_);
		KV_BOOL(tmp, "needsSpectrumReport", spec->needsSpectrumReport, str_, strsize_);									ENCODE_STRCAT( ",", str_, strsize_);
		KV_INT(tmp, "maxPollingSecs", spec->ruleset_info.maxPollingSecs, str_, strsize_);								ENCODE_STRCAT( ",", str_, strsize_);
		KV_FLOAT2(tmp, "maxLocationChange", spec->ruleset_info.maxLocationChange, str_, strsize_);

		ENCODE_STRCAT( "}", str_, strsize_);
	}
}


//#######################################################################################
static void spec_cfg_2_json(char *k, spec_cfg_t* cfg, char* str_, uint32_t strsize_)
{
	if (cfg)
	{
		ENCODE_STRCAT( " \"", str_, strsize_);
		ENCODE_STRCAT( k, str_, strsize_);
		ENCODE_STRCAT( "\":{", str_, strsize_);

		if (cfg->spec)
		{
			{
				ENCODE_STRCAT( " \"spec\" : ", str_, strsize_);
				spectrum_spec_2_json(cfg->spec, str_, strsize_);
				ENCODE_STRCAT( ",", str_, strsize_);
			}
		}

		if (cfg->sched)
		{
			ENCODE_STRCAT( " \"sched\" : ", str_, strsize_);
			spectrum_schedule_2_json(cfg->sched, str_, strsize_);
			ENCODE_STRCAT( ",", str_, strsize_);
		}

		KV_INT(tmp, "start_hz", cfg->start_hz, str_, strsize_);
		ENCODE_STRCAT( ",", str_, strsize_);

		KV_FLOAT2(tmp, "dbm", cfg->dbm, str_, strsize_);
		ENCODE_STRCAT( ",", str_, strsize_);

		KV_INT(tmp, "bandwidth", cfg->bandwidth, str_, strsize_);

		ENCODE_STRCAT( "}", str_, strsize_);
	}
}


//#######################################################################################
static void avail_spectrum_2_json(char *k, avail_spectrum_t* avail_spec, char* str_, uint32_t strsize_)
{
	ENCODE_STRCAT( " \"", str_, strsize_);
	ENCODE_STRCAT( k, str_, strsize_);
	ENCODE_STRCAT( "\":[", str_, strsize_);

	spectrum_spec_t* spec;
	for (spec = avail_spec->spectrum_specs; spec; spec = spec->next)
	{
		spectrum_spec_2_json(spec, str_, strsize_);
		if (spec->next)
			ENCODE_STRCAT( ",", str_, strsize_);
	}
	ENCODE_STRCAT( "]", str_, strsize_);
}



//#######################################################################################
char* sm_state_info_2_json(sm_state_info_t* sm_state_info)
{
	ENCODE_SM_START();

	char* str_ = encode_sm_str;
	uint32_t strsize_ = sizeof(encode_sm_str);


	ENCODE_STRCAT( " \"", str_, strsize_);
	ENCODE_STRCAT( sm_state_info->unique_id, str_, strsize_);
	ENCODE_STRCAT( "\":{", str_, strsize_);

	KV_INT(tmp, "stl_current_state", sm_state_info->stl_current_state, str_, strsize_);

	if (sm_state_info->timer_info)
	{
		ENCODE_STRCAT( ",", str_, strsize_);
		KV_STR(tmp, "timer_info", sm_state_info->timer_info, str_, strsize_);
	}
	ENCODE_STRCAT( ",", str_, strsize_);
	KV_FLOAT2(tmp, "default_max_location_change", sm_state_info->default_max_location_change, str_, strsize_);
	ENCODE_STRCAT( ",", str_, strsize_);
	KV_INT(tmp, "default_max_polling_secs", sm_state_info->default_max_polling_secs, str_, strsize_);
	ENCODE_STRCAT( ",", str_, strsize_);
	KV_FLOAT2(tmp, "specific_max_location_change", sm_state_info->specific_max_location_change, str_, strsize_);
	ENCODE_STRCAT( ",", str_, strsize_);
	KV_INT(tmp, "specific_max_polling_secs", sm_state_info->specific_max_polling_secs, str_, strsize_);

	// gps
	ENCODE_STRCAT( ",", str_, strsize_);
	gps_2_json("gps", &sm_state_info->gps, str_, strsize_);

	// selected_db
	if (strlen(sm_state_info->selected_db.name) > 0)
	{
		ENCODE_STRCAT( ",", str_, strsize_);
		ENCODE_STRCAT( " \"selected_db\": ", str_, strsize_);
		paws_db_item_2_json(&sm_state_info->selected_db, str_, strsize_);
	}

	// avail_spectrum_resp;
	if (sm_state_info->avail_spectrum_resp)
	{
		ENCODE_STRCAT( ",", str_, strsize_);
		avail_spectrum_2_json("avail_spectrum_resp", sm_state_info->avail_spectrum_resp, str_, strsize_);
	}

	if (sm_state_info->available_spectrum)
	{
		ENCODE_STRCAT( ",", str_, strsize_);
		spec_cfg_2_json("available_spectrum", sm_state_info->available_spectrum, str_, strsize_);
	}
	if (sm_state_info->pending_spectrum)
	{
		ENCODE_STRCAT( ",", str_, strsize_);
		spec_cfg_2_json("pending_spectrum", sm_state_info->pending_spectrum, str_, strsize_);
	}
	if (sm_state_info->selected_spectrum)
	{
		ENCODE_STRCAT( ",", str_, strsize_);
		spec_cfg_2_json("selected_spectrum", sm_state_info->selected_spectrum, str_, strsize_);
	}
	if (sm_state_info->spectrum_in_use)
	{
		ENCODE_STRCAT( ",", str_, strsize_);
		spec_cfg_2_json("spectrum_in_use", sm_state_info->spectrum_in_use, str_, strsize_);
	}

	ENCODE_STRCAT( "}", str_, strsize_);

	// is there enough room in encode_str to add this SM
	uint32_t sm_len = strlen(encode_sm_str);
	if (sm_len >= sizeof(encode_sm_str))
	{
		// sm exceeded max sm size
		return NULL;
	}
	uint32_t remaining = (sizeof(encode_str) - strlen(encode_str)) - 5/* for ending '}' */;
	if (sm_len > remaining)
	{
		// it can't fit in
		return NULL;
	}
	return encode_sm_str;
}


//#######################################################################################
char* paws_sm_state_info_2_jsonstr(paws_sm_state_info_t* paws_sm_state_info)
{
	char* str_ = encode_str;
	uint32_t strsize_ = sizeof(encode_str);

	// we want to append to encode_str, so set pointer to end
	pstr_ = encode_str + strlen(encode_str);
	
	ENCODE_STRCAT("{", str_, strsize_);

	KV_INT(tmp, "dl_spec_state", paws_sm_state_info->dl_spec_state, str_, strsize_);
	ENCODE_STRCAT(",", str_, strsize_);
	KV_INT(tmp, "ul_spec_state", paws_sm_state_info->ul_spec_state, str_, strsize_);
	ENCODE_STRCAT(",", str_, strsize_);
	db_discovery_2_json("db_discovery", &paws_sm_state_info->db_discovery_info, str_, strsize_);

	if ((paws_sm_state_info->ul_dl_spec->dl_cfg) && (paws_sm_state_info->ul_dl_spec->dl_cfg))
	{
		ENCODE_STRCAT(",", str_, strsize_);
		ul_dl_spec_2_json("ul_dl_spec", paws_sm_state_info->ul_dl_spec, str_, strsize_);
	}

	// walk the list
	sm_state_info_t* head = llist_get_head(paws_sm_state_info->state_info_ll);
	sm_state_info_t* sm_state = NULL;
	for (sm_state = head; sm_state != NULL; sm_state = get_next_item_entry(sm_state->l_item))
	{
		char *old_pstr_ = pstr_;			// pstr_ is changed in sm_state_info_2_json, so take a copy here and reset it after the call
		char* sm_str = sm_state_info_2_json(sm_state);
		pstr_ = old_pstr_;
		if (sm_str)
		{
			ENCODE_STRCAT(",", str_, strsize_);
			ENCODE_STRCAT(sm_str, str_, strsize_);
		}
	}

	ENCODE_STRCAT("}", str_, strsize_);

	return encode_str;
}


//#######################################################################################
static bool json_2_db_discovery(json_value* db_disc_j, paws_db_discovery_info_t* db_disc)
{
	int64_t i64;

	if ((!db_disc_j) || (!db_disc))
		return false;

	memset(db_disc, 0, sizeof(paws_db_discovery_info_t));

	// db_discovery_duration
	if (!(json_get_int(db_disc_j, "db_discovery_duration", &i64)))
		goto error_hdl;
	db_disc->db_discovery_duration = (int)i64;

	// wsdb_weblist
	json_value* wsbd_weblist_j = NULL;
	if (!((wsbd_weblist_j = json_get(db_disc_j, "items")) && (wsbd_weblist_j->type == json_array) && (wsbd_weblist_j->u.array.length > 0)))
		goto error_hdl;

	// create weblist
	if (!(db_disc->wsbd_weblist = malloc(sizeof(paws_weblist_t))))
		goto error_hdl;
	memset(db_disc->wsbd_weblist, 0, (sizeof(paws_weblist_t)));

	// refresh rate
	if (!(json_get_int(db_disc_j, "refresh_rate", &i64)))
		goto error_hdl;
	db_disc->wsbd_weblist->refresh_rate = (int)i64;

	// create weblist items array
	if (!(db_disc->wsbd_weblist->items = malloc((sizeof(weblist_item_t) * wsbd_weblist_j->u.array.length))))
		goto error_hdl;
	memset(db_disc->wsbd_weblist->items, 0, (sizeof(weblist_item_t) * wsbd_weblist_j->u.array.length));

	// loop through and populate each entry
	for (uint32_t i = 0; i < wsbd_weblist_j->u.array.length; i++)
	{
		weblist_item_t* item = &db_disc->wsbd_weblist->items[i];
		json_value* item_j = wsbd_weblist_j->u.array.values[i];
		if (item_j->type != json_object)
			goto error_hdl;

		// name
		char *tmp;
		if (!(tmp = json_get_string(item_j, "name")))
			goto error_hdl;
		strncpy(item->name, tmp, sizeof(item->name));

		// url
		if (!(tmp = json_get_string(item_j, "url")))
			goto error_hdl;
		strncpy(item->url, tmp, sizeof(item->url));

		// mcwsd_support
		if (!(json_get_bool(item_j, "mcwsd_support", &item->mcwsd_support)))
		{
			goto error_hdl;
		}

		// invalid
		json_get_bool(item_j, "invalid", &item->invalid);

		db_disc->wsbd_weblist->num_items++;
	}

	return true;

error_hdl:
	if (db_disc->wsbd_weblist->items) free(db_disc->wsbd_weblist->items);
	if (db_disc->wsbd_weblist) free(db_disc->wsbd_weblist);
	return false;

}


//#######################################################################################
static ul_dl_spec_cfg_t* json_2_ul_dl_spec(json_value* ul_dl_spec_j)
{
	ul_dl_spec_cfg_t* ul_dl_spec = ul_dl_spec_cfg_new();
	if (!ul_dl_spec)
	{
		goto error_hdl;
	}

	// dl_cfg
	json_value* spec_cfg_j = NULL;
	if ((spec_cfg_j = json_get(ul_dl_spec_j, "dl_cfg")))
		if (!(ul_dl_spec->dl_cfg = json_2_spec_cfg(spec_cfg_j)))
			goto error_hdl;

	// ul_cfg
	spec_cfg_j = NULL;
	if ((spec_cfg_j = json_get(ul_dl_spec_j, "ul_cfg")))
		if (!(ul_dl_spec->ul_cfg = json_2_spec_cfg(spec_cfg_j)))
			goto error_hdl;

	return ul_dl_spec;

error_hdl:
	if (ul_dl_spec) ul_dl_spec_cfg_free(&ul_dl_spec);
	return NULL;
}




//#######################################################################################
static bool json_2_gps(json_value* jval, paws_gps_location_t* gps)
{
	if (!(json_get_bool(jval, "fixed", &gps->fixed)))
		return false;

	if (gps->fixed)
	{
		int64_t i64;
		char *tmp = NULL;

		// latitude
		double latitude;
		if (!(json_get_double(jval, "latitude", (double*)&latitude)))
		{
			return false;
		}
		gps->latitude = latitude;

		// longitude
		double longitude;
		if (!(json_get_double(jval, "longitude", (double*)&longitude)))
		{
			return false;
		}
		gps->longitude = longitude;

		if (!(json_get_int(jval, "height", &i64)))
			return false;
		gps->height = (int)i64;

		if (!(tmp = json_get_string(jval, "heightType")))
			return false;
		strncpy(gps->height_type, tmp, sizeof(gps->height_type));

	}
	return true;
}


//#######################################################################################
bool json_2_paws_db_item(json_value* jval, paws_db_item_t* db_item)
{
	char *tmp = NULL;

	// name 
	if (!(tmp = json_get_string(jval, "name")))
		return false;
	strncpy(db_item->name, tmp, sizeof(db_item->name));

	// db_url
	// db_url - host
	if (!(tmp = json_get_string(jval, "db_url/host")))
		return false;
	strncpy(db_item->db_url.host, tmp, sizeof(db_item->db_url.host));
	// db_url - token
	if (!(tmp = json_get_string(jval, "db_url/token")))
		return false;
	strncpy(db_item->db_url.token, tmp, sizeof(db_item->db_url.token));

	// valid
	bool valid;
	if (!(json_get_bool(jval, "valid", &valid)))
		return false;
	db_item->valid = (valid) ? 1 : 0;

	// barred_utc
	if ((tmp = json_get_string(jval, "barred_utc")))
	{
		// store times as time_t
		if ((db_item->barred_utc = timestamp_to_timet(tmp)) == -1)
			return false;
	}

	return true;
}


//#######################################################################################
static spec_profile_type_t* json_2_prof(json_value* prof_j)
{
	spec_profile_type_t* prof = NULL;
	if (prof_j)
	{
		int64_t i64;

		if (!(prof = malloc(sizeof(spec_profile_type_t))))
			goto error_hdl;
		memset(prof, 0, sizeof(spec_profile_type_t));

		// start_hz
		if (!(json_get_int(prof_j, "start_hz", &i64)))
			goto error_hdl;
		prof->start_hz = (int32_t)i64;

		// end_hz
		if (!(json_get_int(prof_j, "end_hz", &i64)))
			goto error_hdl;
		prof->end_hz = (int32_t)i64;

		// dbm
		double dbm;
		if (!(json_get_double(prof_j, "dbm", (double*)&dbm)))
		{
			goto error_hdl;
		}
		prof->dbm = dbm;
	}
	return prof;

error_hdl:
	if (prof) free(prof);
	return NULL;
  
}


//#######################################################################################
static spectrum_schedule_t* json_2_spectrum_sched(json_value* spectrum_sched_j)
{
	spectrum_schedule_t* spectrum_sched = NULL;

	if ((spectrum_sched_j) && (spectrum_sched_j->type == json_object))
	{
		if (!(spectrum_sched = spectrum_sched_new()))
			goto error_hdl;

		// profiles
		json_value* profiles_j = NULL;
		if (!((profiles_j = json_get(spectrum_sched_j, "profiles")) && (profiles_j->type == json_array) && (profiles_j->u.array.length > 0)))
			goto error_hdl;

		spec_profile_type_t* prof = NULL;
		spec_profile_type_t* prev_prof = NULL;

		for (uint32_t i = 0; i < profiles_j->u.array.length; i++)
		{
			json_value* prof_j = profiles_j->u.array.values[i];
			if (prof_j->type != json_object)
				goto error_hdl;

			if (!(prof = json_2_prof(prof_j)))
				goto error_hdl;

			// add to linked list
			if (prev_prof)
				prev_prof->next = prof;
			if (!(spectrum_sched->profiles))
				spectrum_sched->profiles = prof;
			prev_prof = prof;
		}

		// event_time_range
		json_value* event_time_range_j = NULL;
		char* start_time = NULL;
		char* stop_time = NULL;
		if (!((event_time_range_j = json_get(spectrum_sched_j, "event_time_range")) &&
			  (start_time = json_get_string(event_time_range_j, "start_time")) &&
			  (stop_time = json_get_string(event_time_range_j, "stop_time"))))
		{
			goto error_hdl;
		}
		// store times as time_t
		if ((spectrum_sched->event_time_range.start_time = timestamp_to_timet(start_time)) == -1)
			goto error_hdl;
		if ((spectrum_sched->event_time_range.stop_time = timestamp_to_timet(stop_time)) == -1)
			goto error_hdl;

	}

	return spectrum_sched;

error_hdl:
	if (spectrum_sched) spectrum_sched_free(&spectrum_sched);
	return NULL;
}


//#######################################################################################
static spectrum_spec_t* json_2_spectrum_spec(json_value* spectrum_spec_j)
{
	spectrum_spec_t* spectrum_spec = NULL;
	if ((spectrum_spec_j) && (spectrum_spec_j->type == json_object))
	{
		if (!(spectrum_spec = spectrum_spec_new()))
			goto error_hdl;

		// spectrum schedules
		json_value* spectrum_schedules_j = NULL;
		if (!((spectrum_schedules_j = json_get(spectrum_spec_j, "spectrum_schedules")) && (spectrum_schedules_j->type = json_array) && (spectrum_schedules_j->u.array.length> 0)))
			goto error_hdl;

		spectrum_schedule_t* spectrum_sched = NULL;
		spectrum_schedule_t* prev_spectrum_sched = NULL;

		for (uint32_t i = 0; i < spectrum_schedules_j->u.array.length; i++)
		{
			json_value* spectrum_sched_j = spectrum_schedules_j->u.array.values[i];
			if (spectrum_sched_j->type != json_object)
				goto error_hdl;

			if (!(spectrum_sched = json_2_spectrum_sched(spectrum_sched_j)))
				goto error_hdl;

			// add to linked list
			if (prev_spectrum_sched)
				prev_spectrum_sched->next = spectrum_sched;
			if (!(spectrum_spec->spectrum_schedules))
				spectrum_spec->spectrum_schedules = spectrum_sched;
			prev_spectrum_sched = spectrum_sched;
		}

		// max_contiguous_bw_hz
		double max_contiguous_bw_hz;
		if (!(json_get_double(spectrum_spec_j, "max_contiguous_bw_hz", (double*)&max_contiguous_bw_hz)))
		{
			goto error_hdl;
		}
		spectrum_spec->max_contiguous_bw_hz = max_contiguous_bw_hz;

		// max_contiguous_bw_hz_within_band
		double max_contiguous_bw_hz_within_band;
		if (!(json_get_double(spectrum_spec_j, "max_contiguous_bw_hz_within_band", (double*)&max_contiguous_bw_hz_within_band)))
		{
			goto error_hdl;
		}
		spectrum_spec->max_contiguous_bw_hz_within_band = max_contiguous_bw_hz_within_band;

		// needsSpectrumReport
		if (!(json_get_bool(spectrum_spec_j, "needsSpectrumReport", &spectrum_spec->needsSpectrumReport)))
			goto error_hdl;

		// ruleset_info
		int64_t i64;
		if (!(json_get_int(spectrum_spec_j, "maxPollingSecs", &i64)))
			goto error_hdl;
		spectrum_spec->ruleset_info.maxPollingSecs = (int32_t)i64;

		double maxLocationChange;
		if (!(json_get_double(spectrum_spec_j, "maxLocationChange", (double*)&maxLocationChange)))
		{
			goto error_hdl;
		}
		spectrum_spec->ruleset_info.maxLocationChange = maxLocationChange;
	}

	return spectrum_spec;

error_hdl:
	if (spectrum_spec) spectrum_spec_free(&spectrum_spec);
	return NULL;
}


//#######################################################################################
static spec_cfg_t* json_2_spec_cfg(json_value* jval)
{
	spec_cfg_t* spec_cfg = NULL;

	if (jval)
	{
		int64_t i64;

		if (!(spec_cfg = spec_cfg_new()))
			goto error_hdl;
	
		// spec
		json_value* spec_j = NULL;
		if ((spec_j = json_get(jval, "spec")))
			if (!(spec_cfg->spec = json_2_spectrum_spec(spec_j)))
				goto error_hdl;

		// sched
		json_value* sched_j = NULL;
		if ((sched_j = json_get(jval, "sched")))
			if (!(spec_cfg->sched = json_2_spectrum_sched(sched_j)))
				goto error_hdl;

		// start_hz
		if (!(json_get_int(jval, "start_hz", &i64)))
			goto error_hdl;
		spec_cfg->start_hz = (int32_t)i64;

		// dbm
		double dbm;
		if (!(json_get_double(jval, "dbm", (double*)&dbm)))
		{
			goto error_hdl;
		}
		spec_cfg->dbm = dbm;

		// bandwidth
		if (!(json_get_int(jval, "bandwidth", &i64)))
			goto error_hdl;
		spec_cfg->bandwidth = (int32_t)i64;

	}

	return spec_cfg;

error_hdl:
	if (spec_cfg) spec_cfg_free(&spec_cfg);
	return NULL;
}


//#######################################################################################
static avail_spectrum_t* json_2_avail_spectrum(json_value* spectrum_specs_j)
{
	avail_spectrum_t* avail_spectrum = NULL;
	if (spectrum_specs_j)
	{
		if (!((spectrum_specs_j->type == json_array) && (spectrum_specs_j->u.array.length > 0)))
			goto error_hdl;

		if (!(avail_spectrum = avail_spectrum_new()))
			goto error_hdl;

		spectrum_spec_t* spectrum_spec = NULL; 
		spectrum_spec_t* prev_spectrum_spec = NULL;

		for (uint32_t i = 0; i < spectrum_specs_j->u.array.length; i++)
		{
			json_value* spectrum_spec_j = spectrum_specs_j->u.array.values[i];
			if (spectrum_spec_j->type != json_object)
				goto error_hdl;

			if (!(spectrum_spec = json_2_spectrum_spec(spectrum_spec_j)))
				goto error_hdl;

			// add to linked list
			if (prev_spectrum_spec)
				prev_spectrum_spec->next = spectrum_spec;
			if (!(avail_spectrum->spectrum_specs))
				avail_spectrum->spectrum_specs = spectrum_spec;
			prev_spectrum_spec = spectrum_spec;
		}
	}
	return avail_spectrum;

error_hdl:
	if (avail_spectrum) avail_spectrum_free(&avail_spectrum);
	return NULL;
}


//#######################################################################################
static sm_state_info_t* json2_paws_sm_info(json_value* jval)
{
	sm_state_info_t* sm_state_info = NULL;
		
	if (jval)
	{
		int64_t i64;

		if (!(sm_state_info = sm_state_info_new()))
			goto error_hdl;

		// stl_current_state
		if (!(json_get_int(jval, "stl_current_state", &i64)))
			goto error_hdl;
		sm_state_info->stl_current_state = (int)i64;

		// timer_info
		char *timer_info_str;
		if ((timer_info_str = json_get_string(jval, "timer_info")))
		{
			// there is something, so create memory for string and copy it
			if (!(sm_state_info->timer_info = malloc(strlen(timer_info_str)+1)))
				goto error_hdl;
			strcpy(sm_state_info->timer_info, timer_info_str);
		}

		// default_max_location_change
		double default_max_location_change;
		if (!(json_get_double(jval, "default_max_location_change", (double*)&default_max_location_change)))
		{
			goto error_hdl;
		}
		sm_state_info->default_max_location_change = default_max_location_change;

		// default_max_polling_secs
		if (!(json_get_int(jval, "default_max_polling_secs", &i64)))
			goto error_hdl;
		sm_state_info->default_max_polling_secs = (int32_t)i64;

		// specific_max_location_change
		double specific_max_location_change;
		if (!(json_get_double(jval, "specific_max_location_change", (double*)&specific_max_location_change)))
		{
			goto error_hdl;
		}
		sm_state_info->specific_max_location_change = specific_max_location_change;

		// specific_max_polling_secs
		if (!(json_get_int(jval, "specific_max_polling_secs", &i64)))
			goto error_hdl;
		sm_state_info->specific_max_polling_secs = (int32_t)i64;

		// gps
		json_value* gps_j = NULL;
		if (!(gps_j = json_get(jval, "gps")))
			goto error_hdl;
		if (!(json_2_gps(gps_j, &sm_state_info->gps)))
			goto error_hdl;

		// selected_db
		json_value* selected_db_j = NULL;
		if ((selected_db_j = json_get(jval, "selected_db")))
		{
			if (!(json_2_paws_db_item(selected_db_j, &sm_state_info->selected_db)))
				goto error_hdl;
		}

		// avail_spectrum_resp
		json_value* avail_spec_j = NULL;
		if ((avail_spec_j = json_get(jval, "avail_spectrum_resp")))
			if (!(sm_state_info->avail_spectrum_resp = json_2_avail_spectrum(avail_spec_j)))
				goto error_hdl;

		// available_spectrum
		json_value* spec_cfg_j = NULL;
		if ((spec_cfg_j = json_get(jval, "available_spectrum")))
			if (!(sm_state_info->available_spectrum = json_2_spec_cfg(spec_cfg_j)))
				goto error_hdl;

		// pending_spectrum
		spec_cfg_j = NULL;
		if ((spec_cfg_j = json_get(jval, "pending_spectrum")))
			if (!(sm_state_info->pending_spectrum = json_2_spec_cfg(spec_cfg_j)))
				goto error_hdl;

		// selected_spectrum
		spec_cfg_j = NULL;
		if ((spec_cfg_j = json_get(jval, "selected_spectrum")))
			if (!(sm_state_info->selected_spectrum = json_2_spec_cfg(spec_cfg_j)))
				goto error_hdl;

		// spectrum_in_use
		spec_cfg_j = NULL;
		if ((spec_cfg_j = json_get(jval, "spectrum_in_use")))
			if (!(sm_state_info->spectrum_in_use = json_2_spec_cfg(spec_cfg_j)))
				goto error_hdl;
	}

	return sm_state_info;

error_hdl:
	if (sm_state_info) sm_state_info_free_and_null(&sm_state_info);
	return NULL;
}


//#######################################################################################
paws_sm_state_info_t* json2_paws_sm_state_info(json_value* jval)
{
	paws_sm_state_info_t* paws_sm_state_info = NULL;
	sm_state_info_t* sm_state_info = NULL;

	if (jval)
	{
		int64_t i64;
		// malloc new type
		if (!(paws_sm_state_info = paws_sm_state_info_new()))
			return NULL;

		if (!(paws_sm_state_info->state_info_ll = llist_new()))
			goto error_hdl;

		// get the mandatory stuff first
		if (!(json_get_int(jval, "dl_spec_state", &i64)))
			goto error_hdl;
		paws_sm_state_info->dl_spec_state = (paws_spectrum_state_e)i64;

		if (!(json_get_int(jval, "ul_spec_state", &i64)))
			goto error_hdl;
		paws_sm_state_info->ul_spec_state = (paws_spectrum_state_e)i64;

		// loop through keys, and handle accordingly
		int num_keys = json_get_num_keys(jval);
		for (int i = 0; i < num_keys; i++)
		{
			json_value* sm_info_j = NULL;
			char* key = json_get_key(jval, (unsigned int)i);

			if (strcmp(key, "dl_spec_state") == 0)
				continue;
			else if (strcmp(key, "ul_spec_state") == 0)
				continue;
			else if (strcmp(key, "db_discovery") == 0)
			{
				// db_discovery
				json_value* db_discovery_j = NULL;
				if ((db_discovery_j = json_get(jval, "db_discovery")))
					if (!(json_2_db_discovery(db_discovery_j, &paws_sm_state_info->db_discovery_info)))
						goto error_hdl;
			}
			else if (strcmp(key, "ul_dl_spec") == 0)
			{
				// ul_dl_spec
				json_value* ul_dl_spec_j = NULL;
				if ((ul_dl_spec_j = json_get(jval, "ul_dl_spec")))
					if (!(paws_sm_state_info->ul_dl_spec = json_2_ul_dl_spec(ul_dl_spec_j)))
						goto error_hdl;
			}
			else // its either combiner, master, gop, or a sop
			{
				sm_info_j = NULL;
				if ((sm_info_j = json_get(jval, key)))
					if (!(sm_state_info = json2_paws_sm_info(sm_info_j)))
						goto error_hdl;
				snprintf(sm_state_info->unique_id, MAX_DEVICE_NAME_LEN, "%s", key);
				if (!(sm_state_info->l_item = llist_append(paws_sm_state_info->state_info_ll, sm_state_info)))
					goto error_hdl;
				sm_state_info = NULL;
			}
		}
	}

	return paws_sm_state_info;

error_hdl:
	if (sm_state_info) sm_state_info_free(sm_state_info);
	if (paws_sm_state_info) paws_sm_state_info_free(&paws_sm_state_info);
	return NULL;

}






