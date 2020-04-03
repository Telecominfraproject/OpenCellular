#include "json-parser/json.h"
#include "json-parser/json_utils.h"

#include <stdio.h>


char test_json_string[] =
    "{                                                  "
    "    \"jsonrpc\": \"2.0\",                          "
    "    \"method\": \"spectrum.paws.init\",            "
    "    \"params\": {                                  "
    "        \"type\": \"INIT_REQ\",                    "
    "        \"version\": \"1.0\",                      "
    "        \"deviceDesc\": {                          "
    "            \"serialNumber\": \"M01D201621592159\","
    "            \"manufacturerId\": \"IPAccess\",      "
    "            \"modelId\": \"Radio\",                "
    "            \"rulesetIds\": [                      "
    "                \"ETSI-EN-301-598-1.1.1\"          "
    "            ],                                     "
    "            \"etsiEnDeviceType\": \"A\",           "
    "            \"etsiEnDeviceCategory\": \"master\",  "
    "            \"etsiEnDeviceEmissionsClass\": 3,     "
    "            \"etsiEnTechnologyId\": \"AngularJS\"  "
    "        },                                         "
    "        \"location\": {                            "
    "            \"point\": {                           "
    "                \"center\": {                      "
    "                    \"latitude\": 51.507611,       "
    "                    \"longitude\": -0.111162       "
    "                },                                 "
    "                \"semiMajorAxis\": 0,              "
    "                \"semiMinorAxis\": 0,              "
    "                \"orientation\": 0                 "
    "            },                                     "
    "            \"confidence\": 95                     "
    "        }                                          "
    "    },                                             "
    "    \"id\": 0                                      "
    "}                                                  ";


int main()
{
    {
        json_value *json = json_parse(test_json_string, sizeof(test_json_string));

        json_value *copy = json_deep_copy(json);

        char *and_back_again = json_value_2_string(json);
        puts(and_back_again);
        free(and_back_again);

        json_value_free(copy);

        // Should fail because we can't convert an json_object to a json_integer
        // (or anything else for that matter).
        bool ok = json_set_int(json, "params", 12);
        if (ok) printf("ERROR: ");
        printf("json_set_int(\"params\") ok=%d\n", ok);

        // Should fail because there is no params/missing
        ok = json_set_int(json, "params/missing/foo", 12);
        if (ok) printf("ERROR: ");
        printf("json_set_int(\"params/missing/foo\") ok=%d\n", ok);

        ok = json_set_double(json, "params/location/confidence", 12.34);
        if (!ok) printf("ERROR: ");
        printf("json_set_double(\"params/location/confidence\") ok=%d\n", ok);

        ok = json_set_bool(json, "new_boolean", true);
        if (!ok) printf("ERROR: ");
        printf("json_set_bool(\"new_boolean\") ok=%d\n", ok);

        ok = json_set_int(json, "params/location/new_key", -12);
        if (!ok) printf("ERROR: ");
        printf("json_set_int(\"params/location/new_key\") ok=%d\n", ok);

        ok = json_set_string(json, "params/type", "hello");
        if (!ok) printf("ERROR: ");
        printf("json_set_string(\"params/type\") ok=%d\n", ok);

        and_back_again = json_value_2_string(json);
        puts(and_back_again);
        free(and_back_again);

        if (json_cmp(json, json))
            puts("json_value tree is the same as itself!");
        else
            puts("ERROR: json_value tree is NOT the same as itself!");

        json_value_free(json);
    }

    {
        puts("\nNow attempting to insert a copy of the entire tree in place of the location object");

        json_value *json = json_parse(test_json_string, sizeof(test_json_string));

        if (!json_set_json_value(json, "params/location", json))
            puts("ERROR: couldn't set json value");

        char *and_back_again = json_value_2_string(json);
        puts(and_back_again);
        free(and_back_again);

        json_value_free(json);
    }

    return 0;
}
