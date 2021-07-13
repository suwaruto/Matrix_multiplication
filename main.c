#include <CL/cl.h>
#include <stdio.h>

#include "mult.h"

int main( int argc, char **argv )
{
    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel = 0;
    cl_mem memObjects[3] = {0, 0, 0};
    cl_int errNum;

    context = CreateContext();
    if (context == NULL)
    {
        fprintf(stderr, "Failed to create OpenCL context");
        return 1;
    }

    commandQueue = CreateCommandQueue(context, &device);
    if (commandQueue == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }
    
    program = CreateProgram(context, device, "mult.cl");
    if (program == NULL)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }

    kernel = clCreateKernel(program, "mult", &errNum);
    if (errNum != CL_SUCCESS)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }

    int wa = ARRAY_SIZE, ha = ARRAY_SIZE, wb = ARRAY_SIZE, hb = ARRAY_SIZE;

    float a[wa * ha], b[wb * hb],
        res[ha * wb];
    for (int i = 0; i < ha; i++)
        for (int j = 0; j < wa; j++)
            a[i * wa + j] = 1;
    for (int i = 0; i < hb; i++)
        for (int j = 0; j < wb; j++)
            b[i * wb + j] = 1;

    if (CreateMemObjects(context, memObjects, (float *)a, (float *)b, 
        wa, ha, wb, hb))
    {
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }
   
    errNum = clSetKernelArg(kernel, 0, sizeof(memObjects[0]), &memObjects[0]);
    errNum |= clSetKernelArg(kernel, 1, sizeof(memObjects[1]), &memObjects[1]);
    errNum |= clSetKernelArg(kernel, 2, sizeof(memObjects[2]), &memObjects[2]);
    errNum |= clSetKernelArg(kernel, 3, sizeof(int), &wa);
    errNum |= clSetKernelArg(kernel, 4, sizeof(int), &wb);
    if (errNum != CL_SUCCESS)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }

    size_t globalWorkSize[2] = {ha, wb};
    size_t localWorkSize[1] = {1};

    errNum = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL,
        globalWorkSize, localWorkSize, 0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }
   
    errNum = clEnqueueReadBuffer(commandQueue, memObjects[2], CL_TRUE, 0,
        sizeof(float) * ARRAY_SIZE * ARRAY_SIZE, res, 0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }

    for (int i = 0; i < ARRAY_SIZE; i++)
        for (int j = 0; j < ARRAY_SIZE; j++)
            if (j == ARRAY_SIZE - 1)
                printf("%4.1f\n", res[i * ARRAY_SIZE + j]);
            else
                printf("%4.1f ", res[i * ARRAY_SIZE + j]);
    
    Cleanup(context, commandQueue, program, kernel, memObjects, 3);
    return 0;
}
