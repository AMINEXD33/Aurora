#include "./clientserver.h"

int server_fd;


/**
 * write data in partitions
 * ### return:
 *  `ssize_t ` : the size of the writen data
 */
ssize_t write_all(int fd, const void *buf, size_t count) {
    size_t total_written = 0;
    const uint8_t *ptr = buf;

    while (total_written < count) {
        ssize_t n = write(fd, ptr + total_written, count - total_written);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_written += n;
    }
    return total_written;
}

/**
 * read data in partisions
 * ### return:
 *  `ssize_t`: the size of the read data
 */
ssize_t read_all(int fd, void *buf, size_t count) {
    size_t total_read = 0;
    uint8_t *ptr = buf;

    while (total_read < count) {
        ssize_t n = read(fd, ptr + total_read, count - total_read);
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_read += n;
    }
    return total_read;
}

/**
 * check if the buffer contains the magic number
 * the function will check the first bytes of size uint32_t
 * ### return:
 *  `true` : if the magic number matches
 *  `false` : if the magic number is not a match(data corruption) 
 */
bool check_magic_number(uint8_t *buffer){
    if (!buffer){
        return false;
    }
    uint32_t magic = MAGIC_NUMBER;
    uint32_t is_magic;
    
    memcpy(&is_magic, buffer, sizeof(uint32_t));
    
    if (magic == is_magic){
        return true;
    }
    return false;
}

/**
 * handle receiving data of type Data from the sender process
 * ### return:
 *  `Data *`: if everything is successfull
 *  `NULL` : on read size fail or can't allocate buffer or can't read buffer
 *   or the buffer is incomplete and finally if the magic number is not a match   
 */
Data * handle_data(int client_fd){
    size_t size_of_buff;
    ssize_t n = read_all(client_fd, &size_of_buff, sizeof(size_t));
    if(n != sizeof(size_t))
    {
        perror("read failed 2\n");
        return NULL;
    }
    uint8_t *buffer = calloc(1, size_of_buff + sizeof(uint32_t));
    if (!buffer){
        printf("can't allocate buffer of size %u\n", size_of_buff);
        return NULL;
    }
    n = read_all(client_fd, buffer, size_of_buff);
    if(n != size_of_buff)
    {
        perror("read failed 3\n");
        free(buffer);
        return NULL;
    }

    if (!check_magic_number(buffer)){
        printf("magic number is not a match\n");
        free(buffer);
        return NULL;
    }else{
        printf("magic :)\n");
    }

    size_t offset = sizeof(uint32_t);
    Data *data = deserialize_data(buffer, &offset);
    if (!data){
        printf("can't deserialize data\n");
        return NULL;
    }
    if (data->key)
        printf("data key=> %s\n", data->key);
    printDataPoint(data, "\n");
    free(buffer);
    return data;
}

/**
 * handle receiving data of type Array from the sender process
 * ### return:
 *  `Array *`: if everything is successfull
 *  `NULL` : on read size fail or can't allocate buffer or can't read buffer
 *   or the buffer is incomplete and finally if the magic number is not a match   
 */
Array *handle_array(int client_fd){
    size_t size_of_buff;
    ssize_t n = read_all(client_fd, &size_of_buff, sizeof(size_t));
    if(n != sizeof(size_t))
    {
        perror("read failed 4\n");
        return NULL;
    }
    uint8_t *buffer = calloc(1, size_of_buff);

    n = read_all(client_fd, buffer, size_of_buff);
    if(n != size_of_buff)
    {
        perror("read failed 5\n");
        free(buffer);
        return NULL;
    }
    if (!check_magic_number(buffer)){
        printf("magic number is not a match\n");
        free(buffer);
        return NULL;
    }else{
        printf("magic :)\n");
    }

    size_t offset = sizeof(uint32_t);
    Array *array = deserialize_array_data(buffer, &offset);
    if (array->key)
        printf("array key=> %s\n", array->key);
    printArray(array);
    free(buffer);
    return array;
}

/**
 * check if the connection should be kept alive 
 * the client needs to send a keep alive signal 
 * after each request if it wants the conn to stay
 * alive
 * ### return:
 *  `true`: conn should be left alive
 *  `false`: conn should be close
 */
bool do_keep_alive(int client_fd){
    bool state;
    ssize_t n = read_all(client_fd, &state, sizeof(bool));
    if(n != sizeof(bool))
    {
        perror("read failed keep alive\n");
        return false;
    }
    printf("keep alive ? : %d \n", state);
    return state;
}

/**
 * the thread function to handle incoming clients
 */
void* handle_client(void* arg){
    int client_fd = *(int*)arg;
    free(arg);
    // flag to keep conn alive
    bool flag = true;

    while (flag){
        // assume it will be closed
        flag = false;
        // get the type of the sent data
        mssg_type type;
        ssize_t n = read_all(client_fd, &type, sizeof(mssg_type));
        if(n != sizeof(mssg_type))
        {
            perror("read failed 1\n");
            return NULL;
        }
        switch (type)
        {
            case EXPECT_DATA:
                printf("\tdata \n");
                Data *dt = handle_data(client_fd);
                if (dt == NULL){
                    // handle if this fails  (can't get Data)
                }else{
                    // this need to support keys, the user must send a key
                    // Promise *promise = get_create_promise(store, );
                }
                break;
            case EXPECT_ARRAY:
                printf("\tarray \n");
                Array *arr = handle_array(client_fd);
                if (arr == NULL){
                    // handle if this fails (can't get Array)
                }else{
                    free_array(arr);
                }
                break;
            default:
                break;
        }
        
        if (do_keep_alive(client_fd)){
            flag = true;
        }else{
            printf("killing this mf \n");
        }  
    }
    // close conn
    close(client_fd);
    printf("\nthread done !\n");
    return NULL;
}

/**
 * function that spawns threads to handle incoming 
 * requests
 */
void Server_init_multithread() {
    // set up socket
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);
    unlink(SOCKET_PATH);
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    // bind
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    // listen to up 5 connections
    listen(server_fd, 5);
    printf("[v] server is listening on 127.0.0.1\n");
    bool flag = true;
    while (flag) {
        flag = false;
        // accept incoming client 
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, NULL, NULL);
        // on error
        if (*client_fd < 0) { perror("accept"); free(client_fd); continue; }
        // spawn a handler thread
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_fd);
        pthread_join(tid, NULL);
        //pthread_detach(tid); // don't need to join
    }
}

// int main(){
//     Server_init_multithread();
// }