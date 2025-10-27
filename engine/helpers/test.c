#include "./helpers.h"
#include <stdio.h>
#include <stdlib.h>
// DATA POINT TESTS
int test_allocation_free(){
    Data *data = InitDataPoint();
    FreeDataPoint(data);
    return 1;
}
int test_add_valid_inputs_to_data(){
    Data *data = InitDataPoint();
    int res = WriteData(data, STRING, "test1234", DATA_NOT_OWNED);
    if (res != 1){
        printf("[x][test_add_valid_inputs_to_data] must equal to 1\n");
        return -1;
    }
    FreeDataPoint(data);
    return 1;
}
int test_int_correctness(){
    Data *data = InitDataPoint();
    int number = 12412;
    int res = WriteData(data, INT, &number, DATA_NOT_OWNED);
    if (res == -1){
        printf("[x][test_int_correctness] this shoudn't fail\n");
        return -1;
    }
    int *read_data = ReadDataInt(data);
    if (!res){
        printf("[x][test_int_correctness] this shouldn't return NULL\n");
        return -1;
    }
    if (*read_data != number){
        printf("[x][test_int_correctness] this should return %d\n", number);
        return -1;
    }
    FreeDataPoint(data);
    return 1;
}
int test_float_correctness(){
    Data *data = InitDataPoint();
    float number = 131.124;
    int res = WriteData(data, FLOAT, &number, DATA_NOT_OWNED);
    if (res == -1){
        printf("[x][test_float_correctness] this shoudn't fail\n");
        return -1;
    }
    float *read_data = ReadDataFloat(data);
    if (!res){
        printf("[x][test_float_correctness] this shouldn't return NULL\n");
        return -1;
    }
    if (*read_data != number){
        printf("[x][test_float_correctness] this should return %f\n", number);
        return -1;
    }
    FreeDataPoint(data);
    return 1;
}
int test_double_correctness(){
    Data *data = InitDataPoint();
    double number = 131.124432542;
    int res = WriteData(data, DOUBLE, &number, DATA_NOT_OWNED);
    if (res == -1){
        printf("[x][test_double_correctness] this shoudn't fail\n");
        return -1;
    }
    double *read_data = ReadDataDouble(data);
    if (!read_data){
        printf("[x][test_double_correctness] this shouldn't return NULL\n");
        return -1;
    }
    if (*read_data != number){
        printf("[x][test_double_correctness] this should return %lf\n", number);
        return -1;
    }
    FreeDataPoint(data);
    return 1;
}
int test_bool_correctness(){
    Data *data = InitDataPoint();
    bool bo = true;
    int res = WriteData(data, BOOLEAN, &bo, DATA_NOT_OWNED);
    if (res == -1){
        printf("[x][test_bool_correctness] this shoudn't fail\n");
        return -1;
    }
    bool *read_data = ReadDataBool(data);
    if (!read_data){
        printf("[x][test_bool_correctness] this shouldn't return NULL\n");
        return -1;
    }
    if (*read_data != bo){
        printf("[x][test_bool_correctness] this should return %d\n", bo);
        return -1;
    }
    FreeDataPoint(data);
    return 1;
}
int test_string_correctness(){
    Data *data = InitDataPoint();
    char *str = "this is a test asdoahsdoashasod";
    int res = WriteData(data, STRING, str, DATA_NOT_OWNED);
    if (res == -1){
        printf("[x][test_string_correctness] this shoudn't fail\n");
        return -1;
    }
    char *read_data = ReadDataStr(data);
    if (!read_data){
        printf("[x][test_string_correctness] this shouldn't return NULL\n");
        return -1;
    }
    int len1 = strlen(read_data);
    int len2 = strlen(str);
    if (len1 != len2){
        printf("[x][test_string_correctness] %d != %d\n", len1, len2);
        return -1;
    }
    FreeDataPoint(data);
    return 1;
}
// File tests 
int read_file_test(){
    File_object *fileobj = open_file_read_mode("../../../core.json");
    if (!fileobj){
        printf("[x][read_file] can't allocate file object\n");
        return -1;
    }
    if (fileobj->is_error){
        printf("error accured while reading file : %s\n", fileobj->error);
        return -1;
    }
    free_close_fileobject(fileobj);
    return 1;
}
int read_bs_file_file(){
    File_object *fileobj = open_file_read_mode("thi_is_bs_file.txt");
    if (!fileobj){
        printf("[x][read_bs_file_file] can't allocate file object\n");
        return -1;
    }
    if (!fileobj->is_error){
        printf("[x][read_bs_file_file] this should return an error\n");
        return -1;
    }
    free_close_fileobject(fileobj);
    return 1;
}
// Json/Files 
int read_file_parse_json(){
    File_object *fileobj = open_file_read_mode("../../../core.json");
    if (fileobj->is_error){
        printf("error accured while reading file : %s", fileobj->error);
        return -1;
    }
    char *buffer = read_file(fileobj);
    cJSON *json = cJSON_Parse(buffer);
    printf("%s\n",buffer);
    free_close_fileobject(fileobj);
    free(buffer);
    cJSON_Delete(json);
    return 1;
}
int main (){
    if (test_allocation_free() == -1) return -1;
    if (test_add_valid_inputs_to_data()== -1) return -1;
    if(test_int_correctness()== -1) return -1;
    if(test_float_correctness()== -1) return -1;
    if(test_double_correctness()== -1) return -1;
    if(test_bool_correctness()== -1) return -1;
    if(test_string_correctness()== -1) return -1;
    if(read_file_test()== -1) return -1;
    if(read_bs_file_file()== -1) return -1;

    return 0;
}