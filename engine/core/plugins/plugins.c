#include "plugins.h"

/**
 * get an array of modules names
 * ### return :
 *  `Array *` : if successfull
 *  `NULL`: on error
 */
Array *get_all_modules_names(){
    struct dirent *de;
    DIR *dr = opendir(MODULES_PATH);
    if (dr == NULL){
        printf("can't open the directory %s\n", MODULES_PATH);
        return NULL;
    }
    Array *array = InitDataPointArray();
    if (!array){
        closedir(dr);
        return NULL;
    }
    while ((de = readdir(dr)) != NULL){
        // exclude the . and ..
        if (strncmp(de->d_name, ".", 1) == 0 || strncmp(de->d_name, "..", 2) == 0){
            continue;
        }
        Data *name = InitDataPoint();
        if (!name){
            continue;
        }
        char *str_name = calloc(strlen(de->d_name)+1, sizeof(char));
        strcpy(str_name, de->d_name);
        WriteData(name, STRING, str_name, DATA_OWNED);
        if (append_datapoint(array, name) == -1){
            printf("can't append the data struct to the array on value < %s >\n", de->d_name);
            continue;
        }
    }
    closedir(dr);
    return array;
}

void load_modules(){
    Array *arr = get_all_modules_names();
    if (!arr){
        printf("can't get the modules names\n");
        return;
    }
    // for every module
    for (int x = 0; x < arr->index; x++){
        Data *module = arr->array[x];
        // I did included the numbers like this just to keep things clear
        // <modules_base_path> + / + <module name> + / + module.so + \0
        // the module.so is a constant , each module needs to have the obj named like that
        // just to keep things simple -_-
        unsigned int lib_path_size = strlen(MODULES_PATH) + 1 + strlen(ReadDataStr(module)) + 1 + 9 + 1;

        char *lib_name = calloc(lib_path_size,sizeof(char));
        strcat(lib_name, MODULES_PATH);
        strcat(lib_name, "/");
        strcat(lib_name, ReadDataStr(module));
        strcat(lib_name, "/");
        strcat(lib_name, "module.so");
        lib_name[lib_path_size-1] = '\0';

        printf("lib name == %s\n", lib_name);
        printf("len= %d\n", strlen(lib_name));


        void *handle = dlopen(lib_name, RTLD_NOW);
        if (!handle) {
            fprintf(stderr, "Error: %s\n", dlerror());
            return;
        }

        void (*plugin_run)();
        plugin_run = dlsym(handle, "main");

        char *error = dlerror();
        if (error) {
            fprintf(stderr, "dlsym error: %s\n", error);
            return ;
        }
        plugin_run();
        free(lib_name);
    }
    free_array(arr);
}
int main()
{
    load_modules();
    return 0;
}