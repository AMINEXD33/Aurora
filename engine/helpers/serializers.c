#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "helpers.h"
#include "../core/clientserver/clientserver.h"


bool check_buffer(uint8_t *buffer, char* funcname){
    if (!buffer){
        printf("[%s] no buffer is passed\n", funcname);
        return false;
    }
    return true;
}
bool check_data(Data *data, char* funcname){
    if (!data){
        printf("[%s] no data is passed\n", funcname);
        return false;
    }
    return true;
}
bool check_offset(size_t *offset, char* funcname){
    if (!offset){
        printf("[%s] no offset is passed\n", funcname);
        return false;
    }
    return true;
}
bool check_arr(Array *arr, char* funcname){
    if (!arr){
        printf("[%s] no arr is passed\n", funcname);
        return false;
    }
    return true;
}
/**
 * serialize a Data struct and return the 
 * size of the serialized data
 * ### return:
 *  `size_t`:  writen size
 */
size_t serialize_data(Data *d, uint8_t *buffer) {
    if (!check_data(d, "serialize_data") || !check_buffer(buffer, "serialize_data")){
        return 0;
    }
    size_t offset = 0;

    // 1 byte: store the type
    buffer[offset++] = (uint8_t)d->type;

    switch(d->type) {
        case INT:
            // For INT, just copy 4 bytes
            memcpy(buffer + offset, &d->value.int_val, sizeof(int));
            offset += sizeof(int);
            break;
        case LONG:
            // For INT, just copy 4 bytes
            memcpy(buffer + offset, &d->value.long_val, sizeof(long));
            offset += sizeof(long);
            break;

        case FLOAT:
            memcpy(buffer + offset, &d->value.float_val, sizeof(float));
            offset += sizeof(float);
            break;

        case DOUBLE:
            memcpy(buffer + offset, &d->value.double_val, sizeof(double));
            offset += sizeof(double);
            break;

        case BOOLEAN:
            // For BOOL, store as 1 byte (0 or 1)
            buffer[offset++] = *((bool*)&d->value.bool_val) ? 1 : 0;
            break;

        case STRING: {
            // For STRING, store length first (4 bytes)
            uint32_t len = strlen((char*)d->value.string_val);
            memcpy(buffer + offset, &len, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            // Then store the string bytes (without null terminator)
            memcpy(buffer + offset, d->value.string_val, len);
            offset += len;
            break;
        }
    }
    // if key exist , serialize the key
    if (d->key){
        uint32_t len = strlen(d->key);
        memcpy(buffer + offset, &len, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        // Then store the string bytes (without null terminator)
        memcpy(buffer + offset, d->key, len);
        offset += len;
    }else{
        uint32_t len = 0;
        memcpy(buffer + offset, &len, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
    return offset; // total bytes written
}

/**
 * serialize an array(Array) and return the writen
 * size
 * ### return:
 *  `size_t`: writen size
 */
size_t serialize_array_of_data(Array *arr, uint8_t *buffer){
    if (!check_arr(arr, "serialize_array_of_data") 
            || !check_buffer(buffer, "serialize_array_of_data")){
        return 0;
    }
    size_t offset = 0;
    memcpy(buffer + offset, &arr->index, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Recursively serialize each element
    for(unsigned long i=0; i<arr->index; i++) {
        offset += serialize_data(arr->array[i], buffer + offset);
    }
        // if key exist , serialize the key
    if (arr->key){
        uint32_t len = strlen((char*)arr->key);
        memcpy(buffer + offset, &len, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        // Then store the string bytes (without null terminator)
        memcpy(buffer + offset, arr->key, len);
        offset += len;
    }else{
        // indicate that it's length is 0
        uint32_t len = 0;
        memcpy(buffer + offset, &len, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
    return offset;
}



/**
 * deserialize data (Data) frim a buffer
 * ### return:
 *  `Data *`: if deserialization worked
 *  `NULL`: on error
 */
Data* deserialize_data(uint8_t *buffer, size_t *offset) {
    if (!check_buffer(buffer, "deserialize_data") 
            || !check_offset(offset, "deserialize_data")){
        return NULL;
    }
    Data *d = InitDataPoint(NULL);
    if (!d){
        printf("can't allocate datapoint for deserialized data\n");
    }
    // Read type
    d->type = (type)buffer[(*offset)++];
    d->data_owned = false;

    bool error_detected = false;
    switch(d->type) {
        case INT:
            int int_numb;
            memcpy(&int_numb, buffer + *offset, sizeof(int));
            WriteDataInt(d, int_numb);
            *offset += sizeof(int);
            break;
        case FLOAT:
            float float_numb;
            memcpy(&float_numb, buffer + *offset, sizeof(float));
            WriteDataFloat(d, float_numb);
            *offset += sizeof(float);
            break;
        case LONG:
            long long_numb;
            memcpy(&long_numb, buffer + *offset, sizeof(long));
            WriteDataLong(d, long_numb);
            *offset += sizeof(long);
            break;

        case DOUBLE:
            double double_numb;
            memcpy(&double_numb, buffer + *offset, sizeof(double));
            WriteDataDouble(d, double_numb);
            *offset += sizeof(double);
            break;
        case BOOLEAN:
            bool bool_numb;
            memcpy(&bool_numb, buffer + *offset, sizeof(bool));
            WriteDataBool(d, &bool_numb);
            *offset += sizeof(bool);
            break;
        case STRING: {
            // Read string length
            uint32_t len;
            memcpy(&len, buffer + *offset, sizeof(uint32_t));
            *offset += sizeof(uint32_t);

            // Allocate memory and copy string
            d->value.string_val = calloc(1, len + 1);
            if (!d->value.string_val){
                printf("can't malloc size of string for deserialization\n");
                error_detected = true;
                break;
            }
            memcpy(d->value.string_val, buffer + *offset, len);
            ((char*)d->value.string_val)[len] = '\0'; // null-terminate
            *offset += len;
            d->data_owned = true;
            break;
        }
    }
    // read key value
    uint32_t len;
    memcpy(&len, buffer + *offset, sizeof(uint32_t));
    *offset += sizeof(uint32_t);

    if (len > 0){
        // Allocate memory and copy string
        d->key = calloc(1, len + 1);
        if (!d->key){
            printf("can't malloc size of string for deserialization\n");
            error_detected = true;
        }
        memcpy(d->key, buffer + *offset, len);
        ((char*)d->key)[len] = '\0'; // null-terminate
        *offset += len;
    }

    // if error, free the dataobject and return NULL
    if (error_detected){
        FreeDataPoint(d);
        return NULL;
    }
    return d;
}

/**
 * deserialize a buffer to an Array structure
 * ### return:
 *  `Array *`: deserrialization worked
 *  `NULL`: on error
 */
Array *deserialize_array_data(uint8_t *buffer, size_t *offset){
    if (!check_buffer(buffer, "deserialize_array_data") 
            || !check_offset(offset, "deserialize_array_data")){
        return NULL;
    }
    // not using the Init func for array here , since we're assigning 
    // the datapoints manually
    Array *arr = calloc(1, sizeof(Array));
    if (!arr){
        printf("can't allocate array struct for deserialization\n");
        return NULL;
    }
    // Read number of elements
    memcpy(&arr->index, buffer + *offset, sizeof(uint32_t));
    *offset += sizeof(uint32_t);

    arr->size = arr->index;
    arr->array = calloc(1, sizeof(Data*) * arr->index);
    if (!arr->array){
        printf("can't allocate array pointers for deserialization\n");
        free(arr);
        return NULL;
    }
    // Recursively deserialize each element
    for(unsigned long i=0; i<arr->index; i++) {
        arr->array[i] = deserialize_data(buffer, offset);
    }

    // read key value
    uint32_t len;
    memcpy(&len, buffer + *offset, sizeof(uint32_t));
    *offset += sizeof(uint32_t);

    if (len > 0){
        // Allocate memory and copy string
        arr->key = calloc(1, len + 1);
        if (!arr->key){
            printf("can't malloc size of string for deserialization\n");
        }
        memcpy(arr->key, buffer + *offset, len);
        ((char*)arr->key)[len] = '\0'; // null-terminate
        *offset += len;
    }
    return arr;
}

/**
 * calculate the size that should fit a deserialized Data struct
 * ### return:
 *  `size_t` : the size of buffer
 *      `0`: if no pointer is passed
 */
size_t estimate_size_data(Data *d) {
    if (!check_data(d, "estimate_size_data")){
        return 0;
    }
    size_t sz = 1;
    switch(d->type) {
        case INT:  sz += sizeof(int); break;
        case LONG: sz += sizeof(long); break;
        case FLOAT:  sz += sizeof(float); break;
        case DOUBLE:  sz += sizeof(double); break;
        case BOOLEAN:  sz += 1; break;
        case STRING: {
            uint32_t len = strlen((char*)d->value.string_val);
            sz += sizeof(uint32_t) + len;
            break;
        }
    }
    if (d->key){
        uint32_t len = strlen((char*)d->key);
        sz += sizeof(uint32_t) + len;
    }
    return sz;
}

/**
 * calculate the size that should fit a deserialized Array struct
 * ### return:
 *  `size_t` : the size of buffer
 *      `0`: if no pointer is passed
 */
size_t estimate_size_array_data(Array *arr){
    if (!check_arr(arr, "estimate_size_array_data")){
        return 0;
    }
    size_t sz = 1; 
    sz += sizeof(uint32_t);
    for (size_t i = 0; i < arr->index; i++) {
        sz += estimate_size_data(arr->array[i]);
    }
    if (arr->key){
        uint32_t len = strlen((char*)arr->key);
        sz += sizeof(uint32_t) + len;
    }
    return sz;
}

/**
 * tag a serialized buffer with a magic number 
 */
void TagBuffer(uint8_t *buffer, size_t *offset){
    if (!check_buffer(buffer, "TagBuffer") || !check_offset(offset, "TagBuffer")){
        return;
    }
    uint32_t magic = MAGIC_NUMBER;
    memcpy(buffer, &magic, sizeof(uint32_t));
    *offset += sizeof(uint32_t);
    
}
// int main (){
//     int number = 12314;
//     //  SERIALIZE DATA POINTS <INT>
//     Data *dt1 = InitDataPoint("somerut");
//     WriteDataInt(dt1, number);

//     size_t estimated_size = estimate_size_data(dt1);
//     uint8_t *buffer1 = malloc(estimated_size + sizeof(uint32_t));
    
//     size_t  offset = 0;
//     TagBuffer(buffer1, &offset);
//     serialize_data(dt1, buffer1 +  sizeof(uint32_t));

    
//     Data *dt1_reco1 = deserialize_data(buffer1, &offset);

//     printf("data type = %d\n", dt1_reco1->type);
//     printf("data = %d\n", *ReadDataInt(dt1_reco1));

//     //  SERIALIZE DATA POINTS <FLAOT>
//     float number2 = 123.123;
//     Data *dt2 = InitDataPoint("somerut");
//     WriteDataFloat(dt2, number2);

//     estimated_size = estimate_size_data(dt2);
//     uint8_t *buffer2 = malloc(estimated_size + sizeof(uint32_t));
//     offset = 0;
//     TagBuffer(buffer2, &offset);
//     serialize_data(dt2, buffer2 +  sizeof(uint32_t));

    
//     Data *dt1_reco2 = deserialize_data(buffer2, &offset);

//     printf("data type = %d\n", dt1_reco2->type);
//     printf("data = %f\n", *ReadDataFloat(dt1_reco2));


//     //  SERIALIZE DATA POINTS <DOUBLE>
//     double number3 = 123.123123441;
//     Data *dt3 = InitDataPoint("somerut");
//     WriteDataDouble(dt3, number3);

//     estimated_size = estimate_size_data(dt3);
//     uint8_t *buffer3 = malloc(estimated_size + sizeof(uint32_t));
//     offset = 0;
//     TagBuffer(buffer3, &offset);
//     serialize_data(dt3, buffer3 +  sizeof(uint32_t));

//     Data *dt1_reco3 = deserialize_data(buffer3, &offset);

//     printf("data type = %d\n", dt1_reco3->type);
//     printf("data = %lf\n", *ReadDataDouble(dt1_reco3));


//     //  SERIALIZE DATA POINTS <BOOLEN>
//     Data *dt4 = InitDataPoint("somerut");
//     WriteDataBool(dt4, true);

//     estimated_size = estimate_size_data(dt4);
//     uint8_t *buffer4 = malloc(estimated_size + sizeof(uint32_t));
//     offset = 0;
//     TagBuffer(buffer4, &offset);
//     serialize_data(dt4, buffer4 +  sizeof(uint32_t));

//     Data *dt1_reco4 = deserialize_data(buffer4, &offset);

//     printf("data type = %d\n", dt1_reco4->type);
//     printf("data = %d\n", *ReadDataBool(dt1_reco4));


//     //  SERIALIZE DATA POINTS <STRING>
//     Data *dt5 = InitDataPoint("somerut");
//     WriteDataString(dt5, "LET THERE BE LIGHT");

//     estimated_size = estimate_size_data(dt5);
//     uint8_t *buffer5 = malloc(estimated_size + sizeof(uint32_t));
//     offset = 0;
//     TagBuffer(buffer5, &offset);
//     serialize_data(dt5, buffer5 +  sizeof(uint32_t));

//     Data *dt1_reco5 = deserialize_data(buffer5, &offset);

//     printf("data type = %d\n", dt1_reco5->type);
//     printf("data = %s\n", ReadDataStr(dt1_reco5));



//     // SERIALIZING ARRAY OF DATAPOINTS 
//     // we're going to be using the same ones as above
//     printf("\n\n\nArray:\n");
//     Array *arr = InitDataPointArray("somerut");
//     append_datapoint(arr, dt1);
//     append_datapoint(arr, dt2);
//     append_datapoint(arr, dt3);
//     append_datapoint(arr, dt4);
//     append_datapoint(arr, dt5);

//     size_t est_arr_size = estimate_size_array_data(arr);
//     uint8_t *arr_buffer = malloc(est_arr_size + sizeof(uint32_t));
//     offset = 0;
//     TagBuffer(arr_buffer, &offset);

//     serialize_array_of_data(arr, arr_buffer +  sizeof(uint32_t));

//     Array *arr_reco = deserialize_array_data(arr_buffer, &offset);

//     for (int x = 0; x < arr_reco->index; x++){
//         Data *dt = arr_reco->array[x];
//         printf("data type = %d\n", dt->type);
//         switch (dt->type)
//         {
//         case INT:
//             printf("data = %d\n", *ReadDataInt(dt));
//             break;
//         case FLOAT:
//             printf("data = %f\n", *ReadDataFloat(dt));
//             break;
//         case DOUBLE:
//             printf("data = %lf\n", *ReadDataDouble(dt));
//             break;
//         case BOOLEAN:
//             printf("data = %d\n", *ReadDataBool(dt));
//             break;
//         case STRING:
//             printf("data = %s\n", ReadDataStr(dt));
//             break;
//         default:
//             break;
//         }
        
//     }
//     return 0;
// }