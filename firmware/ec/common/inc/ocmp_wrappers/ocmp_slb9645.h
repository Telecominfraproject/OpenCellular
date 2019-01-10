/*
 * ocmp_slb9645.h
 *
 *  Created on: 27-Aug-2018
 *      Author: BGH49731
 */

#include "common/inc/global/Framework.h"

#ifndef COMMON_INC_OCMP_WRAPPERS_OCMP_SLB9645_H_
#define COMMON_INC_OCMP_WRAPPERS_OCMP_SLB9645_H_

SCHEMA_IMPORT  const Driver_fxnTable SLB9645_fxnTable;

static const Driver SLB9645 = {
    .name = "SLB9645",
    .status = (Parameter[]){
    },
    .config = (Parameter[]){
    },
    .alerts = (Parameter[]){
    },
    .fxnTable = &SLB9645_fxnTable,
};




#endif /* COMMON_INC_OCMP_WRAPPERS_OCMP_SLB9645_H_ */
