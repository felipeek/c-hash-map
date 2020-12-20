#ifndef C_HASH_MAP_H
#define C_HASH_MAP_H

/*
    Author: Felipe Einsfeld Kersting
    The MIT License

    To use this hash map, define HASH_MAP_IMPLEMENT before including hash_map.h in one of your source files.

    This hash map is not thread-safe.

    This hash map works with a single chunk of memory. The memory is pre-allocated when the hash map is created
    (the size depends on the initial capacity provided by the caller). Whenever the hash map is half-full, the chunk is reallocated internally.

    If reallocations need to be avoided, create the hash map with initial capacity two times bigger than the number of maximum elements it will have.
    For example, if the maximum number of elements is 256, create the hash table with initial capacity of 512.

    Get and put operations are optimized. The delete operation is slower, since it might result in rearranging some elements.

    For more information about the API, check the comments in the function signatures.

    A complete usage example:

    #define HASH_MAP_IMPLEMENT
    #include "hash_map.h"
    #include <stdio.h>

    #define KEY_NAME "my_key"

    static int key_compare(const void *_key1, const void *_key2) {
        const char *key1 = *(const char **)_key1;
        const char *key2 = *(const char **)_key2;
        return !strcmp(key1, key2);
    }

    static unsigned int key_hash(const void *key) {
        const char *str = *(const char **)key;
        unsigned int hash = 5381;
        int c;
        while ((c = *str++)) {
            hash = ((hash << 5) + hash) + c;
        }
        return hash;
    }

    int main() {
        Hash_Map hm;
        if (hash_map_create(&hm, 1024, sizeof(char *), sizeof(int), key_compare, key_hash)) {
            printf("error creating the hash map.\n");
            return -1;
        }
        int value = 3;
        char* key = KEY_NAME;
        if (hash_map_put(&hm, &key, &value)) {
            printf("error putting element in the hash map.\n");
            return -1;
        }
        int got;
        if (hash_map_get(&hm, &key, &got)) {
            printf("error getting element from the hash map.\n");
            return -1;
        }
        printf("I got: %d\n", got);
        return 0;
    }
*/

#include <stddef.h>

// Compares two keys. Needs to return 1 if the keys are equal, 0 otherwise.
typedef int (*Key_Compare_Func)(const void *key1, const void *key2);
// Calculates the hash of the key.
typedef unsigned int (*Key_Hash_Func)(const void *key);
// Called for every key-value pair. 'key and 'value' contain the key and the value.
// 'custom_data' contains the custom data sent in the 'hash_map_for_each_entry' call.
typedef void (*For_Each_Func)(const void *key, const void* value, void* custom_data);
// Do not change the Hash_Map struct
typedef struct {
    unsigned int capacity;
    unsigned int num_elements;
    size_t key_size;
    size_t value_size;
    Key_Compare_Func key_compare_func;
    Key_Hash_Func key_hash_func;
    void *data;
} Hash_Map;
// Creates a hash map. 'initial_capacity' indicates the initial capacity of the hash_map, in number of elements.
// 'key_compare_func' and 'key_hash_func' should be provided by the caller.
// Returns 0 if sucess, -1 otherwise.
int hash_map_create(Hash_Map *hm, unsigned int initial_capacity, size_t key_size, size_t value_size,
                    Key_Compare_Func key_compare_func, Key_Hash_Func key_hash_func);
// Put an element in the hash map.
// Returns 0 if sucess, -1 otherwise.
int hash_map_put(Hash_Map *hm, const void *key, const void *value);
// Get an element from the hash map. Note that the received element is a copy and not the actual element in the hash map.
// Returns 0 if sucess, -1 otherwise.
int hash_map_get(Hash_Map *hm, const void *key, void *value);
// Delete an element from the hash map.
// Returns 0 if sucess, -1 otherwise.
int hash_map_delete(Hash_Map *hm, const void *key);
// Destroys the hashmap, freeing the memory.
void hash_map_destroy(Hash_Map *hm);
// Calls a custom function for every entry in the hash map. The function 'for_each_func' must be provided.
// 'custom_data' is a custom pointer that is repassed in every call.
// NOTE: Do not put or delete elements to the same hash table inside the for_each_func.
// Putting or deleting elements might alter the hash table internal data structure, which will cause unexpected behavior.
void hash_map_for_each_entry(Hash_Map *hm, For_Each_Func for_each_func, void *custom_data);

#ifdef HASH_MAP_IMPLEMENT
#include <string.h>
#include <stdlib.h>

typedef struct {
    int valid;
} Hash_Map_Element_Information;

static Hash_Map_Element_Information *get_element_information(Hash_Map *hm, unsigned int index) {
    return (Hash_Map_Element_Information *)((unsigned char *)hm->data +
                                            index * (sizeof(Hash_Map_Element_Information) + hm->key_size + hm->value_size));
}

static void *get_element_key(Hash_Map *hm, unsigned int index) {
    Hash_Map_Element_Information *hmei = get_element_information(hm, index);
    return (unsigned char *)hmei + sizeof(Hash_Map_Element_Information);
}

static void *get_element_value(Hash_Map *hm, unsigned int index) {
    Hash_Map_Element_Information *hmei = get_element_information(hm, index);
    return (unsigned char *)hmei + sizeof(Hash_Map_Element_Information) + hm->key_size;
}

static void put_element_key(Hash_Map *hm, unsigned int index, const void *key) {
    void *target = get_element_key(hm, index);
    memcpy(target, key, hm->key_size);
}

static void put_element_value(Hash_Map *hm, unsigned int index, const void *value) {
    void *target = get_element_value(hm, index);
    memcpy(target, value, hm->value_size);
}

int hash_map_create(Hash_Map *hm, unsigned int initial_capacity, size_t key_size, size_t value_size,
                    Key_Compare_Func key_compare_func, Key_Hash_Func key_hash_func) {
    hm->key_compare_func = key_compare_func;
    hm->key_hash_func = key_hash_func;
    hm->key_size = key_size;
    hm->value_size = value_size;
    hm->capacity = initial_capacity > 0 ? initial_capacity : 1;
    hm->num_elements = 0;
    hm->data = calloc(hm->capacity, sizeof(Hash_Map_Element_Information) + key_size + value_size);
    if (!hm->data) {
        return -1;
    }
    return 0;
}

void hash_map_destroy(Hash_Map *hm) {
    free(hm->data);
}

static int hash_map_grow(Hash_Map *hm) {
    Hash_Map old_hm = *hm;
    if (hash_map_create(hm, old_hm.capacity << 1, old_hm.key_size, old_hm.value_size, old_hm.key_compare_func, old_hm.key_hash_func)) {
        return -1;
    }
    for (unsigned int pos = 0; pos < old_hm.capacity; ++pos) {
        Hash_Map_Element_Information *hmei = get_element_information(&old_hm, pos);
        if (hmei->valid) {
            void *key = get_element_key(&old_hm, pos);
            void *value = get_element_value(&old_hm, pos);
            if (hash_map_put(hm, key, value))
                return -1;
        }
    }
    hash_map_destroy(&old_hm);
    return 0;
}

int hash_map_put(Hash_Map *hm, const void *key, const void *value) {
    unsigned int pos = hm->key_hash_func(key) % hm->capacity;
    for (;;) {
        Hash_Map_Element_Information *hmei = get_element_information(hm, pos);
        if (!hmei->valid) {
            hmei->valid = 1;
            put_element_key(hm, pos, key);
            put_element_value(hm, pos, value);
            ++hm->num_elements;
            break;
        } else {
            void *element_key = get_element_key(hm, pos);
            if (hm->key_compare_func(element_key, key)) {
                put_element_key(hm, pos, key);
                put_element_value(hm, pos, value);
                break;
            }
        }
        pos = (pos + 1) % hm->capacity;
    }
    if ((hm->num_elements << 1) > hm->capacity) {
        if (hash_map_grow(hm)) {
            return -1;
        }
    }
    return 0;
}

int hash_map_get(Hash_Map *hm, const void *key, void *value) {
    unsigned int pos = hm->key_hash_func(key) % hm->capacity;
    for (;;) {
        Hash_Map_Element_Information *hmei = get_element_information(hm, pos);
        if (hmei->valid) {
            void *possible_key = get_element_key(hm, pos);
            if (hm->key_compare_func(possible_key, key)) {
                void *entry_value = get_element_value(hm, pos);
                memcpy(value, entry_value, hm->value_size);
                return 0;
            }
        } else {
            return -1;
        }
        pos = (pos + 1) % hm->capacity;
    }
}

static void adjust_gap(Hash_Map *hm, unsigned int gap_index) {
    unsigned int pos = (gap_index + 1) % hm->capacity;
    for (;;) {
        Hash_Map_Element_Information *current_hmei = get_element_information(hm, pos);
        if (!current_hmei->valid) {
            break;
        }
        void *current_key = get_element_key(hm, pos);
        unsigned int hash_position = hm->key_hash_func(current_key) % hm->capacity;
        unsigned int normalized_gap_index = (gap_index < hash_position) ? gap_index + hm->capacity : gap_index;
        unsigned int normalized_pos = (pos < hash_position) ? pos + hm->capacity : pos;
        if (normalized_gap_index >= hash_position && normalized_gap_index <= normalized_pos) {
            void *current_value = get_element_value(hm, pos);
            current_hmei->valid = 0;
            Hash_Map_Element_Information *gap_hmei = get_element_information(hm, gap_index);
            put_element_key(hm, gap_index, current_key);
            put_element_value(hm, gap_index, current_value);
            gap_hmei->valid = 1;
            gap_index = pos;
        }
        pos = (pos + 1) % hm->capacity;
    }
}

int hash_map_delete(Hash_Map *hm, const void *key) {
    unsigned int pos = hm->key_hash_func(key) % hm->capacity;
    for (;;) {
        Hash_Map_Element_Information *hmei = get_element_information(hm, pos);
        if (hmei->valid) {
            void *possible_key = get_element_key(hm, pos);
            if (hm->key_compare_func(possible_key, key)) {
                hmei->valid = 0;
                adjust_gap(hm, pos);
                --hm->num_elements;
                return 0;
            }
        } else {
            return -1;
        }
        pos = (pos + 1) % hm->capacity;
    }
}

void hash_map_for_each_entry(Hash_Map *hm, For_Each_Func for_each_func, void *custom_data) {
    for (unsigned int pos = 0; pos < hm->capacity; ++pos) {
        Hash_Map_Element_Information *hmei = get_element_information(hm, pos);
        if (hmei->valid) {
            void *key = get_element_key(hm, pos);
            void *value = get_element_value(hm, pos);
            for_each_func(key, value, custom_data);
        }
    }
}
#endif
#endif
