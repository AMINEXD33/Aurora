#include "../../init.h"

/**
 * TEST : 
 * just test standard allocation and free
 */
int main (){
    Shared *shared =  InitSharedMemory();
    FreeSharedMemory(shared);
    return 0;
}