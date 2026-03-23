#include "mex.h"

#include <math.h>
#include <stdio.h>
#include <matrix.h>

// Calculate 3D TV gradient
//      result = tvgrad3_c(image,eps)

void mexFunction(
    int nlhs, mxArray *plhs[],
    int nrhs, const mxArray *prhs[])
{
    int i, j, k;

    // input
    const int *input_dimension;
    int input_dim_x;
    int input_dim_y;
    int input_dim_z;
    float *in;  // pointer to process content of the input
    float *eps; // pointer to process epsilon

    // output
    int output_dimension[3];  // note: you should specify output dimension
    int output_dim_x;
    int output_dim_y;
    int output_dim_z;
    float *out;


    /* -------------------------------- */

    /* NECESSARY  (input processing) */

    in = (float *)mxGetPr(prhs[0]);  // get data pointer
    eps = (float *)mxGetPr(prhs[1]); // get epsilon pointer
    // mx: Matrix
    // P: Pointer
    // r: real
    // mxGetPr() has a counterprt mxGetPi();
    input_dimension = mxGetDimensions(prhs[0]);
    input_dim_x = input_dimension[0];
    input_dim_y = input_dimension[1];
    input_dim_z = input_dimension[2];

//     // print some message about input data
//     printf("nrhs: %d\n", nrhs);
//     printf("mxGetM(prhs[0]): %d\n", mxGetM(prhs[0]));
//     printf("mxGetN(prhs[0]): %d\n", mxGetN(prhs[0]));

//     // print input content
//     for(i=0; i<input_dim_x; i++)    // x
//         for(j=0; j<input_dim_y; j++)    // y
//             // notice: data type is "float", you shall use "%f" insted of "%d"
//             printf("%f\n", in[i + j*input_dim_x]);

    /* -------------------------------- */

    /* NECESSARY  (output allocation) */

    // specify output matrix's dimension
    output_dim_x = input_dim_x;
    output_dim_y = input_dim_y;
    output_dim_z = input_dim_z;
    output_dimension[0] = output_dim_x; // mxCreateNumericArray() required
    output_dimension[1] = output_dim_y;
    output_dimension[2] = output_dim_z;

    // Allocate the output matrix
    plhs[0] = mxCreateNumericArray(
        3,
        output_dimension,
        mxSINGLE_CLASS,
        mxREAL);

    // Get output matrix's data pointer
    out = (float *)mxGetPr(plhs[0]);  // just point at the start


    /* -------------------------------- */

    /* NECESSARY (data processing) */

    // Copy and add 1
    for(i=0; i<output_dim_x; i++)
        for(j=0; j<output_dim_y; j++)
            for(k=0; k<output_dim_z; k++)
            {   
                int ind = i + j*input_dim_x + k*input_dim_x*input_dim_y;
                
                if ( i==0 || i==output_dim_x-1 || j==0 || j==output_dim_y-1 || k==0 || k==output_dim_z-1 ) {
                    out[ind] = 0;
                }
                else {
                    float f  = in[ind];
                    float ax = in[ind-1];
                    float ay = in[ind-input_dim_x];
                    float az = in[ind-input_dim_x*input_dim_y];
                    float b  = in[ind+1];
                    float by = in[ind+1-input_dim_x];
                    float bz = in[ind+1-input_dim_x*input_dim_y];
                    float c  = in[ind+input_dim_x];
                    float cx = in[ind-1+input_dim_x];
                    float cz = in[ind+input_dim_x-input_dim_x*input_dim_y];
                    float d  = in[ind+input_dim_x*input_dim_y];
                    float dx = in[ind-1+input_dim_x*input_dim_y];
                    float dy = in[ind-input_dim_x+input_dim_x*input_dim_y];

                    out[ind] = (3*f - ax - ay - az) / sqrt(powf(f-ax,2) + powf(f-ay,2) + powf(f-az,2) + eps[0])
                                 - (b - f) / sqrt(powf(b-f,2) + powf(b-by,2) + powf(b-bz,2) + eps[0])
                                 - (c - f) / sqrt(powf(c-cx,2) + powf(c-f,2) + powf(c-cz,2) + eps[0])
                                 - (d - f) / sqrt(powf(d-dx,2) + powf(d-dy,2) + powf(d-f,2) + eps[0]);
                    
                }
            }
}