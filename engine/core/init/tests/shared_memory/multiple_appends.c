#include "../../init.h"

/**
 * TEST : 
 * just test standard allocation and free
 */
int main (){
    Shared *shared =  InitSharedMemory();
    for (unsigned int x=0; x < 100000; x++){
        Data *data = InitDataPoint();
        WriteData(data, INT, &x, DATA_NOT_OWNED);
        append_to_share(shared, "number", data);
    };
    FreeSharedMemory(shared);
    return 0;
}