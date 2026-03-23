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

#ifndef __rtkDirectConeBeamNCCTracking_txx
#define __rtkDirectConeBeamNCCTracking_txx

#include "rtkDirectConeBeamNCCTracking.h"

#include <algorithm>
#include <itkTimeProbe.h>

namespace rtk
{
template<class TInputImage, class TOutputImage>
DirectConeBeamNCCTracking<TInputImage, TOutputImage>
::DirectConeBeamNCCTracking()
{
  // Input 0: projections
  this->SetNumberOfRequiredInputs(1);
  // Note that all the f's are also required, but we check that later on

  // Set default parameters
  this->m_Verbose = false;
  this->m_NumberOfIterations = 50;
  this->m_LambdaInDepth = 1;
  this->m_LambdaInPlane = 1;
  this->m_LambdaSI = 1;
  this->m_GammaInDepth = 2;
  this->m_GammaInPlane = 2;
  this->m_GammaSI = 2;
  this->m_OptimizerConfiguration = 0;
  this->m_C1 = 1e-4;
  this->m_C2 = 0.5;
  this->m_StoppingCriterion = 0.01;
  this->m_ConstrainAngle = 0.;
  this->m_MaxDisplacement = 50.;
  this->m_NumberOfMatchPoints = 4;
  this->m_NumberOfHistogramLevels = 32;
  this->m_TargetOrigin.Fill( 0 );
  this->m_PriorOrigin.Fill( 0 );
  this->m_MaxRRShiftX = 20.;
  this->m_MaxRRShiftY = 20.;
  this->m_MaxRRRotateAngle = itk::Math::pi / 9.;
  this->m_RigidTransformParameters.Fill(0.);
  
  // Image pointers
  this->m_p = 0;
  this->m_PriorImage = 0;
  this->m_RfImage = 0;
  this->m_f = 0;
  this->m_fx = 0;
  this->m_fy = 0;
  this->m_fz = 0;
  this->m_fxx = 0;
  this->m_fyy = 0;
  this->m_fzz = 0;
  this->m_fxy = 0;
  this->m_fxz = 0;
  this->m_fyz = 0;
  
  // Multiply filters
  this->m_ZeroP = Superclass::MultiplyFilterType::New();
  this->m_ZeroP->SetConstant( 0 );
  this->m_pRf = Superclass::MultiplyFilterType::New();
  this->m_pRfx = Superclass::MultiplyFilterType::New();
  this->m_pRfy = Superclass::MultiplyFilterType::New();
  this->m_pRfz = Superclass::MultiplyFilterType::New();
  this->m_pRfxx = Superclass::MultiplyFilterType::New();
  this->m_pRfyy = Superclass::MultiplyFilterType::New();
  this->m_pRfzz = Superclass::MultiplyFilterType::New();
  this->m_pRfxy = Superclass::MultiplyFilterType::New();
  this->m_pRfxz = Superclass::MultiplyFilterType::New();
  this->m_pRfyz = Superclass::MultiplyFilterType::New();
  
  // Image statistics filters
  this->m_IS_p = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_Rf = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRf = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRf->SetInput( this->m_pRf->GetOutput() );
  this->m_IS_pRfx = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRfx->SetInput( this->m_pRfx->GetOutput() );
  this->m_IS_pRfy = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRfy->SetInput( this->m_pRfy->GetOutput() );
  this->m_IS_pRfz = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRfz->SetInput( this->m_pRfz->GetOutput() );
  this->m_IS_pRfxx = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRfxx->SetInput( this->m_pRfxx->GetOutput() );
  this->m_IS_pRfyy = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRfyy->SetInput( this->m_pRfyy->GetOutput() );
  this->m_IS_pRfzz = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRfzz->SetInput( this->m_pRfzz->GetOutput() );
  this->m_IS_pRfxy = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRfxy->SetInput( this->m_pRfxy->GetOutput() );
  this->m_IS_pRfxz = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRfxz->SetInput( this->m_pRfxz->GetOutput() );
  this->m_IS_pRfyz = Superclass::ImageStatisticsFilterType::New();
  this->m_IS_pRfyz->SetInput( this->m_pRfyz->GetOutput() );
}

template<class TInputImage, class TOutputImage>
void
DirectConeBeamNCCTracking<TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  typename Superclass::InputImagePointer inputPtr =
    const_cast< TInputImage * >( this->GetInput() );

  if ( !inputPtr || !(this->IsModelProvided()) )
    return;
}

template<class TInputImage, class TOutputImage>
void
DirectConeBeamNCCTracking<TInputImage, TOutputImage>
::GenerateOutputInformation()
{
  // Check and set geometry
  if(this->GetGeometry().GetPointer() == NULL)
    {
    itkGenericExceptionMacro(<< "The geometry of the reconstruction has not been set");
    }
  
  this->m_ZeroP->SetInput( this->GetInput() );  
	
  this->m_Rf->SetGeometry( this->GetGeometry() );
  this->m_Rf->SetInput(0, this->m_ZeroP->GetOutput() );
  this->m_Rf->SetInput(1, this->m_f);
  this->m_Rfx->SetGeometry( this->GetGeometry() );
  this->m_Rfx->SetInput(0, this->m_ZeroP->GetOutput() );
  this->m_Rfx->SetInput(1, this->m_fx);
  this->m_Rfy->SetGeometry( this->GetGeometry() );
  this->m_Rfy->SetInput(0, this->m_ZeroP->GetOutput() );
  this->m_Rfy->SetInput(1, this->m_fy);
  this->m_Rfz->SetGeometry( this->GetGeometry() );
  this->m_Rfz->SetInput(0, this->m_ZeroP->GetOutput() );
  this->m_Rfz->SetInput(1, this->m_fz);
  this->m_Rfxx->SetGeometry( this->GetGeometry() );
  this->m_Rfxx->SetInput(0, this->m_ZeroP->GetOutput() );
  this->m_Rfxx->SetInput(1, this->m_fxx);
  this->m_Rfyy->SetGeometry( this->GetGeometry() );
  this->m_Rfyy->SetInput(0, this->m_ZeroP->GetOutput() );
  this->m_Rfyy->SetInput(1, this->m_fyy);
  this->m_Rfzz->SetGeometry( this->GetGeometry() );
  this->m_Rfzz->SetInput(0, this->m_ZeroP->GetOutput() );
  this->m_Rfzz->SetInput(1, this->m_fzz);
  this->m_Rfxy->SetGeometry( this->GetGeometry() );
  this->m_Rfxy->SetInput(0, this->m_ZeroP->GetOutput() );
  this->m_Rfxy->SetInput(1, this->m_fxy);
  this->m_Rfxz->SetGeometry( this->GetGeometry() );
  this->m_Rfxz->SetInput(0, this->m_ZeroP->GetOutput() );
  this->m_Rfxz->SetInput(1, this->m_fxz);
  this->m_Rfyz->SetGeometry( this->GetGeometry() );
  this->m_Rfyz->SetInput(0, this->m_ZeroP->GetOutput() );
  this->m_Rfyz->SetInput(1, this->m_fyz);
  
  // Pipeline connections
  this->m_pRf->SetInput(1, this->m_Rf->GetOutput() );
  this->m_pRfx->SetInput(1, this->m_Rfx->GetOutput() );
  this->m_pRfy->SetInput(1, this->m_Rfy->GetOutput() );
  this->m_pRfz->SetInput(1, this->m_Rfz->GetOutput() );
  this->m_pRfxx->SetInput(1, this->m_Rfxx->GetOutput() );
  this->m_pRfyy->SetInput(1, this->m_Rfyy->GetOutput() );
  this->m_pRfzz->SetInput(1, this->m_Rfzz->GetOutput() );
  this->m_pRfxy->SetInput(1, this->m_Rfxy->GetOutput() );
  this->m_pRfxz->SetInput(1, this->m_Rfxz->GetOutput() );
  this->m_pRfyz->SetInput(1, this->m_Rfyz->GetOutput() );
  this->m_IS_Rf->SetInput( this->m_Rf->GetOutput() );
}

template<class TInputImage, class TOutputImage>
void
DirectConeBeamNCCTracking<TInputImage, TOutputImage>
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
  this->m_ImageArithmeticProbe.Start();
  TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_ZeroP->Update() );
  this->m_ImageArithmeticProbe.Stop();
	
  // We use different p depending on whether a prior image is given
  if(!this->m_PriorImage)
    this->m_p = const_cast< TInputImage * >( this->GetInput() );
  else
    {
	// Projection registration
	// First we forward project the prior image
	typename Superclass::ForwardProjectionFilterType::Pointer RPrior =
		this->InstantiateForwardProjectionFilter( this->GetForwardProjectionFilter() );
	RPrior->InPlaceOff();
	RPrior->SetGeometry( this->GetGeometry() );
	RPrior->SetInput( 0, this->m_ZeroP->GetOutput() );
	RPrior->SetInput( 1, this->m_PriorImage );
	this->m_ForwardProjectionProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( RPrior->Update() );
	this->m_ForwardProjectionProbe.Stop();
	
	// Tile filter to stack projections back later
	typename Superclass::TileFilterType::Pointer	tileFilter = Superclass::TileFilterType::New();
	typename Superclass::TileFilterType::LayoutArrayType layout;
	layout[0] = 1;
	layout[1] = 1;
	layout[2] = 0;
	tileFilter->SetLayout( layout );
	
	// Arrays of extract, histogram match, then resample filters
	std::vector< typename Superclass::ExtractFilterType::Pointer >	extractP, extractRPrior;
	std::vector< typename Superclass::HistogramMatchType::Pointer >	histogramMatchers;
	std::vector< typename Superclass::ResampleType::Pointer >		resamplers;
	typename Superclass::ImageSizeType size = this->GetInput()->GetLargestPossibleRegion().GetSize();
	typename Superclass::ImageIndexType index = this->GetInput()->GetLargestPossibleRegion().GetIndex();
	size[Dimension-1] = 0;
	for(unsigned int k = 0; k < NProj; ++k)
	  {
	  // Extract
	  index[Dimension-1] = k;
	  typename Superclass::ImageRegionType region(index, size);
	  extractP.push_back( Superclass::ExtractFilterType::New() );
	  extractP[k]->SetInput( this->GetInput() );
	  extractP[k]->SetExtractionRegion( region );
	  extractP[k]->SetDirectionCollapseToSubmatrix();
	  extractRPrior.push_back( Superclass::ExtractFilterType::New() );
	  extractRPrior[k]->SetInput( RPrior->GetOutput() );
	  extractRPrior[k]->SetExtractionRegion( region );
	  extractRPrior[k]->SetDirectionCollapseToSubmatrix();
	  this->m_ImageArithmeticProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( extractP[k]->Update() );
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( extractRPrior[k]->Update() );
	  this->m_ImageArithmeticProbe.Stop();
	  
	  //Histogram match
	  histogramMatchers.push_back( Superclass::HistogramMatchType::New() );
	  histogramMatchers[k]->SetInput( extractRPrior[k]->GetOutput() );
	  histogramMatchers[k]->SetReferenceImage( extractP[k]->GetOutput() );
	  histogramMatchers[k]->ThresholdAtMeanIntensityOff();
	  histogramMatchers[k]->SetNumberOfMatchPoints( this->m_NumberOfMatchPoints );
	  histogramMatchers[k]->SetNumberOfHistogramLevels( this->m_NumberOfHistogramLevels );
	  this->m_ImageArithmeticProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( histogramMatchers[k]->Update() );
	  this->m_ImageArithmeticProbe.Stop();
	  
	  // Rigid registration
	  typename Superclass::MSMetricType::Pointer				metric = Superclass::MSMetricType::New();
	  typename Superclass::RigidTransformType::Pointer		transform = Superclass::RigidTransformType::New();
	  typename Superclass::OptimizerType::Pointer			optimizer = Superclass::OptimizerType::New();
	  typename Superclass::LinearInterpolatorType::Pointer	interpolator = Superclass::LinearInterpolatorType::New();
	  typename Superclass::ImageRegistrationType::Pointer	registration = Superclass::ImageRegistrationType::New();
	  typename Superclass::RigidTransformType::ParametersType	initialParameters( transform->GetNumberOfParameters() );
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
	  this->m_ImageRegistrationProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( registration->Update() );
	  this->m_ImageRegistrationProbe.Stop();
	  
	  // Constrain the transform
	  this->m_RigidTransformParameters = registration->GetOutput()->Get()->GetParameters();
	  if( std::fabs(this->m_RigidTransformParameters[0]) > this->GetMaxRRRotateAngle() )
	    this->m_RigidTransformParameters[0] = (this->GetMaxRRRotateAngle()) * this->m_RigidTransformParameters[0] / std::fabs(this->m_RigidTransformParameters[0]);
	  if( std::fabs(this->m_RigidTransformParameters[1]) > this->GetMaxRRShiftX() )
	    this->m_RigidTransformParameters[1] = (this->GetMaxRRShiftX())  * this->m_RigidTransformParameters[1] / std::fabs(this->m_RigidTransformParameters[1]);
	  if( std::fabs(this->m_RigidTransformParameters[2]) > this->GetMaxRRShiftY() )
	    this->m_RigidTransformParameters[2] = (this->GetMaxRRShiftY())  * this->m_RigidTransformParameters[2] / std::fabs(this->m_RigidTransformParameters[2]);
	  typename Superclass::RigidTransformType::Pointer finalTransform = Superclass::RigidTransformType::New();
	  finalTransform->SetParameters( this->m_RigidTransformParameters );
	  
	  // Resampler
	  resamplers.push_back( Superclass::ResampleType::New() );
	  resamplers[k]->SetInput( histogramMatchers[k]->GetOutput() );
	  resamplers[k]->SetReferenceImage( extractP[k]->GetOutput() );
	  resamplers[k]->SetTransform( finalTransform );
	  resamplers[k]->UseReferenceImageOn();
	  this->m_ImageRegistrationProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( resamplers[k]->Update() );
	  this->m_ImageRegistrationProbe.Stop();
	  
	  // Pass results to tile filter
	  tileFilter->SetInput( k, resamplers[k]->GetOutput() );
	  }
	// Then subtract the results from input projections
	this->m_ImageArithmeticProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( tileFilter->Update() );
	this->m_ImageArithmeticProbe.Stop();
	typename Superclass::InputImagePointer drrNoTumor = tileFilter->GetOutput();
	drrNoTumor->DisconnectPipeline();
	drrNoTumor->SetOrigin( this->GetInput()->GetOrigin() );
	typename Superclass::SubtractFilterType::Pointer projSubtractFilter = Superclass::SubtractFilterType::New();
	projSubtractFilter->SetInput(0, this->GetInput() );
	projSubtractFilter->SetInput(1, drrNoTumor );
	this->m_ImageArithmeticProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( projSubtractFilter->Update() );
	this->m_ImageArithmeticProbe.Stop();
	this->m_p = projSubtractFilter->GetOutput();
	this->m_p->DisconnectPipeline();
	}
  this->m_pRf->SetInput(0, this->m_p);
  this->m_pRfx->SetInput(0, this->m_p);
  this->m_pRfy->SetInput(0, this->m_p);
  this->m_pRfz->SetInput(0, this->m_p);
  this->m_pRfxx->SetInput(0, this->m_p);
  this->m_pRfyy->SetInput(0, this->m_p);
  this->m_pRfzz->SetInput(0, this->m_p);
  this->m_pRfxy->SetInput(0, this->m_p);
  this->m_pRfxz->SetInput(0, this->m_p);
  this->m_pRfyz->SetInput(0, this->m_p);
  this->m_IS_p->SetInput( this->m_p );
  
  // For testing purpose
  // this->GraftOutput(this->m_p); return;
  
  // Qunaities that can be pre-calculated
  double p_mean, p_sigma, Rf_mean, Rf_sigma;
  this->m_ImageStatisticsProbe.Start();
  TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_p->Update() );
  this->m_ImageStatisticsProbe.Stop();
  p_mean = this->m_IS_p->GetMean();
  p_sigma = sqrt( this->m_IS_p->GetVariance() );
  
  for(unsigned int iter = 0; iter < this->m_NumberOfIterations; ++iter)
    {
    // Forward projections
    this->m_ForwardProjectionProbe.Start();
	if(iter == 0)
      TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rf->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rfx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rfy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rfz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rfxx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rfyy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rfzz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rfxy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rfxz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rfyz->Update() );
    this->m_ForwardProjectionProbe.Stop();
  
    // Image arithmetics for gradient calculation
    this->m_ImageArithmeticProbe.Start();
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRfx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRfy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRfz->Update() );
    this->m_ImageArithmeticProbe.Stop();
    this->m_ImageStatisticsProbe.Start();
	if(iter == 0)
	  {
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_Rf->Update() );
	  Rf_mean = this->m_IS_Rf->GetMean();
      Rf_sigma = sqrt( this->m_IS_Rf->GetVariance() );
	  }
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRfx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRfy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRfz->Update() );
    this->m_ImageStatisticsProbe.Stop();
  
    // Gradient of the objective function
    g[0] = -( this->m_IS_pRfx->GetSum() ) / NumOfPPixels / p_sigma / Rf_sigma;
	g[1] = -( this->m_IS_pRfy->GetSum() ) / NumOfPPixels / p_sigma / Rf_sigma;
	g[2] = -( this->m_IS_pRfz->GetSum() ) / NumOfPPixels / p_sigma / Rf_sigma;
	g += this->m_LambdaInDepth * (this->GetInDepthConstraintGradient(r, this->m_PriorOrigin, this->m_ConstrainAngle, 2*this->m_GammaInDepth));
	g += this->m_LambdaInPlane * (this->GetInPlaneConstraintGradient(r, this->m_PriorOrigin, this->m_ConstrainAngle, 2*this->m_GammaInPlane));
	g += this->m_LambdaSI * (this->GetSingleDimensionConstraintGradient(r, this->m_PriorOrigin, 1, 2*this->m_GammaSI));
	
	// The search direction
	if(iter)
	  {
	  double beta = 0.;
	  if( g_prev.GetNorm() > std::numeric_limits<double>::epsilon() && this->m_OptimizerConfiguration == 0 )
	    beta = g.GetNorm() / g_prev.GetNorm();
	  beta *= beta;
	  d = -g + beta * d_prev;
	  }
	else
	  d = -g;
	double rho1 = d * g;
	g_prev = g;	d_prev = d;
	
    // Image arithmetics for Hessian matrix
    this->m_ImageArithmeticProbe.Start();
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRfxx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRfyy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRfzz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRfxy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRfxz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRfyz->Update() );
    this->m_ImageArithmeticProbe.Stop();
    this->m_ImageStatisticsProbe.Start();
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRfxx->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRfyy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRfzz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRfxy->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRfxz->Update() );
    TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRfyz->Update() );
    this->m_ImageStatisticsProbe.Stop();
	
	MatrixType H;
	H(0,0) = -( this->m_IS_pRfxx->GetSum() );
	H(1,1) = -( this->m_IS_pRfyy->GetSum() );
	H(2,2) = -( this->m_IS_pRfzz->GetSum() );
	H(0,1) = -( this->m_IS_pRfxy->GetSum() );	H(1,0) = H(0,1);
	H(0,2) = -( this->m_IS_pRfxz->GetSum() );	H(2,0) = H(0,2);
	H(1,2) = -( this->m_IS_pRfyz->GetSum() );	H(2,1) = H(1,2);
	H = H / NumOfPPixels / p_sigma / Rf_sigma;
	H += this->GetInDepthConstraintHessian(r, this->m_PriorOrigin, this->m_ConstrainAngle, this->m_LambdaInDepth, 2*this->m_GammaInDepth);
	H += this->GetInPlaneConstraintHessian(r, this->m_PriorOrigin, this->m_ConstrainAngle, this->m_LambdaInPlane, 2*this->m_GammaInPlane);
	H += this->GetSingleDimensionConstraintHessian(r, this->m_PriorOrigin, 1, this->m_LambdaSI, 2*this->m_GammaSI);

	// Newton-Raphson approximation of step size
	double rho2 = d * (H * d);
	double eta = 0.;
	if( rho2 > std::numeric_limits<double>::epsilon() )
	  eta = -rho1 / rho2;
	
	if(iter == 0)
	  {
	  // Residual
	  this->m_ImageArithmeticProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRf->Update() );
	  this->m_ImageArithmeticProbe.Stop();
	  this->m_ImageStatisticsProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRf->Update() );
	  this->m_ImageStatisticsProbe.Stop();
	  this->m_ResidualVector.push_back( ( -( this->m_IS_pRf->GetSum() ) / NumOfPPixels + p_mean * Rf_mean ) / p_sigma / Rf_sigma );
	  // Objecive function
	  this->m_ObjectiveVector.push_back( this->m_ResidualVector[iter]
								  + this->m_LambdaInDepth * (this->GetInDepthConstraintObjective(r, this->m_PriorOrigin, this->m_ConstrainAngle, 2*this->m_GammaInDepth))
								  + this->m_LambdaInPlane * (this->GetInPlaneConstraintObjective(r, this->m_PriorOrigin, this->m_ConstrainAngle, 2*this->m_GammaInPlane))
								  + this->m_LambdaSI * (this->GetSingleDimensionConstraintObjective(r, this->m_PriorOrigin, 1, 2*this->m_GammaSI)) );
	  this->m_TargetOriginVector.push_back( r );	  
	  }
	  
	// Preparing for line searching
	double Grhs = this->m_ObjectiveVector[iter] + this->m_C1 * eta * rho1;
	r_new = r + eta * d;
	this->SetTargetOrigin( r_new );
	this->m_ForwardProjectionProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rf->Update() );
	this->m_ForwardProjectionProbe.Stop();
	this->m_ImageArithmeticProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRf->Update() );
	this->m_ImageArithmeticProbe.Stop();
	this->m_ImageStatisticsProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRf->Update() );
	this->m_ImageStatisticsProbe.Stop();
	// Objective function
	double Rlhs = ( -( this->m_IS_pRf->GetSum() ) / NumOfPPixels + p_mean * Rf_mean ) / p_sigma / Rf_sigma;
	double Glhs = Rlhs 
					+ this->m_LambdaInDepth * (this->GetInDepthConstraintObjective(r_new, this->m_PriorOrigin, this->m_ConstrainAngle, 2*this->m_GammaInDepth))
					+ this->m_LambdaInPlane * (this->GetInPlaneConstraintObjective(r_new, this->m_PriorOrigin, this->m_ConstrainAngle, 2*this->m_GammaInPlane))
					+ this->m_LambdaSI * (this->GetSingleDimensionConstraintObjective(r_new, this->m_PriorOrigin, 1, 2*this->m_GammaSI));
	
	while( Glhs >= Grhs && eta > std::numeric_limits<double>::epsilon() )
	  {
	  eta *= this->m_C2;
	  r_new = r + eta * d;
	  this->SetTargetOrigin( r_new );
	  this->m_ForwardProjectionProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_Rf->Update() );
	  this->m_ForwardProjectionProbe.Stop();
	  this->m_ImageArithmeticProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_pRf->Update() );
	  this->m_ImageArithmeticProbe.Stop();
	  this->m_ImageStatisticsProbe.Start();
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( this->m_IS_pRf->Update() );
	  this->m_ImageStatisticsProbe.Stop();
	  // Objective function
	  Rlhs = ( -( this->m_IS_pRf->GetSum() ) / NumOfPPixels + p_mean * Rf_mean ) / p_sigma / Rf_sigma;
	  Glhs = Rlhs 
				+ this->m_LambdaInDepth * (this->GetInDepthConstraintObjective(r_new, this->m_PriorOrigin, this->m_ConstrainAngle, 2*this->m_GammaInDepth))
				+ this->m_LambdaInPlane * (this->GetInPlaneConstraintObjective(r_new, this->m_PriorOrigin, this->m_ConstrainAngle, 2*this->m_GammaInPlane))
				+ this->m_LambdaSI * (this->GetSingleDimensionConstraintObjective(r_new, this->m_PriorOrigin, 1, 2*this->m_GammaSI));
	  Grhs = this->m_ObjectiveVector[iter] + this->m_C1 * eta * rho1;
	  }
	VectorType d_total = r_new - this->m_PriorOrigin;
	if( d_total.GetNorm() > this->m_MaxDisplacement )
	  {
	  // this->SetTargetOrigin( this->m_TargetOriginVector[0] );
	  this->SetTargetOrigin( this->m_PriorOrigin );
	  break;
	  }
	
	this->m_ResidualVector.push_back( Rlhs );
	this->m_ObjectiveVector.push_back( Glhs );
	this->m_TargetOriginVector.push_back( r_new );
	
	this->SetTargetOrigin( r_new );
	r = r_new;
	
	if( std::fabs( eta * d.GetNorm() ) < this->m_StoppingCriterion )
	  break;

	}
	this->m_RfImage = this->m_Rf->GetOutput();
	
	this->GraftOutput( this->m_f );
}
	
template<class TInputImage, class TOutputImage>
void
DirectConeBeamNCCTracking<TInputImage, TOutputImage>
::PrintTiming(std::ostream & os) const
{
  os << "     DirectConeBeamNCCTracking timing:" << std::endl;
  os << "       Image arithmetic: " << this->m_ImageArithmeticProbe.GetTotal()
     << ' ' << this->m_ImageArithmeticProbe.GetUnit() << std::endl;
  os << "       Image statistics: " << this->m_ImageStatisticsProbe.GetTotal()
     << ' ' << this->m_ImageStatisticsProbe.GetUnit() << std::endl;
  os << "       Forward projection: " << this->m_ForwardProjectionProbe.GetTotal()
     << ' ' << this->m_ForwardProjectionProbe.GetUnit() << std::endl;
  os << "       Image registration: " << this->m_ImageRegistrationProbe.GetTotal()
     << ' ' << this->m_ImageRegistrationProbe.GetUnit() << std::endl;
}

} // end namespace rtk

#endif // __rtkDirectConeBeamNCCTracking_txx
