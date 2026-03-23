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

#include "igtCudaLinearCombinationFilter.h"
#include "igtCudaLinearCombinationFilter.hcu"

#include <itkMacro.h>

namespace igt
{

CudaLinearCombinationFilter
::CudaLinearCombinationFilter()
{
}

void
CudaLinearCombinationFilter
::GPUGenerateData()
{
	float *pout = *(float**)( this->GetOutput()->GetCudaDataManager()->GetGPUBufferPointer() ); 

	int size[3] = {this->GetOutput()->GetLargestPossibleRegion().GetSize()[0],
		this->GetOutput()->GetLargestPossibleRegion().GetSize()[1],
		this->GetOutput()->GetLargestPossibleRegion().GetSize()[2]};

	for (unsigned int k = 0; k != this->NInputs(); ++k)
	{
		if (k == 0)
		{
			float *pin = *(float**)( this->GetInputImage(k)->GetCudaDataManager()->GetGPUBufferPointer() );
			CUDA_LinearCombination_Initialize( size,
				this->GetMultiplier(k), pin, pout);
		}
		else if (!this->GetInputImage(k))
			CUDA_LinearCombination_AddConstant(size, this->GetMultiplier(k), pout);
		else
		{
			float *pin = *(float**)( this->GetInputImage(k)->GetCudaDataManager()->GetGPUBufferPointer() );
			CUDA_LinearCombination_AddImage( size, 
				this->GetMultiplier(k), pin, pout);
		}
	}
}

}