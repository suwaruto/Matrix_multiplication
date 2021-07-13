kernel void mult( global const float *a, global const float *b,
    global float *res, int wa, int wb )
{
    int row = get_global_id(0), col = get_global_id(1);

    res[row * wb + col] = 0;
    for (int i = 0; i < wa; i++)
        res[row * wb + col] += a[row * wa + i] * b[i * wb + col];
}
