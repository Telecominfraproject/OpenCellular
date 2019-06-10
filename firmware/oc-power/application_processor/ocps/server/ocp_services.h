#include <iostream>

#include "configHeader.h"
#include "controlHeader.h"
#include "telmHeader.h"

bool ocp_config_rtc_time_req(uint32_t time);
OCPConfigResponse* ocp_config_data_req(OCPConfigRequest* ocpConfReq);
OCPControlResponse* ocp_control_req(OCPControlRequest* ocpCtrlReq);
OCPTelemetryResponse* ocp_telemetry_data_req(OCPTelemetryRequest* ocpTelmReq);
void ocp_init_req();

