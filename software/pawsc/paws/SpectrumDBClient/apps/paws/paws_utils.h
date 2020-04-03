/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_UTILS_H_
#define PAWS_UTILS_H_

#include "json-parser/json.h"
#include "logger/logger.h"

#include "paws_types.h"
#include "paws_globals.h"
#include "paws_common.h"

extern void weblist_free(paws_weblist_t** weblist);
extern paws_weblist_t* json_2_weblist(json_value* src);

extern avail_spectrum_t* json_resp_2_avail_spectrum(void *sm_, json_value* src, lte_direction_e dir, uint8_t lte_band_id, time_t resp_time, float min_dbm_100k);
extern void avail_spectrum_print(avail_spectrum_t* spec);

extern int timestamp_to_timet(char* src);

extern bool json_dbUpdate_2_dblist(json_value* dbUpdate_j, const char* token, uint32_t* num_db, paws_db_item_t* db_list);


extern char logline_[];
#define LOG_PRINT(sm, loglevel, fmt, args...) { \
	if (loglevel >= gPawsAppLoggerCfg.level) \
	{ \
		int slen = snprintf(logline_, LOGGER_MAX_LENGTH, fmt, ##args); \
		if (!((slen <= 0) || (slen >= LOGGER_MAX_LENGTH))) \
		{ \
			LOCAL_FUNC(sm, app_log)(sm, __FILENAME__, __LINE__, __func__, logline_, loglevel); \
		} \
	} \
}

#define APPLOG_TVWSDB_MSG(sm, msg)		{ LOCAL_FUNC(sm, app_log_tvwsdb_msg)(sm, msg); }
#define MSGLOG_TVWSDB_MSG(sm, jmsg)		{ LOCAL_FUNC(sm, msg_log_tvwsdb_msg)(sm, jmsg); }

#define FUNC_DBG(sm)					LOG_PRINT(sm, LOG_FUNC, " ")


#endif // #define PAWS_UTILS_H_

