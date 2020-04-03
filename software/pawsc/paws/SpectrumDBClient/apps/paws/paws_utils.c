/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "utils/utils.h"
#include "json-parser/json_utils.h"

#include "paws_utils.h"

#include "paws_globals.h"
#include "paws_types.h"
#include "paws_dal_types.h"
#include "lte_utils.h"



char logline_[LOGGER_MAX_LENGTH];

#define DBM_NO_POWER	(-999)



//#######################################################################################
static void set_stop_time(time_t* stop_time_t, time_t now_, uint32_t maxPollingSecs)
{
	time_t forced_stop_t = now_ + maxPollingSecs;
	if (g_paws_reg_domain == PAWS_REGDOMAIN_ETSI)					
	{
		if (*stop_time_t > forced_stop_t)					// ETSI 301 598 	4.2.6.3.2
		{
			*stop_time_t = forced_stop_t;
		}
	}
}



//#######################################################################################
void weblist_free(paws_weblist_t** weblist)
{
	if ((weblist) && (*weblist))
	{
		if ((*weblist)->items)
		{
			free_and_null((void*)&(*weblist)->items);
		}
		free_and_null((void**)weblist);
	}
}



//#######################################################################################
paws_weblist_t* json_2_weblist(json_value* src)
{
	paws_weblist_t* weblist = NULL;
	
	// check src
	if (!src)
		goto error_hdl;

	// malloc a new type
	if (!(weblist = malloc(sizeof(paws_weblist_t))))
		goto error_hdl;
	memset(weblist, 0, (sizeof(paws_weblist_t)));

	// get refresh rate
	char* s = NULL;
	weblist->refresh_rate = ((s = json_get_string(src, "ws_databases/refresh_rate"))) ? atoi(s) : 0;

	// now fill in the rest
	json_value* v = json_get(src, "ws_databases/db");
	if (v && v->type == json_array)
	{
		weblist->num_items = v->u.array.length;
		if (weblist->num_items)
		{
			if (!(weblist->items = malloc((sizeof(weblist_item_t) * weblist->num_items))))
				goto error_hdl;
			memset(weblist->items, 0, (sizeof(weblist_item_t) * weblist->num_items));

			weblist_item_t* item = NULL;
			for (int i=0; i<weblist->num_items; i++)
			{
				char *ss = NULL;
				int slen;
				json_value* d = v->u.array.values[i];
				item = &(weblist->items[i]);
				if ((!d) || (d->type!=json_object))
   					goto error_hdl;
				
				// url
				if (!(ss = json_get_string(d, "url")))
					goto error_hdl;
				slen = snprintf(item->url, MAX_URL_LENGTH, "%s", ss);
				if ((slen < 0) || (slen >= MAX_URL_LENGTH))
					goto error_hdl;
				
				// db_provider_name
				if (!(ss = json_get_string(d, "db_provider_name")))
					goto error_hdl;
				slen = snprintf(item->name, MAX_DB_NAME_LENGTH, "%s", ss);
				if ((slen < 0) || (slen >= MAX_DB_NAME_LENGTH))
					goto error_hdl;

				// mcwsd_support
				if (!(json_get_bool(d, "MCWSD_support", &item->mcwsd_support)))
					goto error_hdl;
			}
		}
	}
	return weblist;

error_hdl:
	if (weblist) weblist_free(&weblist);
	return NULL;
}




//#######################################################################################
// convert timestamp to time_t
// returns -1 on failure 
int timestamp_to_timet(char* src)
{
	struct tm tm;
	memset(&tm, 0, sizeof(tm));

	if (!src)
		return -1;

	// remove micosecs
	char *pos = strrchr(src, '.');
	if (pos) 
		*pos = '\0';

	// convert to tm
	if (!(strptime(src, "%Y-%m-%dT%H:%M:%S", &tm)))
		return -1;
	else
		return (mktime(&tm));
}


//#######################################################################################
avail_spectrum_t* json_resp_2_avail_spectrum(void *sm_, json_value* src, lte_direction_e dir, uint8_t lte_band_id, time_t resp_time, float min_dbm_100k)
{
	uint32_t i, j, k, m, n;
	uint32_t band_start_hz, band_end_hz;
	int32_t band_spacing;
	char *timestamp_str;
	avail_spectrum_t* avail_spec = NULL;
	spectrum_spec_t* spectrum_spec = NULL;
	spectrum_schedule_t* spectrum_sched = NULL;
	spec_profile_type_t* profile = NULL;
	int contiguous;

	if (!(get_lte_band_channel(lte_band_id, dir, &band_start_hz, &band_end_hz, &band_spacing)))
		return NULL;

	// calulate the wsdb time phase using the timesdtamp in the message and our local response time
	// get the timestamp
	if (!(timestamp_str = json_get_string(src, "result/timestamp")))
		goto error_hdl;
	// store timestamp as time_t
	time_t db_timestamp, db_phase;
	if ((db_timestamp = timestamp_to_timet(timestamp_str)) == -1)
		goto error_hdl;
	db_phase = db_timestamp - resp_time;

	// malloc the main struct
	if (!(avail_spec = avail_spectrum_new()))
		goto error_hdl;
	spectrum_spec_t* tail_spectrum_spec = NULL;

	json_value* spectrum_specs_j=NULL;
	if (!(spectrum_specs_j = json_get(src, "result/spectrumSpecs")))
		goto error_hdl;

	if (spectrum_specs_j->type != json_array)
		goto error_hdl;

	// for each spectrum spec
	for (i = 0; i < spectrum_specs_j->u.array.length; i++)
	{
		if (!(spectrum_spec = spectrum_spec_new()))
			goto error_hdl;

		spectrum_schedule_t* tail_spectrum_sched = NULL;

		json_value* spectrum_spec_j = spectrum_specs_j->u.array.values[i];
		if (spectrum_spec_j->type != json_object)
			goto error_hdl;

		json_value* spectra_schedules_j = NULL;
		if (!(spectra_schedules_j = json_get(spectrum_spec_j, "spectrumSchedules")))
			continue;
		if (spectra_schedules_j->type != json_array)
			goto error_hdl;

		// ruleset info
		// we get this first because, for ETSI regulation domain, we limit the expiry time to the time of the tUpdate (max_polling_secs)
		json_value* ruleset_j = NULL;
		int64_t maxPollingSecs;
		double maxLocationChange;
		if (!(((ruleset_j = json_get(spectrum_spec_j, "rulesetInfo"))) &&
			((json_get_int(ruleset_j, "maxPollingSecs", &maxPollingSecs))) &&
			((json_get_double(ruleset_j, "maxLocationChange", (double*)&maxLocationChange)))))
		{
			goto error_hdl;
		}
		spectrum_spec->ruleset_info.maxPollingSecs = maxPollingSecs;
		spectrum_spec->ruleset_info.maxLocationChange = (float)maxLocationChange;

		spectrum_spec->max_contiguous_bw_hz_within_band = 0;

		for (j = 0; j < spectra_schedules_j->u.array.length; j++)
		{
			json_value* spectrum_sched_j = spectra_schedules_j->u.array.values[i];
			if (spectrum_sched_j->type != json_object)
				goto error_hdl;

			// get times and check if this spectrum has already expired
			// get an event time if present
			json_value* event_time_range_j = NULL;
			char* start_time = NULL;
			char* stop_time = NULL;
			if (!(((event_time_range_j = json_get(spectrum_sched_j, "eventTime"))) &&
				((start_time = json_get_string(event_time_range_j, "startTime"))) &&
				((stop_time = json_get_string(event_time_range_j, "stopTime")))))
			{
				goto error_hdl;
			}
			// store times as time_t
			time_t start_time_t, stop_time_t;
			if ((start_time_t = timestamp_to_timet(start_time)) == -1)
					goto error_hdl;
			start_time_t -= db_phase;
			if ((stop_time_t = timestamp_to_timet(stop_time)) == -1)
				goto error_hdl;
			stop_time_t -= db_phase;

			time_t now_ = time(NULL);

			// limit start/stop times if regulation body forces it
			set_stop_time(&stop_time_t, now_, spectrum_spec->ruleset_info.maxPollingSecs);

			// if start time > stop_time it is invalid.   This could happen if domain has limited stop time
			if (start_time_t > stop_time_t)
			{
				continue;
			}
			// if start_time is in the past, set it to 'now'
			if (now_ > start_time_t)
				start_time_t = now_;

			// if stop time is already expired, drop this spectrum_sched
			if (now_ > stop_time_t)
			{
				continue;
			}

			if(!(spectrum_sched = spectrum_sched_new()))
				goto error_hdl;

			spec_profile_type_t* tail_profile = NULL;

			// populate the times
			spectrum_sched->event_time_range.start_time = start_time_t;
			spectrum_sched->event_time_range.stop_time = stop_time_t;

			json_value* spectra_j = NULL;
			if (!(spectra_j = json_get(spectrum_sched_j, "spectra")))
				continue;
			if (spectra_j->type != json_array)
				goto error_hdl;

			// for each spectra item
			for (k = 0; k < spectra_j->u.array.length; k++)
			{
				json_value* spectra_item_j = spectra_j->u.array.values[k];
				if (spectra_item_j->type != json_object)
					goto error_hdl;

				// is this for resolutionBwHx = 100000.0
				double res_bw_hz;
				if (!(json_get_double(spectra_item_j, "resolutionBwHz", &res_bw_hz)))
					continue;
				if (res_bw_hz != (double)100000)
					continue;
				
				json_value* profiles_j = NULL;
				if (!(profiles_j = json_get(spectra_item_j, "profiles")))
					continue;
				if (profiles_j->type != json_array)
					goto error_hdl;

				contiguous = 0;

				for (m = 0; m < profiles_j->u.array.length; m++)
				{
					json_value* profile_j = profiles_j->u.array.values[m];
					if (profile_j->type != json_array)
						goto error_hdl;

					// check there are "pairs" of assignments provided
					if (profile_j->u.array.length % 2)
						goto error_hdl;

					for (n = 0; n < (profile_j->u.array.length / 2); n++)
					{
						double start_hz_dbl, end_hz_dbl, dbm;
						uint32_t start_hz, end_hz;

						json_value* item_j = profile_j->u.array.values[n*2];
						if (!(json_get_double(item_j, "hz", &start_hz_dbl)))
							goto error_hdl;
						if (!(json_get_double(item_j, "dbm", &dbm)))
							goto error_hdl;

						item_j = profile_j->u.array.values[(n*2)+1];
						if (!(json_get_double(item_j, "hz", &end_hz_dbl)))
							goto error_hdl;

						start_hz = (uint32_t)start_hz_dbl;
						end_hz = (uint32_t)end_hz_dbl;

						// it is within the band range
						if ((end_hz <= band_start_hz) || (start_hz >= band_end_hz))		
						{
							// update contiguous
							if (contiguous > spectrum_spec->max_contiguous_bw_hz_within_band)
								spectrum_spec->max_contiguous_bw_hz_within_band = contiguous;
							contiguous = 0;
							continue;
						}

						// does it have ANY power
						if (DBM_NO_POWER == dbm)   
						{
							LOG_PRINT(sm_, LOG_NOTICE, "Channel has no power : %.2fMhz->%.2fMhz", start_hz_dbl / 1000000, end_hz_dbl / 1000000);
							// update contiguous
							if (contiguous > spectrum_spec->max_contiguous_bw_hz_within_band)
								spectrum_spec->max_contiguous_bw_hz_within_band = contiguous;
							contiguous = 0;
							continue;
						}
						// does it have ENOUGH power
						if (dbm < min_dbm_100k)										
						{
							LOG_PRINT(sm_, LOG_NOTICE, "Channel has < min (%.2fdBm) power : %.2fMhz->%.2fMhz pwr:%.2fdBm", min_dbm_100k, start_hz_dbl/1000000, end_hz_dbl/1000000, dbm);
							// update contiguous
							if (contiguous > spectrum_spec->max_contiguous_bw_hz_within_band)
								spectrum_spec->max_contiguous_bw_hz_within_band = contiguous;
							contiguous = 0;
							continue;
						}
						// add to contiguous
						uint32_t prof_bw = end_hz - start_hz;
						if (start_hz < band_start_hz)
							prof_bw -= (band_start_hz - start_hz);
						else if (band_end_hz < end_hz)
							prof_bw -= (end_hz - band_end_hz);

						contiguous += prof_bw;

						// malloc an entry 
						profile = malloc(sizeof(spec_profile_type_t));
						memset(profile, 0, sizeof(spec_profile_type_t));
						profile->start_hz = start_hz;
						profile->end_hz = end_hz;
						profile->dbm = (float)dbm;

						// append to spectrum_schedule->profiles
						if (spectrum_sched->profiles == NULL)
						{
							spectrum_sched->profiles = profile;
							tail_profile = profile;
						}
						else
						{
							tail_profile->next = profile;
							tail_profile = profile;
						}
						profile = NULL;
					}
				} // profiles_j
			} // spectra

			// check if something was added for this spectrum_schedule
			if (spectrum_sched->profiles == NULL)
				spectrum_sched_free(&spectrum_sched);
			else
			{
				// append to spectrum_spec->spectrum_schedules
				if (spectrum_spec->spectrum_schedules == NULL)
				{
					spectrum_spec->spectrum_schedules = spectrum_sched;
					tail_spectrum_sched = spectrum_sched;
				}
				else
				{
					tail_spectrum_sched->next = spectrum_sched;
					tail_spectrum_sched = spectrum_sched;
				}
				spectrum_sched = NULL;
			}
		} // spectra_schedules_j

		  // update contiguous
		if (contiguous > spectrum_spec->max_contiguous_bw_hz_within_band)
		{
			spectrum_spec->max_contiguous_bw_hz_within_band = contiguous;
		}
	
		  // check if something was added for this spectrum_spec
		if (spectrum_spec->spectrum_schedules == NULL)
			spectrum_spec_free(&spectrum_spec);
		else
		{
			// max contiguous bw hz
			double max_contiguous_bw_hz;
			if ((json_get_double(spectrum_spec_j, "maxContiguousBwHz", &max_contiguous_bw_hz)))
				spectrum_spec->max_contiguous_bw_hz = (float)max_contiguous_bw_hz;

			// need spectrum report
			if (!(json_get_bool(spectrum_spec_j, "needsSpectrumReport", &spectrum_spec->needsSpectrumReport)))
				spectrum_spec->needsSpectrumReport = false;

			// append to avail_spectrum->spectrum_specs
			if (avail_spec->spectrum_specs == NULL)
			{
				avail_spec->spectrum_specs = spectrum_spec;
			}
			else
			{
				tail_spectrum_spec->next = spectrum_spec;
			}
			tail_spectrum_spec = spectrum_spec;
			spectrum_spec = NULL;
		}
	
	} // spectrum_specs

	return avail_spec;

error_hdl:
	if (profile) free(profile);
	if (spectrum_spec) spectrum_spec_free(&spectrum_spec);
	if (spectrum_sched) spectrum_sched_free(&spectrum_sched);
	if (avail_spec) avail_spectrum_free(&avail_spec);
	return NULL;
}



//#######################################################################################
static void spectrum_sched_print(spectrum_schedule_t* sched, int spec_id, int sched_id)
{
	char tmptime1[50];
	char tmptime2[50];
	strftime(tmptime1, sizeof(tmptime1), "%Y-%m-%dT%H:%M:%S", gmtime(&sched->event_time_range.start_time));
	strftime(tmptime2, sizeof(tmptime2), "%Y-%m-%dT%H:%M:%S", gmtime(&sched->event_time_range.stop_time));
	printf("[spec %d][sched %d]  start=%s     stop=%s \n", spec_id, sched_id, tmptime1, tmptime2);

	spec_profile_type_t* prof = sched->profiles;
	if (prof == NULL)
		printf("[spec %d][sched %d]  profiles=%p\n", spec_id, sched_id, prof);
	while (prof)
	{
		printf("[spec %d][sched %d]      prof:  start:%d  end:%d  dbm:%.2f \n", spec_id, sched_id, prof->start_hz, prof->end_hz, prof->dbm );
		prof = prof->next;
	}
}

static void spectrum_spec_print(spectrum_spec_t* spec, int spec_id)
{
	int sched_id = 0;

	printf("[spec %d]  max_contiguous_bw_hz=%.4f \n", spec_id, spec->max_contiguous_bw_hz);
	printf("[spec %d]  max_contiguous_bw_hz_within_band=%.4f \n", spec_id, spec->max_contiguous_bw_hz_within_band);
	printf("[spec %d]  needsSpectrumReport=%d \n", spec_id, spec->needsSpectrumReport);
	printf("[spec %d]  ruleSetInfo [ maxPollingSecs=%d maxLocationChange=%.2f ] \n", spec_id,  spec->ruleset_info.maxPollingSecs, spec->ruleset_info.maxLocationChange); 

	spectrum_schedule_t* spectrum_sched = spec->spectrum_schedules;
	if (spectrum_sched == NULL)
		printf("   spectrum_schedules = %p \n", spectrum_sched);
	while (spectrum_sched)
	{
		spectrum_sched_print(spectrum_sched, spec_id, sched_id);
		spectrum_sched = spectrum_sched->next;
		sched_id += 1;
	}
}

void avail_spectrum_print(avail_spectrum_t* s)
{
	int spec_id = 0;

	printf("avail_spectrum=%p \n", s);
	if (s)
	{
		spectrum_spec_t* spectrum_spec = s->spectrum_specs;
		if (spectrum_spec == NULL)
			printf("   spectrum_specs = %p \n", spectrum_spec);
		while (spectrum_spec)
		{
			spectrum_spec_print(spectrum_spec, spec_id);
			spectrum_spec = spectrum_spec->next;
			spec_id += 1;
		}
	}
}


//#######################################################################################
bool json_dbUpdate_2_dblist(json_value* dbUpdate_j, const char* token, uint32_t* num_db, paws_db_item_t* db_list)
{
	if (!((dbUpdate_j) && (token) && (db_list)))
		return false;

	if (dbUpdate_j->type != json_array)
		return false;

	// clean the old DB
	memset(db_list, 0, (sizeof(paws_db_item_t) * PAWS_MAX_DB_LIST));
	*num_db = 0;
	
	for (uint32_t i = 0; i < dbUpdate_j->u.array.length; i++)
	{
		json_value* db_j = dbUpdate_j->u.array.values[i];
		char *name = NULL;
		char *uri = NULL;
		if (!(((db_j->type == json_object)) &&
			((name = json_get_string(db_j, "name"))) &&
			((uri = json_get_string(db_j, "uri")))))
		{
			goto error_hdl;
		}

		// add to db_info
		memcpy(&(db_list[i].db_url.token[0]), token, strlen(token));
		memcpy(&(db_list[i].db_url.host[0]), uri, strlen(uri));
		memcpy(&(db_list[i].name[0]), name, strlen(name));
		db_list[i].valid = true;

		(*num_db)++;
	}

	return true;

error_hdl:
	memset(db_list, 0, (sizeof(paws_db_item_t) * PAWS_MAX_DB_LIST));
	*num_db = 0;
	return false;
}

