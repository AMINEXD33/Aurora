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