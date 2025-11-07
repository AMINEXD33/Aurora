#include "./helpers.h"


/**
 * init a Datapoint
 * ### return: 
 *  `Data *` if successful
 *  `NULL` on error
 */
Data *InitDataPoint(){
    Data *data = calloc(1, sizeof(Data));
    if (!data){
        perror("can't allocate mem for a data point\n");
        return NULL;
    }
    data->type = NONE;
    data->value = NULL;
    data->data_owned = DATA_NOT_OWNED;
    return data;
}

Array *InitDataPointArray(){
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
    return array;
}

/**
 * write data to a datapoint
 * the function checks type only if the datapoint is already writen into 
 * ### return :
 *  `void`
 */
int WriteData(Data *data, type type,void *value, bool owned){
    if (!data)
        return -1;
    if (!value)
        return -1;

    if (data->type != NONE && (data->type != type)){
        printf("trying to assign a value of type %c to a data point of type %c\n", type, data->type);
        return -1;
    }
    data->value = value;
    data->type = type;
    data->data_owned = owned;
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
        printf("trying to read a char out of a type %c data point\n", data->type);
        return NULL;
    }
    return (char*) data->value;
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
        printf("trying to read a char out of a type %c data point\n", data->type);
        return NULL;
    }
    return (int*) data->value;
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
        printf("trying to read a char out of a type %c data point\n", data->type);
        return NULL;
    }
    return (bool*) data->value;
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
        printf("trying to read a char out of a type %c data point\n", data->type);
        return NULL;
    }
    return (float*) data->value;
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
        printf("trying to read a char out of a type %c data point\n", data->type);
        return NULL;
    }
    return (double*) data->value;
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
    if (data->data_owned == true)
        free(data->value);
    free(data);
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


int main (){
    Array *array = InitDataPointArray();

    for (int x = 0; x < 100000; x++){
        Data *datapoint = InitDataPoint();
        int *num = malloc(sizeof(int));
        *num = x;
        WriteData(datapoint, INT, num, DATA_OWNED);
        append_datapoint(array,datapoint);
    }
    for (int x = 0; x < array->index; x++){
        Data *datapoint = array->array[x];
        int *value = ReadDataInt(datapoint);
    }
    printf("final size of the array %ld \n", array->size);
    free_array(array);
    return 0;
}