#include "./clientserver.h"


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
        printf("can't allocate buffer for cashe size %ld\n", estimate_size_data);
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
        return -1;
    }
    // send data
    n = write_all(sock, buffer, estimated_size);
    if(n != estimated_size){
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
        printf("can't allocate buffer for cashe size %ld\n", estimate_size_data);
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
        return -1;
    }
    // send data
    n = write_all(sock, buffer, estimated_size);
    if(n != estimated_size){
        return -1;
    }
    free(buffer);
    return 0;
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

void TEST_SEND_VALID_DATA(int sock){
    Data *dt = InitDataPoint("username");
    WriteDataString(dt, "AMine meftah");

    Data *dt2 = InitDataPoint("is_this_alive");
    WriteDataBool(dt2, false);

    Data *dt3 = InitDataPoint("avg_max_packet_size");
    WriteDataFloat(dt3,123.1241);

    Data *dt4 = InitDataPoint("avg_tcp_conn");
    double dt4n = 123.41;
    WriteDataDouble(dt4, 12314.124124);

    Data *dt5 = InitDataPoint("number");
    WriteDataInt(dt5, 124141241);

    Data *dt6 = InitDataPoint("big number");
    WriteDataLong(dt6, 142894172491);

    Array *arr = InitDataPointArray("test");
    append_datapoint(arr, dt);
    append_datapoint(arr, dt2);
    append_datapoint(arr, dt3);
    append_datapoint(arr, dt4);
    append_datapoint(arr, dt5);
    append_datapoint(arr, dt6);

    send_data_with_retry(sock, dt, 10);
    send_keep_alive(sock);
    send_data_with_retry(sock, dt2, 10);
    send_keep_alive(sock);
    send_data_with_retry(sock, dt3, 10);
    send_keep_alive(sock);
    send_data_with_retry(sock, dt4, 10);
    send_keep_alive(sock);
    send_data_with_retry(sock, dt5, 10);
    send_keep_alive(sock);
    send_data_with_retry(sock, dt6, 10);
    send_keep_alive(sock);
    send_array_with_retry(sock, arr, 10);
    free_array(arr);
}

int main() {
    struct sockaddr_un *addr = get_addr();
    if (!addr){
        return -1;
    }
    int sock = start_connection(addr);
    if (!sock){
        printf("can't create sock\n");
        return -1;
    }

    if (set_socket_timeout(sock, 5) == -1){
        printf("can't set timeout\n");
        return -1;
    }
    // send valid data
    TEST_SEND_VALID_DATA(sock);
    send_die(sock);
    // send side cases
    free(addr);
    return 0;
}
