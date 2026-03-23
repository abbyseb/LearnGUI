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

#include "rtkCudaTotalVariationGradientImageFilter.h"
#include "rtkCudaTotalVariationGradientImageFilter.hcu"

#include <itkMacro.h>

rtk::CudaTotalVariationGradientImageFilter
::CudaTotalVariationGradientImageFilter()
{
  m_UseImageSpacing = true;

  m_ScalingFactor = 1;
}

void
rtk::CudaTotalVariationGradientImageFilter
::GPUGenerateData()
{
	m_SpacingCoeffs= this->GetInput()->GetSpacing();
	if (m_UseImageSpacing == false) m_SpacingCoeffs.Fill(1);

    int inputSize[3];
    int outputSize[3];

    for (int i=0; i<3; i++)
      {
      inputSize[i] = this->GetInput()->GetBufferedRegion().GetSize()[i];
      outputSize[i] = this->GetOutput()->GetBufferedRegion().GetSize()[i];

      if (inputSize[i] != outputSize[i])
        {
        std::cerr << "The CUDA Total Variation Gradient filter can only handle input and output regions of equal size " << std::endl;
        exit(1);
        }
      }
	  
    float *pin = *(float**)( this->GetInput()->GetCudaDataManager()->GetGPUBufferPointer() );
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
		
      CUDA_total_variation_gradient_adaptive(inputSize,
							  pin,
							  pout,
							  m_ScalingFactor,
							  pwx,
							  pwy,
							  pwz);
	  } 
    else
      {
      CUDA_total_variation_gradient(inputSize,
							  pin,
							  pout,
							  m_ScalingFactor,
							  m_SpacingCoeffs[0],
							  m_SpacingCoeffs[1],
							  m_SpacingCoeffs[2]);
      }	  

}
