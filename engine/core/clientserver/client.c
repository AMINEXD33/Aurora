#include "./clientserver.h"

unsigned int recheck_times = 0;
/**
 * send a keep alive signal to the server
 * to close conn
 * ### return:
 *  `0`: if signal is sent
 *  `-1`: on error
 */
int send_keep_alive(int sock){
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
 * ### return:
 *  `0`: if signal is sent
 *  `-1`: on error
 */
int send_die(int sock){
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
 * ### return:
 *  `0`: if data is sent
 *  `-1`: an error accured
 */ 
int send_cache_data_protocol(int sock, Data *data){
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

int send_cache_key_claim_work(int sock, char *key){
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
 *  `0`: if data is sent
 *  `-1`: an error accured
 */ 
int send_cache_array_data_protocol(int sock, Array *arr){
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
 * ### return:
 *  `0`: data sent
 *  `-1`: data couldn't be sent
 */
int send_key_with_retry(int sock, char *key, int max_retries) {
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
 * ### return:
 *  `0`: data sent
 *  `-1`: data couldn't be sent
 */
int send_data_with_retry(int sock, Data *data, int max_retries) {
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
 * ### return:
 *  `0`: data sent
 *  `-1`: data couldn't be sent
 */
int send_array_with_retry(int sock, Array *arr, int max_retries) {
    for (int i = 0; i < max_retries; i++) {
        if (send_cache_array_data_protocol(sock, arr) == 0) {
            return 0;  
        }
        fprintf(stderr, "Retry %d/%d\n", i + 1, max_retries);
        usleep(1000 * (1 << i)); 
    }
    return -1;
}




int claim_work_client(int sock, char *key, int max_retries){
    if (send_key_with_retry(sock, key, max_retries) == -1){
        return -1;
    }
    status answer;
    printf("== reading the servers answer\n");
    read_all(sock, &answer, sizeof(status));
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

Array *get_cache_array_data_protocol(int sock, char *key){
    // send type of mssg
    mssg_type type = GET_ARRAY;
    ssize_t n = write_all(sock, &type, sizeof(mssg_type));
    if(n != sizeof(mssg_type)){
        return NULL;
    }
    // calc estimated size
    size_t estimated_size = strlen(key) + (size_t)sizeof(size_t);
    // send the size
    n =  write_all(sock , &estimated_size, sizeof(size_t));
    if (n != sizeof(size_t)){
        return NULL;
    }
    size_t response_size = 0;
    n = read_all(sock , &response_size, sizeof(size_t));
    if (n != sizeof(size_t)){
        return NULL;
    }
    if (response_size == 0){
        // cache miss return NULL
        return NULL;
    }
    uint8_t *buffer = calloc(1, response_size);
    n = read_all(sock, buffer, response_size);
    if (n != response_size){
        return NULL;
    }
    size_t offset = 0;
    Array *array = deserialize_array_data(buffer, &offset);
    return array;
}
/**
 * set socket timeout 
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
    struct sockaddr_un *addr = malloc(sizeof(struct sockaddr_un));
    if (!addr){
        printf("can't allocate mem for addresse\n");
        return NULL;
    }
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, SOCKET_PATH);
    return addr;
}

/**
 * check if connection is still active
 * ### return:
 *  `true`: if conn is still alive
 *  `flase`: if conn is not alive
 */
bool check_connection_alive(int sock) {
    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN | POLLHUP | POLLERR;
    
    int ret = poll(&pfd, 1, 0);  
    if (ret < 0) return false;  
    if (pfd.revents & (POLLHUP | POLLERR)) return false; 
    return true; 
}


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
    if (!sock){
        printf("can't create sock\n");
        return -1;
    }

    if (set_socket_timeout(sock, 100) == -1){
        printf("can't set timeout\n");
        return -1;
    }

    // claim work
    Data *data = InitDataPoint("aminemeftah");
    WriteDataFloat(data, 12141.151);
    // int stat = claim_work_client(sock, "aminemeftah", 10);
    // send_keep_alive(sock);
    // if (stat == PENDING){
    //     sleep(10);
    //     send_data_with_retry(sock, data, 10);
    // }
    // send_keep_alive(sock);
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
    int stat = claim_work_client(sock, "array1", 10);
    send_keep_alive(sock);
    if (stat == PENDING){
        sleep(1);
        send_array_with_retry(sock, arr, 10);
    }
    if (stat == READY){
        send_keep_alive(sock);
        Array *arr2 = NULL;
        if (stat == READY){
            Array *arr2 = NULL;
            arr2 = get_cache_array_data_protocol(sock, "array1");
        }
        printf("we got the stuff\n");
        printArray(arr2);
    }else{
        printf("[LL] promise is not ready to get \n");
    }
    send_die(sock);
    // send side cases
    free(addr);


    return 0;
}
