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
#include "rtkCudaTotalVariationGradientImageFilter.hcu"
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
tvgrad3_kernel(float *in, float* out, float spacingX2, float spacingY2, float spacingZ2)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i1 = i-1, i2 = i+1, j1 = j-1, j2 = j+1, k1 = k-1, k2 = k+1;
  i1 = (i1 < 0) ? 0 : i1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j1 = (j1 < 0) ? 0 : j1;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k1 = (k1 < 0) ? 0 : k1;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  
  float f = in[id];
  float ax = in[(k * c_Size.y + j) * c_Size.x + i1];
  float ay = in[(k * c_Size.y + j1) * c_Size.x + i];
  float az = in[(k1 * c_Size.y + j) * c_Size.x + i];
  float b = in[(k * c_Size.y + j) * c_Size.x + i2];
  float by = in[(k * c_Size.y + j1) * c_Size.x + i2];
  float bz = in[(k1 * c_Size.y + j) * c_Size.x + i2];
  float c = in[(k * c_Size.y + j2) * c_Size.x + i];
  float cx = in[(k * c_Size.y + j2) * c_Size.x + i1];
  float cz = in[(k1 * c_Size.y + j2) * c_Size.x + i];
  float d = in[(k2 * c_Size.y + j) * c_Size.x + i];
  float dx = in[(k2 * c_Size.y + j) * c_Size.x + i1];
  float dy = in[(k2 * c_Size.y + j1) * c_Size.x + i];

  out[id] = ((f-ax)/spacingX2 + (f-ay)/spacingY2 + (f-az)/spacingZ2) / sqrtf( (f-ax)*(f-ax)/spacingX2 + (f-ay)*(f-ay)/spacingY2 + (f-az)*(f-az)/spacingZ2 + 1.19e-07 )
		- (b - f)/spacingX2 / sqrtf( (b-f)*(b-f)/spacingX2 + (b-by)*(b-by)/spacingY2 + (b-bz)*(b-bz)/spacingZ2 + 1.19e-07 )
		- (c - f)/spacingY2 / sqrtf( (c-cx)*(c-cx)/spacingX2 + (c-f)*(c-f)/spacingY2 + (c-cz)*(c-cz)/spacingZ2 + 1.19e-07 )
		- (d - f)/spacingZ2 / sqrtf( (d-dx)*(d-dx)/spacingX2 + (d-dy)*(d-dy)/spacingY2 + (d-f)*(d-f)/spacingZ2 + 1.19e-07 );
}

__global__
void
tvgrad3_scaled_kernel(float *in, float* out, float scalingFactor, float spacingX2, float spacingY2, float spacingZ2)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i1 = i-1, i2 = i+1, j1 = j-1, j2 = j+1, k1 = k-1, k2 = k+1;
  i1 = (i1 < 0) ? 0 : i1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j1 = (j1 < 0) ? 0 : j1;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k1 = (k1 < 0) ? 0 : k1;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  
  float f = scalingFactor * in[id];
  float ax = scalingFactor * in[(k * c_Size.y + j) * c_Size.x + i1];
  float ay = scalingFactor * in[(k * c_Size.y + j1) * c_Size.x + i];
  float az = scalingFactor * in[(k1 * c_Size.y + j) * c_Size.x + i];
  float b = scalingFactor * in[(k * c_Size.y + j) * c_Size.x + i2];
  float by = scalingFactor * in[(k * c_Size.y + j1) * c_Size.x + i2];
  float bz = scalingFactor * in[(k1 * c_Size.y + j) * c_Size.x + i2];
  float c = scalingFactor * in[(k * c_Size.y + j2) * c_Size.x + i];
  float cx = scalingFactor * in[(k * c_Size.y + j2) * c_Size.x + i1];
  float cz = scalingFactor * in[(k1 * c_Size.y + j2) * c_Size.x + i];
  float d = scalingFactor * in[(k2 * c_Size.y + j) * c_Size.x + i];
  float dx = scalingFactor * in[(k2 * c_Size.y + j) * c_Size.x + i1];
  float dy = scalingFactor * in[(k2 * c_Size.y + j1) * c_Size.x + i];

  out[id] = ((f-ax)/spacingX2 + (f-ay)/spacingY2 + (f-az)/spacingZ2) / sqrtf( (f-ax)*(f-ax)/spacingX2 + (f-ay)*(f-ay)/spacingY2 + (f-az)*(f-az)/spacingZ2 + 1.19e-07 )
		- (b - f)/spacingX2 / sqrtf( (b-f)*(b-f)/spacingX2 + (b-by)*(b-by)/spacingY2 + (b-bz)*(b-bz)/spacingZ2 + 1.19e-07 )
		- (c - f)/spacingY2 / sqrtf( (c-cx)*(c-cx)/spacingX2 + (c-f)*(c-f)/spacingY2 + (c-cz)*(c-cz)/spacingZ2 + 1.19e-07 )
		- (d - f)/spacingZ2 / sqrtf( (d-dx)*(d-dx)/spacingX2 + (d-dy)*(d-dy)/spacingY2 + (d-f)*(d-f)/spacingZ2 + 1.19e-07 );
}

__global__
void
tvgrad3_adaptive_kernel(float *in, float* out, float *wx, float *wy, float *wz)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i1 = i-1, i2 = i+1, j1 = j-1, j2 = j+1, k1 = k-1, k2 = k+1;
  i1 = (i1 < 0) ? 0 : i1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j1 = (j1 < 0) ? 0 : j1;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k1 = (k1 < 0) ? 0 : k1;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  
  float f = in[id];
  float ax = in[(k * c_Size.y + j) * c_Size.x + i1];
  float ay = in[(k * c_Size.y + j1) * c_Size.x + i];
  float az = in[(k1 * c_Size.y + j) * c_Size.x + i];
  float b = in[(k * c_Size.y + j) * c_Size.x + i2];
  float by = in[(k * c_Size.y + j1) * c_Size.x + i2];
  float bz = in[(k1 * c_Size.y + j) * c_Size.x + i2];
  float c = in[(k * c_Size.y + j2) * c_Size.x + i];
  float cx = in[(k * c_Size.y + j2) * c_Size.x + i1];
  float cz = in[(k1 * c_Size.y + j2) * c_Size.x + i];
  float d = in[(k2 * c_Size.y + j) * c_Size.x + i];
  float dx = in[(k2 * c_Size.y + j) * c_Size.x + i1];
  float dy = in[(k2 * c_Size.y + j1) * c_Size.x + i];
  
  float wx0 = wx[id]; wx0 *= wx0;
  float wy0 = wy[id]; wy0 *= wy0;
  float wz0 = wz[id]; wz0 *= wz0;
  float wxx = wx[(k * c_Size.y + j) * c_Size.x + i2]; wxx *= wxx;
  float wyx = wy[(k * c_Size.y + j) * c_Size.x + i2]; wyx *= wyx;
  float wzx = wz[(k * c_Size.y + j) * c_Size.x + i2]; wzx *= wzx;
  float wxy = wx[(k * c_Size.y + j2) * c_Size.x + i]; wxy *= wxy;
  float wyy = wy[(k * c_Size.y + j2) * c_Size.x + i]; wyy *= wyy;
  float wzy = wz[(k * c_Size.y + j2) * c_Size.x + i]; wzy *= wzy;
  float wxz = wx[(k2 * c_Size.y + j) * c_Size.x + i]; wxz *= wxz;
  float wyz = wy[(k2 * c_Size.y + j) * c_Size.x + i]; wyz *= wyz;
  float wzz = wz[(k2 * c_Size.y + j) * c_Size.x + i]; wzz *= wzz;

  out[id] = ((f-ax)*wx0 + (f-ay)*wy0 + (f-az)*wz0) / sqrtf( (f-ax)*(f-ax)*wx0 + (f-ay)*(f-ay)*wy0 + (f-az)*(f-az)*wz0 + 1.19e-07 )
		- (b - f)*wxx / sqrtf( (b-f)*(b-f)*wxx + (b-by)*(b-by)*wyx + (b-bz)*(b-bz)*wzx + 1.19e-07 )
		- (c - f)*wyy / sqrtf( (c-cx)*(c-cx)*wxy + (c-f)*(c-f)*wyy + (c-cz)*(c-cz)*wzy + 1.19e-07 )
		- (d - f)*wzz / sqrtf( (d-dx)*(d-dx)*wxz + (d-dy)*(d-dy)*wyz + (d-f)*(d-f)*wzz + 1.19e-07 );
}

__global__
void
tvgrad3_adaptive_scaled_kernel(float *in, float* out, float scalingFactor, float *wx, float *wy, float *wz)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
	  
  int i1 = i-1, i2 = i+1, j1 = j-1, j2 = j+1, k1 = k-1, k2 = k+1;
  i1 = (i1 < 0) ? 0 : i1;
  i2 = (i2 >= c_Size.x) ? (c_Size.x - 1) : i2;
  j1 = (j1 < 0) ? 0 : j1;
  j2 = (j2 >= c_Size.y) ? (c_Size.y - 1) : j2;
  k1 = (k1 < 0) ? 0 : k1;
  k2 = (k2 >= c_Size.z) ? (c_Size.z - 1) : k2;

  long int id = (k * c_Size.y + j) * c_Size.x + i;
  
  float f = scalingFactor * in[id];
  float ax = scalingFactor * in[(k * c_Size.y + j) * c_Size.x + i1];
  float ay = scalingFactor * in[(k * c_Size.y + j1) * c_Size.x + i];
  float az = scalingFactor * in[(k1 * c_Size.y + j) * c_Size.x + i];
  float b = scalingFactor * in[(k * c_Size.y + j) * c_Size.x + i2];
  float by = scalingFactor * in[(k * c_Size.y + j1) * c_Size.x + i2];
  float bz = scalingFactor * in[(k1 * c_Size.y + j) * c_Size.x + i2];
  float c = scalingFactor * in[(k * c_Size.y + j2) * c_Size.x + i];
  float cx = scalingFactor * in[(k * c_Size.y + j2) * c_Size.x + i1];
  float cz = scalingFactor * in[(k1 * c_Size.y + j2) * c_Size.x + i];
  float d = scalingFactor * in[(k2 * c_Size.y + j) * c_Size.x + i];
  float dx = scalingFactor * in[(k2 * c_Size.y + j) * c_Size.x + i1];
  float dy = scalingFactor * in[(k2 * c_Size.y + j1) * c_Size.x + i];
  
  float wx0 = wx[id]; wx0 *= wx0;
  float wy0 = wy[id]; wy0 *= wy0;
  float wz0 = wz[id]; wz0 *= wz0;
  float wxx = wx[(k * c_Size.y + j) * c_Size.x + i2]; wxx *= wxx;
  float wyx = wy[(k * c_Size.y + j) * c_Size.x + i2]; wyx *= wyx;
  float wzx = wz[(k * c_Size.y + j) * c_Size.x + i2]; wzx *= wzx;
  float wxy = wx[(k * c_Size.y + j2) * c_Size.x + i]; wxy *= wxy;
  float wyy = wy[(k * c_Size.y + j2) * c_Size.x + i]; wyy *= wyy;
  float wzy = wz[(k * c_Size.y + j2) * c_Size.x + i]; wzy *= wzy;
  float wxz = wx[(k2 * c_Size.y + j) * c_Size.x + i]; wxz *= wxz;
  float wyz = wy[(k2 * c_Size.y + j) * c_Size.x + i]; wyz *= wyz;
  float wzz = wz[(k2 * c_Size.y + j) * c_Size.x + i]; wzz *= wzz;
  
  out[id] = ((f-ax)*wx0 + (f-ay)*wy0 + (f-az)*wz0) / sqrtf( (f-ax)*(f-ax)*wx0 + (f-ay)*(f-ay)*wy0 + (f-az)*(f-az)*wz0 + 1.19e-07 )
		- (b - f)*wxx / sqrtf( (b-f)*(b-f)*wxx + (b-by)*(b-by)*wyx + (b-bz)*(b-bz)*wzx + 1.19e-07 )
		- (c - f)*wyy / sqrtf( (c-cx)*(c-cx)*wxy + (c-f)*(c-f)*wyy + (c-cz)*(c-cz)*wzy + 1.19e-07 )
		- (d - f)*wzz / sqrtf( (d-dx)*(d-dx)*wxz + (d-dy)*(d-dy)*wyz + (d-f)*(d-f)*wzz + 1.19e-07 );
}

//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
// K E R N E L S -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-( E N D )-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_

void
CUDA_total_variation_gradient(int size[3],
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
	tvgrad3_kernel <<< dimGrid, dimBlock >>> ( dev_in, dev_out, spacingX2, spacingY2, spacingZ2);
  else
	tvgrad3_scaled_kernel <<< dimGrid, dimBlock >>> ( dev_in, dev_out, scalingFactor, spacingX2, spacingY2, spacingZ2);
  cudaDeviceSynchronize();
  CUDA_CHECK_ERROR;
}

void
CUDA_total_variation_gradient_adaptive(int size[3],
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
	tvgrad3_adaptive_kernel <<< dimGrid, dimBlock >>> ( dev_in, dev_out, dev_wx, dev_wy, dev_wz);
  else
	tvgrad3_adaptive_scaled_kernel <<< dimGrid, dimBlock >>> ( dev_in, dev_out, scalingFactor, dev_wx, dev_wy, dev_wz);
  cudaDeviceSynchronize();
  CUDA_CHECK_ERROR;
}
