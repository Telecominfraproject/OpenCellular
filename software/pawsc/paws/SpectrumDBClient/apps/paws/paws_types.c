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
#include "paws_dal_types.h"



//#######################################################################################

// Return code as it per strcmp 
// p1 < p2: return -1
// p1 == p2: return 0
// p1 > p2: return +2
int ul_dl_spec_cfg_freq_compare(ul_dl_spec_cfg_t* p1, ul_dl_spec_cfg_t* p2)
{
	if ((!p1) || (!p1->dl_cfg) || (!p1->ul_cfg) || (!p2) || (!p2->dl_cfg) || (!p2->ul_cfg))
		return -1;

	// frequency
	// dl
	int ret = (p1->dl_cfg->start_hz < p2->dl_cfg->start_hz) ? -1 : ((p1->dl_cfg->start_hz == p2->dl_cfg->start_hz) ? 0 : 1);
	if (ret != 0)
		return ret;
	// ul
	ret = (p1->ul_cfg->start_hz < p2->ul_cfg->start_hz) ? -1 : ((p1->ul_cfg->start_hz == p2->ul_cfg->start_hz) ? 0 : 1);
	return ret;

}



// Return code as it per strcmp 
// p1 < p2: return -1
// p1 == p2: return 0
// p1 > p2: return +2
// Note that bandwidth is checked first, then power.
int ul_dl_spec_cfg_pwr_compare(ul_dl_spec_cfg_t* p1, ul_dl_spec_cfg_t* p2)
{
	if ((!p1) || (!p1->dl_cfg) || (!p1->ul_cfg) || (!p2) || (!p2->dl_cfg) || (!p2->ul_cfg))
		return -1;

	// bandwidth
	// dl
	int ret = (p1->dl_cfg->bandwidth < p2->dl_cfg->bandwidth) ? -1 : ((p1->dl_cfg->bandwidth == p2->dl_cfg->bandwidth) ? 0 : 1);
	if (ret != 0)
		return ret;
	// ul
	ret = (p1->ul_cfg->bandwidth < p2->ul_cfg->bandwidth) ? -1 : ((p1->ul_cfg->bandwidth == p2->ul_cfg->bandwidth) ? 0 : 1);
	if (ret != 0)
		return ret;


	// power
	// dl
	ret = (p1->dl_cfg->dbm < p2->dl_cfg->dbm) ? -1 : ((p1->dl_cfg->dbm == p2->dl_cfg->dbm) ? 0 : 1);
	if (ret != 0)
		return ret;
	// ul
	ret = (p1->ul_cfg->dbm < p2->ul_cfg->dbm) ? -1 : ((p1->ul_cfg->dbm == p2->ul_cfg->dbm) ? 0 : 1);
	return ret;

}
