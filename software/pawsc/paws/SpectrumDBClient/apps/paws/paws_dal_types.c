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

#include "paws_types.h"



//#######################################################################################
spectrum_schedule_t* spectrum_sched_new(void)
{
	spectrum_schedule_t* sched = NULL;
	if ((sched = malloc(sizeof(spectrum_schedule_t))))
	{
		memset(sched, 0, sizeof(spectrum_schedule_t));
		sched->refcnt = 1;
		return sched;
	}
	return NULL;
}

spectrum_schedule_t* spectrum_sched_vcopy(spectrum_schedule_t* sched)
{
	if (sched)
	{
		sched->refcnt++;
	}
	return sched;
}

void spectrum_sched_free(spectrum_schedule_t** sched)
{
	if ((sched) && (*sched))
	{
		(*sched)->refcnt--;
		if ((*sched)->refcnt == 0)
		{
			spec_profile_type_t* prof = (*sched)->profiles;
			spec_profile_type_t* next = NULL;
			while (prof)
			{
				next = prof->next;
				free(prof);
				prof = next;
			}
			free_and_null((void**)sched);
		}
		*sched = NULL;
	}
}

//#######################################################################################
spectrum_spec_t* spectrum_spec_new(void)
{
	spectrum_spec_t* spec = NULL;
	if ((spec = malloc(sizeof(spectrum_spec_t))))
	{
		memset(spec, 0, sizeof(spectrum_spec_t));
		spec->refcnt = 1;
		return spec;
	}
	return NULL;
}

spectrum_spec_t* spectrum_spec_vcopy(spectrum_spec_t* spec)
{
	if (spec)
	{
		spec->refcnt++;
	}
	return spec;
}

void spectrum_spec_free(spectrum_spec_t** spec)
{
	if ((spec) && (*spec))
	{
		(*spec)->refcnt--;
		if ((*spec)->refcnt == 0)
		{
			spectrum_schedule_t* sched = (*spec)->spectrum_schedules;
			spectrum_schedule_t* next = NULL;
			while (sched)
			{
				next = sched->next;
				spectrum_sched_free(&sched);
				sched = next;
			}
			free_and_null((void**)spec);
		}
		*spec = NULL;
	}
}


//#######################################################################################

avail_spectrum_t* avail_spectrum_new(void)
{
	avail_spectrum_t* spec = NULL;
	if ((spec = malloc(sizeof(avail_spectrum_t))))
	{
		memset(spec, 0, sizeof(avail_spectrum_t));
		spec->refcnt = 1;
		return spec;
	}
	return NULL;
}

avail_spectrum_t* avail_spectrum_vcopy(avail_spectrum_t* spec)
{
	if (spec)
	{
		spec->refcnt++;
	}
	return spec;
}

void avail_spectrum_free(avail_spectrum_t** avail_spec)
{
	if ((avail_spec) && (*avail_spec))
	{
		(*avail_spec)->refcnt--;
		if ((*avail_spec)->refcnt == 0)
		{
			avail_spectrum_t* s = *avail_spec;

			spectrum_spec_t* spec = s->spectrum_specs;
			spectrum_spec_t* next = NULL;
			while (spec)
			{
				next = spec->next;
				spectrum_spec_free(&spec);
				spec = next;
			}
			free_and_null((void**)avail_spec);
		}
		*avail_spec = NULL;
	}
}


//#######################################################################################

spec_cfg_t* spec_cfg_new(void)
{
	spec_cfg_t* cfg = NULL;
	if ((cfg = malloc(sizeof(spec_cfg_t))))
	{
		memset(cfg, 0, sizeof(spec_cfg_t));
		cfg->refcnt = 1;
		return cfg;
	}
	return NULL;
}

spec_cfg_t* spec_cfg_vcopy(spec_cfg_t* cfg)
{
	if (cfg)
	{
		cfg->refcnt++;
	}
	return cfg;
}

void spec_cfg_free(spec_cfg_t** cfg)
{
	if ((cfg) && (*cfg))
	{
		(*cfg)->refcnt--;
		if ((*cfg)->refcnt == 0)
		{
			if ((*cfg)->spec)
				spectrum_spec_free((spectrum_spec_t**)&((*cfg)->spec));
			if ((*cfg)->sched)
				spectrum_sched_free((spectrum_schedule_t**)&((*cfg)->sched));
			free_and_null((void**)cfg);
		}
		*cfg = NULL;
	}
}


//#######################################################################################
ul_dl_spec_cfg_t* ul_dl_spec_cfg_new(void)
{
	ul_dl_spec_cfg_t* cfg = NULL;
	if ((cfg = malloc(sizeof(ul_dl_spec_cfg_t))))
	{
		memset(cfg, 0, sizeof(ul_dl_spec_cfg_t));
		cfg->refcnt = 1;
		return cfg;
	}
	return NULL;
}

ul_dl_spec_cfg_t* ul_dl_spec_cfg_vcopy(ul_dl_spec_cfg_t* cfg)
{
	if (cfg)
	{
		cfg->refcnt++;
	}
	return cfg;
}

void ul_dl_spec_cfg_free(ul_dl_spec_cfg_t** cfg)
{
	if ((cfg) && (*cfg))
	{
		(*cfg)->refcnt--;
		if ((*cfg)->refcnt == 0)
		{
			if ((*cfg)->dl_cfg)
				spec_cfg_free(&((*cfg)->dl_cfg));
			if ((*cfg)->ul_cfg)
				spec_cfg_free(&((*cfg)->ul_cfg));
			free_and_null((void**)cfg);
		}
		*cfg = NULL;
	}
}


//#######################################################################################

sm_state_info_t* sm_state_info_new(void)
{
	sm_state_info_t* info = NULL;
	if ((info = malloc(sizeof(sm_state_info_t))))
	{
		memset(info, 0, sizeof(sm_state_info_t));
		return info;
	}
	return NULL;
}

// this is used to free from llist too
int sm_state_info_free(void* info_)
{
	sm_state_info_t* info = (sm_state_info_t*)info_;

	if (info)
	{
		free_and_null((void**)&(info->timer_info));
		avail_spectrum_free(&info->avail_spectrum_resp);
		spec_cfg_free(&info->available_spectrum);
		spec_cfg_free(&info->pending_spectrum);
		spec_cfg_free(&info->selected_spectrum);
		spec_cfg_free(&info->spectrum_in_use);
		free(info);
	}
	return 0;
}

void sm_state_info_free_and_null(sm_state_info_t** info)
{
	if ((info) && (*info))
	{
		sm_state_info_free((void*)*info);
		*info = NULL;
	}
}

paws_sm_state_info_t* paws_sm_state_info_new(void)
{
	paws_sm_state_info_t* info = NULL;
	if ((info = malloc(sizeof(paws_sm_state_info_t))))
	{
		memset(info, 0, sizeof(paws_sm_state_info_t));
		return info;
	}
	return NULL;
}

extern void paws_sm_state_info_free(paws_sm_state_info_t** info)
{
	if ((info) && (*info))
	{
		if ((*info)->ul_dl_spec) ul_dl_spec_cfg_free(&(*info)->ul_dl_spec);
		llist_free(&(*info)->state_info_ll, sm_state_info_free);
		free_and_null((void**)info);
	}
}



