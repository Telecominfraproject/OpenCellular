/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include <string.h>

#include "paws_message_templates.h"
#include "paws_utils.h"
#include "paws_common.h"

// #########################################################################################
// ### json templates
static char init_req_template[] = "{   \"jsonrpc\": \"2.0\",     \"method\": \"spectrum.paws.init\",     \"params\": {         \"type\": \"INIT_REQ\",         \"version\": \"1.0\",         \"deviceDesc\": {             \"serialNumber\": \"M01D201621592159\",             \"manufacturerId\": \"IPAccess\",             \"modelId\": \"Radio\",             \"rulesetIds\": [                \"ETSI-EN-301-598-1.1.1\"            ],            \"etsiEnDeviceType\": \"A\",            \"etsiEnDeviceCategory\": \"master\",            \"etsiEnDeviceEmissionsClass\": 3,            \"etsiEnTechnologyId\": \"AngularJS\"        },        \"location\": {            \"point\": {                \"center\": {                    \"latitude\": 51.507611,                    \"longitude\": -0.111162                },                \"semiMajorAxis\": 0,                \"semiMinorAxis\": 0,                \"orientation\": 0            }     }    },    \"id\": 0}";



static char available_spectrum_req_template[] = "{    \"jsonrpc\": \"2.0\",    \"method\": \"spectrum.paws.getSpectrum\",    \"params\": {        \"type\": \"AVAIL_SPECTRUM_REQ\",        \"version\": \"1.0\",        \"deviceDesc\": {            \"serialNumber\": \"M01D201621592159\",            \"manufacturerId\": \"IPAccess\",            \"modelId\": \"Radio\",            \"rulesetIds\": [                \"ETSI-EN-301-598-1.1.1\"            ],            \"etsiEnDeviceType\": \"A\",            \"etsiEnDeviceCategory\": \"master\",            \"etsiEnDeviceEmissionsClass\": 3,            \"etsiEnTechnologyId\": \"AngularJS\"        },        \"location\": {            \"point\": {                \"center\": {                    \"latitude\": 51.507611,                    \"longitude\": -0.111162                },                \"semiMajorAxis\": 0,                \"semiMinorAxis\": 0,                \"orientation\": 0            }       },        \"antenna\": {            \"height\": 15,            \"heightType\": \"AGL\",            \"heightUncertainty\": 0        }    },    \"id\": 0}";
static char slave_gop_available_spectrum_req_template[] = "{  \"jsonrpc\": \"2.0\",  \"method\": \"spectrum.paws.getSpectrum\",  \"params\": {    \"type\": \"AVAIL_SPECTRUM_REQ\",    \"version\": \"1.0\",    \"deviceDesc\": {      \"serialNumber\": \"M01D201621592159\",      \"manufacturerId\": \"IPAccess\",      \"modelId\": \"Radio\",      \"rulesetIds\": [        \"ETSI-EN-301-598-1.1.1\"      ],      \"etsiEnDeviceType\": \"A\",      \"etsiEnDeviceCategory\": \"master\",      \"etsiEnDeviceEmissionsClass\": \"4\",      \"etsiEnTechnologyId\": \"AngularJS\"    },    \"location\": {      \"point\": {        \"center\": {          \"latitude\": 51.507611,          \"longitude\": -0.111162        },        \"semiMajorAxis\": 0,        \"semiMinorAxis\": 0,        \"orientation\": 0      }    },    \"requestType\": \"Generic Slave\"  },  \"id\": 0}";
static char slave_sop_available_spectrum_req_template[] = "{    \"jsonrpc\": \"2.0\",    \"method\": \"spectrum.paws.getSpectrum\",    \"params\": {        \"type\": \"AVAIL_SPECTRUM_REQ\",        \"version\": \"1.0\",        \"deviceDesc\": {            \"serialNumber\": \"S01D201621592159\",            \"manufacturerId\": \"IPAccess\",            \"modelId\": \"Radio\",            \"rulesetIds\": [                \"ETSI-EN-301-598-1.1.1\"            ],            \"etsiEnDeviceType\": \"B\",            \"etsiEnDeviceCategory\": \"slave\",            \"etsiEnDeviceEmissionsClass\": 5,            \"etsiEnTechnologyId\": \"AngularJS\"        },        \"location\": {            \"point\": {                \"center\": {                    \"latitude\": 51.507611,                    \"longitude\": -0.111162                },                \"semiMajorAxis\": 0,                \"semiMinorAxis\": 0,                \"orientation\": 0            }       },        \"antenna\": {            \"height\": 15,            \"heightType\": \"AGL\",            \"heightUncertainty\": 0        },        \"masterDeviceDesc\": {            \"serialNumber\": \"M01D201621592159\",            \"manufacturerId\": \"IPAccess\",            \"modelId\": \"Radio\",            \"rulesetIds\": [                \"ETSI-EN-301-598-1.1.1\"            ],            \"etsiEnDeviceType\": \"A\",            \"etsiEnDeviceCategory\": \"master\",            \"etsiEnDeviceEmissionsClass\": 3,            \"etsiEnTechnologyId\": \"AngularJS\"        },        \"masterDeviceLocation\": {            \"point\": {                \"center\": {                    \"latitude\": 51.507611,                    \"longitude\": -0.111162                },                \"semiMajorAxis\": 0,                \"semiMinorAxis\": 0,                \"orientation\": 0            }    }    },    \"id\": 0}";
static char spectrum_use_notify_req_template[] = "{    \"jsonrpc\": \"2.0\",     \"method\": \"spectrum.paws.notifySpectrumUse\",    \"id\": 0,     \"params\": {      \"type\": \"SPECTRUM_USE_NOTIFY\",      \"version\": \"1.0\",      \"deviceDesc\": {        \"serialNumber\": \"M01D201621592159\",        \"manufacturerId\": \"IPAccess\",        \"modelId\": \"Radio\",        \"rulesetIds\": [          \"ETSI-EN-301-598-1.1.1\"        ],        \"etsiEnDeviceType\": \"A\",        \"etsiEnDeviceCategory\": \"master\",        \"etsiEnDeviceEmissionsClass\": 3,        \"etsiEnTechnologyId\": \"AngularJS\"      },      \"location\": {        \"point\": {          \"center\": {            \"latitude\": 51.507611,            \"longitude\": -0.111162          },          \"semiMajorAxis\": 0,          \"semiMinorAxis\": 0,          \"orientation\": 0        }   },      \"spectra\": []    }  }";
static char gop_slave_spectrum_use_notify_req_template[] = "{    \"jsonrpc\": \"2.0\",     \"method\": \"spectrum.paws.notifySpectrumUse\",    \"id\": 0,    \"params\": {      \"type\": \"SPECTRUM_USE_NOTIFY\",      \"version\": \"1.0\",      \"deviceDesc\": {        \"serialNumber\": \"S01D201621592159\",        \"manufacturerId\": \"IPAccess\",        \"modelId\": \"Radio\",        \"rulesetIds\": [          \"ETSI-EN-301-598-1.1.1\"        ],        \"etsiEnDeviceType\": \"A\",        \"etsiEnDeviceCategory\": \"slave\",        \"etsiEnDeviceEmissionsClass\": 3,        \"etsiEnTechnologyId\": \"AngularJS\"      },      \"masterDeviceDesc\": {        \"serialNumber\": \"M01D201621592159\",        \"manufacturerId\": \"IPAccess\",        \"modelId\": \"Radio\",        \"rulesetIds\": [          \"ETSI-EN-301-598-1.1.1\"        ],        \"etsiEnDeviceType\": \"A\",        \"etsiEnDeviceCategory\": \"master\",        \"etsiEnDeviceEmissionsClass\": 3,        \"etsiEnTechnologyId\": \"AngularJS\"      },      \"masterDeviceLocation\": {        \"point\": {          \"center\": {            \"latitude\": 51.507611,            \"longitude\": -0.111162          },          \"semiMajorAxis\": 0,          \"semiMinorAxis\": 0,          \"orientation\": 0        }    },  \"spectra\": [ ]    }  }";
static char sop_slave_spectrum_use_notify_req_template[] = "{    \"jsonrpc\": \"2.0\",     \"method\": \"spectrum.paws.notifySpectrumUse\",    \"id\": 0,    \"params\": {      \"type\": \"SPECTRUM_USE_NOTIFY\",      \"version\": \"1.0\",      \"deviceDesc\": {        \"serialNumber\": \"S01D201621592159\",        \"manufacturerId\": \"IPAccess\",        \"modelId\": \"Radio\",        \"rulesetIds\": [          \"ETSI-EN-301-598-1.1.1\"        ],        \"etsiEnDeviceType\": \"A\",        \"etsiEnDeviceCategory\": \"slave\",        \"etsiEnDeviceEmissionsClass\": 3,        \"etsiEnTechnologyId\": \"AngularJS\"      },      \"masterDeviceDesc\": {        \"serialNumber\": \"M01D201621592159\",        \"manufacturerId\": \"IPAccess\",        \"modelId\": \"Radio\",        \"rulesetIds\": [          \"ETSI-EN-301-598-1.1.1\"        ],        \"etsiEnDeviceType\": \"A\",        \"etsiEnDeviceCategory\": \"master\",        \"etsiEnDeviceEmissionsClass\": 3,        \"etsiEnTechnologyId\": \"AngularJS\"      },      \"location\": {        \"point\": {          \"center\": {            \"latitude\": 51.507611,            \"longitude\": -0.111162          },          \"semiMajorAxis\": 0,          \"semiMinorAxis\": 0,          \"orientation\": 0        }    },  \"masterDeviceLocation\": {        \"point\": {          \"center\": {            \"latitude\": 51.507611,            \"longitude\": -0.111162          },          \"semiMajorAxis\": 0,          \"semiMinorAxis\": 0,          \"orientation\": 0        }    },  \"spectra\": [ ]    }  }";


//#######################################################################################
static json_value* str2json(void* sm_, char* j_str)
{
	json_value* jval = NULL;

	if ((!j_str) || (strlen(j_str) == 0))
	{
		LOG_PRINT(sm_, LOG_ERROR, "Unable to parse data");
		return NULL;
	}

	// we have the contents, now convert to json
	if (!(jval = json_parse((json_char*)j_str, strlen(j_str))))
	{
		LOG_PRINT(sm_, LOG_ERROR, "Unable to parse data");
		return NULL;
	}

	// success
	return jval;
}



//#######################################################################################
json_value* get_init_req_template(void* sm_)
{
	return str2json(sm_, init_req_template);
}

//#######################################################################################
json_value* get_available_spectrum_req_template(void* sm_)
{
	return str2json(sm_, available_spectrum_req_template);
}

//#######################################################################################
json_value* get_slave_gop_available_spectrum_req_template(void* sm_)
{
	return str2json(sm_, slave_gop_available_spectrum_req_template);
}

//#######################################################################################
json_value* get_slave_sop_available_spectrum_req_template(void* sm_)
{
	return str2json(sm_, slave_sop_available_spectrum_req_template);
}

//#######################################################################################
json_value* get_spectrum_use_notify_req_template(void* sm_)
{
	return str2json(sm_, spectrum_use_notify_req_template);
}

//#######################################################################################
json_value* get_gop_slave_spectrum_use_notify_req_template(void* sm_)
{
	return str2json(sm_, gop_slave_spectrum_use_notify_req_template);
}

//#######################################################################################
json_value* get_sop_slave_spectrum_use_notify_req_template(void* sm_)
{
	return str2json(sm_, sop_slave_spectrum_use_notify_req_template);
}













