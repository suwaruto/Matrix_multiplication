#include <CL/cl.h>

#define ARRAY_SIZE 1000

cl_context CreateContext();
cl_command_queue CreateCommandQueue( cl_context context, cl_device_id *device );
cl_program CreateProgram( cl_context context, cl_device_id device,
    const char *fileName );
int CreateMemObjects( cl_context context, cl_mem *memObjects, float *a, 
    float *b, int wa, int ha, int wb, int hb );
void Cleanup( cl_context context, cl_command_queue commandQueue,
    cl_program program, cl_kernel kernel, cl_mem *memObjects, 
    int memObjectsLength );

