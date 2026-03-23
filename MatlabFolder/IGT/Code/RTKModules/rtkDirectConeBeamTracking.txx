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

#ifndef __rtkDirectConeBeamTracking_txx
#define __rtkDirectConeBeamTracking_txx

#include "rtkDirectConeBeamTracking.h"

#include <algorithm>
#include <itkTimeProbe.h>

namespace rtk
{
template<class TInputImage, class TOutputImage>
DirectConeBeamTracking<TInputImage, TOutputImage>
::DirectConeBeamTracking()
{
  // Input 0: projections
  this->SetNumberOfRequiredInputs(1);
  // Note that all the f's are also required, but we check that later on

  // Set default parameters
  m_Verbose = false;
  m_NumberOfIterations = 50;
  m_LambdaInDepth = 1;
  m_LambdaInPlane = 1;
  m_LambdaSI = 1;
  m_GammaInDepth = 2;
  m_GammaInPlane = 2;
  m_GammaSI = 2;
  m_OptimizerConfiguration = 0;
  m_C1 = 1e-4;
  m_C2 = 0.5;
  m_StoppingCriterion = 0.01;
  m_ConstrainAngle = 0.;
  m_MaxDisplacement = 50.;
  m_NumberOfMatchPoints = 4;
  m_NumberOfHistogramLevels = 32;
  m_TargetOrigin.Fill( 0 );
  m_PriorOrigin.Fill( 0 );
  m_MaxRRShiftX = 20.;
  m_MaxRRShiftY = 20.;
  m_MaxRRRotateAngle = itk::Math::pi / 9.;
  m_RigidTransformParameters.Fill(0.);
  
  // Image pointers
  m_p = 0;
  m_PriorImage = 0;
  m_RfImage = 0;
  m_f = 0;
  m_fx = 0;
  m_fy = 0;
  m_fz = 0;
  m_fxx = 0;
  m_fyy = 0;
  m_fzz = 0;
  m_fxy = 0;
  m_fxz = 0;
  m_fyz = 0;
  
  // Subtraction filters
  m_s = SubtractFilterType::New();
  
  // Multiply filters
  m_ZeroP = MultiplyFilterType::New();
  m_ZeroP->SetConstant( 0 );
  m_sRfx = MultiplyFilterType::New();
  m_sRfx->SetInput(0, m_s->GetOutput() );
  m_sRfy = MultiplyFilterType::New();
  m_sRfy->SetInput(0, m_s->GetOutput() );
  m_sRfz = MultiplyFilterType::New();
  m_sRfz->SetInput(0, m_s->GetOutput() );
  m_RfxRfy = MultiplyFilterType::New();
  m_RfxRfz = MultiplyFilterType::New();
  m_RfyRfz = MultiplyFilterType::New();
  m_sRfxx = MultiplyFilterType::New();
  m_sRfxx->SetInput(0, m_s->GetOutput() );
  m_sRfyy = MultiplyFilterType::New();
  m_sRfyy->SetInput(0, m_s->GetOutput() );
  m_sRfzz = MultiplyFilterType::New();
  m_sRfzz->SetInput(0, m_s->GetOutput() );
  m_sRfxy = MultiplyFilterType::New();
  m_sRfxy->SetInput(0, m_s->GetOutput() );
  m_sRfxz = MultiplyFilterType::New();
  m_sRfxz->SetInput(0, m_s->GetOutput() );
  m_sRfyz = MultiplyFilterType::New();
  m_sRfyz->SetInput(0, m_s->GetOutput() );
  
  // Image statistics filters
  m_IS_s = ImageStatisticsFilterType::New();
  m_IS_s->SetInput( m_s->GetOutput() );
  m_IS_Rfx = ImageStatisticsFilterType::New();
  m_IS_Rfy = ImageStatisticsFilterType::New();
  m_IS_Rfz = ImageStatisticsFilterType::New();
  m_IS_sRfx = ImageStatisticsFilterType::New();
  m_IS_sRfx->SetInput( m_sRfx->GetOutput() );
  m_IS_sRfy = ImageStatisticsFilterType::New();
  m_IS_sRfy->SetInput( m_sRfy->GetOutput() );
  m_IS_sRfz = ImageStatisticsFilterType::New();
  m_IS_sRfz->SetInput( m_sRfz->GetOutput() );
  m_IS_RfxRfy = ImageStatisticsFilterType::New();
  m_IS_RfxRfy->SetInput( m_RfxRfy->GetOutput() );
  m_IS_RfxRfz = ImageStatisticsFilterType::New();
  m_IS_RfxRfz->SetInput( m_RfxRfz->GetOutput() );
  m_IS_RfyRfz = ImageStatisticsFilterType::New();
  m_IS_RfyRfz->SetInput( m_RfyRfz->GetOutput() );
  m_IS_sRfxx = ImageStatisticsFilterType::New();
  m_IS_sRfxx->SetInput( m_sRfxx->GetOutput() );
  m_IS_sRfyy = ImageStatisticsFilterType::New();
  m_IS_sRfyy->SetInput( m_sRfyy->GetOutput() );
  m_IS_sRfzz = ImageStatisticsFilterType::New();
  m_IS_sRfzz->SetInput( m_sRfzz->GetOutput() );
  m_IS_sRfxy = ImageStatisticsFilterType::New();
  m_IS_sRfxy->SetInput( m_sRfxy->GetOutput() );
  m_IS_sRfxz = ImageStatisticsFilterType::New();
  m_IS_sRfxz->SetInput( m_sRfxz->GetOutput() );
  m_IS_sRfyz = ImageStatisticsFilterType::New();
  m_IS_sRfyz->SetInput( m_sRfyz->GetOutput() );
}

template<class TInputImage, class TOutputImage>
void
DirectConeBeamTracking<TInputImage, TOutputImage>
::SetForwardProjectionFilter (int _arg)
{
  if( _arg != this->GetForwardProjectionFilter() )
    {
	Superclass::m_CurrentForwardProjectionConfiguration = _arg;
    Superclass::SetForwardProjectionFilter( _arg );
	m_Rf = this->InstantiateForwardProjectionFilter( _arg );
	m_Rf->InPlaceOff();
	m_Rfx = this->InstantiateForwardProjectionFilter( _arg );
	m_Rfx->InPlaceOff();
	m_Rfy = this->InstantiateForwardProjectionFilter( _arg );
	m_Rfy->InPlaceOff();
	m_Rfz = this->InstantiateForwardProjectionFilter( _arg );
	m_Rfz->InPlaceOff();
	m_Rfxx = this->InstantiateForwardProjectionFilter( _arg );
	m_Rfxx->InPlaceOff();
	m_Rfyy = this->InstantiateForwardProjectionFilter( _arg );
	m_Rfyy->InPlaceOff();
	m_Rfzz = this->InstantiateForwardProjectionFilter( _arg );
	m_Rfzz->InPlaceOff();
	m_Rfxy = this->InstantiateForwardProjectionFilter( _arg );
	m_Rfxy->InPlaceOff();
	m_Rfxz = this->InstantiateForwardProjectionFilter( _arg );
	m_Rfxz->InPlaceOff();
	m_Rfyz = this->InstantiateForwardProjectionFilter( _arg );
	m_Rfyz->InPlaceOff();
    }
}

template<class TInputImage, class TOutputImage>
void
DirectConeBeamTracking<TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  typename Superclass::InputImagePointer inputPtr =
    const_cast< TInputImage * >( this->GetInput() );

  if ( !inputPtr || !(this->IsModelProvided()) )
    return;
}

template<class TInputImage, class TOutputImage>
void
DirectConeBeamTracking<TInputImage, TOutputImage>
::GenerateOutputInformation()
{
  // Check and set geometry
  if(this->GetGeometry().GetPointer() == NULL)
    {
    itkGenericExceptionMacro(<< "The geometry of the reconstruction has not been set");
    }
  
  m_ZeroP->SetInput( this->GetInput() );  
	
  m_Rf->SetGeometry( this->GetGeometry() );
  m_Rf->SetInput(0, m_ZeroP->GetOutput() );
  m_Rf->SetInput(1, m_f);
  m_Rfx->SetGeometry( this->GetGeometry() );
  m_Rfx->SetInput(0, m_ZeroP->GetOutput() );
  m_Rfx->SetInput(1, m_fx);
  m_Rfy->SetGeometry( this->GetGeometry() );
  m_Rfy->SetInput(0, m_ZeroP->GetOutput() );
  m_Rfy->SetInput(1, m_fy);
  m_Rfz->SetGeometry( this->GetGeometry() );
  m_Rfz->SetInput(0, m_ZeroP->GetOutput() );
  m_Rfz->SetInput(1, m_fz);
  m_Rfxx->SetGeometry( this->GetGeometry() );
  m_Rfxx->SetInput(0, m_ZeroP->GetOutput() );
  m_Rfxx->SetInput(1, m_fxx);
  m_Rfyy->SetGeometry( this->GetGeometry() );
  m_Rfyy->SetInput(0, m_ZeroP->GetOutput() );
  m_Rfyy->SetInput(1, m_fyy);
  m_Rfzz->SetGeometry( this->GetGeometry() );
  m_Rfzz->SetInput(0, m_ZeroP->GetOutput() );
  m_Rfzz->SetInput(1, m_fzz);
  m_Rfxy->SetGeometry( this->GetGeometry() );
  m_Rfxy->SetInput(0, m_ZeroP->GetOutput() );
  m_Rfxy->SetInput(1, m_fxy);
  m_Rfxz->SetGeometry( this->GetGeometry() );
  m_Rfxz->SetInput(0, m_ZeroP->GetOutput() );
  m_Rfxz->SetInput(1, m_fxz);
  m_Rfyz->SetGeometry( this->GetGeometry() );
  m_Rfyz->SetInput(0, m_ZeroP->GetOutput() );
  m_Rfyz->SetInput(1, m_fyz);
  
  // Pipeline connections
  m_s->SetInput(0, m_Rf->GetOutput() );
  m_sRfx->SetInput(1, m_Rfx->GetOutput() );
  m_sRfy->SetInput(1, m_Rfy->GetOutput() );
  m_sRfz->SetInput(1, m_Rfz->GetOutput() );
  m_RfxRfy->SetInput(0, m_Rfx->GetOutput() );
  m_RfxRfy->SetInput(1, m_Rfy->GetOutput() );
  m_RfxRfz->SetInput(0, m_Rfx->GetOutput() );
  m_RfxRfz->SetInput(1, m_Rfz->GetOutput() );
  m_RfyRfz->SetInput(0, m_Rfy->GetOutput() );
  m_RfyRfz->SetInput(1, m_Rfz->GetOutput() );
  m_sRfxx->SetInput(1, m_Rfxx->GetOutput() );
  m_sRfyy->SetInput(1, m_Rfyy->GetOutput() );
  m_sRfzz->SetInput(1, m_Rfzz->GetOutput() );
  m_sRfxy->SetInput(1, m_Rfxy->GetOutput() );
  m_sRfxz->SetInput(1, m_Rfxz->GetOutput() );
  m_sRfyz->SetInput(1, m_Rfyz->GetOutput() );
  m_IS_Rfx->SetInput( m_Rfx->GetOutput() );
  m_IS_Rfy->SetInput( m_Rfy->GetOutput() );
  m_IS_Rfz->SetInput( m_Rfz->GetOutput() );
}

template<class TInputImage, class TOutputImage>
void
DirectConeBeamTracking<TInputImage, TOutputImage>
::GenerateData()
{
  const unsigned int Dimension = TInputImage::ImageDimension;
  
  unsigned int NumOfPPixels = 1.;
  for (unsigned int k = 0; k < Dimension; ++k)
	NumOfPPixels *= this->GetInput()->GetLargestPossibleRegion().GetSize()[k];
	
  unsigned int NProj = this->GetInput()->GetLargestPossibleRegion().GetSize()[Dimension-1];
	
  // Gradient, search direction, and position vectors
  VectorType g, g_prev, d, d_prev, r, r_new;
  r = this->GetTargetOrigin();
  
  // Hessian matrix type
  MatrixType H;
  
  // Generate zero projection stacks
  m_ImageArithmeticProbe.Start();
  TRY_AND_EXIT_ON_ITK_EXCEPTION( m_ZeroP->Update() );
  m_ImageArithmeticProbe.Stop();
	
  // We use different p depending on whether a prior image is given
  if(!m_PriorImage)
    m_p = const_cast< TInputImage * >( this->GetInput() );
  else
    {
	// Projection registration
	// First we forward project the prior image
	typename ForwardProjectionFilterType::Pointer RPrior =
		this->InstantiateForwardProjectionFilter( this->GetForwardProjectionFilter() );
	RPrior->InPlaceOff();
	RPrior->SetGeometry( this->GetGeometry() );
	RPrior->SetInput( 0, m_ZeroP->GetOutput() );
	RPrior->SetInput( 1, m_PriorImage );
	m_ForwardProjectionProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( RPrior->Update() );
	m_ForwardProjectionProbe.Stop();
	
	// Tile filter to stack projections back later
	typename TileFilterType::Pointer	tileFilter = TileFilterType::New();
	typename TileFilterType::LayoutArrayType layout;
	layout[0] = 1;
	layout[1] = 1;
	layout[2] = 0;
	tileFilter->SetLayout( layout );
	
	// Arrays of extract, histogram match, then resample filters
	std::vector< typename ExtractFilterType::Pointer >	extractP, extractRPrior;
	std::vector< typename HistogramMatchType::Pointer >	histogramMatchers;
	std::vector< typename ResampleType::Pointer >		resamplers;
	ImageSizeType size = this->GetInput()->GetLargestPossibleRegion().GetSize();
	ImageIndexType index = this->GetInput()->GetLargestPossibleRegion().GetIndex();
	size[Dimension-1] = 0;
	for(unsigned int k = 0; k < NProj; ++k)
	  {
	  // Extract
	  index[Dimension-1] = k;
	  ImageRegionType region(index, size);
	  extractP.push_back( ExtractFilterType::New() );
	  extractP[k]->SetInput( this->GetInput() );
	  extractP[k]->SetExtractionRegion( region );
	  extractP[k]->SetDirectionCollapseToSubmatrix();
	  extractRPrior.push_back( ExtractFilterType::New() );
	  extractRPrior[k]->SetInput( RPrior->GetOutput() );
	  extractRPrior[k]->SetExtractionRegion( region );
	  extractRPrior[k]->SetDirectionCollapseToSubmatrix();
	  m_ImageArithmeticProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( extractP[k]->Update() );
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( extractRPrior[k]->Update() );
	  m_ImageArithmeticProbe.Stop();
	  
	  //Histogram match
	  histogramMatchers.push_back( HistogramMatchType::New() );
	  histogramMatchers[k]->SetInput( extractRPrior[k]->GetOutput() );
	  histogramMatchers[k]->SetReferenceImage( extractP[k]->GetOutput() );
	  histogramMatchers[k]->ThresholdAtMeanIntensityOff();
	  histogramMatchers[k]->SetNumberOfMatchPoints( m_NumberOfMatchPoints );
	  histogramMatchers[k]->SetNumberOfHistogramLevels( m_NumberOfHistogramLevels );
	  m_ImageArithmeticProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( histogramMatchers[k]->Update() );
	  m_ImageArithmeticProbe.Stop();
	  
	  // Rigid registration
	  typename MSMetricType::Pointer				metric = MSMetricType::New();
	  typename RigidTransformType::Pointer		transform = RigidTransformType::New();
	  typename OptimizerType::Pointer			optimizer = OptimizerType::New();
	  typename LinearInterpolatorType::Pointer	interpolator = LinearInterpolatorType::New();
	  typename ImageRegistrationType::Pointer	registration = ImageRegistrationType::New();
	  typename RigidTransformType::ParametersType	initialParameters( transform->GetNumberOfParameters() );
	  initialParameters.Fill( 0.0 );
	  transform->SetParameters( initialParameters );
	  registration->SetMetric( metric );
	  registration->SetOptimizer( optimizer );
	  registration->SetTransform( transform );
	  registration->SetInitialTransformParameters( transform->GetParameters() );
	  registration->SetInterpolator( interpolator );
	  registration->SetFixedImage( extractP[k]->GetOutput() );
	  registration->SetMovingImage( extractRPrior[k]->GetOutput() );
	  registration->SetFixedImageRegion( extractP[k]->GetOutput()->GetLargestPossibleRegion() );
	  optimizer->SetMaximumStepLength( 1.00 );
	  optimizer->SetMinimumStepLength( 0.01 );
	  optimizer->SetNumberOfIterations( 100 );
	  m_ImageRegistrationProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( registration->Update() );
	  m_ImageRegistrationProbe.Stop();
	  
	  // Constrain the transform
	  m_RigidTransformParameters = registration->GetOutput()->Get()->GetParameters();
	  if( std::fabs(m_RigidTransformParameters[0]) > this->GetMaxRRRotateAngle() )
	    m_RigidTransformParameters[0] = (this->GetMaxRRRotateAngle()) * m_RigidTransformParameters[0] / std::fabs(m_RigidTransformParameters[0]);
	  if( std::fabs(m_RigidTransformParameters[1]) > this->GetMaxRRShiftX() )
	    m_RigidTransformParameters[1] = (this->GetMaxRRShiftX())  * m_RigidTransformParameters[1] / std::fabs(m_RigidTransformParameters[1]);
	  if( std::fabs(m_RigidTransformParameters[2]) > this->GetMaxRRShiftY() )
	    m_RigidTransformParameters[2] = (this->GetMaxRRShiftY())  * m_RigidTransformParameters[2] / std::fabs(m_RigidTransformParameters[2]);
	  RigidTransformType::Pointer finalTransform = RigidTransformType::New();
	  finalTransform->SetParameters( m_RigidTransformParameters );
	  
	  // Resampler
	  resamplers.push_back( ResampleType::New() );
	  resamplers[k]->SetInput( histogramMatchers[k]->GetOutput() );
	  resamplers[k]->SetReferenceImage( extractP[k]->GetOutput() );
	  resamplers[k]->SetTransform( finalTransform );
	  resamplers[k]->UseReferenceImageOn();
	  m_ImageRegistrationProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( resamplers[k]->Update() );
	  m_ImageRegistrationProbe.Stop();
	  
	  // Pass results to tile filter
	  tileFilter->SetInput( k, resamplers[k]->GetOutput() );
	  }
	// Then subtract the results from input projections
	m_ImageArithmeticProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( tileFilter->Update() );
	m_ImageArithmeticProbe.Stop();
	InputImagePointer drrNoTumor = tileFilter->GetOutput();
	drrNoTumor->DisconnectPipeline();
	drrNoTumor->SetOrigin( this->GetInput()->GetOrigin() );
	typename SubtractFilterType::Pointer projSubtractFilter = SubtractFilterType::New();
	projSubtractFilter->SetInput(0, this->GetInput() );
	projSubtractFilter->SetInput(1, drrNoTumor );
	m_ImageArithmeticProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( projSubtractFilter->Update() );
	m_ImageArithmeticProbe.Stop();
	m_p = projSubtractFilter->GetOutput();
	m_p->DisconnectPipeline();
	}
  m_s->SetInput(1, m_p);
  
  // For testing purpose
  // this->GraftOutput(m_p); return;
  
  for(unsigned int iter = 0; iter < m_NumberOfIterations; ++iter)
    {
    // Forward projections
    m_ForwardProjectionProbe.Start();
	if(iter == 0)
      TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rf->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rfx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rfy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rfz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rfxx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rfyy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rfzz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rfxy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rfxz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rfyz->Update() );
    m_ForwardProjectionProbe.Stop();
  
    // Image arithmetics for gradient calculation
    m_ImageArithmeticProbe.Start();
	if(iter == 0)
      TRY_AND_EXIT_ON_ITK_EXCEPTION( m_s->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_sRfx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_sRfy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_sRfz->Update() );
    m_ImageArithmeticProbe.Stop();
    m_ImageStatisticsProbe.Start();
	if(iter == 0)
      TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_s->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_sRfx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_sRfy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_sRfz->Update() );
    m_ImageStatisticsProbe.Stop();
  
    // Gradient of the objective function
    g[0] = m_IS_sRfx->GetSum();
	g[1] = m_IS_sRfy->GetSum();
	g[2] = m_IS_sRfz->GetSum();
	g += m_LambdaInDepth * (this->GetInDepthConstraintGradient(r, m_PriorOrigin, m_ConstrainAngle, 2*m_GammaInDepth));
	g += m_LambdaInPlane * (this->GetInPlaneConstraintGradient(r, m_PriorOrigin, m_ConstrainAngle, 2*m_GammaInPlane));
	g += m_LambdaSI * (this->GetSingleDimensionConstraintGradient(r, m_PriorOrigin, 1, 2*m_GammaSI));
	
	// The search direction
	if(iter)
	  {
	  double beta = 0.;
	  if( g_prev.GetNorm() > std::numeric_limits<double>::epsilon() && m_OptimizerConfiguration == 0 )
	    beta = g.GetNorm() / g_prev.GetNorm();
	  beta *= beta;
	  d = -g + beta * d_prev;
	  }
	else
	  d = -g;
	double rho1 = d * g;
	g_prev = g;	d_prev = d;
	
    // Image arithmetics for Hessian matrix
    m_ImageArithmeticProbe.Start();
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_RfxRfy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_RfxRfz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_RfyRfz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_sRfxx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_sRfyy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_sRfzz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_sRfxy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_sRfxz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_sRfyz->Update() );
    m_ImageArithmeticProbe.Stop();
    m_ImageStatisticsProbe.Start();
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_Rfx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_Rfy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_Rfz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_RfxRfy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_RfxRfz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_RfyRfz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_sRfxx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_sRfyy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_sRfzz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_sRfxy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_sRfxz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_sRfyz->Update() );
    m_ImageStatisticsProbe.Stop();
	
	MatrixType H1, H2;
	H1(0,0) = this->GetSquaredSum(m_IS_Rfx, NumOfPPixels);
	H1(1,1) = this->GetSquaredSum(m_IS_Rfy, NumOfPPixels);
	H1(2,2) = this->GetSquaredSum(m_IS_Rfy, NumOfPPixels);
	H1(0,1) = m_IS_RfxRfy->GetSum();	H1(1,0) = H1(0,1);
	H1(0,2) = m_IS_RfxRfz->GetSum();	H1(2,0) = H1(0,2);
	H1(1,2) = m_IS_RfyRfz->GetSum();	H1(2,1) = H1(1,2);
	H2(0,0) = m_IS_sRfxx->GetSum();
	H2(1,1) = m_IS_sRfyy->GetSum();
	H2(2,2) = m_IS_sRfzz->GetSum();
	H2(0,1) = m_IS_sRfxy->GetSum();		H2(1,0) = H2(0,1);
	H2(0,2) = m_IS_sRfxz->GetSum();		H2(2,0) = H2(0,2);
	H2(1,2) = m_IS_sRfyz->GetSum();		H2(2,1) = H2(1,2);
	H = H1 + H2;
	H += this->GetInDepthConstraintHessian(r, m_PriorOrigin, m_ConstrainAngle, m_LambdaInDepth, 2*m_GammaInDepth);
	H += this->GetInPlaneConstraintHessian(r, m_PriorOrigin, m_ConstrainAngle, m_LambdaInPlane, 2*m_GammaInPlane);
	H += this->GetSingleDimensionConstraintHessian(r, m_PriorOrigin, 1, m_LambdaSI, 2*m_GammaSI);
	
	// Newton-Raphson approximation of step size
	double rho2 = d * (H * d);
	double eta = 0.;
	if( rho2 > std::numeric_limits<double>::epsilon() )
	  eta = -rho1 / rho2;
	
	if(iter == 0)
	  {
	  // Residual
	  m_ResidualVector.push_back( 0.5 * this->GetSquaredSum(m_IS_s, NumOfPPixels) );
	  // Objecive function
	  m_ObjectiveVector.push_back( m_ResidualVector[iter]
								  + m_LambdaInDepth * (this->GetInDepthConstraintObjective(r, m_PriorOrigin, m_ConstrainAngle, 2*m_GammaInDepth))
								  + m_LambdaInPlane * (this->GetInPlaneConstraintObjective(r, m_PriorOrigin, m_ConstrainAngle, 2*m_GammaInPlane))
								  + m_LambdaSI * (this->GetSingleDimensionConstraintObjective(r, m_PriorOrigin, 1, 2*m_GammaSI)) );
	  m_TargetOriginVector.push_back( r );	  
	  }
	  
	// Preparing for line searching
	double Grhs = m_ObjectiveVector[iter] + m_C1 * eta * rho1;
	r_new = r + eta * d;
	this->SetTargetOrigin( r_new );
	m_ForwardProjectionProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rf->Update() );
	m_ForwardProjectionProbe.Stop();
	m_ImageArithmeticProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( m_s->Update() );
	m_ImageArithmeticProbe.Stop();
	m_ImageStatisticsProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_s->Update() );
	m_ImageStatisticsProbe.Stop();
	// Objective function
	double Rlhs = 0.5 * this->GetSquaredSum(m_IS_s, NumOfPPixels);
	double Glhs = Rlhs 
					+ m_LambdaInDepth * (this->GetInDepthConstraintObjective(r_new, m_PriorOrigin, m_ConstrainAngle, 2*m_GammaInDepth))
					+ m_LambdaInPlane * (this->GetInPlaneConstraintObjective(r_new, m_PriorOrigin, m_ConstrainAngle, 2*m_GammaInPlane))
					+ m_LambdaSI * (this->GetSingleDimensionConstraintObjective(r_new, m_PriorOrigin, 1, 2*m_GammaSI));
	
	while( Glhs >= Grhs && eta > std::numeric_limits<double>::epsilon() )
	  {
	  eta *= m_C2;
	  r_new = r + eta * d;
	  this->SetTargetOrigin( r_new );
	  m_ForwardProjectionProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( m_Rf->Update() );
	  m_ForwardProjectionProbe.Stop();
	  m_ImageArithmeticProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( m_s->Update() );
	  m_ImageArithmeticProbe.Stop();
	  m_ImageStatisticsProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( m_IS_s->Update() );
	  m_ImageStatisticsProbe.Stop();
	  // Objective function, constraints to be added
	  Rlhs = 0.5 * this->GetSquaredSum(m_IS_s, NumOfPPixels);
	  Glhs = Rlhs 
				+ m_LambdaInDepth * (this->GetInDepthConstraintObjective(r_new, m_PriorOrigin, m_ConstrainAngle, 2*m_GammaInDepth))
				+ m_LambdaInPlane * (this->GetInPlaneConstraintObjective(r_new, m_PriorOrigin, m_ConstrainAngle, 2*m_GammaInPlane))
				+ m_LambdaSI * (this->GetSingleDimensionConstraintObjective(r_new, m_PriorOrigin, 1, 2*m_GammaSI));
	  Grhs = m_ObjectiveVector[iter] + m_C1 * eta * rho1;
	  }
	VectorType d_total = r_new - m_PriorOrigin;
	if( d_total.GetNorm() > m_MaxDisplacement )
	  {
	  // this->SetTargetOrigin( m_TargetOriginVector[0] );
	  this->SetTargetOrigin( m_PriorOrigin );
	  break;
	  }
	
	m_ResidualVector.push_back( Rlhs );
	m_ObjectiveVector.push_back( Glhs );
	m_TargetOriginVector.push_back( r_new );
	
	this->SetTargetOrigin( r_new );
	r = r_new;
	
	if( std::fabs( eta * d.GetNorm() ) < m_StoppingCriterion )
	  break;
	  
	}
	m_RfImage = m_Rf->GetOutput();
	
	this->GraftOutput( m_f );
}
	
template<class TInputImage, class TOutputImage>
void
DirectConeBeamTracking<TInputImage, TOutputImage>
::PrintTiming(std::ostream & os) const
{
  os << "     DirectConeBeamTracking timing:" << std::endl;
  os << "       Image arithmetic: " << m_ImageArithmeticProbe.GetTotal()
     << ' ' << m_ImageArithmeticProbe.GetUnit() << std::endl;
  os << "       Image statistics: " << m_ImageStatisticsProbe.GetTotal()
     << ' ' << m_ImageStatisticsProbe.GetUnit() << std::endl;
  os << "       Forward projection: " << m_ForwardProjectionProbe.GetTotal()
     << ' ' << m_ForwardProjectionProbe.GetUnit() << std::endl;
  os << "       Image registration: " << m_ImageRegistrationProbe.GetTotal()
     << ' ' << m_ImageRegistrationProbe.GetUnit() << std::endl;
}

} // end namespace rtk

#endif // __rtkDirectConeBeamTracking_txx
