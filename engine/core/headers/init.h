#ifndef _INIT_HEADER
#define _INIT_HEADER 


typedef struct module
{
    char name[200];
    char *path;
}Module;

typedef struct job
{
    Module **modules;
    unsigned int jobid; 
    unsigned int joblength;
};








#endif