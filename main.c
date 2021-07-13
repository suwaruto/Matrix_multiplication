#include <CL/cl.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "mult.h"

void output_matrix( float *arr, int harr, int warr )
{
    for (int row = 0; row < harr; row++)
        for (int col = 0; col < warr; col++)
            if (col == warr - 1)
                printf("%4.1f\n", arr[row * warr + col]);
            else
                printf("%4.1f ", arr[row * warr + col]);
}

int main( int argc, char **argv )
{
    srand(time(NULL));
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
        fprintf(stderr, "Failed to create command queue");
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }
    
    program = CreateProgram(context, device, "mult.cl");
    if (program == NULL)
    {
        fprintf(stderr, "Failed to create command queue");
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }

    kernel = clCreateKernel(program, "mult", &errNum);
    if (errNum != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to create kernel");
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }

    int wa = ARRAY_SIZE, ha = ARRAY_SIZE, wb = ARRAY_SIZE, hb = ARRAY_SIZE;

    printf("Sizes of matrices: (%d, %d) and (%d, %d)\n", ha, wa, hb, wb);

    if (wa != hb)
    {
        fprintf(stderr, "Invalid matrices dimensions, not possible to multiply");
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    } 

    //only large matrices are of interest, large arrays' definition without static
    //would result in segmentation fault 
    static float a[ARRAY_SIZE * ARRAY_SIZE], b[ARRAY_SIZE * ARRAY_SIZE],
        res[ARRAY_SIZE * ARRAY_SIZE];
    //generate random float entries for these matrices in range [0, 10]
    for (int i = 0; i < ha; i++)
        for (int j = 0; j < wa; j++)
            a[i * wa + j] = (float)rand()/(float)(RAND_MAX/10);
    for (int i = 0; i < hb; i++)
        for (int j = 0; j < wb; j++)
            b[i * wb + j] = (float)rand()/(float)(RAND_MAX/10);

    clock_t t1 = clock();

    //compute matrices' product without OpenCL, print result and time
    for (int i = 0; i < ha; i++)
        for (int j = 0; j < wb; j++)
        {
            res[i * wb + j] = 0;
            for (int k = 0; k < wa; k++)
                res[i * wb + j] += a[i * wa + k] * b[k * wb + j];
        }

    clock_t t2 = clock();

    printf("Time of multiplication without OpenCL: %15.2f ms\n", ((float)(t2 - t1)) * 1000 / CLOCKS_PER_SEC);
    //output_matrix(res, ha, wb);

    if (CreateMemObjects(context, memObjects, (float *)a, (float *)b, 
        wa, ha, wb, hb))
    {
        fprintf(stderr, "Failed to create memory objects");
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }

    t1 = clock();
   
    //here product is computed with the OpenCL help
    errNum = clSetKernelArg(kernel, 0, sizeof(memObjects[0]), &memObjects[0]);
    errNum |= clSetKernelArg(kernel, 1, sizeof(memObjects[1]), &memObjects[1]);
    errNum |= clSetKernelArg(kernel, 2, sizeof(memObjects[2]), &memObjects[2]);
    errNum |= clSetKernelArg(kernel, 3, sizeof(int), &wa);
    errNum |= clSetKernelArg(kernel, 4, sizeof(int), &wb);
    if (errNum != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to initialize kernel's arguments");
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }

    size_t globalWorkSize[2] = {ha, wb};
    size_t localWorkSize[1] = {1};

    errNum = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL,
        globalWorkSize, localWorkSize, 0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to enqueue kernel's execution");
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }

    t2 = clock();
   
    errNum = clEnqueueReadBuffer(commandQueue, memObjects[2], CL_TRUE, 0,
        sizeof(float) * ARRAY_SIZE * ARRAY_SIZE, res, 0, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        fprintf(stderr, "Failure while trying to read buffer");
        Cleanup(context, commandQueue, program, kernel, memObjects, 3);
        return 1;
    }

    printf("Time of multiplication with OpenCL: %15.2f ms\n", ((float)(t2 - t1)) * 1000 / CLOCKS_PER_SEC);
    //output_matrix(res, ha, wb);

    Cleanup(context, commandQueue, program, kernel, memObjects, 3);
    return 0;
}
