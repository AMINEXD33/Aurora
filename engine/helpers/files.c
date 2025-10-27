#include "./helpers.h"


/**
 * this function allocates memory for a File_object
 * ### return:
 *  `File_object` if successfull
 *  `NULL` on error
 */
File_object *init_fileobject(){
    File_object * fileobj = NULL;
    fileobj = calloc(1, sizeof(File_object));
    if (!fileobj){
        printf("can't allocate mem for fileobject\n");
        return NULL;
    } 
    fileobj->is_error = false;
    return fileobj;
}
/**
 * this function closes the file pointer if valid and 
 * frees File_object
 * ### return:
 *  `void` 
 */
void free_close_fileobject(File_object *fileobject){
    if (!fileobject){
        return;
    }
    if (fileobject->error)
        free(fileobject->error);
    if (fileobject->fptr)
        fclose(fileobject->fptr);
    free(fileobject);
}

char *getFileName(const char *path) {
    char *filename = strrchr(path, '/');
    return (filename == NULL) ? (char *)path : filename + 1;
}

/**
 * assign an str value based on the error thrown while opening the file
 * ### return:
 *  `File_object` if successful 
 *  `NULL` on error
 */
File_object * assign_error(File_object *fileobj){
    // is error
    fileobj->is_error = true;
    fileobj->fptr = NULL;
    // calculate error lenght, filename and the message
    unsigned int error_length = strlen(fileobj->filename) + 50;
    // allocated it
    char * error = calloc(error_length, sizeof(char));
    error[49] = '\0';
    // assign message
    switch(errno){
            case(ENOENT):
                strcat(error,"file doesn't exist : ");
                break;
            case(EACCES):
                strcat(error,"no permition to read file : ");
                break;// noo permisions
            case(EISDIR):
                strcat(error,"this is a directory : ");
                break; // this is a dir
            case(ENAMETOOLONG):
                strcat(error, "this file name is too long : ");
                break; // name is too long 
            default:
                strcat(error, "this is other error accured while reading file : ");
    }
    // include the file name
    strcat(error, fileobj->filename);
    // attache the error
    fileobj->error = error;
    return fileobj;
}

/**
 * the function takes a path and tries to open a file
 * then returns a `File_object`or `NULL` on error
 * ### return:
 *  `File_object *obj` if successfull
 *  `NULL` on error
 */
File_object * open_file_read_mode(char *path){
    if (!path){
        return NULL;
    }

    FILE *file = NULL;
    // struct file object
    File_object *fileobj = init_fileobject();
    // if no allocation is made
    if (!fileobj)
        return NULL;
    // add some headers
    fileobj->filename = getFileName(path);
    fileobj->filepath = path;
    // try to open the file
    if (! (file = fopen(path, "r"))){
        File_object *file_object = assign_error(fileobj);
        return file_object;
    }
    fileobj->fptr = file;
    return fileobj;
} 

/**
 * read the whole file and return the buffer
 * ### return:
 *    `char * buffer` if successful
 *    `NULL` on error
 */
char * read_file(File_object *fileobject){
    // find the end of the file
    fseek(fileobject->fptr, 0, SEEK_END);
    // get the size of the file (bytes)
    long int filesize = ftell(fileobject->fptr);
    // rewind to the start of the file
    rewind(fileobject->fptr);

    // alloc the size of the file to a buffer
    char *buffer = malloc(sizeof(char) * (filesize + 1));
    if (!buffer){
        perror("can't allocate the buffer for the file\n");
        return NULL;
    }
    fread(buffer, 1, filesize, fileobject->fptr);
    buffer[filesize] = '\0';
    return buffer;
}

// int main (){
//     File_object *fileobject =  open_file_read_mode("../../core.json");
//     if (fileobject){
//         if (fileobject->is_error){
//             printf("ann error accured -> %s \n", fileobject->error);
//         }
//     }
//     char *filebuff = read_file(fileobject);
//     cJSON *json = cJSON_Parse(filebuff);
//     char *string = cJSON_Print(json);
//     // printf("%s",string);
//     // printf("file name : %s \n file path : %s \n an error accured : %d\n", 
//     // fileobject->filename,
//     // fileobject->filepath,
//     // fileobject->is_error
//     // );
//     if(fileobject)
//         free_close_fileobject(fileobject);
//     cJSON_Delete(json);
//     free(string);
//     free(filebuff);
//     return 0;
// }