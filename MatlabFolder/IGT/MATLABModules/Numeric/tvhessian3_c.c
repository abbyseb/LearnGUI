#include "mex.h"

#include <math.h>
#include <stdio.h>
#include <matrix.h>

// Calculate 3D TV Hessian operation
//      result = tvhessian3_c(M,v,epsilon)

void mexFunction(
    int nlhs, mxArray *plhs[],
    int nrhs, const mxArray *prhs[])
{
    unsigned int x, y, z;

    // input
    const unsigned int *input_dimension;
    unsigned int input_dim_x;
    unsigned int input_dim_y;
    unsigned int input_dim_z;
    float *M;  // pointer to process content of M
	float *v;  // pointer to process content of v
    float *epsilon; // pointer to process epsilon

    // output
    unsigned int output_dimension[3];  // note: you should specify output dimension
    unsigned int output_dim_x;
    unsigned int output_dim_y;
    unsigned int output_dim_z;
    float *out;


    /* -------------------------------- */

    /* NECESSARY  (input processing) */

    M = (float *)mxGetPr(prhs[0]);  // get M pointer
	v = (float *)mxGetPr(prhs[1]);  // get v pointer
    epsilon = (float *)mxGetPr(prhs[2]); // get epsilon pointer
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

    for(x=0; x<output_dim_x; x++)
        for(y=0; y<output_dim_y; y++)
            for(z=0; z<output_dim_z; z++)
            {   
                unsigned int ind = x + y*input_dim_x + z*input_dim_x*input_dim_y;
                
                if ( x==0 || x==output_dim_x-1 || y==0 || y==output_dim_y-1 || z==0 || z==output_dim_z-1 ) {
                    out[ind] = 0;
                }
                else {
                    float f0  = M[ind];
                    float fx1 = M[ind-1];
					float fx2 = M[ind+1];
                    float fy1 = M[ind-input_dim_x];
					float fy2 = M[ind+input_dim_x];
                    float fz1 = M[ind-input_dim_x*input_dim_y];
					float fz2 = M[ind+input_dim_x*input_dim_y];
					float fx1y2 = M[ind-1+input_dim_x];
					float fx1z2 = M[ind-1+input_dim_x*input_dim_y];
					float fx2y1 = M[ind+1-input_dim_x];
					float fx2z1 = M[ind+1-input_dim_x*input_dim_y];
					float fy1z2 = M[ind-input_dim_x+input_dim_x*input_dim_y];
					float fy2z1 = M[ind-input_dim_x-input_dim_x*input_dim_y];
					float d0 = powf( (f0 - fx1)*(f0 - fx1) + (f0 - fy1)*(f0 - fy1) + (f0 - fz1)*(f0 - fz1) + epsilon[0] ,-1.5);
					float dx = powf( (fx2 - f0)*(fx2 - f0) + (fx2 - fx2y1)*(fx2 - fx2y1) + (fx2 - fx2z1)*(fx2 - fx2z1) + epsilon[0] ,-1.5);
					float dy = powf( (fy2 - fx1y2)*(fy2 - fx1y2) + (fy2 - f0)*(fy2 - f0) + (fy2 - fy2z1)*(fy2 - fy2z1) + epsilon[0] ,-1.5);
					float dz = powf( (fz2 - fx1z2)*(fz2 - fx1z2) + (fz2 - fy1z2)*(fz2 - fy1z2) + (fz2 - f0)*(fz2 - f0) + epsilon[0] ,-1.5);

                    out[ind] = 
					v[ind] * ( ( (fx1 - fy1)*(fx1 - fy1) + (fx1 - fz1)*(fx1 - fz1) + (fy1 - fz1)*(fy1 - fz1) ) * d0 + ( (fx2 - fx2y1)*(fx2 - fx2y1) + (fx2 - fx2z1)*(fx2 - fx2z1) ) * dx + ( (fy2 - fx1y2)*(fy2 - fx1y2) + (fy2 - fy2z1)*(fy2 - fy2z1) ) * dy + ( (fz2 - fx1z2)*(fz2 - fx1z2) + (fz2 - fy1z2)*(fz2 - fy1z2) ) * dz ) 
					+ v[ind-1] * ( (fx1 + f0)*(fy1 + fz1) - 2*f0*fx1 - fy1*fy1 - fz1*fz1 ) * d0
					+ v[ind-input_dim_x] * ( (fy1 + f0)*(fx1 + fz1) - 2*f0*fy1 - fx1*fx1 - fz1*fz1 ) * d0 
					+ v[ind-input_dim_x*input_dim_y] * ( (fz1 + f0)*(fx1 + fy1) - 2*f0*fz1 - fx1*fx1 - fy1*fy1 ) * d0 
					+ v[ind+1] * ( (f0 + fx2)*(fx2y1 + fx2z1) - 2*fx2*f0 - fx2y1*fx2y1 - fx2z1*fx2z1 ) * dx 
					+ v[ind+input_dim_x] * ( (f0 + fy2)*(fx1y2 + fy2z1) - 2*fy2*f0 - fx1y2*fx1y2 - fy2z1*fy2z1 ) * dy 
					+ v[ind+input_dim_x*input_dim_y] * ( (f0 + fz2)*(fx1z2 + fy1z2) - 2*fz2*f0 - fx1z2*fx1z2 - fy1z2*fy1z2 ) * dz 
					- v[ind+1-input_dim_x] * (fx2 - f0) * (fx2 - fx2y1) * dx 
					- v[ind+1-input_dim_x*input_dim_y] * (fx2 - f0) * (fx2 - fx2z1) * dx 
					- v[ind-1+input_dim_x] * (fy2 - f0) * (fy2 - fx1y2) * dy 
					- v[ind+input_dim_x-input_dim_x*input_dim_y] * (fy2 - f0) * (fy2 - fy2z1) * dy
					- v[ind-1+input_dim_x*input_dim_y] * (fz2 - f0) * (fz2 - fx1z2) * dz
					- v[ind-input_dim_x+input_dim_x*input_dim_y] * (fz2 - f0) * (fz2 - fy1z2) * dz;
                    
                }
            }
}