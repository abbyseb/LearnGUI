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

#include "rtkCudaTotalVariationHessianOperatorImageFilter.h"
#include "rtkCudaTotalVariationHessianOperatorImageFilter.hcu"

#include <itkMacro.h>

rtk::CudaTotalVariationHessianOperatorImageFilter
::CudaTotalVariationHessianOperatorImageFilter()
{
  // Input 0: The image based on which TV hessian matrix is calculated
  // Input 1: The v vector the matrix operates on
  this->SetNumberOfRequiredInputs(2);
  
  m_UseImageSpacing = true;

  m_ScalingFactor = 1;
}

void
rtk::CudaTotalVariationHessianOperatorImageFilter
::GPUGenerateData()
{
	m_SpacingCoeffs= this->GetInput()->GetSpacing();
	if (m_UseImageSpacing == false) m_SpacingCoeffs.Fill(1);

    int inputSize[3], input1Size[3];
    int outputSize[3];

    for (int i=0; i<3; i++)
      {
      inputSize[i] = this->GetInput(0)->GetBufferedRegion().GetSize()[i];
	  input1Size[i] = this->GetInput(1)->GetBufferedRegion().GetSize()[i];
      outputSize[i] = this->GetOutput()->GetBufferedRegion().GetSize()[i];

      if (inputSize[i] != outputSize[i] || inputSize[i] != input1Size[i])
        {
        std::cerr << "The CUDA Total Variation Hessian operator filter can only handle input and output regions of equal size " << std::endl;
        exit(1);
        }
      }

    float *pin0 = *(float**)( this->GetInput(0)->GetCudaDataManager()->GetGPUBufferPointer() );
	float *pin1 = *(float**)( this->GetInput(1)->GetCudaDataManager()->GetGPUBufferPointer() );
    float *pout = *(float**)( this->GetOutput()->GetCudaDataManager()->GetGPUBufferPointer() );

    // Different operations depending on whether WeightingImage is provided
    if ( this->GetWeightingImage() )
      {	
	  // Check dimension of weighting image
	  if (this->GetWeightingImage()->GetVectorLength() != 3)
	    {
	    std::cerr << "The dimension of the adaptive weighting vector image does not match with the input image " << std::endl;
        exit(1);
	    }
      for (unsigned int k = 0; k < 3; ++k)
	    {
	    if (this->GetInput(0)->GetLargestPossibleRegion().GetSize(k) != this->GetWeightingImage()->GetLargestPossibleRegion().GetSize(k))
	      {
		  std::cerr << "The dimension of the adaptive weighting vector image does not match with the input image " << std::endl;
		  exit(1);
		  }
	    }

	  IndexSelectionType::Pointer wx = IndexSelectionType::New();
	  IndexSelectionType::Pointer wy = IndexSelectionType::New();
	  IndexSelectionType::Pointer wz = IndexSelectionType::New();
	  wx->SetIndex(0);
	  wy->SetIndex(1);
	  wz->SetIndex(2);
	  wx->SetInput( m_WeightingImage );
	  wy->SetInput( m_WeightingImage );
	  wz->SetInput( m_WeightingImage );
	  wx->Update();
	  wy->Update();
	  wz->Update();
	  
	  float *pwx = *(float**)( wx->GetOutput()->GetCudaDataManager()->GetGPUBufferPointer() );
	  float *pwy = *(float**)( wy->GetOutput()->GetCudaDataManager()->GetGPUBufferPointer() );
	  float *pwz = *(float**)( wz->GetOutput()->GetCudaDataManager()->GetGPUBufferPointer() );
		
      CUDA_total_variation_hessian_operator_adaptive(inputSize,
							  pin0,
							  pin1,
							  pout,
							  m_ScalingFactor,
							  pwx,
							  pwy,
							  pwz);
	  } 
    else
      {
      CUDA_total_variation_hessian_operator(inputSize,
							  pin0,
							  pin1,
							  pout,
							  m_ScalingFactor,
							  m_SpacingCoeffs[0],
							  m_SpacingCoeffs[1],
							  m_SpacingCoeffs[2]);
      }	
}
