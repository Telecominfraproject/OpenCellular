/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef PAWS_DAL_DATABASE_H_
#define PAWS_DAL_DATABASE_H_


#include <stdint.h>
#include <stdbool.h>
#include <time.h>


// This is the PAWS Data Abstraction layer.
// It is responsible for reading and writing all data to stroage.
// It is expected that developers will implment individual source for this entity, on a per-platform basis.


// *********************************************************************
// Database
// *********************************************************************

#define MAX_URL_LENGTH				(100)
#define MAX_WEBLIST_NAME_LENGTH		(30)
#define MAX_DB_TOKEN_LENGTH	    	(128)
#define MAX_DB_NAME_LENGTH	    	(64)

typedef char paws_url_t[MAX_URL_LENGTH];
typedef char paws_weblist_name_t[MAX_WEBLIST_NAME_LENGTH];
typedef char paws_db_token_t[MAX_DB_TOKEN_LENGTH];
typedef char paws_db_name_t[MAX_DB_NAME_LENGTH];


typedef struct {
	paws_url_t			host;
	paws_weblist_name_t	fname;
} paws_weblist_url_t;

typedef struct {
	paws_url_t			host;
	paws_db_token_t		token;
} paws_db_url_t;

typedef struct {
	paws_db_name_t		name;
	paws_url_t			url;
	bool				mcwsd_support;
	bool				invalid;
} weblist_item_t;

typedef struct
{
	uint32_t			refresh_rate;
	uint16_t			num_items;
	weblist_item_t*		items;
} paws_weblist_t;

typedef struct
{
	paws_db_name_t		name;
	paws_db_url_t		db_url;
	uint8_t				valid;			// is it present in the weblist
	time_t				barred_utc;		// what time was it barred.  0 = not barred
} paws_db_item_t;

#define PAWS_MAX_DB_LIST	(10)

typedef struct {
	paws_weblist_url_t	weblist_url;
	uint32_t			num_db;
	paws_db_item_t		db_list[PAWS_MAX_DB_LIST];
} paws_db_info_t;




// *********************************************************************
// Read/write functons
// *********************************************************************

// Return True if read OK.
extern bool paws_read_db(paws_db_info_t* db);			

// Return True if written OK.
extern bool paws_write_db(paws_db_info_t* db);



#endif // PAWS_DAL_GPS_H_


