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

#ifndef __rtkPICCSConeBeamReconstructionFilter_txx
#define __rtkPICCSConeBeamReconstructionFilter_txx

#include "rtkTotalVariationImageFilter.h"
#include "rtkTotalVariationGradientImageFilter.h"
#include "rtkTotalVariationHessianOperatorImageFilter.h"
#include "rtkDisplacedDetectorImageFilter.h"

#ifdef IGT_USE_CUDA
  #include "rtkCudaTotalVariationImageFilter.h"
  #include "rtkCudaTotalVariationGradientImageFilter.h"
  #include "rtkCudaTotalVariationHessianOperatorImageFilter.h"
  #include "rtkCudaDisplacedDetectorImageFilter.h"
#endif

#include "rtkPICCSConeBeamReconstructionFilter.h"

#include <algorithm>
#include <itkTimeProbe.h>

namespace rtk
{
template<class TInputImage, class TOutputImage>
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::PICCSConeBeamReconstructionFilter()
{
  // Input 0: Initial image
  // Input 1: projections
  // Input 2: Prior image
  this->SetNumberOfRequiredInputs(3);
  
  m_CurrentTVConfiguration = -1;
  m_CurrentTVGradientConfiguration = -1;
  m_CurrentTVHessianConfiguration = -1;
  m_CurrentDisplacedDetectorConfiguration = -1;

  // Set default parameters
  m_NumberOfIterations = 20;
  m_Lambda = 1;
  m_Alpha = 0.5;
  m_C1 = 1e-4;
  m_C2 = 0.5;
  m_TVScalingFactor = 1e4;
  m_StoppingCriterion = 0.;
  
  m_TruncationCalibrationMap = 0;

  // Zero multiply filter
  m_ZeroMultiplyFilterP = MultiplyFilterType::New();
  m_ZeroMultiplyFilterX = MultiplyFilterType::New();
  m_ZeroMultiplyFilterP->SetConstant( itk::NumericTraits<typename InputImageType::PixelType>::ZeroValue() );
  m_ZeroMultiplyFilterX->SetConstant( itk::NumericTraits<typename InputImageType::PixelType>::ZeroValue() );
  
  // s = s_old + eta*Rd
  m_MinusP_SubtractFilter = SubtractFilterType::New();
  m_Rd_MultiplyFilter = MultiplyFilterType::New();
  m_s_AddFilter = AddFilterType::New();
  m_MinusP_SubtractFilter->SetConstant1( itk::NumericTraits<typename InputImageType::PixelType>::ZeroValue() );
  m_Rd_MultiplyFilter->SetConstant( 1. );
  m_s_AddFilter->SetInput1( m_MinusP_SubtractFilter->GetOutput() );
  m_s_AddFilter->SetInput2( m_Rd_MultiplyFilter->GetOutput() );
  
  // Calculate norm(s)^2
  m_sNormSquared_StatsFilter = ImageStatisticsFilterType::New();
  m_sNormSquared_StatsFilter->SetInput( m_s_AddFilter->GetOutput() );
  
  // x-xp term
  m_Xp_SubtractFilter = SubtractFilterType::New();
  
  // g, image TV gradient term
  m_TVGradX_MultiplyFilter = MultiplyFilterType::New();
  
  // g, prior image TV gradient term
  m_TVGradXp_MultiplyFilter = MultiplyFilterType::New();
  
  // g, Adding terms up
  m_TVGrad_AddFilter = AddFilterType::New();
  m_g_AddFilter = AddFilterType::New();
  m_TVGrad_AddFilter->SetInput1( m_TVGradX_MultiplyFilter->GetOutput() );
  m_TVGrad_AddFilter->SetInput2( m_TVGradXp_MultiplyFilter->GetOutput() );
  m_g_AddFilter->SetInput2( m_TVGrad_AddFilter->GetOutput() );
  
  // d = beta*d_prev - g
  m_betad_MultiplyFilter = MultiplyFilterType::New();
  m_d_SubtractFilter = SubtractFilterType::New();
  m_betad_MultiplyFilter->SetInput( m_ZeroMultiplyFilterX->GetOutput() );
  m_betad_MultiplyFilter->SetConstant( itk::NumericTraits<typename InputImageType::PixelType>::ZeroValue() );
  m_d_SubtractFilter->SetInput1( m_betad_MultiplyFilter->GetOutput() );
  m_d_SubtractFilter->SetInput2( m_g_AddFilter->GetOutput() );
  
  // xnew = x + eta*d
  m_d_etaMultiplyFilter = MultiplyFilterType::New();
  m_xnew_AddFilter = AddFilterType::New();
  m_d_etaMultiplyFilter->SetInput( m_d_SubtractFilter->GetOutput() );
  m_d_etaMultiplyFilter->SetConstant( 1. );
  m_xnew_AddFilter->SetInput2( m_d_etaMultiplyFilter->GetOutput() );
  
  // Calculate the norm of change in image
  m_etad_StatisticsFilter = ImageStatisticsFilterType::New();
  m_etad_StatisticsFilter->SetInput( m_d_etaMultiplyFilter->GetOutput() );
  
  // g2, image TV hessian term
  m_TVHessianX_MultiplyFilter = MultiplyFilterType::New();
  
  // g2, prior image TV hessian term
  m_TVHessianXp_MultiplyFilter = MultiplyFilterType::New();
  
  // g2, Adding terms up
  m_TVHessian_AddFilter = AddFilterType::New();
  m_g2_AddFilter = AddFilterType::New();
  m_TVHessian_AddFilter->SetInput1( m_TVHessianX_MultiplyFilter->GetOutput() );
  m_TVHessian_AddFilter->SetInput2( m_TVHessianXp_MultiplyFilter->GetOutput() );
  m_g2_AddFilter->SetInput2( m_TVHessian_AddFilter->GetOutput() );
  
  // Calculate g norm squared
  m_gNormSquared_StatsFilter = ImageStatisticsFilterType::New();
  m_gNormSquared_StatsFilter->SetInput( m_g_AddFilter->GetOutput() );
  
  // Calculate rho1
  m_dg_MultiplyFilter = MultiplyFilterType::New();
  m_rho1_StatsFilter = ImageStatisticsFilterType::New();
  m_dg_MultiplyFilter->SetInput2( m_g_AddFilter->GetOutput() );
  m_rho1_StatsFilter->SetInput( m_dg_MultiplyFilter->GetOutput() );

  // Calculate rho2
  m_dg2_MultiplyFilter = MultiplyFilterType::New();
  m_rho2_StatsFilter = ImageStatisticsFilterType::New();
  m_dg2_MultiplyFilter->SetInput2( m_g2_AddFilter->GetOutput() );
  m_rho2_StatsFilter->SetInput( m_dg2_MultiplyFilter->GetOutput() );
}

template<class TInputImage, class TOutputImage>
typename PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>::DisplacedDetectorPointerType
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::InstantiateDisplacedDetectorFilter (int ddtype)
{
  DisplacedDetectorPointerType dd;
  switch(ddtype)
    {
    case(0): // CPU
      dd = rtk::DisplacedDetectorImageFilter<TOutputImage>::New();
    break;
    case(1): // Cuda
	#ifdef IGT_USE_CUDA
      dd = rtk::CudaDisplacedDetectorImageFilter::New();
    #else
      std::cerr << "The program has not been compiled with cuda option" << std::endl;
    #endif
    break;
    default:
      std::cerr << "Unhandled displaced detector type value." << std::endl;
    }
  return dd;
}

template<class TInputImage, class TOutputImage>
void
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::SetDisplacedDetectorFilter (int ddtype)
{
  if (m_CurrentDisplacedDetectorConfiguration != ddtype)
    {
    this->Modified();
	m_RTs_DDFilter = this->InstantiateDisplacedDetectorFilter( ddtype );
	m_RTRd_DDFilter = this->InstantiateDisplacedDetectorFilter( ddtype );
	m_RTs_DDFilter->InPlaceOff();
	m_RTs_DDFilter->SetPadOnTruncatedSide(false);
	m_RTRd_DDFilter->InPlaceOff();
	m_RTRd_DDFilter->SetPadOnTruncatedSide(false);
    }
}

template<class TInputImage, class TOutputImage>
typename PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>::TVFilterPointerType
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::InstantiateTVFilter (int tvtype)
{
  TVFilterPointerType tv;
  switch(tvtype)
    {
    case(0): // CPU
      tv = rtk::TotalVariationImageFilter<TOutputImage>::New();
    break;
    case(1): // Cuda
	#ifdef IGT_USE_CUDA
      tv = rtk::CudaTotalVariationImageFilter::New();
    #else
      std::cerr << "The program has not been compiled with cuda option" << std::endl;
    #endif
    break;
    default:
      std::cerr << "Unhandled TV type value." << std::endl;
    }
  return tv;
}

template<class TInputImage, class TOutputImage>
void
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::SetTVFilter (int tvtype)
{
  if (m_CurrentTVConfiguration != tvtype)
    {
    this->Modified();

    m_TVX_TVFilter = this->InstantiateTVFilter( tvtype );
    m_TVXp_TVFilter = this->InstantiateTVFilter( tvtype );
    }
}

template<class TInputImage, class TOutputImage>
typename PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>::TVGradientPointerType
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::InstantiateTVGradientFilter (int tvgradtype)
{
  TVGradientPointerType tvgrad;
  switch(tvgradtype)
    {
    case(0): // CPU
      tvgrad = rtk::TotalVariationGradientImageFilter<TOutputImage,TOutputImage>::New();
    break;
    case(1): // Cuda
	#ifdef IGT_USE_CUDA
      tvgrad = rtk::CudaTotalVariationGradientImageFilter::New();
    #else
      std::cerr << "The program has not been compiled with cuda option" << std::endl;
    #endif
    break;
    default:
      std::cerr << "Unhandled TV gradient type value." << std::endl;
    }
  return tvgrad;
}

template<class TInputImage, class TOutputImage>
void
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::SetTVGradientFilter (int tvgradtype)
{
  if (m_CurrentTVGradientConfiguration != tvgradtype)
    {
    this->Modified();

	m_TVGradX_TVGradFilter = this->InstantiateTVGradientFilter( tvgradtype );
	m_TVGradXp_TVGradFilter = this->InstantiateTVGradientFilter( tvgradtype );
    }
}

template<class TInputImage, class TOutputImage>
typename PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>::TVHessianPointerType
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::InstantiateTVHessianFilter (int tvhessiantype)
{
  TVHessianPointerType tvhessian;
  switch(tvhessiantype)
    {
    case(0): // CPU
      tvhessian = rtk::TotalVariationHessianOperatorImageFilter<TOutputImage,TOutputImage>::New();
	  tvhessian->SetNumberOfThreads(1);
    break;
    case(1): // Cuda
	#ifdef IGT_USE_CUDA
      tvhessian = rtk::CudaTotalVariationHessianOperatorImageFilter::New();
    #else
      std::cerr << "The program has not been compiled with cuda option" << std::endl;
    #endif
    break;
    default:
      std::cerr << "Unhandled TV Hessian type value." << std::endl;
    }
  return tvhessian;
}

template<class TInputImage, class TOutputImage>
void
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::SetTVHessianFilter (int tvhessiantype)
{
  if (m_CurrentTVHessianConfiguration != tvhessiantype)
    {
    this->Modified();
	m_TVHessianX_TVHessianFilter = this->InstantiateTVHessianFilter( tvhessiantype );
	m_TVHessianXp_TVHessianFilter = this->InstantiateTVHessianFilter( tvhessiantype );
    }
}

template<class TInputImage, class TOutputImage>
void
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::SetForwardProjectionFilter (int _arg)
{
  if( _arg != this->GetForwardProjectionFilter() )
    {
    Superclass::SetForwardProjectionFilter( _arg );
    m_Rd_FPFilter = this->InstantiateForwardProjectionFilter( _arg );
	// m_Rd_FPFilter->InPlaceOff();
    }
}

template<class TInputImage, class TOutputImage>
void
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::SetBackProjectionFilter (int _arg)
{
  if( _arg != this->GetBackProjectionFilter() )
    {
    Superclass::SetBackProjectionFilter( _arg );
    m_RTs_BPFilter = this->InstantiateBackProjectionFilter( _arg );
	m_RTRd_BPFilter = this->InstantiateBackProjectionFilter( _arg );
	// m_RTs_BPFilter->InPlaceOff();
	// m_RTRd_BPFilter->InPlaceOff();
    }
}

template<class TInputImage, class TOutputImage>
void
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  typename Superclass::InputImagePointer inputPtr0 =
    const_cast< TInputImage * >( this->GetInput(0) );
	
  typename Superclass::InputImagePointer inputPtr1 =
    const_cast< TInputImage * >( this->GetInput(1) );

  typename Superclass::InputImagePointer inputPtr2 =
    const_cast< TInputImage * >( this->GetInput(2) );
	
  if ( !inputPtr0 || !inputPtr1 || !inputPtr2 )
    return;

  m_RTs_BPFilter->GetOutput()->SetRequestedRegion(this->GetOutput()->GetRequestedRegion() );
  m_RTs_BPFilter->GetOutput()->PropagateRequestedRegion();
}

template<class TInputImage, class TOutputImage>
void
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::GenerateOutputInformation()
{
  // Check if input 0 and input 2 has the same dimension
  for (unsigned int k = 0; k < this->InputImageDimension; ++k)
    {
	if (this->GetInput(0)->GetLargestPossibleRegion().GetSize(k) != this->GetInput(2)->GetLargestPossibleRegion().GetSize(k))
	  {
	  itkGenericExceptionMacro(<< "The reconstruction dimension does not match with the prior image dimension");
	  return;
	  }
    }

  // Check dimension of W_TV if input, and then set it to all tv filters
  if ( m_TVWeightingImage )
    {	
	if (m_TVWeightingImage->GetVectorLength() != TInputImage::ImageDimension)
	  {
	  itkGenericExceptionMacro(<< "The dimension of the adaptive TV weighting vector image does not match with the input image");
      return;
	  }
	
    for (unsigned int k = 0; k < TInputImage::ImageDimension; ++k)
	  {
	  if (this->GetInput(0)->GetLargestPossibleRegion().GetSize(k) != m_TVWeightingImage->GetLargestPossibleRegion().GetSize(k))
	    {
		itkGenericExceptionMacro(<< "The dimension of the adaptive TV weighting vector image does not match with the input image");
		return;
		}
	  }
	m_TVX_TVFilter->SetWeightingImage( m_TVWeightingImage );
	m_TVGradX_TVGradFilter->SetWeightingImage( m_TVWeightingImage );
	m_TVHessianX_TVHessianFilter->SetWeightingImage( m_TVWeightingImage );
	}

  // Check dimension of W_Prior if input, and then set it to all tv filters
  if ( m_PriorWeightingImage )
    {	
	if (m_PriorWeightingImage->GetVectorLength() != TInputImage::ImageDimension)
	  {
	  itkGenericExceptionMacro(<< "The dimension of the adaptive prior weighting vector image does not match with the input image");
      return;
	  }
	
    for (unsigned int k = 0; k < TInputImage::ImageDimension; ++k)
	  {
	  if (this->GetInput(0)->GetLargestPossibleRegion().GetSize(k) != m_PriorWeightingImage->GetLargestPossibleRegion().GetSize(k))
	    {
		itkGenericExceptionMacro(<< "The dimension of the adaptive prior weighting vector image does not match with the input image");
		return;
		}
	  }
	m_TVXp_TVFilter->SetWeightingImage( m_PriorWeightingImage );
	m_TVGradXp_TVGradFilter->SetWeightingImage( m_PriorWeightingImage );
	m_TVHessianXp_TVHessianFilter->SetWeightingImage( m_PriorWeightingImage );
	}

  // Check and set geometry
  if(this->GetGeometry().GetPointer() == NULL)
    {
    itkGenericExceptionMacro(<< "The geometry of the reconstruction has not been set");
    }
  // Zero multiply filter
  m_ZeroMultiplyFilterP->SetInput( this->GetInput(1) );
  m_ZeroMultiplyFilterX->SetInput( this->GetInput(0) );
	
  // s = s_old + eta*Rd
  if (!m_TruncationCalibrationMap)
	m_MinusP_SubtractFilter->SetInput2( this->GetInput(1) );
  else
  {
	m_TruncationCalibrationFilter = MultiplyFilterType::New();
	m_TruncationCalibrationFilter->SetInput(0, this->GetInput(1));
	m_TruncationCalibrationFilter->SetInput(1, m_TruncationCalibrationMap);
	m_MinusP_SubtractFilter->SetInput2( m_TruncationCalibrationFilter->GetOutput() );
  }
  m_Rd_FPFilter->SetGeometry(this->m_Geometry);
  m_Rd_FPFilter->SetInput( 0, m_ZeroMultiplyFilterP->GetOutput() );
  m_Rd_FPFilter->SetInput( 1, this->GetInput(0) );
  m_Rd_MultiplyFilter->SetInput( m_Rd_FPFilter->GetOutput() );
  
  // x - xp term
  m_Xp_SubtractFilter->SetInput1( this->GetInput(0) );
  m_Xp_SubtractFilter->SetInput2( this->GetInput(2) );
  
  // Calculate TV(x) and TV(x-xp)
  m_TVX_TVFilter->SetInput( this->GetInput(0) );
  m_TVXp_TVFilter->SetInput( m_Xp_SubtractFilter->GetOutput() );
  m_TVX_TVFilter->SetScalingFactor( m_TVScalingFactor );
  m_TVXp_TVFilter->SetScalingFactor( m_TVScalingFactor );

  // g, data fidelity term RTs
  m_RTs_DDFilter->SetGeometry(this->m_Geometry);
  m_RTs_BPFilter->SetGeometry(this->m_Geometry.GetPointer());
  m_RTs_DDFilter->SetInput( m_s_AddFilter->GetOutput() );
  m_RTs_BPFilter->SetInput( 0, m_ZeroMultiplyFilterX->GetOutput() );
  m_RTs_BPFilter->SetInput( 1, m_RTs_DDFilter->GetOutput() );

  // g, image TV gradient term
  m_TVGradX_TVGradFilter->SetScalingFactor( m_TVScalingFactor );
  m_TVGradX_TVGradFilter->SetInput( this->GetInput(0) );
  m_TVGradX_MultiplyFilter->SetConstant( m_Lambda * (1 - m_Alpha) );
  m_TVGradX_MultiplyFilter->SetInput( m_TVGradX_TVGradFilter->GetOutput() );
  
  // g, prior image TV gradient term
  m_TVGradXp_TVGradFilter->SetScalingFactor( m_TVScalingFactor );
  m_TVGradXp_TVGradFilter->SetInput( m_Xp_SubtractFilter->GetOutput() );
  m_TVGradXp_MultiplyFilter->SetConstant( m_Lambda * m_Alpha );
  m_TVGradXp_MultiplyFilter->SetInput( m_TVGradXp_TVGradFilter->GetOutput() );
  
  // g, Adding terms up
  m_g_AddFilter->SetInput1( m_RTs_BPFilter->GetOutput() );
  
  // g2, R^TRd term
  m_RTRd_DDFilter->SetGeometry(this->m_Geometry);
  m_RTRd_BPFilter->SetGeometry(this->m_Geometry.GetPointer());
  m_RTRd_DDFilter->SetInput( m_Rd_FPFilter->GetOutput() );
  m_RTRd_BPFilter->SetInput( 0, m_ZeroMultiplyFilterX->GetOutput() );
  m_RTRd_BPFilter->SetInput( 1, m_RTRd_DDFilter->GetOutput() );
  
  // g2, image TV hessian term
  m_TVHessianX_TVHessianFilter->SetScalingFactor( m_TVScalingFactor );
  m_TVHessianX_TVHessianFilter->SetInput( 0, this->GetInput(0) );
  m_TVHessianX_MultiplyFilter->SetConstant( m_Lambda * (1 - m_Alpha) );
  m_TVHessianX_MultiplyFilter->SetInput( m_TVHessianX_TVHessianFilter->GetOutput() );
  
  // g2, prior image TV hessian term
  m_TVHessianXp_TVHessianFilter->SetScalingFactor( m_TVScalingFactor );
  m_TVHessianXp_TVHessianFilter->SetInput( 0, m_Xp_SubtractFilter->GetOutput() );
  m_TVHessianXp_MultiplyFilter->SetConstant( m_Lambda * m_Alpha );
  m_TVHessianXp_MultiplyFilter->SetInput( m_TVHessianXp_TVHessianFilter->GetOutput() );

  // g2, adding terms up
  m_g2_AddFilter->SetInput1( m_RTRd_BPFilter->GetOutput() );

  // xnew
  m_xnew_AddFilter->SetInput1( this->GetInput(0) );
  
  // Skip some filters when m_Alpha = 0
  if(m_Alpha == 0)
    {
    m_g_AddFilter->SetInput2( m_TVGradX_MultiplyFilter->GetOutput() );
    m_g2_AddFilter->SetInput2( m_TVHessianX_MultiplyFilter->GetOutput() );
	}
	
  // Skip some filters when m_Alpha = 1
  if(m_Alpha == 1)
    {
    m_g_AddFilter->SetInput2( m_TVGradXp_MultiplyFilter->GetOutput() );
    m_g2_AddFilter->SetInput2( m_TVHessianXp_MultiplyFilter->GetOutput() );
	}

  // Skip some filters when m_Lambda = 0
  if(m_Lambda == 0)
    {
    m_d_SubtractFilter->SetInput2( m_RTs_BPFilter->GetOutput() );
	m_gNormSquared_StatsFilter->SetInput( m_RTs_BPFilter->GetOutput() );
	m_dg_MultiplyFilter->SetInput2( m_RTs_BPFilter->GetOutput() );
	m_dg2_MultiplyFilter->SetInput2( m_RTRd_BPFilter->GetOutput() );
    }

  // Update output information  
  m_RTs_BPFilter->UpdateOutputInformation();
  this->GetOutput()->SetOrigin( m_RTs_BPFilter->GetOutput()->GetOrigin() );
  this->GetOutput()->SetSpacing( m_RTs_BPFilter->GetOutput()->GetSpacing() );
  this->GetOutput()->SetDirection( m_RTs_BPFilter->GetOutput()->GetDirection() );
  this->GetOutput()->SetLargestPossibleRegion( m_RTs_BPFilter->GetOutput()->GetLargestPossibleRegion() );
}

template<class TInputImage, class TOutputImage>
void
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::GenerateData()
{
  const unsigned int Dimension = this->InputImageDimension;
  
  unsigned int NumOfXPixels = 1.;
  unsigned int NumOfPPixels = 1.;
  for (unsigned int k = 0; k < Dimension; ++k)
    {
	NumOfXPixels *= this->GetInput(0)->GetLargestPossibleRegion().GetSize()[k];
	NumOfPPixels *= this->GetInput(1)->GetLargestPossibleRegion().GetSize()[k];
	}

  typename TInputImage::Pointer x, s, d;
  
  double gNormSquared = 0., gprevNormSquared = 0.;
  double sprevNormSquared = 0., sNormSquared = 0.;
  double f0 = 0.;
  double etadNormFirst = 0., etadNorm = 0.;
  
  // X-Xp
  if(m_Lambda !=0 && m_Alpha != 0)
    {
    m_ImageArithmeticProbe.Start();
    m_Xp_SubtractFilter->Update();
    m_ImageArithmeticProbe.Stop();
	}
  
  // Create empty image and projection for fileters that use them
  m_ImageArithmeticProbe.Start();
  m_ZeroMultiplyFilterP->Update();
  m_ZeroMultiplyFilterX->Update();
  m_ImageArithmeticProbe.Stop();

  // For the first iteration, s_old = -p, Rd = Rx0, eta = 1
  m_ForwardProjectionProbe.Start();
  m_Rd_FPFilter->Update();
  m_ForwardProjectionProbe.Stop();

  m_ImageArithmeticProbe.Start();
  m_s_AddFilter->Update();
  m_ImageArithmeticProbe.Stop();

  // Calculate sprev norm squared
  m_ImageStatisticsProbe.Start();
  m_sNormSquared_StatsFilter->Update();
  m_ImageStatisticsProbe.Stop();
  sNormSquared = m_sNormSquared_StatsFilter->GetMean();
  sNormSquared *= NumOfPPixels * sNormSquared;
  sNormSquared += NumOfPPixels * m_sNormSquared_StatsFilter->GetVariance();
  m_ResidualVectors.push_back( 0.5 * sNormSquared );

  // Calculate Initial objective function value f0 for line searching later
  if(m_Lambda != 0)
    {
    m_TVProbe.Start();
    if(m_Alpha != 1)
      m_TVX_TVFilter->Update();
    if(m_Alpha != 0)
      m_TVXp_TVFilter->Update();
    m_TVProbe.Stop();
	}
  f0 = 0.5 * sNormSquared + m_Lambda * 
  	      ( (1 - m_Alpha) * m_TVX_TVFilter->GetTotalVariation() + m_Alpha * m_TVXp_TVFilter->GetTotalVariation() );
  m_ObjectiveVectors.push_back( f0 );	  

  // Reset s = s_old + eta*d
  s = m_s_AddFilter->GetOutput();
  s->DisconnectPipeline();
  m_s_AddFilter->SetInput1( s );
  m_sNormSquared_StatsFilter->SetInput( m_s_AddFilter->GetOutput() );
  
  m_RTs_DDFilter->SetInput( s );
  
  // Iterations start
  for(unsigned int iter = 0; iter < m_NumberOfIterations; iter++)
    {
	if (m_Verbose)
		std::cout << "PICCS Iteration #" << iter+1 << " ......" << std::endl;
		
	// Calculate gradient of objective function
	m_DisplacedDetectorProbe.Start();
	m_RTs_DDFilter->Update();
	m_DisplacedDetectorProbe.Stop();
	
	m_BackProjectionProbe.Start();
	m_RTs_BPFilter->Update();
	m_BackProjectionProbe.Stop();
	
	if(m_Lambda != 0)
	  {
	  m_TVGradientProbe.Start();
	  if(m_Alpha != 1)
	    m_TVGradX_TVGradFilter->Update();
      if(m_Alpha != 0)
	    m_TVGradXp_TVGradFilter->Update();
	  m_TVGradientProbe.Stop();
	  }
	if(m_Lambda != 0)
	  {
	  m_ImageArithmeticProbe.Start();
	  m_g_AddFilter->Update();
	  m_ImageArithmeticProbe.Stop();
	  }
	
    // Calculate g norm squared
	m_ImageStatisticsProbe.Start();
	m_gNormSquared_StatsFilter->Update();
	m_ImageStatisticsProbe.Stop();
	gprevNormSquared = gNormSquared;
	gNormSquared = m_gNormSquared_StatsFilter->GetMean();
	gNormSquared *= NumOfXPixels * gNormSquared;
    gNormSquared += NumOfXPixels * m_gNormSquared_StatsFilter->GetVariance();
	
	// Calculate conjugate gradient search direction
	if(iter) // Reset d pipeline
	  {
	  m_betad_MultiplyFilter->SetConstant( gNormSquared / gprevNormSquared );
	  m_betad_MultiplyFilter->SetInput( d );
	  }
	m_ImageArithmeticProbe.Start();
	m_d_SubtractFilter->Update();
	m_ImageArithmeticProbe.Stop();
	d = m_d_SubtractFilter->GetOutput();
	d->DisconnectPipeline();
	
	// Calculate rho1 = d^T * g
	m_dg_MultiplyFilter->SetInput1( d );
	m_ImageStatisticsProbe.Start();
	m_rho1_StatsFilter->Update();
	m_ImageStatisticsProbe.Stop();
	double rho1 = m_rho1_StatsFilter->GetSum();
	
	// Set Rd
	m_Rd_FPFilter->SetInput( 1, d );
	
	// Calculate Rd and R^TRd
	m_ForwardProjectionProbe.Start();
	m_Rd_FPFilter->Update();
	m_ForwardProjectionProbe.Stop();
	
	m_DisplacedDetectorProbe.Start();
	m_RTRd_DDFilter->Update();
	m_DisplacedDetectorProbe.Stop();
	
	m_BackProjectionProbe.Start();
	m_RTRd_BPFilter->Update();
	m_BackProjectionProbe.Stop();
	
	// TV Hessian operations
	m_TVHessianX_TVHessianFilter->SetInput( 1, d );
	m_TVHessianXp_TVHessianFilter->SetInput( 1, d );
	if(m_Lambda != 0)
	  {
	  m_TVHessianProbe.Start();
	  if(m_Alpha != 1)
	    m_TVHessianX_TVHessianFilter->Update();
      if(m_Alpha != 0)
	    m_TVHessianXp_TVHessianFilter->Update();
	  m_TVHessianProbe.Stop();
	  }
	  
	// Calculate second order derivative g2
	if(m_Lambda != 0)
	  {
	  m_ImageArithmeticProbe.Start();
      if(m_Alpha != 0 && m_Alpha != 1)
	    m_TVHessian_AddFilter->Update();
	  m_g2_AddFilter->Update();
	  m_ImageArithmeticProbe.Stop();
	  }
	
	// Calculate rho2 = d^T * g2
	m_dg2_MultiplyFilter->SetInput1( d );
	m_ImageStatisticsProbe.Start();
	m_rho2_StatsFilter->Update();
	m_ImageStatisticsProbe.Stop();
	double rho2 = m_rho2_StatsFilter->GetSum();
	
	// eta
	double eta = - rho1 / rho2;
	m_Rd_MultiplyFilter->SetConstant( eta );
	
	// frhs
	double frhs = f0 + m_C1 * eta * rho1;
	
	// xnew = x + eta * d
	m_d_etaMultiplyFilter->SetInput( d );
	m_d_etaMultiplyFilter->SetConstant( eta );
	m_ImageArithmeticProbe.Start();
	m_xnew_AddFilter->Update();
	m_ImageArithmeticProbe.Stop();
	
	// Set xnew to relevant filters
	m_Xp_SubtractFilter->SetInput1( m_xnew_AddFilter->GetOutput() );
	m_TVX_TVFilter->SetInput( m_xnew_AddFilter->GetOutput() );
	m_TVGradX_TVGradFilter->SetInput( m_xnew_AddFilter->GetOutput() );
	m_TVHessianX_TVHessianFilter->SetInput( 0, m_xnew_AddFilter->GetOutput() );
	  
	// Relevant updates
	m_ImageArithmeticProbe.Start();
    if(m_Alpha != 0)
	  m_Xp_SubtractFilter->Update();
	m_s_AddFilter->Update();
	m_ImageArithmeticProbe.Stop();

	// flhs
	m_ImageStatisticsProbe.Start();
	m_sNormSquared_StatsFilter->Update();
	m_ImageStatisticsProbe.Stop();
    if(m_Lambda != 0)
	  {
	  m_TVProbe.Start();
	  if(m_Alpha != 1)
        m_TVX_TVFilter->Update();
      if(m_Alpha != 0)
        m_TVXp_TVFilter->Update();
      m_TVProbe.Stop();
	  }
	
	sNormSquared = m_sNormSquared_StatsFilter->GetMean();
	sNormSquared *= NumOfPPixels * sNormSquared;
	sNormSquared += NumOfPPixels * m_sNormSquared_StatsFilter->GetVariance();
    double flhs = 0.5 * sNormSquared + m_Lambda * 
  			    ( (1 - m_Alpha) * m_TVX_TVFilter->GetTotalVariation() + m_Alpha * m_TVXp_TVFilter->GetTotalVariation() );
				
	// Line searching loop
	unsigned int ls_count = 0;
	while ( flhs >= frhs && eta > std::numeric_limits<double>::epsilon() )
	  {
	  if (!ls_count && m_Verbose)
        std::cout << "   Line searching required...... f0 = " << f0 << std::endl;
		
	  ++ls_count;	
	  if (m_Verbose)
		std::cout << "     Line searching iteration#" << ls_count << "......"
				  << " flhs = " << flhs << " , "
				  << " frhs = " << frhs << " , "
				  << " eta = " << eta << std::endl; 
	  
	  eta *= m_C2;
	  m_d_etaMultiplyFilter->SetConstant( eta );
	  m_Rd_MultiplyFilter->SetConstant( eta );
	  m_ImageArithmeticProbe.Start();
	  m_xnew_AddFilter->Update();
      if(m_Alpha != 0)
	    m_Xp_SubtractFilter->Update();
	  m_s_AddFilter->Update();
	  m_ImageArithmeticProbe.Stop();
	  m_ImageStatisticsProbe.Start();
	  m_sNormSquared_StatsFilter->Update();
	  m_ImageStatisticsProbe.Stop();
	  if(m_Lambda != 0)
	    {
        m_TVProbe.Start();
        if(m_Alpha != 1)
		  m_TVX_TVFilter->Update();
        if(m_Alpha != 0)
          m_TVXp_TVFilter->Update();
        m_TVProbe.Stop();
	    }
	  
	  frhs = f0 + m_C1 * eta * rho1;
	  sNormSquared = m_sNormSquared_StatsFilter->GetMean();
	  sNormSquared *= NumOfPPixels * sNormSquared;
	  sNormSquared += NumOfPPixels * m_sNormSquared_StatsFilter->GetVariance();
      flhs = 0.5 * sNormSquared + m_Lambda * 
  			 ( (1 - m_Alpha) * m_TVX_TVFilter->GetTotalVariation() + m_Alpha * m_TVXp_TVFilter->GetTotalVariation() );
	  }
	  
	  // Calculate the norm of image change
	  m_ImageStatisticsProbe.Start();
	  m_etad_StatisticsFilter->Update();
	  m_ImageStatisticsProbe.Stop();
	  etadNorm = m_etad_StatisticsFilter->GetMean();
	  etadNorm *= NumOfXPixels * etadNorm;
	  etadNorm += NumOfXPixels * m_etad_StatisticsFilter->GetVariance();
	  etadNorm = sqrt(etadNorm);
	  if(!iter)
		etadNormFirst = etadNorm;
	  
	  // Set new x
	  x = m_xnew_AddFilter->GetOutput();
	  x->DisconnectPipeline();
	  m_xnew_AddFilter->SetInput1( x );
	  m_Xp_SubtractFilter->SetInput1( x );
	  m_TVX_TVFilter->SetInput( x );
	  m_TVGradX_TVGradFilter->SetInput( x );
	  m_TVHessianX_TVHessianFilter->SetInput( 0, x );
	  
      // Reset s
      s = m_s_AddFilter->GetOutput();
      s->DisconnectPipeline();
      m_s_AddFilter->SetInput1( s );
      m_sNormSquared_StatsFilter->SetInput( m_s_AddFilter->GetOutput() );
	  m_RTs_DDFilter->SetInput( s );

 	  // Record objective function values and sNormSquared
	  m_ObjectiveVectors.push_back( flhs );
	  m_ResidualVectors.push_back( 0.5 * sNormSquared );
	  sprevNormSquared = sNormSquared;
	  f0 = flhs;
	  
	  // Stop iteration if stopping criterion met
	  if( etadNorm < m_StoppingCriterion * etadNormFirst )
	    {
		if (m_Verbose)
			std::cout << "Stopping criterion is met" << std::endl;
		break;
		}
	}
	if (m_Verbose && etadNorm >= m_StoppingCriterion * etadNormFirst )
		std::cout << "Stopping criterion is not met but maximum number of iterations is reached" << std::endl;
	this->GraftOutput( x );
}
	
template<class TInputImage, class TOutputImage>
void
PICCSConeBeamReconstructionFilter<TInputImage, TOutputImage>
::PrintTiming(std::ostream & os) const
{
  os << "PICCSConeBeamReconstructionFilter timing:" << std::endl;
  os << "  Image arithmetic: " << m_ImageArithmeticProbe.GetTotal()
     << ' ' << m_ImageArithmeticProbe.GetUnit() << std::endl;
  os << "  Image statistics: " << m_ImageStatisticsProbe.GetTotal()
     << ' ' << m_ImageStatisticsProbe.GetUnit() << std::endl;
  os << "  Displaced detector: " << m_DisplacedDetectorProbe.GetTotal()
     << ' ' << m_DisplacedDetectorProbe.GetUnit() << std::endl;
  os << "  Forward projection: " << m_ForwardProjectionProbe.GetTotal()
     << ' ' << m_ForwardProjectionProbe.GetUnit() << std::endl;
  os << "  Back projection: " << m_BackProjectionProbe.GetTotal()
     << ' ' << m_BackProjectionProbe.GetUnit() << std::endl;
  os << "  Total Variation: " << m_TVProbe.GetTotal()
     << ' ' << m_TVProbe.GetUnit() << std::endl;
  os << "  Total Variation Gradient: " << m_TVGradientProbe.GetTotal()
     << ' ' << m_TVGradientProbe.GetUnit() << std::endl;
  os << "  Total Variation Hessian: " << m_TVHessianProbe.GetTotal()
     << ' ' << m_TVHessianProbe.GetUnit() << std::endl;
}

} // end namespace rtk

#endif // __rtkPICCSConeBeamReconstructionFilter_txx
