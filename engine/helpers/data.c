#include "./helpers.h"

/**
 * init a Datapoint
 * ### return: 
 *  `Data *` if successful
 *  `NULL` on error
 */
Data *InitDataPoint(char *key){
    Data *data = calloc(1, sizeof(Data));
    if (!data){
        perror("can't allocate mem for a data point\n");
        return NULL;
    }
    data->type = NONE;
    data->data_owned = DATA_NOT_OWNED;
    
    if (key){
        char *cpy_key = calloc(1, strlen(key)+1);
        strcpy(cpy_key, key);
        data->key = cpy_key;
    }
    return data;
}

/**
 * initiat an array for datapoints
 * ### return:
 *  `Array *`: if successfull
 *  `NULL`: if the structure can't be allocated or the pointers array can't be allocated
 */
Array *InitDataPointArray(char *key){
    Array *array = malloc(sizeof(Array));
    if (!array){
        printf("can'r allocate mem for array structure\n");
        return NULL;
    }
    array->array = calloc(10, sizeof(Data *));
    if (!array->array){
        printf("can't allocate mem for the array of data \n");
        return NULL;
    }

    array->index = 0;
    array->size = 10;
    if (key){
        char *cpy_key = calloc(1, strlen(key)+1);
        strcpy(cpy_key, key);
        array->key = cpy_key;
    }
    return array;
}


/**
 * write a string into a Data struct
 * ### return:
 *  `1`: if successfull
 *  `-1`: no Data struct is passed or no value is passed
 */
int WriteDataString(Data *data,char *value){
    if (!data)
        return -1;
    if (!value)
        return -1;
    char *cpy = calloc(1, strlen(value) + 1);
    strcpy(cpy, value);
    data->value.string_val = cpy;
    data->type = STRING;
    data->data_owned = DATA_OWNED;
    return 1;
}

/**
 * write a int into a Data struct
 * ### return:
 *  `1`: if successfull
 *  `-1`: no Data struct is passed
 */
int WriteDataInt(Data *data,int value){
    if (!data)
        return -1;
    data->value.int_val = value;
    data->type = INT;
    data->data_owned = DATA_NOT_OWNED;
    return 1;
}

/**
 * write a long into a Data struct
 * ### return:
 *  `1`: if successfull
 *  `-1`: no Data struct is passed 
 */
int WriteDataLong(Data *data,long value){
    if (!data)
        return -1;
    data->value.long_val = value;
    data->type = LONG;
    data->data_owned = DATA_NOT_OWNED;
    return 1;
}

/**
 * write a float into a Data struct
 * ### return:
 *  `1`: if successfull
 *  `-1`: no Data struct is passed
 */
int WriteDataFloat(Data *data,float value){
    if (!data)
        return -1;
    data->value.float_val = value;
    data->type = FLOAT;
    data->data_owned = DATA_NOT_OWNED;
    return 1;
}

/**
 * write a double into a Data struct
 * ### return:
 *  `1`: if successfull
 *  `-1`: no Data struct is passed
 */
int WriteDataDouble(Data *data,double value){
    if (!data)
        return -1;
    data->value.double_val = value;
    data->type = DOUBLE;
    data->data_owned = DATA_NOT_OWNED;
    return 1;
}

/**
 * write a bool into a Data struct
 * ### return:
 *  `1`: if successfull
 *  `-1`: no Data struct is passed
 */
int WriteDataBool(Data *data,bool value){
    if (!data)
        return -1;
    data->value.bool_val = value;
    data->type = BOOLEAN;
    data->data_owned = DATA_NOT_OWNED;
    return 1;
}

/**
 * read a string from a data type
 * ### return:
 *  `char *` if successful 
 *  `NULL` on error
 */
char *ReadDataStr(Data *data){
    if (!data)
        return NULL;
    if (data->type != STRING){
        printf("trying to read a string out of a type %c data point\n", data->type);
        return NULL;
    }
    return data->value.string_val;
}

/**
 * read an int from a data type
 * ### return:
 *  `int *` if successful 
 *  `NULL` on error
 */
int *ReadDataInt(Data *data){
    if (!data)
        return NULL;
    if (data->type != INT){
        printf("trying to read a int out of a type %c data point\n", data->type);
        return NULL;
    }
    return &data->value.int_val;
}

/**
 * read a boolean from a data type
 * ### return:
 *  `bool *` if successful 
 *  `NULL` on error
 */
bool *ReadDataBool(Data *data){
    if (!data)
        return NULL;
    if (data->type != BOOLEAN){
        printf("trying to read a bool out of a type %c data point\n", data->type);
        return NULL;
    }
    return &data->value.bool_val;
}

/**
 * read a float from a data type
 * ### return:
 *  `float *` if successful 
 *  `NULL` on error
 */
float *ReadDataFloat(Data *data){
    if (!data)
        return NULL;
    if (data->type != FLOAT){
        printf("trying to read a float out of a type %c data point\n", data->type);
        return NULL;
    }
    return &data->value.float_val;
}

/**
 * read a double from a data type
 * ### return:
 *  `double *` if successful 
 *  `NULL` on error
 */
double *ReadDataDouble(Data *data){
    if (!data)
        return NULL;
    if (data->type != DOUBLE){
        printf("trying to read a double out of a type %c data point\n", data->type);
        return NULL;
    }
    return &data->value.double_val;
}

/**
 * read a long from a data type
 * ### return:
 *  `double *` if successful 
 *  `NULL` on error
 */
long *ReadDataLong(Data *data){
    if (!data)
        return NULL;
    if (data->type != LONG){
        printf("trying to read a double out of a type %c data point\n", data->type);
        return NULL;
    }
    return &data->value.long_val;
}

/**
 * resize an array , each resize adds 1.3 the size of the array
 * ### return:
 *  `1` : resized
 *  `-1`: on error
 */
int resize(Array *array){
    size_t new_size = ceil((double)array->size * 1.3);
    Data **new_mem = realloc(array->array, new_size * sizeof(Data *));
    if (!new_mem){
        printf("couldn't reallocate memory for the array\n");
        return -1;
    }
    array->array = new_mem;
    array->size = new_size;
    return 1;
}

/**
 * free an array 
 */
void free_array(Array *array){
    for (unsigned long int index = 0; index < array->index; index++){
        FreeDataPoint(array->array[index]);
    }
    free(array->array);
    if (array->key)
        free(array->key);
    free(array);
    
}

/**
 * append a datapoint to an array 
 * ### return:
 *  `1`: if the data is appended
 *  `-1`: if the data is not appended(error accured)
 */
int append_datapoint(Array *array, Data *appended_value){
    if (!array){
        return -1;
    }
    // check bounds
    if (array->index >= array->size){
        if (resize(array) == -1){
            return -1;
        }
    }

    // append 
    array->array[array->index] = appended_value;
    array->index++;
    return 1;
}

/**
 * free the datapoint , free the value only if it's owned ,
 */
void FreeDataPoint(Data *data){
    if (!data)
        return;
    if (data->data_owned == true){
        if (data->type == STRING)
            free(data->value.string_val);
    }
    if (data->key)
        free(data->key);
    free(data);
}

/**
 * print a datapoint 
 */
void printDataPoint(Data *d, char *end_with){
    char *format = end_with;
    if (!format){
        format = "\n";
    }
    switch (d->type)
    {
    case STRING:
        printf("(str)'%s'\n", ReadDataStr(d));
        break;
    case INT:
        printf("(int)%d\n", *ReadDataInt(d));
        break;
    case LONG:
        printf("(long)%ld\n", *ReadDataLong(d));
        break;
    case FLOAT:
        printf("(float)%f\n", *ReadDataFloat(d));
        break;
    case DOUBLE:
        printf("(double)%lf\n", *ReadDataDouble(d));
        break;
    case BOOLEAN:
        printf("(bool)%d\n", *ReadDataBool(d));
    default:
        break;
    }
}

/**
 * print an array
 */
void printArray(Array *arr){
    printf("[ \n");
    for (unsigned int x = 0; x < arr->index; x++){
        Data *d = arr->array[x];
        printDataPoint(d, " ,\n");
    }
    printf("]\n");
}


// remove comment to see how it works 



// int main(){
//     // assign multiple datatypes

//     Data * string_datapoint = malloc(sizeof(Data));
//     string_datapoint->type = STRING;

//     Data * int_datapoint = malloc(sizeof(Data));
//     int_datapoint->type = INT;

//     Data *bool_datapoint = InitDataPoint();// no need to check if this is null (just testing)
//     bool_datapoint->type = BOOLEAN;

//     Data *float_datapoint = malloc(sizeof(Data));
//     float_datapoint->type = FLOAT;

//     Data *double_datapoint = malloc(sizeof(Data));
//     double_datapoint->type = DOUBLE;

//     // this int is manually allocated
//     int *numb = malloc(sizeof(int));
//     *numb = 12345;

//     float fl = 12.13;
//     double db = 12314.124254353;
//     bool bl = true;

//     // write data to the datapoints
//     WriteData(string_datapoint, STRING, "aminemeftah2214", DATA_NOT_OWNED);
//     // set the DATA_OWNED flag
//     WriteData(int_datapoint, INT, numb, DATA_OWNED);
//     WriteData(bool_datapoint, BOOLEAN, &bl, DATA_NOT_OWNED);
//     WriteData(float_datapoint, FLOAT, &fl, DATA_NOT_OWNED);
//     WriteData(double_datapoint, DOUBLE, &db, DATA_NOT_OWNED);

//     // read the data 
//     int *result_int = ReadDataInt(int_datapoint);
//     char *result_string = ReadDataStr(string_datapoint);
//     double *result_double = ReadDataDouble(double_datapoint);
//     float *result_float = ReadDataFloat(float_datapoint);
//     bool *result_bool = ReadDataBool(bool_datapoint);

//     // print the results
//     printf("int : %d\n", *result_int);
//     printf("string : %s\n", result_string);
//     printf("double : %lf\n", *result_double);
//     printf("float : %f\n", *result_float);
//     printf("bool : %d\n", *result_bool);

//     // free 
//     FreeDataPoint(string_datapoint);
//     FreeDataPoint(bool_datapoint);
//     // here the integer will be freed since we own it
//     FreeDataPoint(int_datapoint);
//     FreeDataPoint(double_datapoint);
//     FreeDataPoint(float_datapoint);
// }


// int main (){
//     Array *array = InitDataPointArray("test");

//     for (int x = 0; x < 100000; x++){
//         Data *datapoint = InitDataPoint("test2");
//         if (x % 2){
//             WriteDataFloat(datapoint, 1214.124);
//         }else{
//             WriteDataString(datapoint, "asdioahfa");
//         }
//         append_datapoint(array,datapoint);
//     }
//     for (int x = 0; x < array->index; x++){
//         Data *datapoint = array->array[x];
//         printDataPoint(datapoint, "\n");
//     }
//     printf("final size of the array %ld \n", array->size);
//     free_array(array);
//     return 0;
// }