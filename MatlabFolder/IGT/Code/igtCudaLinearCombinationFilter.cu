/*=========================================================================
 *
 *  Copyright IGT Consortium
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
#include "igtCudaLinearCombinationFilter.hcu"
#include "RTKModules\rtkCudaUtilities.hcu"

#include <itkMacro.h>

// cuda includes
#include <cuda.h>

// TEXTURES AND CONSTANTS //

__constant__ int3 c_Size;

//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
// K E R N E L S -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_( S T A R T )_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_

__global__
void
initialize_kernel(float m, float *in, float* out)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
      
  long int id = (k * c_Size.y + j) * c_Size.x + i;
  
  out[id] = m * in[id];
}

__global__
void
addconstant_kernel(float m, float* out)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
      
  long int id = (k * c_Size.y + j) * c_Size.x + i;
  
  out[id] = out[id] + m;
}

__global__
void
addimage_kernel(float m, float* in, float* out)
{
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i >= c_Size.x || j >= c_Size.y || k >= c_Size.z)
      return;
      
  long int id = (k * c_Size.y + j) * c_Size.x + i;
  
  out[id] = out[id] + m * in[id];
}

//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
// K E R N E L S -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-( E N D )-_-_
//_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_

void
CUDA_LinearCombination_Initialize(int size[3], float m, float* dev_in, float* dev_out)
{
	int3 dev_Size = make_int3(size[0], size[1], size[2]);
	cudaMemcpyToSymbol(c_Size, &dev_Size, sizeof(int3));
	long int memorySizeOutput = size[0] * size[1] * size[2] * sizeof(float);
    cudaMemset((void *)dev_out, 0, memorySizeOutput );
    
    dim3 dimBlock = dim3(16, 4, 4);
	int blocksInX = iDivUp(size[0], dimBlock.x);
	int blocksInY = iDivUp(size[1], dimBlock.y);
	int blocksInZ = iDivUp(size[2], dimBlock.z);
	dim3 dimGrid  = dim3(blocksInX, blocksInY, blocksInZ);
	
	initialize_kernel <<< dimGrid, dimBlock >>> ( m, dev_in, dev_out);
	
	cudaDeviceSynchronize();
	CUDA_CHECK_ERROR;
}
				   
void
CUDA_LinearCombination_AddConstant(int size[3], float m, float* dev_out)
{
	int3 dev_Size = make_int3(size[0], size[1], size[2]);
	cudaMemcpyToSymbol(c_Size, &dev_Size, sizeof(int3));
    
    dim3 dimBlock = dim3(16, 4, 4);
	int blocksInX = iDivUp(size[0], dimBlock.x);
	int blocksInY = iDivUp(size[1], dimBlock.y);
	int blocksInZ = iDivUp(size[2], dimBlock.z);
	dim3 dimGrid  = dim3(blocksInX, blocksInY, blocksInZ);
	
	addconstant_kernel <<< dimGrid, dimBlock >>> ( m, dev_out);
	
	cudaDeviceSynchronize();
	CUDA_CHECK_ERROR;
}

void
CUDA_LinearCombination_AddImage(int size[3], float m, float* dev_in, float* dev_out)
{
	int3 dev_Size = make_int3(size[0], size[1], size[2]);
	cudaMemcpyToSymbol(c_Size, &dev_Size, sizeof(int3));
    
    dim3 dimBlock = dim3(16, 4, 4);
	int blocksInX = iDivUp(size[0], dimBlock.x);
	int blocksInY = iDivUp(size[1], dimBlock.y);
	int blocksInZ = iDivUp(size[2], dimBlock.z);
	dim3 dimGrid  = dim3(blocksInX, blocksInY, blocksInZ);
	
	addimage_kernel <<< dimGrid, dimBlock >>> ( m, dev_in, dev_out);
	
	cudaDeviceSynchronize();
	CUDA_CHECK_ERROR;
}