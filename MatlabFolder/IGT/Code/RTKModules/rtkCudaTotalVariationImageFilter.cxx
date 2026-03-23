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

#include "rtkCudaTotalVariationImageFilter.h"
#include "rtkCudaTotalVariationImageFilter.hcu"

#include <itkMacro.h>

rtk::CudaTotalVariationImageFilter
::CudaTotalVariationImageFilter()
{
  m_UseImageSpacing = true;

  m_ScalingFactor = 1;

  m_ImageSumFilter = ImageSumFilterType::New();
  
  m_TVSqrtImage = InputImageType::New();
  
  // allocate the data object for the output which is
  // just a decorator around real type
  RealObjectType::Pointer output =
      static_cast< RealObjectType * >( this->MakeOutput(1).GetPointer() );
    this->ProcessObject::SetNthOutput( 1, output.GetPointer() );

  this->GetTotalVariationOutput()->Set(NumericTraits< RealType >::Zero);
}

void
rtk::CudaTotalVariationImageFilter
::GPUGenerateData()
{
	m_SpacingCoeffs= this->GetInput()->GetSpacing();
	if (m_UseImageSpacing == false) m_SpacingCoeffs.Fill(1);

    int inputSize[3];

    for (int i=0; i<3; i++)
      {
      inputSize[i] = this->GetInput()->GetBufferedRegion().GetSize()[i];
      }
	  
    float *pin = *(float**)( this->GetInput()->GetCudaDataManager()->GetGPUBufferPointer() );

    m_TVSqrtImage->SetRegions( this->GetInput()->GetLargestPossibleRegion() );
	m_TVSqrtImage->Allocate();
	m_TVSqrtImage->FillBuffer(0);
	float *pout = *(float**)( m_TVSqrtImage->GetCudaDataManager()->GetGPUBufferPointer() );
	  
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
	    if (this->GetInput()->GetLargestPossibleRegion().GetSize(k) != this->GetWeightingImage()->GetLargestPossibleRegion().GetSize(k))
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
		
      CUDA_total_variation_adaptive(inputSize,
									pin,
									pout,
									m_ScalingFactor,
									pwx,
									pwy,
									pwz);
	  } 
    else
      {
      CUDA_total_variation(inputSize,
							pin,
							pout,
							m_ScalingFactor,
							m_SpacingCoeffs[0],
							m_SpacingCoeffs[1],
							m_SpacingCoeffs[2]);
      }	  
	  
	m_ImageSumFilter->SetInput( m_TVSqrtImage );
	m_ImageSumFilter->Update();
	this->GetTotalVariationOutput()->Set( (m_ImageSumFilter->GetSum()) / m_ScalingFactor);
}
