#include "./helpers.h"
#include <stdarg.h>


/**
 * get_args_strict:
 *  Extracts multiple values from a cJSON object and stores pointers in argsarr.
 *
 *  Each element in argsarr will be a heap-allocated pointer:
 *    - (char *) for strings
 *    - (int *) for integers and booleans
 * 
 *  The caller owns all returned memory and must free it
 *  (either manually or using free_void_array()).
 */
void get_args_strict(cJSON *json, void **argsarr ,unsigned int argcount, ...){
  int counter = 0;
  /* Declare a variable of type va_list. */
  va_list argptr;
  /* Initialize that variable.. */
  va_start (argptr, argcount);
  for (counter = 0; counter < argcount; counter++)
  {
    // null the curr argarr
    argsarr[counter] = NULL;
    /* Get the next additional argument. */
    char *curr_arg = va_arg (argptr, char *);
    cJSON *var = cJSON_GetObjectItemCaseSensitive(json, curr_arg);
    if (!var) {
        fprintf(stderr, "Warning: key '%s' not found in JSON\n", curr_arg);
        continue;
    }
    if (cJSON_IsString(var)){
      char *string = calloc(strlen(var->valuestring)+1, sizeof(char));
      if (!string){
        perror("can't allocate memory for a string json value\n");
        continue;
      }
      strcpy(string, var->valuestring);
      argsarr[counter] = string;
    }
    else if(cJSON_IsNumber(var) || cJSON_IsBool(var)){
      int *number = calloc(1, sizeof(int));
      if (!number){
        perror("can't allocate memory for a number json value\n");
        continue;
      }
      *number = var->valueint;
      argsarr[counter] =  number;
    }
    else if (cJSON_IsObject(var)){
      argsarr[counter] = var;
    }
  }
  /* End use of the argptr variable. */
  va_end (argptr);
  return;
}
/**
 * a function that frees any void array, now depending on the array 
 * you can choose to free self or not ,
 * if you allocated from the heap then yes
 * if it's a stack allocation then no
 * ### return:
 *  `void`
 */
void free_void_array(void **array, unsigned int count, bool freeself){
  if (!array)
    return;
  for (unsigned int x = 0; x < count; x++){
    if (array[x])
      free(array[x]);
  }
  if (freeself)
    free(array);
}

// i will leave this here for future me to see how the flow is supp to be done
int main(){
  // get the file
  File_object *fileobj =  open_file_read_mode("../../core.json");
  if (!fileobj){
    return -1;
  }
  // alloc a buffer for the content
  char * buffer = read_file(fileobj);
  // parse it 
  cJSON *json = cJSON_Parse(buffer);
  // an array to hold some 3 values from the json file 
  // note : i already know the type of the args , be craful when casting
  void **argsarr = calloc(4, sizeof(void *));
  // array to hold some nested object values
  void **argsarr2 = calloc(2, sizeof(void *));
  // get the arguments, json, argument array , the cound of args, arg names.....
  // cpu here is an object 
  get_args_strict(json, argsarr, 4, "age", "name", "threads", "cpu");
  // from cpu get 2 values cores and clock
  get_args_strict(argsarr[3], argsarr2, 2, "cores", "clock");
  
  printf("cores %d\n, clock : %s\n", *(int *)argsarr2[0], (char *)argsarr2[1]);
  // valid casting needs to be accounted for
  printf("age : %d \nthreads : %d\nname : %s",
    *(int *)argsarr[0],
    *(int *)argsarr[2],
    (char *)argsarr[1]
  );
  // free the void list 
  free_void_array(argsarr, 3, true);
  // free the json obj
  cJSON_Delete(json);
  // free buffer
  free(buffer);
  // free the file object
  free_close_fileobject(fileobj);
}