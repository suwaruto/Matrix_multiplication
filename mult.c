#include <CL/cl.h>
#include <stdio.h>

#include "mult.h"

cl_context CreateContext()
{
    cl_int errNum;
    cl_uint numPlatforms;
    cl_platform_id firstPlatformId;
    cl_context context = NULL;

    errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
    if (errNum != CL_SUCCESS || numPlatforms <= 0)
    {
        fprintf(stderr, "Failed to find any OpenCL platforms");
        return NULL;
    }
    
    cl_context_properties contextProperties[] = 
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)firstPlatformId,
        0
    };
    context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
        NULL, NULL, &errNum);

    if (errNum != CL_SUCCESS)
    {
        printf("Could not create GPU context, trying CPU...");
        context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU,
            NULL, NULL, &errNum);

        if (errNum != CL_SUCCESS)
        {
            fprintf(stderr, "Failed to create an OpenCL GPU or CPU context");
            return NULL;
        }
    }

    return context;
}

cl_command_queue CreateCommandQueue( cl_context context, cl_device_id *device )
{
    cl_int errNum;
    cl_device_id *devices;
    cl_command_queue commandQueue = NULL;
    size_t deviceBufferSize = -1;

    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, 
        &deviceBufferSize);

    if (errNum != CL_SUCCESS)
    {
        fprintf(stderr, "Failed call to clGetContextInfo(...,"
            "CL_CONTEXT_DEVICES, ...)");
        return NULL;
    }

    if (deviceBufferSize <= 0)
    {
        fprintf(stderr, "No devices available");
        return NULL;
    }

    devices = (cl_device_id *)malloc(deviceBufferSize);
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize,
        devices, NULL);

    if (errNum != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to get device IDs");
        return NULL;
    }

    commandQueue = clCreateCommandQueueWithProperties(context, devices[0], NULL,
        &errNum);

    if (errNum != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to create commandQueue for device 0");
        return NULL;
    }

    *device = devices[0];
    free(devices);
    return commandQueue;
}

char *FileToString( FILE *file )
{
    char *buffer = NULL;
    size_t length = 0;

    if (file != NULL)
    {
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);
        buffer = (char *)malloc(length);
        if (buffer != NULL)
            fread(buffer, 1, length, file);
    }

    return buffer;
}

cl_program CreateProgram( cl_context context, cl_device_id device, 
    const char *fileName)
{
    cl_int errNum;
    cl_program program;
    
    FILE *kernelFile = fopen(fileName, "r");
    if (kernelFile == NULL)
    {
        fprintf(stderr, "Failed to open file for reading");
        return NULL;
    }
    char *srcStr = FileToString(kernelFile);
    fclose(kernelFile);

    program = clCreateProgramWithSource(context, 1, (const char **)&srcStr, 
        NULL, &errNum);

    free(srcStr);

    if (errNum != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to create CL program from source");
        return NULL;
    }

    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        char buildLog[16384];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
            sizeof(buildLog), buildLog, NULL);
        
        fprintf(stderr, "Error in kernel: ");
        fprintf(stderr, buildLog);
        clReleaseProgram(program);
        return NULL;
    }

    return program;
}

int CreateMemObjects( cl_context context, cl_mem *memObjects, float *a, 
    float *b, int wa, int ha, int wb, int hb )
{
    cl_int errNum1, errNum2, errNum3;

    memObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY |
        CL_MEM_COPY_HOST_PTR, wa * ha * sizeof(float), a,
        &errNum1);
    memObjects[1] = clCreateBuffer(context, CL_MEM_READ_ONLY |
        CL_MEM_COPY_HOST_PTR, wb * hb * sizeof(float), b,
        &errNum2);
    memObjects[2] = clCreateBuffer(context, CL_MEM_READ_WRITE, 
        sizeof(float) * ha * wb, NULL, &errNum3);

    if (errNum1 != CL_SUCCESS || errNum2 != CL_SUCCESS || errNum3 != CL_SUCCESS)
    {
        fprintf(stderr, "Error creating memory objects");
        return 1;
    }

    return 0;
}

void Cleanup( cl_context context, cl_command_queue commandQueue,
    cl_program program, cl_kernel kernel, cl_mem *memObjects,
    int memObjectsLength )
{
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(commandQueue);
    clReleaseContext(context);
    for (int i = 0; i < memObjectsLength; i++)
        clReleaseMemObject(memObjects[i]);
}
