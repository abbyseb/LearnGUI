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

#ifndef __igtConeBeamTargetTracking_h
#define __igtConeBeamTargetTracking_h

#include "rtkForwardProjectionImageFilter.h"

#include <itkMultiplyImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkAddImageFilter.h>
#include <itkTimeProbe.h>
#include <itkStatisticsImageFilter.h>
#include <itkMatrix.h>
#include <itkVector.h>

#include "rtkConstantImageSource.h"
#include "rtkIterativeConeBeamReconstructionFilter.h"

namespace igt
{

/** \class ConeBeamTargetTracking
 * \brief The filter class for cone beam target tracking.
 *		  This code aims to track the target 3D position based on
 *        the target model f, measured projections p, and the geometric
 *		  relation between the two m_Geometry
 *		  The solution is modeled as the minimization of the following
 *		  objective function:
 *        argmin 1/2||Rf-p||^2
 *		  A Kalman filter is used to incorporate prior knowledge of the target location
 * \author Andy Shieh 2016
 */
template<class TOutputImage>
class ITK_EXPORT ConeBeamTargetTracking :
	public rtk::IterativeConeBeamReconstructionFilter<TOutputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef ConeBeamTargetTracking                               			   		Self;
  typedef rtk::IterativeConeBeamReconstructionFilter<TOutputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>												Pointer;
  typedef itk::SmartPointer<const Self>											ConstPointer;

  /** Some convenient typedefs. */
  typedef TOutputImage OutputImageType;
  typedef typename OutputImageType::Pointer         ImagePointer;
  typedef typename OutputImageType::PixelType		ImagePixelType;
  typedef typename OutputImageType::Pointer			ImagePointerType;
  typedef typename OutputImageType::RegionType 		ImageRegionType;
  typedef typename OutputImageType::SizeType 		ImageSizeType;
  typedef typename OutputImageType::IndexType 		ImageIndexType;
  typedef typename OutputImageType::SpacingType		ImageSpacingType;
  typedef typename OutputImageType::PointType		ImagePointType;
  typedef typename OutputImageType::DirectionType	ImageDirectionType;

  /** Typedefs of each subfilter of this composite filter */
  typedef itk::StatisticsImageFilter< OutputImageType >										 ImageStatisticsFilterType;
  typedef rtk::ForwardProjectionImageFilter< OutputImageType, OutputImageType >              ForwardProjectionFilterType;
  typedef itk::SubtractImageFilter< OutputImageType, OutputImageType >                       SubtractFilterType;
  typedef itk::MultiplyImageFilter< OutputImageType, OutputImageType, OutputImageType >      MultiplyFilterType;
  
  /* Typedefs for matrix operation */
  typedef itk::Matrix<double, 3, 3>			MatrixType;
  typedef itk::Vector<double, 3>			VectorType;  

/** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(ConeBeamTargetTracking, rtk::IterativeConeBeamReconstructionFilter);
  
  /** Get / Set the object pointer to projection geometry */
  itkGetMacro(Geometry, rtk::ThreeDCircularProjectionGeometry::Pointer);
  virtual void SetGeometry(rtk::ThreeDCircularProjectionGeometry::Pointer geometry)
  {
	  m_Geometry = geometry;

	  m_RotationMatrix.GetVnlMatrix().fill_diagonal(0);
	  m_RotationMatrix(1, 1) = 1;
	  m_RotationMatrix(0, 0) = cos(-1 * m_Geometry->GetMeanAngle());
	  m_RotationMatrix(0, 2) = sin(-1 * m_Geometry->GetMeanAngle());
	  m_RotationMatrix(2, 0) = -m_RotationMatrix(0, 2);
	  m_RotationMatrix(2, 2) = m_RotationMatrix(0, 0);
	  m_ReverseRotationMatrix = m_RotationMatrix;
	  m_ReverseRotationMatrix(0, 2) = -m_ReverseRotationMatrix(0, 2);
	  m_ReverseRotationMatrix(2, 0) = -m_ReverseRotationMatrix(2, 0);
  }

  virtual void PrintTiming(std::ostream& os) const;

  /** Get / Set the number of iterations. Default is 20. */
  itkGetMacro(NumberOfIterations, unsigned int);
  itkSetMacro(NumberOfIterations, unsigned int);
  
  /** Get / Set verbosity. Default is false. */
  itkGetMacro(Verbose, bool);
  itkSetMacro(Verbose, bool);

  void SetVerboseOn()
  { this->SetVerbose(true); }

  void SetVerboseOff()
  { this->SetVerbose(false); }
  
  // Get / Set the optimizer. 0: conjugate grdaient; 1: steepest-descent;
  itkGetMacro(OptimizerConfiguration, unsigned int);
  itkSetMacro(OptimizerConfiguration, unsigned int);
  
  /** Get / Set C1, C2. Default is C1 = 1e-4, C2 = 0.5 */
  itkGetMacro(C1, double);
  itkSetMacro(C1, double);
  itkGetMacro(C2, double);
  itkSetMacro(C2, double);

  /** Get / Set stopping criterion */
  itkGetMacro(StoppingCriterion, double);
  itkSetMacro(StoppingCriterion, double);
  
  /** Get / Set residual and objective function vectors */
  itkGetMacro(ResidualVector, std::vector<double>);
  
  /** Get / Set target origin vector */
  itkGetMacro(TargetOriginVector, std::vector<VectorType>);

  // Get final residual value
  double GetFinalResidual()
    {
	if( m_ResidualVector.size() > 0)
	  {
	  std::vector<double>::reverse_iterator rit = m_ResidualVector.rbegin();
	  return *rit;
	  }
	else
	  return 0;
    }
  
  // Get m_RfImage
  itkGetMacro(RfImage, ImagePointerType);
  
  /** Get the boolean indicating whether f and its derivatives are given */
  bool IsModelProvided() { return ( m_f && m_fx && m_fy && m_fz
						         && m_fxx && m_fyy && m_fzz 
							     && m_fxy && m_fxz && m_fyz ); }
  
  /** Get/ Set f and its derivatives */
  itkGetMacro(f, ImagePointerType);
  itkSetMacro(f, ImagePointerType);
  itkGetMacro(fx, ImagePointerType);
  itkSetMacro(fx, ImagePointerType);
  itkGetMacro(fy, ImagePointerType);
  itkSetMacro(fy, ImagePointerType);
  itkGetMacro(fz, ImagePointerType);
  itkSetMacro(fz, ImagePointerType);
  itkGetMacro(fxx, ImagePointerType);
  itkSetMacro(fxx, ImagePointerType);
  itkGetMacro(fyy, ImagePointerType);
  itkSetMacro(fyy, ImagePointerType);
  itkGetMacro(fzz, ImagePointerType);
  itkSetMacro(fzz, ImagePointerType);
  itkGetMacro(fxy, ImagePointerType);
  itkSetMacro(fxy, ImagePointerType);
  itkGetMacro(fxz, ImagePointerType);
  itkSetMacro(fxz, ImagePointerType);
  itkGetMacro(fyz, ImagePointerType);
  itkSetMacro(fyz, ImagePointerType);
  
  /** Get / Set the target origin */
  void SetTargetOrigin(VectorType origin)
  { 
    if(!(this->IsModelProvided()))
      itkGenericExceptionMacro(<< "The target model and its derivatives are not set");
    ImagePointType originPoint;
	for(unsigned int k = 0; k < 3; ++k)
	  originPoint[k] = origin[k];
    m_TargetOrigin = origin;
    m_f->SetOrigin(originPoint);
	m_fx->SetOrigin(originPoint);
	m_fy->SetOrigin(originPoint);
	m_fz->SetOrigin(originPoint);
	m_fxx->SetOrigin(originPoint);
	m_fyy->SetOrigin(originPoint);
	m_fzz->SetOrigin(originPoint);
	m_fxy->SetOrigin(originPoint);
	m_fxz->SetOrigin(originPoint);
	m_fyz->SetOrigin(originPoint);
  }
  
  void SetTargetOrigin(ImagePointType originPoint)
  { 
    if(!(this->IsModelProvided()))
      itkGenericExceptionMacro(<< "The target model and its derivatives are not set");
    VectorType origin;
	for(unsigned int k = 0; k < 3; ++k)
	  origin[k] = originPoint[k];
    m_TargetOrigin = origin;
    m_f->SetOrigin(originPoint);
	m_fx->SetOrigin(originPoint);
	m_fy->SetOrigin(originPoint);
	m_fz->SetOrigin(originPoint);
	m_fxx->SetOrigin(originPoint);
	m_fyy->SetOrigin(originPoint);
	m_fzz->SetOrigin(originPoint);
	m_fxy->SetOrigin(originPoint);
	m_fxz->SetOrigin(originPoint);
	m_fyz->SetOrigin(originPoint);
  }
 
  itkGetMacro(TargetOrigin, VectorType);

  itkSetMacro(PredictedTargetOrigin, VectorType);
  itkGetMacro(PredictedTargetOrigin, VectorType);

  itkSetMacro(MeasurementUncertainty, MatrixType);
  itkGetMacro(MeasurementUncertainty, MatrixType);

  itkSetMacro(StateUncertainty, MatrixType);
  itkGetMacro(StateUncertainty, MatrixType);

  itkSetMacro(PriorUncertainty, MatrixType);
  itkGetMacro(PriorUncertainty, MatrixType);

  //itkSetMacro(PredictedUncertainty, MatrixType);
  itkGetMacro(PredictedUncertainty, MatrixType);

  //itkSetMacro(UpdatedUncertainty, MatrixType);
  itkGetMacro(UpdatedUncertainty, MatrixType);

  itkSetMacro(KalmanGain, MatrixType);
  itkGetMacro(KalmanGain, MatrixType);

  itkSetMacro(SearchRadius, std::vector<VectorType>);
  itkGetMacro(SearchRadius, std::vector<VectorType>);
  
  itkSetMacro(SearchResolution, std::vector<VectorType>);
  itkGetMacro(SearchResolution, std::vector<VectorType>);

  itkGetMacro(RotationMatrix, MatrixType);
  itkGetMacro(ReverseRotationMatrix, MatrixType);

  /** Select the ForwardProjection filter */
  void SetForwardProjectionFilter (int _arg);
  
  /** Set global maximum number of threads for multi thread methods, to prevent crashes sometime */
  void SetMaxNumberOfThreads( int _arg) { itk::MultiThreader::SetGlobalMaximumNumberOfThreads( _arg ); }
  
  // Forward project target model (at current target position)
  void ForwardProjectTargetModel();

  // Forward project target model derivatives (at current target position)
  void ForwardProjectTargetModelDerivatives();

  // Kalman prediction
  virtual void KalmanStatePrediction();

  // Measurement using L2 norm based optimization
  virtual void ConeBeamOptimizationMeasurement();

  // Measurement using spatial loop-based template matching
  virtual void ConeBeamTemplateMatching();

  // Get time probes
  itkGetMacro(ForwardProjectionProbe, itk::TimeProbe);
  itkGetMacro(ImageArithmeticProbe, itk::TimeProbe);
  itkGetMacro(ImageStatisticsProbe, itk::TimeProbe);

protected:
  ConeBeamTargetTracking();
  ~ConeBeamTargetTracking(){}

  virtual void GenerateInputRequestedRegion();

  virtual void GenerateOutputInformation();

  virtual void GenerateData();

  // Kalman gain correction
  virtual void KalmanGainCorrection();

  /** The two inputs should not be in the same space so there is nothing
   * to verify. */
  virtual void VerifyInputInformation() {}
  
  // Get image pixel squared sum
  double GetSquaredSum(typename ImageStatisticsFilterType::Pointer isFilter, unsigned int N)
  {
	double result = isFilter->GetMean();
	result *= N * result;
	result += N * ( isFilter->GetVariance() );
	return result;
  }
  
  virtual VectorType GetGradient(VectorType r)
  {
	  VectorType result;
	  result = this->GetL2NormGradient();
	  return result;
  }
  
  VectorType GetL2NormGradient()
  {
	  VectorType result;
	  result[0] = this->m_IS_sRfx->GetSum();
	  result[1] = this->m_IS_sRfy->GetSum();
	  result[2] = this->m_IS_sRfz->GetSum();
	  return result;
  }
  
  virtual MatrixType GetHessian(VectorType r)
  {
	  MatrixType result;
	  result = this->GetL2NormHessian();
	  return result;
  }
	
  MatrixType GetL2NormHessian()
  {
	  MatrixType result, H1, H2;
	  H1(0, 0) = this->GetSquaredSum(this->m_IS_Rfx, this->m_NumOfPPixels);
	  H1(1, 1) = this->GetSquaredSum(this->m_IS_Rfy, this->m_NumOfPPixels);
	  H1(2, 2) = this->GetSquaredSum(this->m_IS_Rfy, this->m_NumOfPPixels);
	  H1(0, 1) = this->m_IS_RfxRfy->GetSum();	H1(1, 0) = H1(0, 1);
	  H1(0, 2) = this->m_IS_RfxRfz->GetSum();	H1(2, 0) = H1(0, 2);
	  H1(1, 2) = this->m_IS_RfyRfz->GetSum();	H1(2, 1) = H1(1, 2);
	  H2(0, 0) = this->m_IS_sRfxx->GetSum();
	  H2(1, 1) = this->m_IS_sRfyy->GetSum();
	  H2(2, 2) = this->m_IS_sRfzz->GetSum();
	  H2(0, 1) = this->m_IS_sRfxy->GetSum();	H2(1, 0) = H2(0, 1);
	  H2(0, 2) = this->m_IS_sRfxz->GetSum();	H2(2, 0) = H2(0, 2);
	  H2(1, 2) = this->m_IS_sRfyz->GetSum();	H2(2, 1) = H2(1, 2);
	  result = H1 + H2;
	  return result;
  }
  
  /** Geometry object */
  rtk::ThreeDCircularProjectionGeometry::Pointer m_Geometry;

  /** Pointers to each subfilter */
  typename ForwardProjectionFilterType::Pointer	m_Rf, m_Rfx, m_Rfy, m_Rfz,
												m_Rfxx, m_Rfyy, m_Rfzz,
												m_Rfxy, m_Rfxz, m_Rfyz;
  typename SubtractFilterType::Pointer			m_s;
  typename MultiplyFilterType::Pointer			m_ZeroP,
	  m_sRfx, m_sRfy, m_sRfz,
	  m_RfxRfy, m_RfxRfz, m_RfyRfz,
	  m_sRfxx, m_sRfyy, m_sRfzz,
	  m_sRfxy, m_sRfxz, m_sRfyz,
	  m_pRf;
  typename ImageStatisticsFilterType::Pointer	m_IS_s, m_IS_Rfx, m_IS_Rfy, m_IS_Rfz,
	  m_IS_sRfx, m_IS_sRfy, m_IS_sRfz,
	  m_IS_RfxRfy, m_IS_RfxRfz, m_IS_RfyRfz,
	  m_IS_sRfxx, m_IS_sRfyy, m_IS_sRfzz,
	  m_IS_sRfxy, m_IS_sRfxz, m_IS_sRfyz,
	  m_IS_p, m_IS_Rf, m_IS_pRf;

  /** Number of iterations */
  unsigned int m_NumberOfIterations;
  
  /** Stop iterations if change in x is smaller this value. Default = 0.01 mm */
  double m_StoppingCriterion;
  
  // Optimizer configuration
  unsigned int m_OptimizerConfiguration;
  
  /** c1 and c2 for line searching */
  double m_C1;
  double m_C2;
  
  /** Verbose */
  bool m_Verbose;
  
  // Vectors recording the residual and objective function and position vector in each iteration
  std::vector< double > m_ResidualVector;
  // The last target origin is the one predicted from Kalman filter.
  // The rest are from residual minimization
  std::vector< VectorType > m_TargetOriginVector;
  
  /** The image f and its derivatives */
  ImagePointerType	m_f, m_fx, m_fy, m_fz, m_fxx, m_fyy, m_fzz, m_fxy, m_fxz, m_fyz;
  
  // Output from m_Rf
  ImagePointerType m_RfImage;
  
  /** The current target origin */
  VectorType	m_TargetOrigin;

  // For Kalman filter
  VectorType	m_PredictedTargetOrigin;
  MatrixType	m_MeasurementUncertainty, m_StateUncertainty;
  MatrixType	m_PriorUncertainty, m_PredictedUncertainty, m_UpdatedUncertainty;
  MatrixType	m_KalmanGain;
  
  // Number of projections and pixels
  unsigned int	m_NProj, m_NumOfPPixels;
  
  /** Time probes */
  itk::TimeProbe m_ForwardProjectionProbe;
  itk::TimeProbe m_ImageArithmeticProbe;
  itk::TimeProbe m_ImageStatisticsProbe;

  // Template matching search radius and resolution in mm. 
  // Length of std vector = levels of multiresolution search
  std::vector<VectorType> m_SearchRadius;
  std::vector<VectorType> m_SearchResolution;

  MatrixType m_RotationMatrix, m_ReverseRotationMatrix;

private:
  //purposely not implemented
  ConeBeamTargetTracking(const Self&);
  void operator=(const Self&);
}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtConeBeamTargetTracking.txx"
#endif

#endif
