/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "utils/utils.h"
#include "http-client/http.h"
#include "https-client/https.h"
#include "json-parser/json_utils.h"

#include "paws_messages.h"
#include "paws_message_templates.h"
#include "paws_utils.h"
#include "paws_common.h"
#include "lte_utils.h"




#define TOKEN_HEADER					"?token="
#define TOKEN_HEADER_LEN				(7)
#define MAX_FULL_TOKEN_URL				(TOKEN_HEADER_LEN + MAX_DB_TOKEN_LENGTH)

typedef char full_token_t[MAX_FULL_TOKEN_URL];




// ############################################################################
// macro to help logging

#define MAX_LOG_J_LEN				(300)
static char log_j[MAX_LOG_J_LEN];
static char ltmp[100];
#define LOG_STRCAT(s)     strncat(log_j, s, MAX_LOG_J_LEN - strlen(log_j) - 1);

#define KV_INT(ltmp, k,v)  { sprintf(ltmp, "\"%s\" : %d",k,v); LOG_STRCAT( ltmp); }
#define KV_FLOAT2(ltmp, k,v)  { sprintf(ltmp, "\"%s\" : %.2f",k,v); LOG_STRCAT( ltmp); }
#define KV_FLOAT6(ltmp, k,v)  { sprintf(ltmp, "\"%s\" : %.6f",k,v); LOG_STRCAT( ltmp); }
#define KV_BOOL(ltmp, k,v)  { if (v) {sprintf(ltmp, "\"%s\" : true",k);} else {sprintf(ltmp, "\"%s\" : false",k);} LOG_STRCAT( ltmp); }
#define KV_STR(ltmp, k,v)  { if ((v) && (strlen(v) > 0)) { sprintf(ltmp, "\"%s\" : \"%s\"",k,v); LOG_STRCAT( ltmp); } }

//#######################################################################################
static bool decode_device_names(json_value* req, char* m_device_name, char* s_device_name)
{
	char* mdev = NULL;
	char* sdev = NULL;
	if ((mdev = json_get_string(req, "params/masterDeviceDesc/serialNumber")))
	{
		sprintf(m_device_name, "%s", mdev);
	}
	if ((sdev = json_get_string(req, "params/deviceDesc/serialNumber")))
	{
		if (mdev)
			sprintf(s_device_name, "%s", sdev);
		else
			sprintf(m_device_name, "%s", sdev);

		return true;
	}
	return false;
}

//#######################################################################################
void log_req_msg(void* sm_, json_value* req_j, device_name_t m_device_name, device_name_t s_device_name)
{
	bool added = false;
	log_j[0] = '\0';
	
	if ((!sm_) || (!req_j))
	{
		return;
	}

	LOG_STRCAT("{");

	// m_deviceid
	if (strlen(m_device_name))
	{
		KV_STR(ltmp, "master-device-id", m_device_name);
		added = true;
	}

	// s_deviceid
	if (strlen(s_device_name))
	{
		KV_STR(ltmp, "slave-device-id", s_device_name); 
		added = true;
	}
	
	// message type
	char* mtype = NULL;
	if ((mtype = json_get_string(req_j, "params/type")))
	{
		if (added) LOG_STRCAT(",");
		KV_STR(ltmp, "msg-type", mtype);
		added = true;
	}

	// requestType
	char *reqtype = NULL;
	if ((reqtype = json_get_string(req_j, "params/requestType")))
	{
		if (added) LOG_STRCAT(",");
		KV_STR(ltmp, "request-type", reqtype);
		added = true;
	}

	// longitude
	double longitude;
	if ((json_get_double(req_j, "params/location/point/center/longitude", (double*)&longitude)))
	{
		if (added) LOG_STRCAT(",");
		KV_FLOAT6(ltmp, "longitude", longitude);
		added = true;
	}

	// longitude
	double latitude;
	if ((json_get_double(req_j, "params/location/point/center/latitude", (double*)&latitude)))
	{
		if (added) LOG_STRCAT(",");
		KV_FLOAT6(ltmp, "latitude", latitude);
		added = true;
	}

	LOG_STRCAT("}");

	MSGLOG_TVWSDB_MSG(sm_, log_j);
}


//#######################################################################################
void log_resp_msg(void* sm_, char* resp, json_value* resp_j, device_name_t m_device_name, device_name_t s_device_name, int http_resp_code)
{
	bool added = false;
	log_j[0] = '\0';

	if (!sm_)
	{
		return;
	}

	LOG_STRCAT("{");

	// m_deviceid
	if (strlen(m_device_name))
	{
		KV_STR(ltmp, "master-device-id", m_device_name);
		added = true;
	}

	// s_deviceid
	if (strlen(s_device_name))
	{
		KV_STR(ltmp, "slave-device-id", s_device_name);
		added = true;
	}

	// resp can be NULL if there was a HTTP error response
	if (!resp)
	{
		KV_INT(ltmp, "http-error-resp-code", http_resp_code);
	}
	else
	{
		// check if the json was parsed OK
		if (!resp_j)
		{
			char tmp[70];
			if (added) LOG_STRCAT(","); 
			snprintf(tmp, sizeof(tmp)-1, "%s", resp);
			KV_STR(ltmp, "resp-parse-failure", &tmp[0]);
		}
		else
		{
			//printf("resp=%s\n", resp);

			// message type
			char* mtype = NULL;
			if ((mtype = json_get_string(resp_j, "result/type")))
			{
				if (added) LOG_STRCAT(",");
				KV_STR(ltmp, "msg-type", mtype);
				added = true;
			}

			// it is an error resp
			json_value* error = NULL;
			if ((error = json_get(resp_j, "error")))
			{
				int64_t code = 0;
				if (json_get_int(error, "code", &code))
				{
					if ((code >= PAWS_ERROR_COMPATIBILITY_MIN) && (code <= PAWS_ERROR_COMPATIBILITY_MAX))
					{
						if (added) LOG_STRCAT(",");
						KV_STR(ltmp, "error-code", "DB COMPATIBILITY ERROR");
						added = true;
					}
					else if ((code >= PAWS_ERROR_REQERROR_MIN) && (code <= PAWS_ERROR_REQERROR_MAX))
					{
						if (added) LOG_STRCAT(",");
						KV_STR(ltmp, "error-code", "DB REQUEST ERROR");
						added = true;
					}
					else if ((code >= PAWS_ERROR_AUTHORISATION_MIN) && (code <= PAWS_ERROR_AUTHORISATION_MIN))
					{
						if (added) LOG_STRCAT(",");
						KV_STR(ltmp, "error-code", "DB AUTHORISATION ERROR");
						added = true;
					}
					else
					{
						if (added) LOG_STRCAT(",");
						KV_STR(ltmp, "error-code", "DB GENERAL ERROR");
						added = true;
					}
				}
				else
				{
					if (added) LOG_STRCAT(",");
					KV_STR(ltmp, "error-code", "DB GENERAL ERROR");
					added = true;
				}
			}
			else
			{
				bool result;
				if (json_get_bool(resp_j, "result/result", &result))
				{
					// add result				
					if (added) LOG_STRCAT(",");
					KV_BOOL(ltmp, "result", result);
					added = true;

					// more stuff if result=True
					if (result)
					{
						// message 
						char *message = NULL;
						if ((message = json_get_string(resp_j, "result/message")))
						{
							if (added) LOG_STRCAT(",");
							KV_STR(ltmp, "message", message);
							added = true;
						}

					}
				}
			}
		}
	}

	LOG_STRCAT("}");

	MSGLOG_TVWSDB_MSG(sm_, log_j);
}







//#######################################################################################
// unused at present
#if 0
#define BASE_MESSAGES_PATH				"messages"
#define MAX_BASE_PATH_NAME				(100)
#define INIT_REQ_FILE 					"init_req.json"
#define AVAILABLE_SPECTRUM_REQ_FILE		"available_spectrum_req.json"
#define GOP_AVAILABLE_SPECTRUM_REQ_FILE	"slave_gop_available_spectrum_req.json"
#define SPECTRUM_USE_NOTIFY_FILE 		"spectrum_use_notify.json"
#define SLAVE_SPECTRUM_USE_NOTIFY_FILE 	"slave_spectrum_use_notify.json"

json_value* get_base_message(void* sm_, const char* base_filename)
{
	static char filename[100];
	struct stat filestatus;
	int file_size = 0;
	char *file_contents = NULL;
	FILE *fp = NULL;
	json_value *jval = NULL;

	// check the base_filename
	if (!base_filename)
	{
		LOG_PRINT(sm_, LOG_ERROR, "NULL base_filename");
		goto error_hdl;
	}

	// create the filename
	int slen = snprintf(filename, MAX_BASE_PATH_NAME, "%s/%s", BASE_MESSAGES_PATH, base_filename);
	if ((slen < 0) || (slen >= MAX_BASE_PATH_NAME))
	{
		LOG_PRINT(sm_, LOG_ERROR, "Unable to write filename %s\n", base_filename);
		goto error_hdl;
	}
	
	// find the file
	if (stat(filename, &filestatus) != 0)
	{
		LOG_PRINT(sm_, LOG_ERROR, "File %s not found\n", filename);
		goto error_hdl;
	}

	// create space for the contents
	file_size = filestatus.st_size;
	if (!(file_contents = (char*)malloc(filestatus.st_size + 1)))
	{
		LOG_PRINT(sm_, LOG_ERROR, "Memory error: unable to allocate %d bytes\n", file_size);
		goto error_hdl;
	}
	file_contents[filestatus.st_size] = '\0';

	// open the file
	if (!(fp = fopen(filename, "rt")))
	{
		LOG_PRINT(sm_, LOG_ERROR, "Unable to open %s\n", filename);
		goto error_hdl;
	}

	// read the contents
	if (fread(file_contents, file_size, 1, fp) != 1)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Unable to read content of %s\n", filename);
		goto error_hdl;
	}

	// we have the contents, now convert to json
	if (!(jval = json_parse((json_char*)file_contents, file_size)))
	{
		LOG_PRINT(sm_, LOG_ERROR, "Unable to parse data");
		goto error_hdl;
	}

	// free up temp dynamic data 
	free(file_contents);
	fclose(fp);

	// success
	return jval;

error_hdl:
	if (file_contents) free(file_contents);
	if (fp) fclose(fp);
	return NULL;
}
#endif


//#######################################################################################
static char* post_it_http(void* sm_, char* host, char* token, char* req, int *body_offset)
{
	char *response = NULL;
	int sockfd = -1;

	// check params
	if (!(host && strlen(host) && token && strlen(token) && (req)))
	{
		return NULL;
	}

	uint8_t db_attempts = MAX_DB_ACCESS_ATTEMPTS;
	while (db_attempts--)
	{
		// open socket
		char const *port = HTTP_PORT;
		sockfd = http_init(host, port);
		if (sockfd < 0)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Could not socket to '%s'", host);
			goto error_hdl;
		}

		// send POST
		int len = strlen(req);
		if ((http_send_post(sockfd, host, token, req, len)) < 0)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Sending request failed");
			goto error_hdl;
		}

		// get the response
		response = http_fetch_response(sockfd, body_offset);
		if (!response)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Fetching response failed");
			goto error_hdl;
		}

		// free up mallocd items
		if (sockfd >= 0) http_close(sockfd);

		return response;

error_hdl:
		if (sockfd >= 0) http_close(sockfd);
		if (response) free(response);
	}
	return NULL;
}




//#######################################################################################
static char* post_it_https(void* sm_, char* host, char* token, char* req, int *body_offset)
{
	char *response = NULL; 
	int sockfd = -1;
	SSL* conn = NULL;

	// check params
	if (!(host && strlen(host) && token && strlen(token) && (req)))
	{
		return NULL;
	}

	uint8_t db_attempts = MAX_DB_ACCESS_ATTEMPTS;
	while (db_attempts--)
	{
		// open socket
		char const *port = HTTPS_PORT;
		sockfd = http_init(host, port);
		if (sockfd < 0)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Could not make connection to '%s", host);
			goto error_hdl;
		}

		// make an ssl connection
		conn = https_connect(sockfd);
		if (!(conn))
		{
			LOG_PRINT(sm_, LOG_ERROR, "Failure makling ssl connection to %s", host);
			goto error_hdl;
		}

		// send the POST
		if (https_send_post(conn, host, token, req, strlen(req)) < 0)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Sending request failed");
			goto error_hdl;
		}

		// get the response
		*body_offset = 0;
		response = https_fetch_response(conn, body_offset);
		if (!response)
		{
			LOG_PRINT(sm_, LOG_ERROR, "Fetching response failed");
			goto error_hdl;
		}

		if (conn) https_disconnect(conn);
		if (sockfd >= 0) http_close(sockfd);
		return response;

error_hdl:
		if (response) free(response);
		if (conn) https_disconnect(conn);
		if (sockfd >= 0) http_close(sockfd);
	}
	return NULL;
}




//#######################################################################################
static json_value* post_it(void* sm_, paws_db_url_t* db_url, json_value* j_req, int *http_resp_code)
{
	char *response = NULL;
	json_value *value = NULL;
	*http_resp_code = 0;
	char* req = NULL;
	int body_offset = 0;
	full_token_t token_;

	// check params
	if (!((db_url) && strlen(db_url->host) && strlen(db_url->token) && (j_req)))
	{
		goto error_hdl;
	}

	// convert the json to string
	if (!(req = json_value_2_string(j_req)))
	{
		goto error_hdl;
	}

	if ((!req) || (strlen(req) == 0))
	{
		goto error_hdl;
	}

	// get device ID
	device_name_t m_device_name;
	device_name_t s_device_name;
	memset(m_device_name, 0, sizeof(device_name_t));
	memset(s_device_name, 0, sizeof(device_name_t));
	if (!(decode_device_names(j_req, m_device_name, s_device_name)))
	{
		LOG_PRINT(sm_, LOG_ERROR, "No DeviceId in JSON Req");
		goto error_hdl;
	}

	log_req_msg(sm_, j_req, m_device_name, s_device_name);

	// create the token
	int slen = snprintf(token_, MAX_FULL_TOKEN_URL, "%s%s", TOKEN_HEADER, db_url->token);
	if ((slen < 0) || (slen >= MAX_FULL_TOKEN_URL))
	{
		LOG_PRINT(sm_, LOG_ERROR, "Unable to write full token");
		goto error_hdl;
	}

	// determine if this is http or https
	const char* HTTP_HEADER = "http://";
	const char* HTTPS_HEADER = "https://";
	char *host=NULL;
	// move past http header
	if ((host = strstr(db_url->host, HTTP_HEADER)))
	{
		host += strlen(HTTP_HEADER);
		response = post_it_http(sm_, host, &token_[0], req, &body_offset);
	}
	else if ((host = strstr(db_url->host, HTTPS_HEADER)))
	{
		host += strlen(HTTPS_HEADER);
		response = post_it_https(sm_, host, &token_[0], req, &body_offset);
	}
	else
	{
		LOG_PRINT(sm_, LOG_ERROR, "URL must start with \"http://\" or \"https://\"");
		goto error_hdl;
	}

	if (!response)
	{
		LOG_PRINT(sm_, LOG_ERROR, "Failure in http/https POST");
		goto error_hdl;
	}

	// get the HTTP response code
	*http_resp_code = http_get_status_code(response);

	if (*http_resp_code >= 400)
	{
		// these are HTTP error codes
		log_resp_msg(sm_, NULL, NULL, m_device_name, s_device_name, *http_resp_code);
		APPLOG_TVWSDB_MSG(sm_, req);
		LOG_PRINT(sm_, LOG_ERROR, "[url=%s] HTTP ERROR RESP %d", db_url->host, *http_resp_code);
		goto error_hdl;
	}

	// log the Req and Resp
	APPLOG_TVWSDB_MSG(sm_, req);
	APPLOG_TVWSDB_MSG(sm_, response + body_offset);

	// convert to json
	json_char *json = (json_char*)response + body_offset;
	int len = strlen(response) - body_offset;
	value = json_parse(json, len);
	if (value == NULL) {
		log_resp_msg(sm_, response + body_offset, NULL, m_device_name, s_device_name, *http_resp_code);
		LOG_PRINT(sm_, LOG_ERROR, "Unable to parse data [body_offset=%d body_len=%d %.*s]", body_offset, len, 100, json);
		LOG_PRINT(sm_, LOG_ERROR, "    full response = %s", response);
		goto error_hdl;
	}

	log_resp_msg(sm_, response + body_offset, value, m_device_name, s_device_name, *http_resp_code);

	// free up mallocd items
	if (response) free(response);
	if (req) free(req);
	return value;

error_hdl:
	if (response) free(response);
	if (value) json_value_free(value);
	if (req) free(req);
	return NULL;
}



//#######################################################################################
static bool set_deviceDesc(json_value* req, paws_device_info_t* device_info, char* device_desc_key)
{
	char path[60];

	if ((!req) || (!device_info))
		return false;

	// serialNumber
	sprintf(path, "params/%s/serialNumber", device_desc_key);
	if (!(json_set_string(req, path, device_info->unique_id)))
		return false;

	// etsiEnDeviceType
	sprintf(path, "params/%s/etsiEnDeviceType", device_desc_key);
	if (!(json_set_string(req, path, device_info->device_characteristics.type)))
		return false;

	// etsiEnDeviceCategory
	sprintf(path, "params/%s/etsiEnDeviceCategory", device_desc_key);
	if (!(json_set_string(req, path, device_info->device_characteristics.cat)))
		return false;

	// etsiEnDeviceCategory
	sprintf(path, "params/%s/etsiEnDeviceEmissionsClass", device_desc_key);
	if (!(json_set_int(req, path, device_info->device_characteristics.emission_class)))
		return false;

	// etsiEnTechnologyId
	sprintf(path, "params/%s/etsiEnTechnologyId", device_desc_key);
	if (!(json_set_string(req, path, device_info->device_characteristics.technology_id)))
		return false;

	// manufacturerId
	sprintf(path, "params/%s/manufacturerId", device_desc_key);
	if (!(json_set_string(req, path, device_info->device_identity.manufacturer)))
		return false;

	// modelId
	sprintf(path, "params/%s/modelId", device_desc_key);
	if (!(json_set_string(req, path, device_info->device_identity.model)))
		return false;

	return true;
}

//#######################################################################################
static bool set_gps(json_value* req, paws_gps_location_t* gps, char* location_k)
{
	if ((!req) || (!location_k) || (!gps) || (!gps->fixed))
		return false;

	char path[100];

	// latitude
	sprintf(path, "params/%s/point/center/latitude", location_k);
	if (!(json_set_double(req, path, gps->latitude)))
		return false;
	
	// longitude
	sprintf(path, "params/%s/point/center/longitude", location_k);
	if (!(json_set_double(req, path, gps->longitude)))
		return false;


	return true;
}



//#######################################################################################
static bool set_antenna_info(json_value* req, uint16_t height, char* height_type)
{
	if (!req)
		return false;

	// height, and heightType
	if ((json_set_int(req, "params/antenna/height", height)) &&
		(json_set_string(req, "params/antenna/heightType", height_type)))
	{
		return true;
	}

	return false;
}



//#######################################################################################
static bool build_Init_Req(json_value* req, paws_device_info_t* device_info, paws_gps_location_t* gps)
{
	// timestamp
	{
		// ## req['timestamp'] = utcnow.strftime("%d/%m/%Y @ %H:%M:%S")
		char tmptime[100];
		time_t cur_time = time(NULL);
		strftime(tmptime, sizeof(tmptime), "%d/%m/%Y @ %H:%M:%S", gmtime(&cur_time));
		if (!(json_set_string(req, "timestamp", tmptime)))
		{
			return false;
		}
	}

	// deviceDesc 
	if (!(set_deviceDesc(req, device_info, "deviceDesc")))
	{
		return false;
	}

	// gps 
	if (!(set_gps(req, gps, "location")))
	{
		return false;
	}

	return true;
}
//#######################################################################################
json_value* post_Init_Request(void* sm_, paws_device_info_t* device_info, paws_gps_location_t* gps, paws_db_url_t* url)
{
	json_value* j_req = NULL;
	json_value* j_resp = NULL;

	if (!(j_req = get_init_req_template(sm_)))
	{
		goto error_hdl;
	}

	// update req with specific params
	if (!(build_Init_Req(j_req, device_info, gps)))
	{
		goto error_hdl;
	}

	// post it to db url
	int ret_code;
	if (!(j_resp = post_it(sm_, url, j_req, &ret_code)))
	{
		goto error_hdl;
	}

	// it posted OK
	json_value_free(j_req);
	return j_resp;

error_hdl:
	if (j_req) json_value_free(j_req);
	if (j_resp) json_value_free(j_resp);
	return NULL;
}






//#######################################################################################
static bool build_Avail_Spectrum_Req(json_value* req, paws_device_info_t* device_info, paws_gps_location_t* gps)
{
	// timestamp
	{
		// ## req['timestamp'] = utcnow.strftime("%d/%m/%Y @ %H:%M:%S")
		char tmptime[100];
		time_t cur_time = time(NULL);
		strftime(tmptime, sizeof(tmptime), "%d/%m/%Y @ %H:%M:%S", gmtime(&cur_time));
		if (!(json_set_string(req, "timestamp", tmptime)))
		{
			return false;
		}
	}

	// deviceDesc 
	if (!(set_deviceDesc(req, device_info, "deviceDesc")))
	{
		return false;
	}

	// gps 
	if (!(set_gps(req, gps, "location")))
	{
		return false;
	}

	// antenna/
	if (!(set_antenna_info(req, gps->height, gps->height_type)))
	{
		return false;
	}

	return true;
}
//#######################################################################################
json_value* post_Avail_Spectrum_Request(void* sm_, paws_device_info_t *device_info, paws_gps_location_t* gps, paws_db_url_t* url)
{
	json_value* j_req = NULL;
	json_value* j_resp = NULL;

	if (!(j_req = get_available_spectrum_req_template(sm_)))
	{
		goto error_hdl;
	}

	// update req with specific params
	if (!(build_Avail_Spectrum_Req(j_req, device_info, gps)))
	{
		goto error_hdl;
	}

	// post it to db url
	int ret_code;
	if (!(j_resp = post_it(sm_, url, j_req, &ret_code)))
	{
		goto error_hdl;
	}

	// it posted OK
	json_value_free(j_req);
	return j_resp;

error_hdl:
	if (j_req) json_value_free(j_req);
	if (j_resp) json_value_free(j_resp);
	return NULL;
}




//#######################################################################################
static bool build_slave_GOP_Avail_Spectrum_Req(json_value* req, paws_device_info_t* device_info, paws_gps_location_t* gps)
{
	// timestamp
	{
		// ## req['timestamp'] = utcnow.strftime("%d/%m/%Y @ %H:%M:%S")
		char tmptime[100];
		time_t cur_time = time(NULL);
		strftime(tmptime, sizeof(tmptime), "%d/%m/%Y @ %H:%M:%S", gmtime(&cur_time));
		if (!(json_set_string(req, "timestamp", tmptime)))
		{
			return false;
		}
	}

	// deviceDesc 
	if (!(set_deviceDesc(req, device_info, "deviceDesc")))
	{
		return false;
	}

	// gps 
	if (!(set_gps(req, gps, "location")))
	{
		return false;
	}

	return true;
}
//#######################################################################################
json_value* post_Slave_GOP_Available_Spectrum_Request(void* sm_, paws_device_info_t *device_info, paws_gps_location_t* gps, paws_db_url_t* url)
{
	json_value* j_req = NULL;
	json_value* j_resp = NULL;

	if (!(j_req = get_slave_gop_available_spectrum_req_template(sm_)))
	{
		goto error_hdl;
	}

	// update req with specific params
	if (!(build_slave_GOP_Avail_Spectrum_Req(j_req, device_info, gps)))
	{
		goto error_hdl;
	}

	// post it to db url
	int ret_code;
	if (!(j_resp = post_it(sm_, url, j_req, &ret_code)))
	{
		goto error_hdl;
	}

	// it posted OK
	json_value_free(j_req);
	return j_resp;

error_hdl:
	if (j_req) json_value_free(j_req);
	if (j_resp) json_value_free(j_resp);
	return NULL;
}



//#######################################################################################
static bool build_slave_SOP_Avail_Spectrum_Req(json_value* req, paws_device_info_t* master_info, paws_device_info_t* slave_device_info, paws_gps_location_t* gps)
{
	// timestamp
	{
		// ## req['timestamp'] = utcnow.strftime("%d/%m/%Y @ %H:%M:%S")
		char tmptime[100];
		time_t cur_time = time(NULL);
		strftime(tmptime, sizeof(tmptime), "%d/%m/%Y @ %H:%M:%S", gmtime(&cur_time));
		if (!(json_set_string(req, "timestamp", tmptime)))
		{
			return false;
		}
	}

	// deviceDesc 
	if (!(set_deviceDesc(req, slave_device_info, "deviceDesc")))
	{
		return false;
	}
	// masterdeviceDesc 
	if (!(set_deviceDesc(req, master_info, "masterDeviceDesc")))
	{
		return false;
	}

	// gps 
	if (!(set_gps(req, gps, "location")))
	{
		return false;
	}

	// antenna/
	if (!(set_antenna_info(req, gps->height, gps->height_type)))
	{
		return false;
	}


	return true;
}
//#######################################################################################
json_value* post_Slave_SOP_Available_Spectrum_Request(void* sm_, paws_device_info_t* master_info, paws_device_info_t* slave_device_info, paws_gps_location_t* gps, paws_db_url_t* url)
{
	json_value* j_req = NULL;
	json_value* j_resp = NULL;

	if (!(j_req = get_slave_sop_available_spectrum_req_template(sm_)))
	{
		goto error_hdl;
	}

	// update req with specific params
	if (!(build_slave_SOP_Avail_Spectrum_Req(j_req, master_info, slave_device_info, gps)))
	{
		goto error_hdl;
	}

	// post it to db url
	int ret_code;
	if (!(j_resp = post_it(sm_, url, j_req, &ret_code)))
	{
		goto error_hdl;
	}

	// it posted OK
	json_value_free(j_req);
	return j_resp;

error_hdl:
	if (j_req) json_value_free(j_req);
	if (j_resp) json_value_free(j_resp);
	return NULL;
}




//#######################################################################################
bool set_sprectrum_use_spectra(json_value* req, spec_profile_type_t* profiles, lte_direction_e dir, uint8_t band_id)
{
	uint32_t band_start, band_end;
	int32_t band_spacing;
	static char profiles_str[2000];
    profiles_str[0] = '\0';

	if (!profiles)
		return false;

	// get band info
	if (!(get_lte_band_channel(band_id, dir, &band_start, &band_end, &band_spacing)))
		return false;


	// walk profiles and count how many to use (i.e. ignore any which fall outside of the band)
	int num_prof = 0;
	spec_profile_type_t* start_prof = NULL;
	spec_profile_type_t* prof = profiles;
	for (prof = profiles; prof != NULL; prof = prof->next)
	{
		if ((band_start >= prof->end_hz) || (band_end <= prof->start_hz))
			continue;

		// lets mark start profile
		if (!start_prof)
			start_prof = prof;

		num_prof++;
	}

	if (num_prof)
	{
		strcat(profiles_str, "[ ");

		// 100k
		strcat(profiles_str, "{ \"resolutionBwHz\": 100000, \"profiles\" : [ [ ");
		prof = start_prof;
		for (int i = 0; ((i < num_prof) && (prof)); i++, prof=prof->next)
		{
			char prof_str[100];
			sprintf(prof_str, "{ \"hz\": %.1f, \"dbm\" : %.2f }, { \"hz\": %.0f, \"dbm\" : %.2f }", (double) prof->start_hz, prof->dbm, (double)prof->end_hz, prof->dbm);
			strcat(profiles_str, prof_str);
			if (i != (num_prof - 1))
				strcat(profiles_str, ",");
		}
		strcat(profiles_str, " ] ]");
		strcat(profiles_str, " } , ");

		// 8M
		strcat(profiles_str, "{ \"resolutionBwHz\": 8000000, \"profiles\" : [ [ ");
		prof = start_prof;
		for (int i = 0; ((i < num_prof) && (prof)); i++, prof = prof->next)
		{
			char prof_str[100];
			sprintf(prof_str, "{ \"hz\": %.1f, \"dbm\" : -999 }, { \"hz\": %.0f, \"dbm\" : -999 }", (double)prof->start_hz, (double)prof->end_hz);
			strcat(profiles_str, prof_str);
			if (i != (num_prof - 1))
				strcat(profiles_str, ",");
		}
		strcat(profiles_str, " ] ]");
		strcat(profiles_str, " }  ");
		strcat(profiles_str, " ]");

		// convert it to json
		json_value* jval = NULL;
		if ((jval = json_parse(profiles_str, strlen(profiles_str))))
		{
			if ((json_set_json_value(req, "params/spectra", jval)))
			{
				json_value_free(jval);
				return true;
			}
		}
	}
	return false;
}



//#######################################################################################
static bool build_Spectrum_Use_Notify(json_value* req, paws_device_info_t* device_info, paws_gps_location_t* gps, spec_cfg_t*	cfg, uint8_t band_id)
{
	// timestamp
	{
		// ## req['timestamp'] = utcnow.strftime("%d/%m/%Y @ %H:%M:%S")
		char tmptime[100];
		time_t cur_time = time(NULL);
		strftime(tmptime, sizeof(tmptime), "%d/%m/%Y @ %H:%M:%S", gmtime(&cur_time));
		if (!(json_set_string(req, "timestamp", tmptime)))
		{
			return false;
		}
	}

	// deviceDesc 
	if (!(set_deviceDesc(req, device_info, "deviceDesc")))
	{
		return false;
	}

	// gps 
	if (!(set_gps(req, gps, "location")))
	{
		return false;
	}

	// spectra
	if (!(set_sprectrum_use_spectra(req, cfg->sched->profiles, LTE_DL, band_id)))
	{
		return false;
	}

	return true;
}
//#######################################################################################
json_value* post_Notify_Request(void* sm_, paws_device_info_t *device_info, paws_gps_location_t* gps, paws_db_url_t* url, spec_cfg_t*	cfg, uint8_t band_id)
{
	json_value* j_req = NULL;
	json_value* j_resp = NULL;

	if (!(j_req = get_spectrum_use_notify_req_template(sm_)))
	{
		goto error_hdl;
	}

	// update req with specific params
	if (!(build_Spectrum_Use_Notify(j_req, device_info, gps, cfg, band_id)))
	{
		goto error_hdl;
	}

	// post it to db url
	int ret_code;
	if (!(j_resp = post_it(sm_, url, j_req, &ret_code)))
	{
		goto error_hdl;
	}

	// it posted OK
	json_value_free(j_req);
	return j_resp;

error_hdl:
	if (j_req) json_value_free(j_req);
	if (j_resp) json_value_free(j_resp);
	return NULL;
}



//#######################################################################################
static bool build_GOP_slave_Spectrum_Use_Notify(json_value* req, paws_device_info_t* master_info, paws_device_info_t* slave_device_info, paws_gps_location_t* master_gps, spec_cfg_t* cfg, uint8_t band_id)
{
	// timestamp
	{
		// ## req['timestamp'] = utcnow.strftime("%d/%m/%Y @ %H:%M:%S")
		char tmptime[100];
		time_t cur_time = time(NULL);
		strftime(tmptime, sizeof(tmptime), "%d/%m/%Y @ %H:%M:%S", gmtime(&cur_time));
		if (!(json_set_string(req, "timestamp", tmptime)))
		{
			return false;
		}
	}

	// deviceDesc 
	if (!(set_deviceDesc(req, slave_device_info, "deviceDesc")))
	{
		return false;
	}
	// masterdeviceDesc 
	if (!(set_deviceDesc(req, master_info, "masterDeviceDesc")))
	{
		return false;
	}

	// gps 
	if (!(set_gps(req, master_gps, "masterDeviceLocation")))
	{
		return false;
	}

	// spectra
	if (!(set_sprectrum_use_spectra(req, cfg->sched->profiles, LTE_UL, band_id)))
	{
		return false;
	}

	return true;
}
//#######################################################################################
json_value* post_GOP_slave_Notify_Request(void* sm_, paws_device_info_t* master_info, paws_device_info_t* slave_device_info, paws_gps_location_t* master_gps, paws_db_url_t* url, spec_cfg_t* cfg, uint8_t band_id)
{
	json_value* j_req = NULL;
	json_value* j_resp = NULL;

	if (!(j_req = get_gop_slave_spectrum_use_notify_req_template(sm_)))
	{
		goto error_hdl;
	}

	// update req with specific params
	if (!(build_GOP_slave_Spectrum_Use_Notify(j_req, master_info, slave_device_info, master_gps, cfg, band_id)))
	{
		goto error_hdl;
	}

	// post it to db url
	int ret_code;
	if (!(j_resp = post_it(sm_, url, j_req, &ret_code)))
	{
		goto error_hdl;
	}

	// it posted OK
	json_value_free(j_req);
	return j_resp;

error_hdl:
	if (j_req) json_value_free(j_req);
	if (j_resp) json_value_free(j_resp);
	return NULL;
}




//#######################################################################################
static bool build_SOP_slave_Spectrum_Use_Notify(json_value* req, paws_device_info_t* master_info, paws_device_info_t* slave_device_info, paws_gps_location_t* master_gps, spec_cfg_t* cfg, uint8_t band_id)
{
	// timestamp
	{
		// ## req['timestamp'] = utcnow.strftime("%d/%m/%Y @ %H:%M:%S")
		char tmptime[100];
		time_t cur_time = time(NULL);
		strftime(tmptime, sizeof(tmptime), "%d/%m/%Y @ %H:%M:%S", gmtime(&cur_time));
		if (!(json_set_string(req, "timestamp", tmptime)))
		{
			return false;
		}
	}

	// deviceDesc 
	if (!(set_deviceDesc(req, slave_device_info, "deviceDesc")))
	{
		return false;
	}
	// masterdeviceDesc 
	if (!(set_deviceDesc(req, master_info, "masterDeviceDesc")))
	{
		return false;
	}

	// master gps 
	if (!(set_gps(req, master_gps, "masterDeviceLocation")))
	{
		return false;
	}
	// gps 
	if (!(set_gps(req, &slave_device_info->gps, "location")))
	{
		return false;
	}

	// spectra
	if (!(set_sprectrum_use_spectra(req, cfg->sched->profiles, LTE_UL, band_id)))
	{
		return false;
	}

	return true;
}
//#######################################################################################
json_value* post_SOP_slave_Notify_Request(void* sm_, paws_device_info_t* master_info, paws_device_info_t* slave_device_info, paws_gps_location_t* master_gps, paws_db_url_t* url, spec_cfg_t* cfg, uint8_t band_id)
{
	json_value* j_req = NULL;
	json_value* j_resp = NULL;

	if (!(j_req = get_sop_slave_spectrum_use_notify_req_template(sm_)))
	{
		goto error_hdl;
	}

	// update req with specific params
	if (!(build_SOP_slave_Spectrum_Use_Notify(j_req, master_info, slave_device_info, master_gps, cfg, band_id)))
	{
		goto error_hdl;
	}

	// post it to db url
	int ret_code;
	if (!(j_resp = post_it(sm_, url, j_req, &ret_code)))
	{
		goto error_hdl;
	}

	// it posted OK
	json_value_free(j_req);
	return j_resp;

error_hdl:
	if (j_req) json_value_free(j_req);
	if (j_resp) json_value_free(j_resp);
	return NULL;
}