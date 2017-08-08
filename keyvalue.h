#ifndef __KEYVALUE_H
#define __KEYVALUE_H
/*
* Returns the value associated with the key, or NULL
* if the key is not in the store.
*/
char *get(char *key);
/*
* Puts the <key,value> pair into the store.
* If the key already existed, its previous value
* is discarded and replaced by the new one.
*/
void put(char *key, char *value);
#endif