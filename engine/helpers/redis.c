 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include "./helpers.h"
 
redisContext *create_redis_conn(){
  redisContext *c;

  c = redisConnect("127.0.0.1", 6379);
  if (c->err) {
      printf("error: %s\n", c->errstr);
      return NULL;
  }
  return c;
}

int cache_to_redis(
  redisContext *c, 
  uint8_t *buffer, 
  unsigned long int len,
  char *key,
  complex_structures type
){

  // construct key
  const char *prefix;
  switch(type) {
      case DATA:  prefix = "DA:"; break;
      case ARRAY: prefix = "AR:"; break;
      default:
          printf("[x] can't cache this type\n");
          return -1;
  }

  size_t nkey_len = strlen(prefix) + strlen(key) + 1; // +1 for '\0'
  char *nkey = calloc(1, nkey_len);
  if (!nkey) return -1;

  strcpy(nkey, prefix); 
  strcat(nkey, key);


  redisReply *reply = redisCommand(
    c,
    "SET %s %b",
    nkey,
    buffer, len
  );
  free(nkey);
  freeReplyObject(reply);
  return 0;
}



int cache_Data(redisContext *c, 
  Data *data, 
  char *key){
  if (!data || !key)
    return -1;
  size_t estimated_size = estimate_size_data(data);
  uint8_t *buffer = malloc(estimated_size);
  size_t  offset = 0;
  size_t n = serialize_data(data, buffer);
  cache_to_redis(c, buffer, n, key, DATA);
  free(buffer);
}

int cache_Array(  redisContext *c, 
  Array *array, 
  char *key){
  if (!array || !key)
    return -1;
  size_t estimated_size = estimate_size_array_data(array);
  uint8_t *buffer = malloc(estimated_size);
  size_t  offset = 0;
  size_t n = serialize_array_of_data(array, buffer);
  cache_to_redis(c, buffer, n, key, ARRAY);
  free(buffer);
 }


 void * get_cache_from_redis(redisContext *c, char *key, complex_structures type){
  if (!key)
    return NULL;
  // construct key
  const char *prefix;
  switch(type) {
      case DATA:  prefix = "DA:"; break;
      case ARRAY: prefix = "AR:"; break;
      default:
          printf("[x] can't cache this type\n");
          return NULL;
  }

  size_t nkey_len = strlen(prefix) + strlen(key) + 1; // +1 for '\0'
  char *nkey = malloc(nkey_len);
  if (!nkey) return NULL;


  strcpy(nkey, prefix); 
  strcat(nkey, key); 

  redisReply *reply = redisCommand(c, "GET %s", nkey);
  free(nkey);
  if (reply->type == REDIS_REPLY_STRING) {
    void *data = reply->str;
    size_t len = reply->len;



    size_t offset = 0;
    switch (type)
    {
      case DATA:
        Data *dt =  deserialize_data(data, &offset);
        freeReplyObject(reply);
        return dt;
      case ARRAY:
        Array *arr = deserialize_array_data(data, &offset);
        freeReplyObject(reply);
        return arr;
      default:
        printf("[x] can't cache this type");
        return NULL;
    }
  }
  freeReplyObject(reply);
  return NULL;
}

Array * get_Array_from_cache(redisContext *c, char *key){
  return get_cache_from_redis(c,key,ARRAY);
}

Data * get_Data_from_cache(redisContext *c, char *key){
  return get_cache_from_redis(c,key,DATA);
}