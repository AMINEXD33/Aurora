#include "./clientserver.h"

unsigned int recheck_times = 0;

/**
 * check if connection is still active
 * ### args:
 *  `sock`: the server socket indentifier
 * ### return:
 *  `true`: if conn is still alive
 *  `flase`: if conn is not alive
 */
bool check_connection_alive(int sock) {
    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN | POLLHUP | POLLERR;
    
    int ret = poll(&pfd, 1, 0);  
    if (ret < 0){
        printf("[CONN CLOSED] connection closed!\n");
        return false;
    }
    if (pfd.revents & (POLLHUP | POLLERR)) return false; 
    return true; 
}

/**
 * send a keep alive signal to the server
 * to close conn
 * ### args:
 *  `sock`: the server socket indentifier
 * ### return:
 *  `0`: if signal is sent
 *  `-1`: on error
 */
int send_keep_alive(int sock){
    if (!check_connection_alive(sock)){
        return -1;
    }
    bool flag = true;
    ssize_t n = write_all(sock, &flag, sizeof(bool));
    if (n != sizeof(bool)){
        return -1;
    }
    return 0;
}

/**
 * send a kill signal to the server
 * to close conn
* ### args:
 *  `sock`: the server socket indentifier
 * ### return:
 *  `0`: if signal is sent
 *  `-1`: on error
 */
int send_die(int sock){
    if (!check_connection_alive(sock)){
        return -1;
    }
    bool flag = false;
    ssize_t n = write_all(sock, &flag, sizeof(bool));
    if (n != sizeof(bool)){
        return -1;
    }
    return 0;
}


/**
 * a protocol so sending Data type to the server
 * the functions sends:
 *         ##### 1: the type of the object (EXPECT_DATA)
 *         ##### 2: the size of the serialized data
 *         ##### 3: serialized data
 * ### args:
 *  `sock`: server identifier
 *  `data`: the Data struct that would be sent
 * ### return:
 *  `0`: if data is sent
 *  `-1`: an error accured
 */ 
int send_cache_data_protocol(int sock, Data *data){
    if (!check_connection_alive(sock)){
        return -1;
    }
    // send type of mssg
    mssg_type type = EXPECT_DATA;
    ssize_t n = write_all(sock, &type, sizeof(mssg_type));
    if(n != sizeof(mssg_type)){
        return -1;
    }
    // calc estimated size
    size_t estimated_size = estimate_size_data(data) + (size_t)sizeof(uint32_t);
    // allocate buffer
    uint8_t *buffer = calloc(1, estimated_size);
    if (!buffer){
        printf("can't allocate buffer for cache size %ld\n", estimated_size);
        free(buffer);
        return -1;
    }
    // set offset
    size_t  offset = 0;
    // tag with magic number
    TagBuffer(buffer, &offset);
    // serialize data
    serialize_data(data, buffer +  sizeof(uint32_t));
    // send the size of data 
    n = write_all(sock, &estimated_size, sizeof(size_t));
    if(n != sizeof(size_t)){
        free(buffer);
        return -1;
    }
    // send data
    n = write_all(sock, buffer, estimated_size);
    if(n != estimated_size){
        free(buffer);
        return -1;
    }
    free(buffer);
    return 0;
}

/**
 * send the key to the server
 * ### args
 *  `sock`: server identifier
 *  `key`: the cache key
 * ### return: 
 *  `0`: if successfull
 *  `-1`: on error
 */
int send_cache_key_claim_work(int sock, char *key){
    if (!check_connection_alive(sock)){
        return -1;
    }
    // send type of mssg
    mssg_type type = EXPECT_CLAIM_WORK;
    ssize_t n = write_all(sock, &type, sizeof(mssg_type));
    if(n != sizeof(mssg_type)){
        return -1;
    }
    // calc estimated size
    size_t estimated_size = strlen(key) + (size_t)sizeof(size_t);
    // allocate buffer
    uint8_t *buffer = calloc(1, estimated_size);
    if (!buffer){
        printf("can't allocate buffer for cache size %ld\n", estimated_size);
        free(buffer);
        return -1;
    }
    // set offset
    size_t  offset = 0;
    // tag with magic number
    TagBuffer(buffer, &offset);
    // send the size of data 
    n = write_all(sock, &estimated_size, sizeof(size_t));
    if(n != sizeof(size_t)){
        free(buffer);
        return -1;
    }
    // cpy the buffer
    memcpy(buffer, key, estimated_size);
    n = write_all(sock, buffer, estimated_size);
    if(n != estimated_size){
        free(buffer);
        return -1;
    }
    free(buffer);
    return 0;
}


/**
 * a protocol so sending Array type to the server
 * the functions sends:
 *         ##### 1: the type of the object (EXPECT_ARRAY)
 *         ##### 2: the size of the serialized array
 *         ##### 3: serialized array
 * ### return:
 *  `sock`: the server indentifier
 *  `arr`: the Array struct that would be sent
 * ### return:
 *  `0`: if data is sent
 *  `-1`: an error accured
 */ 
int send_cache_array_data_protocol(int sock, Array *arr){
    if (!check_connection_alive(sock)){
        return -1;
    }
    // send type of mssg
    mssg_type type = EXPECT_ARRAY;
    ssize_t n = write_all(sock, &type, sizeof(mssg_type));
    if(n != sizeof(mssg_type)){
        return -1;
    }
    // calc estimated size
    size_t estimated_size = estimate_size_array_data(arr) + sizeof(uint32_t);
    // allocate buffer
    uint8_t *buffer = calloc(1, estimated_size);
    if (!buffer){
        printf("can't allocate buffer for cache size %ld\n", estimated_size);
        free(buffer);
        return -1;
    }
    // set offset
    size_t  offset = 0;
    // tag with magic number
    TagBuffer(buffer, &offset);
    // serialize data
    serialize_array_of_data(arr, buffer +  sizeof(uint32_t));
    // send the size of data 
    n = write_all(sock, &estimated_size, sizeof(size_t));
    if(n != sizeof(size_t)){
        free(buffer);
        return -1;
    }
    // send data
    n = write_all(sock, buffer, estimated_size);
    if(n != estimated_size){
        free(buffer);
        return -1;
    }
    free(buffer);
    return 0;
}


/**
 * send key but retry if an error accured
 * ### args:
 *  `sock`: the server identifier
 *  `key`: the promise key 
 *  `max_retries`: the max retries before returning an error
 * ### return:
 *  `0`: data sent
 *  `-1`: data couldn't be sent
 */
int send_key_with_retry(int sock, char *key, int max_retries) {
    if (!check_connection_alive(sock)){
        return -1;
    }
    for (int i = 0; i < max_retries; i++) {
        if (send_cache_key_claim_work(sock, key) == 0) {
            return 0;  
        }
        fprintf(stderr, "Retry %d/%d\n", i + 1, max_retries);
        usleep(1000 * (1 << i)); 
    }
    return -1;  
}

/**
 * send data but retry if an error accured
 * ### args:
 *  `sock`: the server identifier
 *  `data`: the Data struct 
 *  `max_retries`: the max retries before returning an error
 * ### return:
 *  `0`: data sent
 *  `-1`: data couldn't be sent
 */
int send_data_with_retry(int sock, Data *data, int max_retries) {
    if (!check_connection_alive(sock)){
        return -1;
    }
    for (int i = 0; i < max_retries; i++) {
        if (send_cache_data_protocol(sock, data) == 0) {
            return 0;  
        }
        fprintf(stderr, "Retry %d/%d\n", i + 1, max_retries);
        usleep(1000 * (1 << i)); 
    }
    return -1;  
}

/**
 * send array but retry if an error accured
* ### args:
 *  `sock`: the server identifier
 *  `arr`: the arr key 
 *  `max_retries`: the max retries before returning an error
 * ### return:
 *  `0`: data sent
 *  `-1`: data couldn't be sent
 */
int send_array_with_retry(int sock, Array *arr, int max_retries) {
    if (!check_connection_alive(sock)){
        return -1;
    }
    for (int i = 0; i < max_retries; i++) {
        if (send_cache_array_data_protocol(sock, arr) == 0) {
            return 0;  
        }
        fprintf(stderr, "Retry %d/%d\n", i + 1, max_retries);
        usleep(1000 * (1 << i)); 
    }
    return -1;
}



/**
 * wrapper function to send the key and try to claim the work
   ### args:
 *  `sock`: the server identifier
 *  `key`: the promise key 
 *  `max_retries`: the max retries before returning an error
 * ### return:
 *  `COMPUTING`: if the work is still computing
 *  `PENDING`: the key is still pending(no work is being done)
 *  `READY`: the work is ready
 *  `-1`: on error
 */
int claim_work_client(int sock, char *key, int max_retries){
    if (!check_connection_alive(sock)){
        return -1;
    }
    if (send_key_with_retry(sock, key, max_retries) == -1){
        return -1;
    }
    status answer;
    printf("== reading the servers answer\n");
    ssize_t n = read_all(sock, &answer, sizeof(status));
    if (n != sizeof(status)){
        return -1;
    }
    printf("SERVER ANSWER = %d\n", answer);
    switch (answer)
    {
        // an other thread is still computing this value
        case COMPUTING:
            printf("[O] other thread is computing, ill wait\n");

            return COMPUTING;
        // this thread claimed the work
        case PENDING:
            printf("[O] got the work\n");
            return PENDING;
        // the key is already computed
        case READY:
            printf("[O] the work is already computed\n");
            return READY;
    }
    return -1;
}

/**
 * get a cached array
 *  ### args:
 *  `sock`: the server identifier
 *  `key`: the promise key 
 *  `datatype`: a defined datatype 
 * ### return:
 *  `Array *`: if successfull
 *  `NULL`: on error
 */
void *get_cache_datatype_protocol(int sock, char *key, complex_structures datatype){
    if (!check_connection_alive(sock)){
        return NULL;
    }
    // send type of mssg
    mssg_type type;
    switch (datatype)
    {
        case DATA:
            type = GET_DATA;
            break;
        case ARRAY:
            type = GET_ARRAY;
            break;
        default:
            printf("[ERROR] can't retrived an undefined datatype\n");
            return NULL;
            break;
    }

    printf("[DEBUG] Sending message type: %d\n", type);
    ssize_t n = write_all(sock, &type, sizeof(mssg_type));
    if(n != sizeof(mssg_type)){
        printf("[ERROR] Failed to send message type\n");
        return NULL;
    }
    
    // calc estimated size
    size_t estimated_size = strlen(key);
    printf("[DEBUG] Key size: %zu, Key: %s\n", estimated_size, key);
    
    // send the size
    n = write_all(sock, &estimated_size, sizeof(size_t));
    if (n != sizeof(size_t)){
        printf("[ERROR] Failed to send key size\n");
        return NULL;
    }
    
    // send key
    n = write_all(sock, key, estimated_size);
    if (n != estimated_size){
        printf("[ERROR] Failed to send key, sent %zd, expected %zu\n", n, estimated_size);
        return NULL;
    }
    
    printf("[DEBUG] Waiting for response size...\n");
    size_t response_size = 0;
    n = read_all(sock, &response_size, sizeof(size_t));
    printf("[DEBUG] Read result: %zd, response_size: %zu\n", n, response_size);

    if (n != sizeof(size_t)){
        printf("[ERROR] Failed to read response size, got %zd bytes\n", n);
        return NULL;
    }
    
    if (response_size == 0){
        printf("[x] CACHE MISS DUDE\n");
        return NULL;
    }
    
    printf("[DEBUG] Allocating buffer of size: %zu\n", response_size);
    uint8_t *buffer = calloc(1, response_size);
    if (!buffer){
        printf("[ERROR] Failed to allocate buffer\n");
        return NULL;
    }
    
    printf("[DEBUG] Reading buffer...\n");
    n = read_all(sock, buffer, response_size);
    printf("[DEBUG] Read %zd bytes, expected %zu\n", n, response_size);
    
    if (n != response_size){
        printf("[x] mismatch in size: read %zd, expected %zu\n", n, response_size);
        free(buffer);
        return NULL;
    }
    
    printf("[DEBUG] Checking magic number...\n");
    if (check_magic_number(buffer)){
        printf("[v] good magic number\n");
    }else{
        printf("[x] bad magic number\n");
        free(buffer);
        return NULL;
    }
    
    size_t offset = sizeof(uint32_t);
    printf("[DEBUG] Deserializing array...\n");
    switch (datatype)
    {
        case DATA:
            Data *data = deserialize_data(buffer, &offset);
            free(buffer);
            return data;
            break;
        case ARRAY:
            Array *array = deserialize_array_data(buffer, &offset);
            free(buffer);
            return array;
            break;
        default:
            printf("[ERROR] NO DESERIALIZATION FUNCTION FOR DATATYPE %d\n", datatype);
            free(buffer);
            return NULL;
            break;
    }
    return NULL;
}

/**
 * set socket timeout 
 * ### args:
 *  `sock`: the socket indentifier
 *  `seconds`: the timeout in seconds
 * ### return:
 *  `0`: timeoute is set
 *  `-1`: timeout is not set
 */
int set_socket_timeout(int sock, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        return -1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        return -1;
    }
    return 0;
}

/**
 * create a socket and connect to the server 
 */
int start_connection(struct sockaddr_un *addr){
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) { 
        perror("socket creation error"); 
        return -1;
    }
    if (connect(sock, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("can't connect to the server");
        return -1;
    }
    return sock;
}

/**
 * get the addresse of the server(localhost)
 */
struct sockaddr_un * get_addr(){
    struct sockaddr_un *addr = calloc(1, sizeof(struct sockaddr_un));
    if (!addr){
        printf("can't allocate mem for addresse\n");
        return NULL;
    }
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, SOCKET_PATH);
    return addr;
}



/**
 * just a test and show of concept function of how a work flow example
 */
int sendstuff() {
    struct sockaddr_un *addr = get_addr();
    if (!addr){
        return -1;
    }
    while (access(SOCKET_PATH, F_OK) != 0) {
        printf("DAMN waiting for server tp start \n");
        usleep(1000); // wait 1ms
    }
    int sock = start_connection(addr);
    if (sock < 0){
        printf("can't create sock\n");
        return -1;
    }

    if (set_socket_timeout(sock, 10) == -1){
        printf("can't set timeout\n");
        close(sock);
        return -1;
    }

    // claim work
    Data *data = InitDataPoint("aminemeftah");
    WriteDataFloat(data, 12141.151);
    int stat1 = claim_work_client(sock, "aminemeftah", 10);
    if (stat1 == -1){
        printf("ERROR ACURED while claiming work quiting\n");
        close(sock);
        return -1;
    }

    if (stat1 == PENDING){
        sleep(2);
        send_keep_alive(sock);
        send_data_with_retry(sock, data, 10);
        printf("[V] data was sent\n");
    }
    else if(stat1 == READY){
        send_keep_alive(sock);
        Data *data_retrieved = (Data *)get_cache_datatype_protocol(sock, "aminemeftah", DATA);
        if (data_retrieved){
            printf("PRINTING DATA RECIEVED\n");
            printDataPoint(data_retrieved, "\n");
            FreeDataPoint(data_retrieved);
        }else{
            printf("[XXXXX] READY but nothing is returning(DATA)\n");
        }
    }else{
        printf("[LL] promise is not ready to get\n");
    }


    Data *data1 = InitDataPoint("data1");
     WriteDataFloat(data1, 121441.151);
    Data *data2 = InitDataPoint("data2");
     WriteDataFloat(data2, 6565.151);
    Data *data3 = InitDataPoint("data3");
     WriteDataFloat(data3, 68678.151);
    Data *data4 = InitDataPoint("data4");
     WriteDataFloat(data4, 234645.151);
    Array *arr = InitDataPointArray("array1");
    append_datapoint(arr, data1);
    append_datapoint(arr, data2);
    append_datapoint(arr, data3);
    append_datapoint(arr, data4);
    /** WORK FLOW TO CACHE A COMPUTE VALUE */
    // claim the work so no other process recomputes it
    send_keep_alive(sock);
    int stat2 = claim_work_client(sock, "array1", 10);
    if (stat2 == -1){
        printf("ERROR ACURED while claiming work quiting\n");
        close(sock);
        return -1;
    }
    // if the status is PENDING that means that the promise is created
    if (stat2 == PENDING){
        // keep the conn alive for our next request
        sleep(2);
        send_keep_alive(sock);
        // send the array (the same key)
        send_array_with_retry(sock, arr, 10);
        close(sock);
        return -1;
    }
    // the compute for the key is already done so you can retrieve 
    // the value
    else if (stat2 == READY){
        // keep the conn alive
        send_keep_alive(sock);
        // get the cached value
        Array *arr2 = (Array *)get_cache_datatype_protocol(sock, "array1", ARRAY);
        if (arr2){
            printf("PRINTING ARRAY RECIEVED\n");
            printArray(arr2);
            free_array(arr2);
        }else{
            printf("[XXXXX] READY but nothing is returning(ARRAY)\n");
        }
    }else{
        // the promise is still computing , you can do something else
        printf("[LL] promise is not ready to get \n");
    }

    // if (stat1 == READY){
    //     send_keep_alive(sock);
    //     Data *data_retrieved = (Data *)get_cache_datatype_protocol(sock, "aminemeftah", DATA);
    //     if (data_retrieved){
    //         printf("PRINTING DATA RECIEVED\n");
    //         printDataPoint(data_retrieved, "\n");
    //         FreeDataPoint(data_retrieved);
    //     }else{
    //         printf("[XXXXX] READY but nothing is returning(DATA)\n");
    //     }
    // }else{
    //     printf("[LL] promise is not ready to get\n");
    // }
    // kill the connection
    send_die(sock);
    // send side cases
    free(addr);
    free_array(arr);
    FreeDataPoint(data);
    close(sock);

    return 0;
}
