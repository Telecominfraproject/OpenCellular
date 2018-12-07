/* OC Includes */
#include <occli_common.h>

#ifndef _OCMW_SCHEMA_H_
#    define _OCMW_SCHEMA_H_

#    define OCMW_VALUE_TYPE_UINT8 1
#    define OCMW_VALUE_TYPE_INT8 2
#    define OCMW_VALUE_TYPE_UINT16 3
#    define OCMW_VALUE_TYPE_INT16 6
#    define OCMW_VALUE_TYPE_UINT32 11
#    define OCMW_VALUE_TYPE_ENUM 9
#    define OCMW_VALUE_TYPE_MFG 10
#    define OCMW_VALUE_TYPE_MODEL 4
#    define OCMW_VALUE_TYPE_GETMODEL 5
#    define OCMW_VALUE_TYPE_STRUCT 7
#    define OCMW_VALUE_TYPE_NWOP_STRUCT 12
#    define IRIDIUM_LASTERR_ERROR_CODE_OFFSET 2
#    define TWO_G_SIM_NET_OPTR_STATUS_OFFSET 2
#    define BUF_SIZE 50
#    define ENUM_BUF_SIZE 30
#    define OCMW_VALUE_TYPE_COMPLEX 3

static const char *DATA_TYPE_MAP[] = {
    [TYPE_NULL] = "NULL",     [TYPE_INT8] = "int8",
    [TYPE_UINT8] = "uint8",   [TYPE_INT16] = "int16",
    [TYPE_UINT16] = "uint16", [TYPE_INT32] = "int32",
    [TYPE_UINT32] = "uint32", [TYPE_INT64] = "int64",
    [TYPE_UINT64] = "uint64", [TYPE_STR] = "string",
    [TYPE_BOOL] = "bool",     [TYPE_ENUM] = "enum",
};

typedef enum ErrorSource {
    ERR_RC_INTERNAL = 0,
    ERR_SRC_CMS = 1,
    ERR_SRC_CME = 2
} Source;

typedef enum OperatorStat {
    TWOG_SIM_STAT_UNKNOWN = 0x00,
    TWOG_SIM_STAT_AVAILABLE = 0x01,
    TWOG_SIM_STAT_CURRENT = 0x02,
    TWOG_SIM_STAT_FORBIDDEN = 0x03,
} eOperatorStat;
#endif /* _OCMW_SCHEMA_H_ */
