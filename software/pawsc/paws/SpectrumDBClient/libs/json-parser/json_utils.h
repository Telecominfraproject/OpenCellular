#pragma once

#include <stdbool.h>

#include "json.h"

// Convert a json_value tree to a JSON document in a C string. You are
// responsible for free'ing the returned string.
char* json_value_2_string(json_value* src);

// Sets (deep copy) a json_value string param. Creates entry if leaf-key is not present, or overwrites otherwise.
bool json_set_string(json_value* dst, const char* path, const char* val);

// Set a json_value integer param. Creates entry if leaf-key is not present, or overwrites otherwise.
bool json_set_int(json_value* json_root, const char* path, int64_t val);

// Set a json_value double param. Creates entry if leaf-key is not present, or overwrites otherwise.
bool json_set_double(json_value* dst, const char* path, double val);

// Set a json_value bool param. Creates entry if leaf-key is not present, or overwrites otherwise.
bool json_set_bool(json_value* dst, const char* path, bool val);

// Deep copies src and adds it to the json tree at the specified path. Creates entry if 
// leaf-key is not present, or otherwise frees existing json_value and overwrites.
bool json_set_json_value(json_value* dst, const char* path, json_value* src);

// Deep copy a json_value param. You are responsible for free'ing the returned
// tree of json_values. Use json_value_free().
json_value *json_deep_copy(json_value* src);

// Deep compare two json_value params.
bool json_cmp(json_value* jv1, json_value* jv2);

// Searches down a tree of JSON objects for a node at the specified path.
// Returns the node if found or NULL on failure.
// path is like "foo/bar/baz". That will search first for an object entry
// named "foo", then within that node for an entry called "bar" etc.
json_value* json_get(json_value* src, const char* path);

// Helper function. Searches for the specified node then returns its data if
// it is a string type of value. Returns NULL if node not found or not of
// string type.
char *json_get_string(json_value* src, const char* path);

// Helper function. Searches for the specified node then, if it is an integer type, copies
// its value into the "val" parameter.
// Returns true if param was found and had integer type, false otherwise.
bool json_get_int(json_value* src, const char* path, int64_t *val);

// Helper function. Searches for the specified node then, if it is a double type, copies
// its value into the "val" parameter.
// Returns true if param was found and had double type, false otherwise.
bool json_get_double(json_value* src, const char* path, double *val);

// Helper function. Searches for the specified node then, if it is an bool type, copies
// its value into the "val" parameter.
// Returns true if param was found and had bool type, false otherwise.
bool json_get_bool(json_value* src, const char* path, bool *val);

// Helper function. Returns the number of keys in a json_object (which is synonymous with a dictionary)
// Returns <0 if object passed in is not a json_object
int json_get_num_keys(json_value* src);

// Helper function. Returns the Nth key (starting with n=0) in a json_object (which is synonymous with a dictionary)
// Returns NULL if object passed in is not a json_object, or if 'n' is > than the number of keys
char* json_get_key(json_value* src, unsigned int n);
