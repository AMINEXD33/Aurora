#include "./clientserver.h"

/**
* send a buffer in chunks to the client 
* ### args:
 *  `client_fd`: the client indentifier
 *  `buff`: a buffer
 *  `count`: the size of the mssg 
 * ### return:
 *  `ssize_t ` : the size of the writen data
 */
ssize_t write_all(int client_fd, const void *buff, size_t count) {
    size_t total_written = 0;
    const uint8_t *ptr = buff;

    while (total_written < count) {
        ssize_t n = send(client_fd, ptr + total_written, count - total_written, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_written += n;
    }
    return total_written;
}

/**
 * read a response in chunks into a buffer
 * ### args:
 *  `client_fd`: the client indentifier
 *  `buff`: a buffer
 *  `count`: the expected size of the response 
 * ### return:
 *  `ssize_t`: the size of the read data
 */
ssize_t read_all(int client_fd, void *buff, size_t count) {
size_t total_read = 0;
    uint8_t *ptr = buff;

    while (total_read < count) {
        ssize_t n = read(client_fd, ptr + total_read, count - total_read);
        
        if (n == 0) {
            break; 
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1; 
        }
        total_read += n;
    }
    
    if (total_read != count) {
        return -1;
    }
    return total_read;
}

/**
 * check if the buffer contains the magic number
 * the function will check the first bytes of size uint32_t
 * ### args:
 *  `buffer`: some buffer
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
* ### args:
 *  `client_fd`: the client identifier
 *  `store`: the promise store
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
    else{
        FreeDataPoint(data);
        free(buffer);
        return NULL;
    }
    printDataPoint(data, "\n");
    free(buffer);
    return data;
}

/**
 * send a void * buffer of somes size with retry on error
 * ### args:
 *  `client_fd`: the client identifier
 *  `buffer`: the src buffer that would be sent
 *  `size`: the size of the buffer
 *  `max_retries`: max retries before returning an error
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
 * ### args:
 *  `client_fd`: client indentifier
 *  `store`: the promise store
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
            return (char *)key;
        }
        printf("sent , PENDING\n");
        return (char *)key;
    }
    // else the promise is either , READY or COMPUTING
    if (promise->status == COMPUTING){
        status resp = COMPUTING;
        if (send_buffer_with_retry(client_fd, &resp, sizeof(status), 10) == -1){
            printf("can't send computing response to the client\n");
            return (char *)key;
        }
        printf("sent , COMPUTING\n");
    }else if(promise->status == READY){
        status resp = READY;
        if (send_buffer_with_retry(client_fd, &resp, sizeof(status), 10) == -1){
            printf("can't send ready response to the client\n");
            return (char *)key;
        }
        printf("sent , READY\n");
    }
    return (char *)key;
}

/**
 * send some defined datatype to the client (serialized)
 * ### args:
 *  `client_fd`: client identifier
 *  `store`: the promise store
 *  `datatype`: the defined datatype
 * ### return:
 *  `0`: on success
 *  `-1`: on error
 */
int hadle_sending_datatype(int client_fd, PromiseStore *store, complex_structures datatype){
    //printf("[SERVER] hadle_sending_datatype called\n");
    
    // get the size of the buffer for the key
    size_t size_of_buff;
    ssize_t n = read_all(client_fd, &size_of_buff, sizeof(size_t));
    //printf("[SERVER] Read key size: %zd (expected %zu)\n", n, sizeof(size_t));
    
    if(n != sizeof(size_t))
    {
        perror("[SERVER] read failed 4\n");
        return -1;
    }
    
    //printf("[SERVER] Key size to read: %zu\n", size_of_buff);
    
    // read the key (string)
    uint8_t *key = calloc(1, size_of_buff + 1);
    n = read_all(client_fd, key, size_of_buff);
    //printf("[SERVER] Read key: %zd bytes (expected %zu)\n", n, size_of_buff);
    
    if(n != size_of_buff)
    {
        perror("[SERVER] read failed 3(ARRAY)\n");
        free(key);
        return -1;
    }
    
    // null the last byte
    key[size_of_buff] = '\0';
    //printf("[SERVER] Looking for key: '%s'\n", key);
    
    // try to find the Promise
    Promise *promise = get_promise(store, key);
    // null checks for promise 
    if (promise == NULL){
        printf("[SERVER]promise is null\n");
        size_t size = -2;
        n = send_buffer_with_retry(client_fd, &size, sizeof(size_t), 10);
        free(key);
        return -1;
    }
    //printf("[SERVER] Found promise with key %s\n", key);
    // null checks for datatypes
    switch (datatype)
    {
        case DATA:
            if (!promise->datatype.data){
                //printf("[SERVER] Can't find cache of type DATA (cache miss)\n");
                size_t size = -1;
                // send 0 to the client to notify that the promise data is NULL
                n = send_buffer_with_retry(client_fd, &size, sizeof(size_t), 10);
                free(key);
                return -1;
            }
            break;
        case ARRAY:
            if (!promise->datatype.array){
                //printf("[SERVER] Can't find cache of type ARRAY (cache miss)\n");
                size_t size = -1;
                // send 0 to the client to notify that the promise array is NULL
                n = send_buffer_with_retry(client_fd, &size, sizeof(size_t), 10);
                free(key);
                return -1;
            }
        default:
            break;
    }
    // handle when the promise is not ready
    if (promise->status != READY){
        //printf("[SERVER] Promise is still computing (status: %d)\n", promise->status);
        size_t size = 0;
        n = send_buffer_with_retry(client_fd, &size, sizeof(size_t), 10);
        free(key);
        return -1;
    }


    // calculate estimated size for the datatype
    size_t estimated_size = 0;
    switch (datatype)
    {
        case DATA:
            estimated_size = estimate_size_data(promise->datatype.data) + sizeof(uint32_t);
            break;
        case ARRAY:
            estimated_size = estimate_size_array_data(promise->datatype.array) + sizeof(uint32_t);
            break;
        default:
            printf("[SERVER] NO ESTIMATING SIZE FUNCTION FOR TYPE %d\n", datatype);
            free(key);
            return -1;
            break;
    }

    //printf("[SERVER] Estimated size: %zu\n", estimated_size);
    
    // serialize data 
    uint8_t *buffer = calloc(1, estimated_size);
    if (!buffer){
        printf("[SERVER] Failed to allocate buffer\n");
        free(key);
        return -1;
    }
    
    // tag
    size_t offset = 0;
    TagBuffer(buffer, &offset);
    
    // serialize the datatype
    switch (datatype)
    {
    case DATA:
        serialize_data(promise->datatype.data, buffer + sizeof(uint32_t));
        break;
    case ARRAY:
        serialize_array_of_data(promise->datatype.array, buffer + sizeof(uint32_t));
        break;
    default:
        printf("[SERVER] NO SERIALIZATION FUNCTION FOR DATATYPE %d\n", datatype);
        break;
    }

    // send the expected size to the client
    //printf("[SERVER] Sending size: %zu\n", estimated_size);
    n = send_buffer_with_retry(client_fd, &estimated_size, sizeof(size_t), 10);
    //printf("[SERVER] Send size result: %zd\n", n);
    
    if(n == -1){
        printf("[SERVER] Failed to send size\n");
        free(buffer);
        free(key);
        return -1;
    }
    
    // send the serialized datatype
    //printf("[SERVER] Sending buffer of size: %zu\n", estimated_size);
    n = send_buffer_with_retry(client_fd, buffer, estimated_size, 10);
    //printf("[SERVER] Send buffer result: %zd\n", n);
    
    if(n == -1){
        printf("[SERVER] Failed to send buffer\n");
        free(buffer);
        free(key);
        return -1;
    }
    
    //printf("[SERVER] Successfully sent datatype\n");
    free(buffer);
    free(key);
    free(promise);
    return 0;
}



/**
 * handle receiving data of type Array from the sender process
 * ### args:
 *  `client_fd`: client identifier (sock)
 *  `store`: the promise store
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
    else{
        free_array(array);
        free(buffer);
        return NULL;
    }
    printArray(array);
    free(buffer);
    return array;
}

/**
 * check if the connection should be kept alive 
 * the client needs to send a keep alive signal 
 * after each request if it wants the conn to stay
 * alive
 * ### args:
 *  `client_fd`: client identifier(sock)
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
 * the thread function to handle incoming clients based on 
 * a message type sent by the client
 * 
 * ### args:
 *  `void *arg`: is a void pointer to a server_arg struct
 * ### return:
 *  `NULL`: retrurn NULL 
 */
void* handle_client_thread(void* arg){
    if (!arg){
        return NULL;
    }

    server_arg *args = (server_arg *)arg;
    int client_fd = args->client_fd;
    PromiseStore *store = args->store;
    // flag to keep conn alive
    while (true){
        // assume it will be closed
        // get the type of the sent data
        mssg_type type;
        ssize_t n = read_all(client_fd, &type, sizeof(mssg_type));
        if(n != sizeof(mssg_type))
        {
            break;
        }
        // based on the type of request
        switch (type)
        {
            // handle a cache request for Data (SET)
            case EXPECT_DATA:
                //printf("\t[EXPECTED DATA] \n");
                Data *dt = handle_data(client_fd, store);
                if (dt){
                    // cache the data
                    printf("adding data into cache\n");
                    Promise *promise = get_create_promise(store, dt->key);
                    publishData(promise, dt);
                    done_with_promise_data(promise);
                    printf("[v] data piblished\n");
                }
                // handle if this fails  (can't get Data)
                //printf("[SERVER] can't get expected data\n");
                break;
            // handle a cache request for Array (SET)
            case EXPECT_ARRAY:
                //printf("\t[EXPECTED ARRAY HIT] \n");
                Array *arr = handle_array(client_fd, store);
                if (arr){
                    // cache the array
                   // printf("adding array into cache\n");
                    Promise *promise = get_create_promise(store, arr->key);
                    if (promise->status == PENDING){
                        publishArray(promise, arr);
                    }
                    //printf("[v] array is published\n");
                }
                // handle if this fails (can't get Array)
                //printf("[SERVER] can't get expected array!\n");
                break;
            // handle a work claim for some key
            case EXPECT_CLAIM_WORK:
                //printf("\t[CLAIMING WORK\n");
                char *key = handle_claiming_work(client_fd, store);
                // if (key)
                //     free(key);
                break;
            // handle getting a promise of type Array
            case GET_ARRAY: 
                int arr_resp = hadle_sending_datatype(client_fd, store, ARRAY);
                if (arr_resp == -1){
                    printf("[SERVER] can't handle sending an array\n");
                }
                break;
            // handle getting a promise of type Data
            case GET_DATA:
                int data_resp = hadle_sending_datatype(client_fd, store, DATA);
                if (data_resp == -1){
                    printf("[SERVER] can't handle sending an data\n");
                }
                break;
            default:
                break;
        }
    }
    // close conn
    close(client_fd);
    printf("\nthread done !\n");

    //free(arg);
    return NULL;
}


// int main(){
//     Server_init_multithread();
// }