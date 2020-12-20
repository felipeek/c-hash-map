# C Hash Map

To use this hash map, define `HASH_MAP_IMPLEMENT` before including hash_map.h in one of your source files.

This hash map is not thread-safe.

This hash map works with a single chunk of memory. The memory is pre-allocated when the hash map is created (the size depends on the initial capacity provided by the caller). Whenever the hash map is half-full, the chunk is reallocated internally.

If reallocations need to be avoided, create the hash map with initial capacity two times bigger than the number of maximum elements it will have. For example, if the maximum number of elements is 256, create the hash table with initial capacity of 512.

Get and put operations are optimized. The delete operation is slower, since it might result in rearranging some elements.

For more information about the API, check the comments in the function signatures.

A complete usage example:

```c
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
```

```
$ gcc -o main main.c
$ ./main
I got: 3
```
