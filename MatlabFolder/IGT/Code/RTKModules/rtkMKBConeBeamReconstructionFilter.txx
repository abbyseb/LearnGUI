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

#ifndef __rtkMKBConeBeamReconstructionFilter_txx
#define __rtkMKBConeBeamReconstructionFilter_txx

#include "rtkMKBConeBeamReconstructionFilter.h"

#include <algorithm>
#include <itkTimeProbe.h>

namespace rtk
{
template<class TInputImage, class TOutputImage>
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::MKBConeBeamReconstructionFilter()
{
  // Input 0 is the initial image
  // Input 1 is the projection we read in.
  this->SetNumberOfRequiredInputs(2);

  // Default FDK type
  m_FDKType = "cpu";
  
  m_PositivityThreshold = true;
  
  m_ScalingFactor = 1;
  
  // Filter used to extract input image
  m_ExtractImageFilter = ExtractFilterType::New();
  
  // Zero multiply filter to create empty projection stack
  m_ZeroMultiplyFilter = MultiplyFilterType::New();
  m_ZeroMultiplyFilter->SetInput1( itk::NumericTraits<typename InputImageType::PixelType>::ZeroValue() );

  // Create Subtraction filter
  m_SubtractFilter = SubtractFilterType::New();

  // Create the filter that enforces positivity
  m_ThresholdFilter = ThresholdFilterType::New();
  m_ThresholdFilter->SetOutsideValue(0);
  m_ThresholdFilter->ThresholdBelow(0);
  
  // Multiply filter for scaling
  m_ScalingMultiplyFilter = MultiplyFilterType::New();
}

template<class TInputImage, class TOutputImage>
void
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::SetForwardProjectionFilter (int _arg)
{
  if( _arg != this->GetForwardProjectionFilter() )
    {
    Superclass::SetForwardProjectionFilter( _arg );
    m_ForwardProjectionFilter = this->InstantiateForwardProjectionFilter( _arg );
    }
}

template<class TInputImage, class TOutputImage>
void
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::SetFDKReconstructionFilter ( const char * _arg )
{
  m_FDKType = _arg;
  // Initialize FDK filter here
  if(!strcmp(m_FDKType, "cpu"))
  {
	  m_FDKReconstructionCPUFilter = FDKReconstructionFilterType::New();
  }
#ifdef IGT_USE_CUDA
  else if(!strcmp(m_FDKType, "cuda"))
  {
	  m_FDKReconstructionCUDAFilter = FDKReconstructionCUDAFilterType::New();
  }
#endif
}

template<class TInputImage, class TOutputImage>
void
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::InstantiateParkerWeightingFilter ( )
{
    m_ParkerWeightingFilter = ParkerWeightingFilterType::New();
}

#ifdef IGT_USE_CUDA
template<class TInputImage, class TOutputImage>
void
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::InstantiateParkerWeightingCUDAFilter ( )
{
    m_ParkerWeightingFilter = ParkerWeightingCUDAFilterType::New();
}
#endif

template<class TInputImage, class TOutputImage>
void
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::InstantiateDisplacedDetectorFilter ( )
{
    m_DisplacedDetectorFilter = DisplacedDetectorFilterType::New();
}

#ifdef IGT_USE_CUDA
template<class TInputImage, class TOutputImage>
void
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::InstantiateDisplacedDetectorCUDAFilter ( )
{
    m_DisplacedDetectorFilter = DisplacedDetectorCUDAFilterType::New();
}
#endif

template<class TInputImage, class TOutputImage>
void
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  typename Superclass::InputImagePointer inputPtr0 =
    const_cast< TInputImage * >( this->GetInput(0) );
	
  typename Superclass::InputImagePointer inputPtr1 =
    const_cast< TInputImage * >( this->GetInput(1) );

  if ( !inputPtr0 || !inputPtr1 )
    return;

  if(!strcmp(m_FDKType, "cpu"))
  {
	  m_FDKReconstructionCPUFilter->GetOutput()->SetRequestedRegion(this->GetOutput()->GetRequestedRegion() );
	  m_FDKReconstructionCPUFilter->GetOutput()->PropagateRequestedRegion();
  }
#ifdef IGT_USE_CUDA
  else if(!strcmp(m_FDKType, "cuda"))
  {
	  m_FDKReconstructionCUDAFilter->GetOutput()->SetRequestedRegion( this->GetOutput()->GetRequestedRegion() );
	  m_FDKReconstructionCUDAFilter->GetOutput()->PropagateRequestedRegion();
	  m_ParkerWeightingFilter->GetOutput()->SetRequestedRegion( m_FDKReconstructionCUDAFilter->GetBackProjectionFilter()->GetInput(1)->GetRequestedRegion() );
	  m_ParkerWeightingFilter->GetOutput()->PropagateRequestedRegion();
  }
#endif
}

template<class TInputImage, class TOutputImage>
void
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::GenerateOutputInformation()
{
  const unsigned int Dimension = this->InputImageDimension;

  // Set geometry
  if(this->GetGeometry().GetPointer() == NULL)
  {
	itkGenericExceptionMacro(<< "The geometry of the reconstruction has not been set");
  }
  m_ForwardProjectionFilter->SetGeometry( this->m_Geometry );
  m_DisplacedDetectorFilter->SetGeometry( this->m_Geometry );
  m_ParkerWeightingFilter->SetGeometry( this->m_Geometry );

  // Links filters and create pipeline here
  m_ZeroMultiplyFilter->SetInput2( this->GetInput(1) );
  m_ThresholdFilter->SetInput( this->GetInput(0) );
  m_ForwardProjectionFilter->SetInput( 0, m_ZeroMultiplyFilter->GetOutput() );
  if (m_PositivityThreshold)
    m_ForwardProjectionFilter->SetInput( 1, m_ThresholdFilter->GetOutput() );
  else
	m_ForwardProjectionFilter->SetInput( 1, this->GetInput(0) );
  m_SubtractFilter->SetInput(0, this->GetInput(1) );
  m_SubtractFilter->SetInput(1, m_ForwardProjectionFilter->GetOutput() );
  
  if ( m_ScalingFactor == 1 )
    m_DisplacedDetectorFilter->SetInput( m_SubtractFilter->GetOutput() );
  else
    {
	m_ScalingMultiplyFilter->SetConstant( m_ScalingFactor );
	m_ScalingMultiplyFilter->SetInput( m_SubtractFilter->GetOutput() );
	m_DisplacedDetectorFilter->SetInput( m_ScalingMultiplyFilter->GetOutput() );
	}
  
  m_ParkerWeightingFilter->SetInput( m_DisplacedDetectorFilter->GetOutput() );
  
  m_ExtractImageFilter->SetInput( this->GetInput(0) );
  m_ExtractImageFilter->SetExtractionRegion( this->GetInput(0)->GetLargestPossibleRegion() );
  //m_ExtractImageFilter->SetExtractionRegion( this->GetOutput()->GetRequestedRegion() );
  
  // Update output information using FDK
  if(!strcmp(m_FDKType, "cpu"))
  {
	  m_FDKReconstructionCPUFilter->SetGeometry( this->m_Geometry );
	  m_FDKReconstructionCPUFilter->SetInput(0, m_ExtractImageFilter->GetOutput() );
	  m_FDKReconstructionCPUFilter->SetInput( 1, m_ParkerWeightingFilter->GetOutput() );
	  m_FDKReconstructionCPUFilter->UpdateOutputInformation();
  }
#ifdef IGT_USE_CUDA
  else if(!strcmp(m_FDKType, "cuda"))
  {
	  m_FDKReconstructionCUDAFilter->SetGeometry( this->m_Geometry );
	  m_FDKReconstructionCUDAFilter->SetInput(0, m_ExtractImageFilter->GetOutput() );
	  m_FDKReconstructionCUDAFilter->SetInput( 1, m_ParkerWeightingFilter->GetOutput() );
	  m_FDKReconstructionCUDAFilter->UpdateOutputInformation();
  }
#endif
  
  #define SetOutputInfo_UsingFDK(f) \
	this->GetOutput()->SetOrigin( f->GetOutput()->GetOrigin() ); \
    this->GetOutput()->SetSpacing( f->GetOutput()->GetSpacing() ); \
    this->GetOutput()->SetDirection( f->GetOutput()->GetDirection() ); \
    this->GetOutput()->SetLargestPossibleRegion( f->GetOutput()->GetLargestPossibleRegion() )

  if(!strcmp(m_FDKType, "cpu"))
  {
	  SetOutputInfo_UsingFDK( m_FDKReconstructionCPUFilter );
  }
#ifdef IGT_USE_CUDA
  else if(!strcmp(m_FDKType, "cuda"))
  {
	  SetOutputInfo_UsingFDK( m_FDKReconstructionCUDAFilter );
  } 
#endif
}

template<class TInputImage, class TOutputImage>
void
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::GenerateData()
{
  m_ExtractImageProbe.Start();
  m_ExtractImageFilter->Update();
  m_ExtractImageProbe.Stop();

  m_MKBReconstructionProbe.Start();
  if(!strcmp(m_FDKType, "cpu"))
  {
	  m_FDKReconstructionCPUFilter->Update();
  }
#ifdef IGT_USE_CUDA
  else if(!strcmp(m_FDKType, "cuda"))
  {
	  m_FDKReconstructionCUDAFilter->Update();
  }
#endif
  m_MKBReconstructionProbe.Stop();

  if(!strcmp(m_FDKType, "cpu"))
  {
	  this->GraftOutput( m_FDKReconstructionCPUFilter->GetOutput() );
  }
#ifdef IGT_USE_CUDA
  else if(!strcmp(m_FDKType, "cuda"))
  {
	  this->GraftOutput( m_FDKReconstructionCUDAFilter->GetOutput() );
  }
#endif
}

template<class TInputImage, class TOutputImage>
void
MKBConeBeamReconstructionFilter<TInputImage, TOutputImage>
::PrintTiming(std::ostream & os) const
{
  os << "MKBConeBeamReconstructionFilter timing:" << std::endl;
  os << "  Extract input: " << m_ExtractImageProbe.GetTotal()
     << ' ' << m_ExtractImageProbe.GetUnit() << std::endl;
  os << "  MKB reconstruction: " << m_MKBReconstructionProbe.GetTotal()
     << ' ' << m_MKBReconstructionProbe.GetUnit() << std::endl;
}

} // end namespace rtk

#endif // __rtkMKBConeBeamReconstructionFilter_txx
