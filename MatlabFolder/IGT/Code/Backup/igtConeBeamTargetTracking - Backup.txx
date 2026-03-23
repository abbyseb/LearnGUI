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

#ifndef __igtConeBeamTargetTracking_txx
#define __igtConeBeamTargetTracking_txx

#include "igtConeBeamTargetTracking.h"

#include <algorithm>
#include <itkTimeProbe.h>

namespace igt
{
template<class TOutputImage>
ConeBeamTargetTracking<TOutputImage>
::ConeBeamTargetTracking()
{
  // Input 0: projections
  this->SetNumberOfRequiredInputs(1);
  // Note that all the f's are also required, but we check that later on

  // Set default parameters
  m_Verbose = false;
  m_NumberOfIterations = 10;
  m_OptimizerConfiguration = 0;
  m_C1 = 1e-4;
  m_C2 = 0.5;
  m_StoppingCriterion = 0.1;

  // Initialize search radius and resolution in terms of lateral, SI, in-depth
  VectorType searchRadius;
  searchRadius[0] = 1; searchRadius[1] = 1; searchRadius[2] = 8;
  m_SearchRadius.push_back(searchRadius);

  VectorType searchResolution;
  searchResolution[0] = 1; searchResolution[1] = 1; searchResolution[2] = 4;
  m_SearchResolution.push_back(searchResolution);
  
  m_TargetOrigin.Fill( 0 );
  m_PredictedTargetOrigin.Fill(0);
  m_MeasurementUncertainty.GetVnlMatrix().fill_diagonal(4);
  m_MeasurementUncertainty(2,2) = 25;
  m_StateUncertainty.GetVnlMatrix().fill_diagonal(4);
  m_PriorUncertainty.GetVnlMatrix().fill_diagonal(4);
  m_PredictedUncertainty.GetVnlMatrix().fill_diagonal(4);
  m_UpdatedUncertainty.GetVnlMatrix().fill_diagonal(4);
  m_KalmanGain.GetVnlMatrix().fill_diagonal(2./3.);
  m_KalmanGain(2,2) = 4./(4.+25.);
  
  m_NProj = 0;
  m_NumOfPPixels = 0;
  
  // Image pointers
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
  m_pRf = MultiplyFilterType::New();

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

  m_IS_p = ImageStatisticsFilterType::New();
  m_IS_Rf = ImageStatisticsFilterType::New();
  m_IS_pRf = ImageStatisticsFilterType::New();
  m_IS_pRf->SetInput(m_pRf->GetOutput());
}

template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
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

template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
::ForwardProjectTargetModel()
{
  if(this->IsModelProvided())
	{m_Rf->Update();}
  else
	itkGenericExceptionMacro(<< "The target model is not provided");
}

template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
::ForwardProjectTargetModelDerivatives()
{
  if (this->IsModelProvided())
  {
	m_Rfx->Update();
	m_Rfy->Update();
	m_Rfz->Update();
	m_Rfxx->Update();
	m_Rfyy->Update();
	m_Rfzz->Update();
	m_Rfxy->Update();
	m_Rfxz->Update();
	m_Rfyz->Update();
  }
  else
	itkGenericExceptionMacro(<< "The target model is not provided");
}

template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
::GenerateInputRequestedRegion()
{
  ImagePointer inputPtr =
    const_cast< TOutputImage * >( this->GetInput() );

  if ( !inputPtr || !(this->IsModelProvided()) )
    return;
}

template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
::GenerateOutputInformation()
{
  // Check and set geometry
  if(this->GetGeometry().GetPointer() == NULL)
    itkGenericExceptionMacro(<< "The geometry of the reconstruction has not been set");
	
  unsigned int NumOfPPixels = 1.;
  for (unsigned int k = 0; k < TOutputImage::ImageDimension; ++k)
	NumOfPPixels *= this->GetInput()->GetLargestPossibleRegion().GetSize()[k];
	
  this->m_NumOfPPixels = NumOfPPixels;
	
  unsigned int NProj = this->GetInput()->GetLargestPossibleRegion().GetSize()[TOutputImage::ImageDimension-1];
  this->m_NProj = NProj;

  m_pRf->SetInput1(this->GetInput());
  m_s->SetInput(1, this->GetInput());
  m_ZeroP->SetInput( this->GetInput() );
  m_IS_p->SetInput(this->GetInput());

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

  m_IS_Rf->SetInput(m_Rf->GetOutput());
  m_pRf->SetInput2(m_Rf->GetOutput());

  m_RfImage = m_Rf->GetOutput();
}

template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
::KalmanStatePrediction()
{
	this->SetTargetOrigin(m_PredictedTargetOrigin);

	m_PredictedUncertainty = m_PriorUncertainty + m_StateUncertainty;
	m_KalmanGain = m_PredictedUncertainty * (( m_PredictedUncertainty + m_MeasurementUncertainty ).GetInverse());
}

template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
::KalmanGainCorrection()
{
	MatrixType I;
	I.SetIdentity();

	VectorType rotatedPredictedTargetOrigin = m_RotationMatrix * m_PredictedTargetOrigin;
	VectorType rotatedOrigin = rotatedPredictedTargetOrigin
		+ m_KalmanGain * (m_RotationMatrix * m_TargetOrigin - rotatedPredictedTargetOrigin);
	m_TargetOrigin = m_ReverseRotationMatrix * rotatedOrigin;
	m_UpdatedUncertainty = (I - m_KalmanGain) * m_PredictedUncertainty;
	m_TargetOriginVector.push_back( m_TargetOrigin );

	this->SetTargetOrigin( m_TargetOrigin );
	m_ForwardProjectionProbe.Start();
	this->ForwardProjectTargetModel();
	m_ForwardProjectionProbe.Stop();
	m_ImageArithmeticProbe.Start();
	m_s->Update();
	m_ImageArithmeticProbe.Stop();
	m_ImageStatisticsProbe.Start();
	m_IS_s->Update();
	m_ImageStatisticsProbe.Stop();
	m_ResidualVector.push_back( 0.5 * this->GetSquaredSum(m_IS_s, m_NumOfPPixels) );
}

template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
::ConeBeamOptimizationMeasurement()
{
  const unsigned int Dimension = TOutputImage::ImageDimension;
	
  // Gradient, search direction, and position vectors
  VectorType g, g_prev, d, d_prev, r, r_new;
  r = this->GetTargetOrigin();
  
  // Hessian matrix type
  MatrixType H;

  // Generate zero projection stacks
  m_ImageArithmeticProbe.Start();
  m_ZeroP->Update();
  m_ImageArithmeticProbe.Stop();
  
  for(unsigned int iter = 0; iter < m_NumberOfIterations; ++iter)
    {
    // Forward projections
    m_ForwardProjectionProbe.Start();
	if(iter == 0)
      this->ForwardProjectTargetModel();
    this->ForwardProjectTargetModelDerivatives();
    m_ForwardProjectionProbe.Stop();
  
    // Image arithmetics for gradient calculation
    m_ImageArithmeticProbe.Start();
	if(iter == 0)
      m_s->Update();
    m_sRfx->Update();
    m_sRfy->Update();
    m_sRfz->Update();
    m_ImageArithmeticProbe.Stop();
    m_ImageStatisticsProbe.Start();
	if(iter == 0)
      m_IS_s->Update();
    m_IS_sRfx->Update();
    m_IS_sRfy->Update();
    m_IS_sRfz->Update();
    m_ImageStatisticsProbe.Stop();
  
    // Gradient of the objective function
	g = this->GetGradient(r);
	
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
    m_RfxRfy->Update();
    m_RfxRfz->Update();
    m_RfyRfz->Update();
    m_sRfxx->Update();
    m_sRfyy->Update();
    m_sRfzz->Update();
    m_sRfxy->Update();
    m_sRfxz->Update();
    m_sRfyz->Update();
    m_ImageArithmeticProbe.Stop();
    m_ImageStatisticsProbe.Start();
    m_IS_Rfx->Update();
    m_IS_Rfy->Update();
    m_IS_Rfz->Update();
    m_IS_RfxRfy->Update();
    m_IS_RfxRfz->Update();
    m_IS_RfyRfz->Update();
    m_IS_sRfxx->Update();
    m_IS_sRfyy->Update();
    m_IS_sRfzz->Update();
    m_IS_sRfxy->Update();
    m_IS_sRfxz->Update();
    m_IS_sRfyz->Update();
    m_ImageStatisticsProbe.Stop();
	
	H = this->GetHessian(r);
	
	// Newton-Raphson approximation of step size
	double rho2 = d * (H * d);
	double eta = 0.;
	if( rho2 > std::numeric_limits<double>::epsilon() )
	  eta = -rho1 / rho2;
	
	if(iter == 0)
	  {
	  // Residual
	  m_ResidualVector.push_back( 0.5 * this->GetSquaredSum(m_IS_s, m_NumOfPPixels) );
	  m_TargetOriginVector.push_back( r );	  
	  }
	  
	// Preparing for line searching
	double Rrhs = m_ResidualVector[iter] + m_C1 * eta * rho1;
	r_new = r + eta * d;
	this->SetTargetOrigin( r_new );
	m_ForwardProjectionProbe.Start();
	m_Rf->Update();
	m_ForwardProjectionProbe.Stop();
	m_ImageArithmeticProbe.Start();
	m_s->Update();
	m_ImageArithmeticProbe.Stop();
	m_ImageStatisticsProbe.Start();
	m_IS_s->Update();
	m_ImageStatisticsProbe.Stop();
	// Objective function
	double Rlhs = 0.5 * this->GetSquaredSum(m_IS_s, m_NumOfPPixels);
	
	while( Rlhs >= Rrhs && eta > std::numeric_limits<double>::epsilon() )
	  {
	  eta *= m_C2;
	  r_new = r + eta * d;
	  this->SetTargetOrigin( r_new );
	  m_ForwardProjectionProbe.Start();
	  m_Rf->Update();
	  m_ForwardProjectionProbe.Stop();
	  m_ImageArithmeticProbe.Start();
	  m_s->Update();
	  m_ImageArithmeticProbe.Stop();
	  m_ImageStatisticsProbe.Start();
	  m_IS_s->Update();
	  m_ImageStatisticsProbe.Stop();
	  // Objective function, Constraints to be added
	  Rlhs = 0.5 * this->GetSquaredSum(m_IS_s, m_NumOfPPixels);
	  Rrhs = m_ResidualVector[iter] + m_C1 * eta * rho1;
	  }

	m_ResidualVector.push_back( Rlhs );
	m_TargetOriginVector.push_back( r_new );
	
	this->SetTargetOrigin( r_new );
	r = r_new;
	
	if( std::fabs( eta * d.GetNorm() ) < m_StoppingCriterion )
	  break;
	}
}
	
template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
::ConeBeamTemplateMatching()
{

  if (m_SearchRadius.size() != m_SearchResolution.size())
  {
	itkGenericExceptionMacro(<< "SearchRadius and SearchResolution must have the same number of entries.");
  }

  const unsigned int Dimension = TOutputImage::ImageDimension;
	
  // Qunaities that can be pre-calculated
  double p_mean, p_sigma;
  this->m_ImageStatisticsProbe.Start();
  m_IS_p->Update();
  this->m_ImageStatisticsProbe.Stop();
  p_mean = m_IS_p->GetMean();
  p_sigma = sqrt( m_IS_p->GetVariance() );

  VectorType r, r0, r_rot;
  r = this->GetTargetOrigin();
  r0 = r;
  r_rot = m_RotationMatrix * r;

  // Generate zero projection stacks
  m_ImageArithmeticProbe.Start();
  m_ZeroP->Update();
  m_ImageArithmeticProbe.Stop();
	
  m_s->SetInput(1, this->GetInput());

  for (unsigned int k_res = 0; k_res != m_SearchRadius.size(); ++k_res)
  {
    std::vector<VectorType> searchPoints;
	std::vector<double> residualValues;
	VectorType r_tmp;

	// Loop through LR, SI, AP
	for (double lat = r_rot[0] - m_SearchRadius[k_res][0]; lat <= r_rot[0] + m_SearchRadius[k_res][0]; lat += m_SearchResolution[k_res][0])
	{
	  for (double si = r_rot[1] - m_SearchRadius[k_res][1]; si <= r_rot[1] + m_SearchRadius[k_res][1]; si += m_SearchResolution[k_res][1])
	  {
		for (double idep = r_rot[2] - m_SearchRadius[k_res][2]; idep <= r_rot[2] + m_SearchRadius[k_res][2]; idep += m_SearchResolution[k_res][2])
		{
		  r_tmp[0] = lat; r_tmp[1] = si; r_tmp[2] = idep;
		  searchPoints.push_back(r_tmp);
		  this->SetTargetOrigin(m_ReverseRotationMatrix*r_tmp);
		  m_ForwardProjectionProbe.Start();
		  this->ForwardProjectTargetModel();
		  m_ForwardProjectionProbe.Stop();
		  
		  m_ImageStatisticsProbe.Start();
		  m_IS_Rf->Update();
		  m_ImageStatisticsProbe.Stop();
		  double Rf_mean = m_IS_Rf->GetMean();
		  double Rf_sigma = sqrt( m_IS_Rf->GetVariance() );

		  m_ImageArithmeticProbe.Start();
		  m_pRf->Update();
		  m_ImageArithmeticProbe.Stop();
		  m_ImageStatisticsProbe.Start();
		  m_IS_pRf->Update();
		  m_ImageStatisticsProbe.Stop();
		  double pRfSum = m_IS_pRf->GetSum();
		  residualValues.push_back( ( pRfSum / m_NumOfPPixels + p_mean * Rf_mean ) / p_sigma / Rf_sigma );

		  // L2 norm matching
		  /*
		  m_ImageArithmeticProbe.Start();
		  m_s->Update();
		  m_ImageArithmeticProbe.Stop();
		  m_ImageStatisticsProbe.Start();
		  m_IS_s->Update();
		  m_ImageStatisticsProbe.Stop();
		  residualValues.push_back( 0.5 * this->GetSquaredSum(m_IS_s, NumOfPPixels) );
		  */
		}
	  }
	}
	std::vector<double>::iterator maxIt = std::max_element(std::begin(residualValues), std::end(residualValues));
	unsigned int maxIndex = std::distance(std::begin(residualValues), maxIt);
	m_ResidualVector.push_back( *maxIt );
	r_rot = searchPoints[maxIndex];
	r = m_ReverseRotationMatrix * r_rot;
	this->SetTargetOrigin(r);
	m_TargetOriginVector.push_back( r );

	// L2 norm matching
	/*
	std::vector<double>::iterator minIt = std::min_element(std::begin(residualValues), std::end(residualValues));
	unsigned int minIndex = std::distance(std::begin(residualValues), minIt);
	m_ResidualVector.push_back( *minIt );
	r = searchPoints[minIndex];
	this->SetTargetOrigin(r);
	m_TargetOriginVector.push_back( r );
	*/
  }
}

template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
::GenerateData()
{
	this->KalmanStatePrediction();

	this->ConeBeamOptimizationMeasurement();
	//this->ConeBeamTemplateMatching();

	this->KalmanGainCorrection();

	this->GraftOutput( m_f );
}

template<class TOutputImage>
void
ConeBeamTargetTracking<TOutputImage>
::PrintTiming(std::ostream & os) const
{
  os << "     ConeBeamTargetTracking timing:" << std::endl;
  os << "       Image arithmetic: " << m_ImageArithmeticProbe.GetTotal()
     << ' ' << m_ImageArithmeticProbe.GetUnit() << std::endl;
  os << "       Image statistics: " << m_ImageStatisticsProbe.GetTotal()
     << ' ' << m_ImageStatisticsProbe.GetUnit() << std::endl;
  os << "       Forward projection: " << m_ForwardProjectionProbe.GetTotal()
     << ' ' << m_ForwardProjectionProbe.GetUnit() << std::endl;
}

} // end namespace igt

#endif // __igtConeBeamTargetTracking_txx
