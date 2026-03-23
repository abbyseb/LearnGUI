/*=========================================================================
 *
 *  Copyright RTK Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

// rtk includes
#include "rtkCudaTotalVariationImageFilter.hcu"
#include "rtkCudaUtilities.hcu"

#include <itkMacro.h>

// cuda includes
#include <cuda.h>

// TEXTURES AND CONSTANTS //

__constant__ int3 c_Size;

//inline int iDivUp(int a, int b){
//    return (a % b != 0) ? (a / b + 1) : (a / b);
//}

//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
// K E R N E L S -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_( S T A R T )_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_

__global__
void
tv3_kernel(float *in, float *out, float spacingX2, float spacingY2, float spacingZ2)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i2 = i+1, j2 = j+1, k2 = k+1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  
  float f = in[id];
  float ax = in[(k * c_Size.y + j) * c_Size.x + i2];
  float ay = in[(k * c_Size.y + j2) * c_Size.x + i];
  float az = in[(k2 * c_Size.y + j) * c_Size.x + i];
  
  out[id] = sqrtf( (f-ax)*(f-ax)/spacingX2 + (f-ay)*(f-ay)/spacingY2 + (f-az)*(f-az)/spacingZ2 );
}

__global__
void
tv3_scaled_kernel(float *in, float *out, float scalingFactor, float spacingX2, float spacingY2, float spacingZ2)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i2 = i+1, j2 = j+1, k2 = k+1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  
  float f = scalingFactor * in[id];
  float ax = scalingFactor * in[(k * c_Size.y + j) * c_Size.x + i2];
  float ay = scalingFactor * in[(k * c_Size.y + j2) * c_Size.x + i];
  float az = scalingFactor * in[(k2 * c_Size.y + j) * c_Size.x + i];
  
  out[id] = sqrtf( (f-ax)*(f-ax)/spacingX2 + (f-ay)*(f-ay)/spacingY2 + (f-az)*(f-az)/spacingZ2 );
}

__global__
void
tv3_adaptive_kernel(float *in, float *out, float *wx, float *wy, float *wz)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i2 = i+1, j2 = j+1, k2 = k+1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  
  float f = in[id];
  float ax = in[(k * c_Size.y + j) * c_Size.x + i2];
  float ay = in[(k * c_Size.y + j2) * c_Size.x + i];
  float az = in[(k2 * c_Size.y + j) * c_Size.x + i];
  
  float wx0 = wx[id]; wx0 *= wx0;
  float wy0 = wy[id]; wy0 *= wy0;
  float wz0 = wz[id]; wz0 *= wz0;
  
  out[id] = sqrtf( (f-ax)*(f-ax)*wx0 + (f-ay)*(f-ay)*wy0 + (f-az)*(f-az)*wz0 );
}

__global__
void
tv3_adaptive_scaled_kernel(float *in, float *out, float scalingFactor, float *wx, float *wy, float *wz)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i2 = i+1, j2 = j+1, k2 = k+1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;

  float f = scalingFactor * in[id];
  float ax = scalingFactor * in[(k * c_Size.y + j) * c_Size.x + i2];
  float ay = scalingFactor * in[(k * c_Size.y + j2) * c_Size.x + i];
  float az = scalingFactor * in[(k2 * c_Size.y + j) * c_Size.x + i];
  
  float wx0 = wx[id]; wx0 *= wx0;
  float wy0 = wy[id]; wy0 *= wy0;
  float wz0 = wz[id]; wz0 *= wz0;

  out[id] = sqrtf( (f-ax)*(f-ax)*wx0 + (f-ay)*(f-ay)*wy0 + (f-az)*(f-az)*wz0 );
}

//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
// K E R N E L S -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-( E N D )-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_

void
CUDA_total_variation(int size[3],
						float* dev_in,
						float* dev_out,
						float scalingFactor,
						float spacingX,
						float spacingY,
						float spacingZ)
{
  int3 dev_Size = make_int3(size[0], size[1], size[2]);
  cudaMemcpyToSymbol(c_Size, &dev_Size, sizeof(int3));

  // Reset output volume
  long int memorySizeOutput = size[0] * size[1] * size[2] * sizeof(float);
  cudaMemset((void *)dev_out, 0, memorySizeOutput );
  
  // Thread Block Dimensions
  dim3 dimBlock = dim3(16, 4, 4);

  int blocksInX = iDivUp(size[0], dimBlock.x);
  int blocksInY = iDivUp(size[1], dimBlock.y);
  int blocksInZ = iDivUp(size[2], dimBlock.z);

  dim3 dimGrid  = dim3(blocksInX, blocksInY, blocksInZ);
  
  float spacingX2 = spacingX * spacingX;
  float spacingY2 = spacingY * spacingY;
  float spacingZ2 = spacingZ * spacingZ;
  
  if (scalingFactor==1)
	tv3_kernel <<< dimGrid, dimBlock >>> ( dev_in, dev_out, spacingX2, spacingY2, spacingZ2);
  else
	tv3_scaled_kernel <<< dimGrid, dimBlock >>> ( dev_in, dev_out, scalingFactor, spacingX2, spacingY2, spacingZ2);
  cudaDeviceSynchronize();
  CUDA_CHECK_ERROR;
}

void
CUDA_total_variation_adaptive(int size[3],
						float* dev_in,
						float* dev_out,
						float scalingFactor,
						float* dev_wx,
						float* dev_wy,
						float* dev_wz)
{
  int3 dev_Size = make_int3(size[0], size[1], size[2]);
  cudaMemcpyToSymbol(c_Size, &dev_Size, sizeof(int3));

  // Reset output volume
  long int memorySizeOutput = size[0] * size[1] * size[2] * sizeof(float);
  cudaMemset((void *)dev_out, 0, memorySizeOutput );
  
  // Thread Block Dimensions
  dim3 dimBlock = dim3(16, 4, 4);

  int blocksInX = iDivUp(size[0], dimBlock.x);
  int blocksInY = iDivUp(size[1], dimBlock.y);
  int blocksInZ = iDivUp(size[2], dimBlock.z);

  dim3 dimGrid  = dim3(blocksInX, blocksInY, blocksInZ);
  
  if (scalingFactor==1)
	tv3_adaptive_kernel <<< dimGrid, dimBlock >>> ( dev_in, dev_out, dev_wx, dev_wy, dev_wz);
  else
	tv3_adaptive_scaled_kernel <<< dimGrid, dimBlock >>> ( dev_in, dev_out, scalingFactor, dev_wx, dev_wy, dev_wz);
  cudaDeviceSynchronize();
  CUDA_CHECK_ERROR;
}
