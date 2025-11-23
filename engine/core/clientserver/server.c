#include "./clientserver.h"

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
Data * handle_data(int client_fd, PromiseStore *store){
    size_t size_of_buff;
    // read the size of the buffer
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
    // read the buffer 
    n = read_all(client_fd, buffer, size_of_buff);
    if(n != size_of_buff)
    {
        perror("read failed 3 (DATA)\n");
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
 * send a void * buffer of somes size with retry on error
 * ### return:
 *  `0`: if buffer is read successfully
 *  `-1`: on error
 */
int send_buffer_with_retry(int client_fd, void *buffer,size_t size, int max_retries) {
    for (int i = 0; i < max_retries; i++) {
        if (write_all(client_fd, buffer, size) == size){
            return 0;
        }
        fprintf(stderr, "Retry %d/%d\n", i + 1, max_retries);
        usleep(1000 * (1 << i)); 
    }
    return -1;  
}

/**
 * handle claiming work for a promise
 * ### return:
 *  `char *`: returns the key if successfull
 *  `NULL`: on error
 */
char *handle_claiming_work(int client_fd, PromiseStore *store){
    // get the size of the buffer
    size_t size_of_buff;
    ssize_t n = read_all(client_fd, &size_of_buff, sizeof(size_t));
    if(n != sizeof(size_t))
    {
        perror("read failed 4\n");
        return NULL;
    }
    // read the key (string)
    uint8_t *key = calloc(1, size_of_buff + 1);
    n = read_all(client_fd, key, size_of_buff);
    if(n != size_of_buff)
    {
        perror("read failed 3(claim work)");
        free(key);
        return NULL;
    }
    // null the last byte
    key[size_of_buff] = '\0';
    // try to find the promise
    Promise *promise = get_create_promise(store, (char *)key);
    // if we claimed the work for this promise , notify the client
    if (claim_work(promise)){
        status resp = PENDING;
        if (send_buffer_with_retry(client_fd, &resp, sizeof(status), 10) == -1){
            printf("can't send pending response to the client\n");
            free(key);
            return NULL;
        }
        printf("sent , PENDING\n");
        return (char *)key;
    }
    // else the promise is either , READY or COMPUTING
    if (promise->status == COMPUTING){
        status resp = COMPUTING;
        if (send_buffer_with_retry(client_fd, &resp, sizeof(status), 10) == -1){
            printf("can't send computing response to the client\n");
            free(key);
            return NULL;
        }
        printf("sent , COMPUTING\n");
    }else if(promise->status == READY){
        status resp = READY;
        if (send_buffer_with_retry(client_fd, &resp, sizeof(status), 10) == -1){
            printf("can't send ready response to the client\n");
            free(key);
            return NULL;
        }
        printf("sent , READY\n");
    }
    return (char *)key;
}

int hadle_sending_array(int client_fd, PromiseStore *store){
    // get the size of the buffer for the key
    size_t size_of_buff;
    ssize_t n = read_all(client_fd, &size_of_buff, sizeof(size_t));
    if(n != sizeof(size_t))
    {
        perror("read failed 4\n");
        return -1;
    }
    // read the key (string)
    uint8_t *key = calloc(1, size_of_buff + 1);
    n = read_all(client_fd, key, size_of_buff);
    if(n != size_of_buff)
    {
        perror("read failed 3(ARRAY)\n");
        free(key);
        return -1;
    }
    // null the last byte
    key[size_of_buff] = '\0';
    // try to find the Promise
    Promise *promise = get_promise(store, key);
    
    if (promise == NULL || !promise->datatype.array){
        // cache miss send a size 0
        printf("[L] NOT F PROMISE WITH KEY %s\n", key);
        size_t size = 0;
        n = send_buffer_with_retry(client_fd, &size, sizeof(size_t), 10);
        if(n != sizeof(size_t)){
            free(key);
            return -1;
        }
        return 0;
    }
    if (promise->status != READY){
        printf("[L]promise is still computing\n");
        return -1;
    }
    printf("WE FOUND THE PROMISE WITH THE KEY %s\n", key);
    printArray(promise->datatype.array);
    // valid promise 
    size_t estimated_size = estimate_size_array_data(promise->datatype.array) + sizeof(uint32_t);
    // serialize data 
    uint8_t *buffer = calloc(1, estimated_size);
    // tag
    size_t offset = 0;
    TagBuffer(buffer, &offset);
    // serialize the array
    serialize_array_of_data(promise->datatype.array, buffer);
    // send the expected size to the client
    n = send_buffer_with_retry(client_fd, &estimated_size, sizeof(size_t), 10);
    if(n != sizeof(size_t)){
        free(buffer);
        return -1;
    }
    // send the serialized array
    n = send_buffer_with_retry(client_fd, buffer, estimated_size, 10);
    if(n != sizeof(size_t)){
        free(buffer);
        return -1;
    }
    return 0;
}
/**
 * handle receiving data of type Array from the sender process
 * ### return:
 *  `Array *`: if everything is successfull
 *  `NULL` : on read size fail or can't allocate buffer or can't read buffer
 *   or the buffer is incomplete and finally if the magic number is not a match   
 */
Array *handle_array(int client_fd, PromiseStore *store){
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
    if (state == true)
        printf("keep alive [true]\n");
    else
        printf("keep alive [false]\n");
    return state;
}

/**
 * the thread function to handle incoming clients
 */
void* handle_client(void* arg){
    server_arg *args = (server_arg *)arg;
    int client_fd = args->client_fd;
    PromiseStore *store = args->store;
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
        // based on the type of request
        switch (type)
        {
            // handle a cache request for Data (SET)
            case EXPECT_DATA:
                printf("\tdata \n");
                Data *dt = handle_data(client_fd, store);
                if (dt == NULL){
                    // handle if this fails  (can't get Data)
                }else{
                    printf("adding data into cache\n");
                    Promise *promise = get_create_promise(store, dt->key);
                    // when calling this function to publish work we expect that you've already 
                    // claimed the work for this key, thus we're expecting this
                    // promise state to be COMPUTING
                    if(promise->status != COMPUTING){
                        printf("[x] we expect the promise to be COMPUTING\n");
                        // return a status to the client
                        
                    }
                    // publish
                    publishData(promise, dt);
                    printf("[v] data piblished\n");
                }
                break;
            // handle a cache request for Array (SET)
            case EXPECT_ARRAY:
                printf("\tarray \n");
                Array *arr = handle_array(client_fd, store);
                if (arr == NULL){
                    // handle if this fails (can't get Array)
                }else{
                    printf("adding array into cache\n");
                    Promise *promise = get_create_promise(store, arr->key);
                    // when calling this function to publish work we expect that you've already 
                    // claimed the work for this key, thus we're expecting this
                    // promise state to be COMPUTING
                    if (promise->status != COMPUTING){
                        printf("[x] we expect the promise to be COMPUTING\n");
                        // return a status to the client
                    }
                    // publish 
                    publishArray(promise, arr);
                    printf("[v] array is published\n");
                }
                break;
            // handle a work claim for some key
            case EXPECT_CLAIM_WORK:
                printf("claiming work\n");
                char *key = handle_claiming_work(client_fd, store);
                printf("[=++++=] key ===== %s\n", key);
                break;
            // handle getting a promise of type Array
            case GET_ARRAY: 
                int sent = hadle_sending_array(client_fd, store);
                break;
            default:
                break;
        }
        // if no keep alive signal is recieved  kill the connections
        if (do_keep_alive(client_fd)){
            flag = true;
        }else{
            printf("killing this mf \n");
        }  
    }
    // close conn
    close(client_fd);
    printf("\nthread done !\n");
    free(arg);
    return NULL;
}


// int main(){
//     Server_init_multithread();
// }