// Own header
#include "json_utils.h"

// Project headers
#include "json.h"
#include "string-buffer/string_buffer.h"

// Standard headers
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>


static void json_write_internal(StrBuf *str_buf, json_value *json);


static void write_object(StrBuf *str_buf, json_value *json)
{
    strbuf_append_cstr(str_buf, "{ ");
    for (unsigned i = 0; i < json->u.object.length; i++) {
        strbuf_append_cstr(str_buf, "\"");
        strbuf_append_cstr(str_buf, json->u.object.values[i].name);
        strbuf_append_cstr(str_buf, "\": ");
        json_write_internal(str_buf, json->u.object.values[i].value);
        if (i < (json->u.object.length - 1))
            strbuf_append_cstr(str_buf, ", ");
        else
            strbuf_append_cstr(str_buf, " ");
    }
    strbuf_append_cstr(str_buf, "}");
}


static void write_array(StrBuf *str_buf, json_value *json)
{
    strbuf_append_cstr(str_buf, "[ ");
    for (unsigned i = 0; i < json->u.array.length; i++) {
        json_write_internal(str_buf, json->u.array.values[i]);
        if (i < (json->u.array.length - 1))
            strbuf_append_cstr(str_buf, ", ");
        else
            strbuf_append_cstr(str_buf, " ");
    }
    strbuf_append_cstr(str_buf, "]");
}


static void write_integer(StrBuf *str_buf, json_value *json)
{
    char buf[22];
    snprintf(buf, sizeof(buf), "%" PRId64, json->u.integer);
    strbuf_append_cstr(str_buf, buf);
}


static void write_double(StrBuf *str_buf, json_value *json)
{
    char buf[22];
    snprintf(buf, sizeof(buf), "%.6f", json->u.dbl);
    strbuf_append_cstr(str_buf, buf);
}


static void write_string(StrBuf *str_buf, json_value *json)
{
    strbuf_append_cstr(str_buf, "\"");
    strbuf_append_cstr(str_buf, json->u.string.ptr);
    strbuf_append_cstr(str_buf, "\"");
}


static void write_boolean(StrBuf *str_buf, json_value *json)
{
    if (json->u.boolean)
        strbuf_append_cstr(str_buf, "true");
    else
        strbuf_append_cstr(str_buf, "false");
}


static void json_write_internal(StrBuf *str_buf, json_value *json)
{
    switch (json->type) {
    case json_object:   write_object(str_buf, json);     break;
    case json_array:    write_array(str_buf, json);      break;
    case json_integer:  write_integer(str_buf, json);    break;
    case json_double:   write_double(str_buf, json);     break;
    case json_string:   write_string(str_buf, json);     break;
    case json_boolean:  write_boolean(str_buf, json);    break;
        break;
    default:
        break;
    }
}


char* json_value_2_string(json_value* json)
{
    StrBuf *str_buf = strbuf_alloc();
    json_write_internal(str_buf, json);
    char *rv = strbuf_to_cstr(str_buf);
    strbuf_free(str_buf);
    return rv;
}


static bool does_name_start_path(const char *name, const char *path)
{
    while (*name && *path) {
        if (*name != *path)
            return false;
        name++;
        path++;
    }

    if (*name == '\0' && (*path == '\0' || *path == '/'))
        return true;

    return false;
}


static const char *get_next_part_of_path(const char *path)
{
    const char *rv = strchr(path, '/');
    if (!rv)
        return NULL;
    return rv + 1;
}


// Walks the tree looking for the json_value at the specified path.
// Cases:
// 1. value isn't an object. Return NULL.
// 2. path is foo and not present in value. Return NULL but set parent.
// 3. path is foo and is present in value. Return value and set parent.
// 4. path is foo/bar and foo is not present in value. Return NULL.
static json_value *find_node(json_value *value, const char *path, json_value **parent)
{
    while (value) {
        if (value->type != json_object)
            return NULL;

        json_value *child_value = NULL;
        for (unsigned i = 0; i < value->u.object.length; i++)
            if (does_name_start_path(value->u.object.values[i].name, path))
                child_value = value->u.object.values[i].value;

        const char *next_part_of_path = get_next_part_of_path(path);
        if (!next_part_of_path) {
            *parent = value;
            return child_value;
        }

        path = next_part_of_path;
        value = child_value;
    }

    return NULL;
}


static json_value* add_item_to_object(json_value *parent, const char *item_name)
{
    // The memory allocation scheme of json_parse_ex is "optimized" so that
    // the object_entries array and the entry names are allocated in the
    // same block. As a result, the free we do below will free the object entry
    // names. So, we need to allocate a new single block of memory
    // big enough for:
    //   * Our new array of object entries (one bigger than the current
    //     array).
    //   * Plus, all the name strings referenced by all the existing object
    //     entries.
    //   * Plus, our new object entry name.

    // Count the length of all the names
    size_t total_names_length = 0;
    for (unsigned i = 0; i < parent->u.object.length; i++)
        total_names_length += strlen(parent->u.object.values[i].name) + 1;
    size_t old_names_length = total_names_length;
    total_names_length += strlen(item_name) + 1;

    // Now we need to allocate a block of memory, where different parts of it
    // have different types.
    size_t num_bytes_needed_by_new_object_entries_array = sizeof(json_object_entry) * (parent->u.object.length + 1);
    void *stupid_polymorphic_block = malloc(total_names_length + num_bytes_needed_by_new_object_entries_array);

    // Copy in the old array contents
    json_object_entry *new_object_entries = stupid_polymorphic_block;
    memcpy(new_object_entries, parent->u.object.values, sizeof(json_object_entry) * parent->u.object.length);

    // Copy in the object entry names
    char *old_names = (char*)(parent->u.object.values + parent->u.object.length);
    char *new_names = (char*)stupid_polymorphic_block + num_bytes_needed_by_new_object_entries_array;
    memcpy(new_names, old_names, old_names_length);

    // Append the new name
    strcpy(new_names + old_names_length, item_name);

    // Patch up all the object entry name pointers to point into the new block
    char *current_string = new_names;
    for (unsigned i = 0; i < parent->u.object.length + 1; i++) {
        new_object_entries[i].name = current_string;
        current_string += strlen(current_string) + 1;
    }

    free(parent->u.object.values);
    parent->u.object.values = new_object_entries;

    json_object_entry *new_entry = new_object_entries + parent->u.object.length;
    new_entry->value = malloc(sizeof(json_value));
    new_entry->value->parent = parent;
    new_entry->value->type = json_null;
    parent->u.object.length++;

    return new_entry->value;
}


static json_value* add_or_get_leaf(json_value* root, const char* path)
{
    json_value *parent = NULL;
    json_value *node = find_node(root, path, &parent);
    if (!node && !parent)
        return NULL;

    if (!node) {
        if (parent->type != json_object)
            return NULL;

        // OK, item doesn't exist, but parent is an object, so we can add it.
        const char *item_name = get_next_part_of_path(path);
        if (!item_name)
            item_name = path;
        node = add_item_to_object(parent, item_name);
    }
    else {
        // If the node is not a leaf node, then things get too complex for the current implementation
        if (node->type != json_boolean && node->type != json_integer &&
            node->type != json_double && node->type != json_string)
            return NULL;

        // OK, node exists and is a leaf. This is easy!
    }

    return node;
}


bool json_set_string(json_value* dst, const char* path, const char* val)
{
    json_value* leaf = add_or_get_leaf(dst, path);
    if (leaf) {
        if (leaf->type == json_string)
            free(leaf->u.string.ptr);
        leaf->type = json_string;
        leaf->u.string.ptr = strdup(val);
        leaf->u.string.length = strlen(val);
        return true;
    }

    return false;
}


bool json_set_int(json_value* dst, const char* path, int64_t val)
{
    json_value* leaf = add_or_get_leaf(dst, path);
    if (leaf) {
        if (leaf->type == json_string)
            free(leaf->u.string.ptr);
        leaf->type = json_integer;
        leaf->u.integer = val;
        return true;
    }

    return false;
}


bool json_set_double(json_value* dst, const char* path, double val)
{
    json_value* leaf = add_or_get_leaf(dst, path);
    if (leaf) {
        if (leaf->type == json_string)
            free(leaf->u.string.ptr);
        leaf->type = json_double;
        leaf->u.dbl = val;
        return true;
    }

    return false;
}


bool json_set_bool(json_value* dst, const char* path, bool val)
{
    json_value* leaf = add_or_get_leaf(dst, path);
    if (leaf) {
        if (leaf->type == json_string)
            free(leaf->u.string.ptr);
        leaf->type = json_boolean;
        leaf->u.boolean = val;
        return true;
    }

    return false;
}


bool json_set_json_value(json_value* dst, const char* path, json_value* src)
{
    json_value *parent = NULL;
    json_value *node_to_free = find_node(dst, path, &parent);
    
    if (!parent)
        return false;

    json_value *src_deep_copy = json_deep_copy(src);
    json_value_free(node_to_free);
    const char *last_part_of_path = strrchr(path, '/') + 1;

    // Find the index of the pointer to the json_value that we need to replace.
    // Todo - maybe this loop could be moved out into a separate function
    // and that function could be called by find_node()
    for (unsigned i = 0; i < parent->u.object.length; i++) {
        json_object_entry *joe = parent->u.object.values + i;

        if (strcmp(joe->name, last_part_of_path) == 0) {
            joe->value = src_deep_copy;
            src_deep_copy->parent = parent;
            return true;
        }       
    }

    // There is no json_object_entry with name == last_part_of_path, so we need
    // to add one.
    dst = add_item_to_object(parent, last_part_of_path);
    parent->u.object.values[parent->u.object.length - 1].value = src_deep_copy;

    return true;
}


bool json_set_list(json_value* dst, const char* path, json_value* src)
{
    (void)dst;
    (void)path;
    (void)src;
    return true;
}


static void copy_object(json_value *src, json_value *dst)
{
    dst->u.object.length = src->u.object.length;

    // Count the length of all the names
    size_t total_names_length = 0;
    for (unsigned i = 0; i < src->u.object.length; i++)
        total_names_length += strlen(src->u.object.values[i].name) + 1;

    size_t num_bytes_needed_by_new_object_entries_array = sizeof(json_object_entry) * src->u.object.length;
    void *stupid_polymorphic_block = malloc(total_names_length + num_bytes_needed_by_new_object_entries_array);

    // Copy in the old array contents
    json_object_entry *new_object_entries = stupid_polymorphic_block;
    dst->u.object.values = new_object_entries;
    memcpy(new_object_entries, src->u.object.values, sizeof(json_object_entry) * src->u.object.length);

    // Copy in the object entry names
    char *old_names = (char*)(src->u.object.values + src->u.object.length);
    char *new_names = (char*)stupid_polymorphic_block + num_bytes_needed_by_new_object_entries_array;
    memcpy(new_names, old_names, total_names_length);

    // Patch up all the object entry name pointers to point into the new block
    char *current_string = new_names;
    for (unsigned i = 0; i < src->u.object.length; i++) {
        new_object_entries[i].name = current_string;
        current_string += strlen(current_string) + 1;
    }

    // Recursively copy all the child values
    for (unsigned i = 0; i < src->u.object.length; i++) {
        new_object_entries[i].value = json_deep_copy(src->u.object.values[i].value);
        new_object_entries[i].value->parent = dst;
    }
}


json_value *json_deep_copy(json_value* src)
{
    json_value *dst = calloc(1, sizeof(json_value));
    dst->type = src->type;

    switch (src->type) {
    case json_object:
        copy_object(src, dst);
        break;

    case json_array:
        dst->u.array.length = src->u.array.length;
        dst->u.array.values = calloc(dst->u.array.length, sizeof(json_value*));
        // Recursively copy all the child values
        for (unsigned i = 0; i < src->u.array.length; i++) {
            dst->u.array.values[i] = json_deep_copy(src->u.array.values[i]);
            dst->u.array.values[i]->parent = dst;
        }
        break;

    case json_integer:
        dst->u.integer = src->u.integer;
        break;

    case json_double:
        dst->u.dbl = src->u.dbl;
        break;

    case json_string:
        dst->u.string.ptr = strdup(src->u.string.ptr);
        dst->u.string.length = src->u.string.length;
        break;

    case json_boolean:
        dst->u.boolean = src->u.boolean;
        break;

    default:
        assert(0); // Should never happen
        break;
    }

    return dst;
}


bool json_cmp(json_value* v1, json_value* v2)
{
    if (v1->type != v2->type)
        return false;

    switch (v1->type) {
    case json_object:
        if (v1->u.object.length != v2->u.object.length)
            return false;
        for (unsigned i = 0; i < v1->u.object.length; i++) {
            json_object_entry *e1 = v1->u.object.values + i;
            json_object_entry *e2 = v2->u.object.values + i;
            if (strcmp(e1->name, e2->name) != 0)
                return false;
            if (!json_cmp(e1->value, e2->value))
                return false;
        }
        break;

    case json_array:
        if (v1->u.array.length != v2->u.array.length)
            return false;
        for (unsigned i = 0; i < v1->u.array.length; i++) {
            if (!json_cmp(v1->u.array.values[i], v2->u.array.values[i]))
                return false;
        }
        break;

    case json_integer: return v1->u.integer == v2->u.integer;
    case json_double: return fabs(v1->u.dbl - v2->u.dbl) < 0.0001f;
    case json_string: return strcmp(v1->u.string.ptr, v2->u.string.ptr) == 0;
    case json_boolean: return v1->u.boolean == v2->u.boolean;

    default:
        assert(0); // Should never happen
        break;
    }

    return true;
}


json_value* json_get(json_value* src, const char* path)
{
    json_value *parent;
    return find_node(src, path, &parent);
}


char* json_get_string(json_value* src, const char* path)
{
    json_value* v = json_get(src, path);

    if (v && v->type == json_string)
        return v->u.string.ptr;

    return NULL;
}


bool json_get_int(json_value* src, const char* path, int64_t *val)
{
    json_value* v = json_get(src, path);

    if (v && v->type == json_integer) {
        *val = v->u.integer;
        return true;
    }

    return false;
}


bool json_get_double(json_value* src, const char* path, double *val)
{
    json_value* v = json_get(src, path);

    if (v && v->type == json_double) {
        *val = v->u.dbl;
        return true;
    }

    return false;
}


bool json_get_bool(json_value* src, const char* path, bool *val)
{
    json_value* v = json_get(src, path);

    if (v && v->type == json_boolean) {
        *val = (bool)v->u.boolean;
        return true;
    }

    return false;
}


json_value* json_get_deep(json_value* src, const char* path)
{
    (void)src;
    (void)path;
    return NULL;
}


void json_value_print(json_value* val)
{
    (void)val;
}


// Helper function. Returns the number of keys in a json_object (which is synonymous with a dictionary)
// Returns <0 if object passed in is not a json_object
int json_get_num_keys(json_value* src)
{
	if (src && src->type == json_object) 
	{
		return src->u.object.length;
	}
	return -1;
}


// Helper function. Returns the Nth key in a json_object (which is synonymous with a dictionary)
// Returns NULL if object passed in is not a json_object, or if 'n' is > than the number of keys
char* json_get_key(json_value* src, unsigned int n)
{
	if (src && src->type == json_object && (n < src->u.object.length))
	{
		return src->u.object.values[n].name;
	}
	return NULL;
}

