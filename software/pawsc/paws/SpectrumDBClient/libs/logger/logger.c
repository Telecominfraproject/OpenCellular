/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "logger.h"
#include "utils/types.h"
#include "utils/utils.h"

static char g_log_[LOGGER_MAX_LENGTH];
static char g_log_json[LOGGER_MAX_LENGTH+200];
#define LOG_STRCAT(tmp)     strncat(g_log_, tmp, LOGGER_MAX_LENGTH - strlen(g_log_) - 1);


// type for internal logger info
typedef struct {
	logger_cfg_t	cfg;
	uint32_t		file_size;
	filename_t		dirname_;			// directory of logname
	filename_t		basename_;			// basename of logname
} logger_t;




//#######################################################################################
char* loglevel_2_str(log_level_e level)
{
	switch (level)
	{
	case LOG_FUNC:		return "FUNC";
	case LOG_DEBUG:		return "DEBUG";
	case LOG_INFO:		return "INFO";
	case LOG_NOTICE:	return "NOTICE";
	case LOG_WARNING:	return "WARNING";
	case LOG_ERROR:		return "ERROR";
	case LOG_CRITICAL:	return "CRITICAL";
	default:
		return "????";
	}
}



//#######################################################################################
static FILE* open_file(char* filename)
{
	if ((!filename) || (strlen(filename) < 1))
		return NULL;

	// If a filepath is specified, and it doesnt exist, create it
	char* pos = NULL;
	if ((pos = strrchr(filename, '/')))
	{
		// a path is specified
		*pos = '\0';

		// If the path doesn't exist on the filesystem, create it 
		char cmd[200];
		sprintf(cmd, "mkdir -p %s", filename);
		system(cmd);

		*pos = '/';
	}

	// open the file for appending
	return fopen(filename, "a");
}



//#######################################################################################
static void rotate_files(logger_t* logger, uint32_t loglen)
{
	struct stat filestatus;

	// get the file details
	if (stat(logger->cfg.logname, &filestatus) == -1)				// this shouldnt fail, so just return if issue	
		return;

	// if we append this line to the log, will the max file_size be exceeded
	if (!((loglen + filestatus.st_size) > logger->file_size))
	{
		// if file size will NOT be exceeded, no rotate will be needed
		return;
	}
	
	// rotate needed
	char command[MAX_FILENAME_LEN + 50];
	snprintf(command, sizeof(command), "ls -alrQ %s/%s*", logger->dirname_, logger->basename_);	
	//printf("%s\n", command);

	FILE* pf = NULL;
	pf = popen(command, "r");
	if (!pf)
	{
		return;
	}

	const int LS_LINE_SIZE = 120;
	char ls_line[LS_LINE_SIZE];
	int idx = 0;
	while (fgets(ls_line, LS_LINE_SIZE, pf))
	{
		idx++;
		if (idx >= logger->cfg.max)
		{
			char *basename_ = strchr(ls_line, '"');
			if (!basename_)
				continue;
			basename_++;			// move past the first "
			char* endmark = strrchr(basename_, '"');
			if (!endmark)
				continue;
			*endmark = '\0';
			
			// if this is the current file skip it
			if (strcmp(basename_, logger->cfg.logname) == 0)
				continue;

			// if it has a timestamp appended, then it must be older than the others, so delete it
			snprintf(command, sizeof(command), "rm -f %s", basename_);
			//printf("cmd = %s\n", command);
			system(command);
		}
	}
	fclose(pf);


	// create the new filename
	char ts[50];
	time_t now_ = time(NULL);
	strftime(ts, sizeof(ts), "%Y-%m-%dT%H-%M-%SZ", gmtime(&now_));
	filename_t new_filename;
	snprintf(new_filename, sizeof(new_filename), "%s.%s", logger->cfg.logname, ts);
	snprintf(command, sizeof(command), "mv %s %s", logger->cfg.logname, new_filename);
	//printf("cmd = %s\n", command);
	system(command);

	// compress the file if configured to
	if (logger->cfg.compress)
	{
		snprintf(command, sizeof(command), "gzip %s", new_filename);
		//printf("cmd = %s\n", command);
		system(command);
	}
}



//#######################################################################################
static void write_to_file(logger_t* logger, const char* data)
{
	int len = 0;
	if ((!logger) || (!logger->cfg.logname) || (strlen(logger->cfg.logname) < 1) || (!data) || ((len = strlen(data)) < 1))
		return;

	// check whether the files nede to be rotated
	rotate_files(logger, len);

	FILE* fp = NULL;
	if ((fp = open_file(logger->cfg.logname)))
	{
		fputs(data, fp);
		fclose(fp);
	}
}



//#######################################################################################
static uint32_t map_cfg_filesize(char *file_size)
{
	int len = 0;
	if ((file_size) && ((len = strlen(file_size)) >= 2))
	{
		char* strpart = NULL;
		uint32_t size = strtoul(file_size, &strpart, 10);
		if (size == 0)
			return 0;
		if (strpart != (file_size + len - 1))
			return 0;			// multipler should be last character

		if (strpart[0] == 'k')
		{
			return size *= 1000;
		}
		else if (strpart[0] == 'M')
		{
			return size *= 1000000;
		}
		else if (strpart[0] == 'G')
		{
			return size *= 1000000000;
		}
		else
			return 0;	// unknown
	}
	return 0;
}



//#######################################################################################
void* logger_create(logger_cfg_t* cfg)
{
	logger_t *logger = NULL;

	if (!cfg)
	{
		return NULL;
	}

	//################# validate the config
	// is the name defined
	if (strlen(cfg->logname) <= 0)
	{
		return NULL;
	}
	// can the file actually be opened
	FILE* fp = NULL;
	if (!(fp = open_file(cfg->logname)))
	{
		return NULL;
	}
	fclose(fp);
	// is the size str a valid definition
	uint32_t file_size = map_cfg_filesize(cfg->size);
	if (file_size == 0)
		return NULL;


	// validation is OK, so create entity
	if (!(logger = malloc(sizeof(logger_t))))
	{ 
		return NULL;
	}

	memset(logger, 0, sizeof(logger_t));

	memcpy(&logger->cfg, cfg, sizeof(logger_cfg_t));

	logger->file_size = file_size;

	// split out the dirname and basename
	filename_t tmp;
	strcpy(tmp, cfg->logname);
	char *tmp_dirname = dirname(tmp);
	strcpy(logger->dirname_, tmp_dirname);
	strcpy(tmp, cfg->logname);
	char *tmp_basename = basename(tmp);
	strcpy(logger->basename_, tmp_basename);

	return (void*)logger;
}


//#######################################################################################
void logger_log(void *logger_, const char *log, log_level_e level, log_app_t app, bool add_level, bool add_timestamp, const char* file_, int line_, const char* func_, void *cloud_logger, device_name_t device_name, char *type)
{
	logger_t* logger = (logger_t*)logger_;

	if ((!logger) || (!log) || (!type))
		return;

	// check level
	if (level < logger->cfg.level)
		return;

	g_log_[0] = '\0';

	// add timestamp if necessary
	if (add_timestamp)
	{
		char timestamp_[100];
		int len = logger_get_datetimeUTC_ms(timestamp_, sizeof(timestamp_));
		snprintf(timestamp_+len, sizeof(timestamp_) - len, "(%d)", (int)time(NULL));
		char tmp_[150];
		snprintf(tmp_, sizeof(tmp_), "%-36s ", timestamp_);
		LOG_STRCAT(tmp_);
	}

	// include the context
	if ((app) && (strlen(app) > 0))
	{
		char tmp_[LOGGER_MAX_APP_LEN + 5];
		char format_str[10];
		snprintf(format_str, sizeof(format_str), "%%-%ds ", LOGGER_MAX_APP_LEN);
		snprintf(tmp_, sizeof(tmp_), format_str, app);
		LOG_STRCAT(tmp_);
	}

	// include the level
	if (add_level)
	{
		char tmp_[20];
		snprintf(tmp_, sizeof(tmp_), "%-12s ", loglevel_2_str(level));
		LOG_STRCAT(tmp_);
	}

	// include the line
	if ((file_) && (func_))
	{
		char tmp_[60];
		char tmp2_[65];
		snprintf(tmp_, sizeof(tmp_), "%s:[%s:%d]", func_, file_, line_);
		snprintf(tmp2_, sizeof(tmp2_), "%-65s", tmp_);
		LOG_STRCAT(tmp2_);
	}

	// finally include the log itself
	LOG_STRCAT(log);
	LOG_STRCAT("\n");

	write_to_file(logger, g_log_);

	// write to cloug logger
	g_log_json[0] = '\0';
	sprintf(g_log_json, "{\"log\":\"%s\",\"level\":\"%s\"}", g_log_, loglevel_2_str(level));

	// if cloud logger configured, log with it
	if (cloud_logger && (level >= logger->cfg.cloud_level))
	{
		cloud_logger_log(cloud_logger, app, device_name, CLOUD_LOGGER_STDOUT, type, g_log_json);
	}
}


//#######################################################################################
void logger_free(void** logger)
{
	if ((logger) && (*logger))
	{
		free_and_null((void**)logger);
	}
}