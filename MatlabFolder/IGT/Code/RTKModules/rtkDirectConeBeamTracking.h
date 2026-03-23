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

#ifndef __rtkDirectConeBeamTracking_h
#define __rtkDirectConeBeamTracking_h

#include "rtkBackProjectionImageFilter.h"
#include "rtkForwardProjectionImageFilter.h"

#include <itkExtractImageFilter.h>
#include <itkTileImageFilter.h>
#include <itkMultiplyImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkAddImageFilter.h>
#include <itkHistogramMatchingImageFilter.h>
#include <itkTimeProbe.h>
#include <itkStatisticsImageFilter.h>
#include <itkMatrix.h>
#include <itkVector.h>

#include "itkImageRegistrationMethod.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkMattesMutualInformationImageToImageMetric.h"
#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkResampleImageFilter.h"
#include "itkEuler3DTransform.h"
#include "itkRigid2DTransform.h"
#include "itkTranslationTransform.h"

#include "rtkConstantImageSource.h"
#include "rtkIterativeConeBeamReconstructionFilter.h"

namespace rtk
{

/** \class DirectConeBeamTracking
 * \brief Direct tumor tracking method by minimizing the objective function
 *   f_obj = |Rf-p|^2 + (r-r0)dot(u)
 *
 * \author Andy Shieh 2015
 *
 * \ingroup ReconstructionAlgorithm
 */
template<class TInputImage, class TOutputImage=TInputImage>
class ITK_EXPORT DirectConeBeamTracking :
  public rtk::IterativeConeBeamReconstructionFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef DirectConeBeamTracking                               			   Self;
  typedef IterativeConeBeamReconstructionFilter<TInputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>                                          Pointer;
  typedef itk::SmartPointer<const Self>                                    ConstPointer;

  /** Some convenient typedefs. */
  typedef TInputImage  InputImageType;
  typedef TOutputImage OutputImageType;
  typedef typename InputImageType::Pointer          InputImagePointer;
  typedef typename OutputImageType::PixelType		ImagePixelType;
  typedef typename OutputImageType::Pointer			ImagePointerType;
  typedef typename OutputImageType::RegionType 		ImageRegionType;
  typedef typename OutputImageType::SizeType 		ImageSizeType;
  typedef typename OutputImageType::IndexType 		ImageIndexType;
  typedef typename OutputImageType::SpacingType		ImageSpacingType;
  typedef typename OutputImageType::PointType		ImagePointType;
  typedef typename OutputImageType::DirectionType	ImageDirectionType;
  
  // Typedefs for registration image
  typedef itk::Image< ImagePixelType, TInputImage::ImageDimension-1 >	RegistrationImageType;

  /** Typedefs of each subfilter of this composite filter */
  typedef itk::StatisticsImageFilter< OutputImageType >										 ImageStatisticsFilterType;
  typedef rtk::ForwardProjectionImageFilter< OutputImageType, OutputImageType >              ForwardProjectionFilterType;
  typedef itk::SubtractImageFilter< OutputImageType, OutputImageType >                       SubtractFilterType;
  typedef itk::MultiplyImageFilter< OutputImageType, OutputImageType, OutputImageType >      MultiplyFilterType;
  typedef itk::ExtractImageFilter< OutputImageType, RegistrationImageType >					 ExtractFilterType;
  typedef itk::TileImageFilter< RegistrationImageType, OutputImageType >					 TileFilterType;
  typedef itk::HistogramMatchingImageFilter< RegistrationImageType, RegistrationImageType >	 HistogramMatchType;
  
  /** Typedefs for rigid registration */
  typedef itk::ImageRegistrationMethod< RegistrationImageType, RegistrationImageType >		 ImageRegistrationType;
  typedef itk::LinearInterpolateImageFunction< RegistrationImageType, double >				 LinearInterpolatorType;
  typedef itk::MattesMutualInformationImageToImageMetric< RegistrationImageType, RegistrationImageType>  
																							 MattesMIMetricType;
  typedef itk::MeanSquaresImageToImageMetric< RegistrationImageType, RegistrationImageType>  MSMetricType;
  typedef itk::RegularStepGradientDescentOptimizer											 OptimizerType;
  typedef itk::Rigid2DTransform< double >									  				 RigidTransformType;
  typedef itk::ResampleImageFilter< RegistrationImageType, RegistrationImageType >			 ResampleType;
  
  /* Typedefs for matrix operation */
  typedef itk::Matrix<double, 3, 3>			MatrixType;
  typedef itk::Vector<double, 3>			VectorType;  

/** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(DirectConeBeamTracking, IterativeConeBeamReconstructionFilter);
  
  /** Get / Set the object pointer to projection geometry */
  itkGetMacro(Geometry, ThreeDCircularProjectionGeometry::Pointer);
  itkSetMacro(Geometry, ThreeDCircularProjectionGeometry::Pointer);

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

  /** Get / Set regularization parameter Lambda. Default is 1. */
  itkGetMacro(LambdaInDepth, double);
  itkSetMacro(LambdaInDepth, double);
  itkGetMacro(LambdaInPlane, double);
  itkSetMacro(LambdaInPlane, double);
  itkGetMacro(LambdaSI, double);
  itkSetMacro(LambdaSI, double);
  
  // Get / Set regularization order, gamma. Default is 2
  itkGetMacro(GammaInDepth, int);
  itkSetMacro(GammaInDepth, int);
  itkGetMacro(GammaInPlane, int);
  itkSetMacro(GammaInPlane, int);
  itkGetMacro(GammaSI, int);
  itkSetMacro(GammaSI, int);
  
  // Get / Set the optimizer
  itkGetMacro(OptimizerConfiguration, int);
  itkSetMacro(OptimizerConfiguration, int);
  
  /** Get / Set C1, C2. Default is C1 = 1e-4, C2 = 0.5 */
  itkGetMacro(C1, double);
  itkSetMacro(C1, double);
  itkGetMacro(C2, double);
  itkSetMacro(C2, double);
  
  /** Get / Set the constrain angle */
  itkGetMacro(ConstrainAngle, double);
  itkSetMacro(ConstrainAngle, double);
  
  // Get / Set the maximum overall displacement
  itkGetMacro(MaxDisplacement, double);
  itkSetMacro(MaxDisplacement, double);
  
  /** Get / Set residual and objective function vectors */
  itkGetMacro(ResidualVector, std::vector<double>);
  itkGetMacro(ObjectiveVector, std::vector<double>);
  
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
  // Get final objective value
  double GetFinalObjective()
    {
	if( m_ObjectiveVector.size() > 0)
	  {
	  std::vector<double>::reverse_iterator rit = m_ObjectiveVector.rbegin();
	  return *rit;
	  }
	else
	  return 0;
    }
  
  /** Get / Set stopping criterion */
  itkGetMacro(StoppingCriterion, double);
  itkSetMacro(StoppingCriterion, double);
  
  /** Get / Set numbers of histogram match points and levels */
  itkGetMacro(NumberOfMatchPoints, unsigned int);
  itkSetMacro(NumberOfMatchPoints, unsigned int);
  itkGetMacro(NumberOfHistogramLevels, unsigned int);
  itkSetMacro(NumberOfHistogramLevels, unsigned int);
  
  // Get / Set rigid registration constraints
  itkGetMacro(MaxRRShiftX, double);
  itkSetMacro(MaxRRShiftX, double);
  itkGetMacro(MaxRRShiftY, double);
  itkSetMacro(MaxRRShiftY, double);
  itkGetMacro(MaxRRRotateAngle, double);
  itkSetMacro(MaxRRRotateAngle, double);
  
  // Get / Set rigid registration parameters
  itkGetMacro(RigidTransformParameters, RigidTransformType::ParametersType);
  itkSetMacro(RigidTransformParameters, RigidTransformType::ParametersType);
  
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
  
  // Get/Set p
  itkGetMacro(p, ImagePointerType);
  itkSetMacro(p, ImagePointerType);
  
  /** Get/ Set the prior image */
  itkGetMacro(PriorImage, ImagePointerType);
  itkSetMacro(PriorImage, ImagePointerType);
  
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

  // Get / Set prior origin
  itkSetMacro(PriorOrigin, VectorType);
  itkGetMacro(PriorOrigin, VectorType);
  
  /** Select the ForwardProjection filter */
  void SetForwardProjectionFilter (int _arg);
  
  /** Set global maximum number of threads for multi thread methods, to prevent crashes sometime */
  void SetMaxNumberOfThreads( int _arg) { itk::MultiThreader::SetGlobalMaximumNumberOfThreads( _arg ); }

protected:
  DirectConeBeamTracking();
  ~DirectConeBeamTracking(){}

  virtual void GenerateInputRequestedRegion();

  virtual void GenerateOutputInformation();

  virtual void GenerateData();

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
  
  // Get in-depth constraints
  // In-depth directional vector is (sine theta, 0, cosine theta)
  double GetInDepthConstraintObjective(VectorType r, VectorType r0, double angle, int gamma)
    {
	double result = -sin(angle) * (r[0] - r0[0]) + cos(angle) * (r[2] - r0[2]);
	result = pow(result, gamma);
	return result;
    }
  VectorType GetInDepthConstraintGradient(VectorType r, VectorType r0, double angle, int gamma)
    {
	VectorType result;
	result.Fill(0);
	if(gamma >= 1 || gamma < 0)
	  {
	  double factor = pow( -sin(angle) * (r[0] - r0[0]) + cos(angle) * (r[2] - r0[2]), gamma - 1);
	  factor *= gamma;
	  result[0] = -factor * sin(angle);
	  result[2] = factor * cos(angle);
	  }
	return result;
    }
  MatrixType GetInDepthConstraintHessian(VectorType r, VectorType r0, double angle, double lambda, int gamma)
    {
	MatrixType result;
	result.Fill(0);
	if(gamma >= 2 || gamma < 0)
	  {
	  double factor = pow( -sin(angle) * (r[0] - r0[0]) + cos(angle) * (r[2] - r0[2]), gamma - 2);
	  result(0,0) = gamma * (gamma - 1) * sin(angle) * sin(angle) * factor * lambda;
	  result(2,2) = gamma * (gamma - 1) * cos(angle) * cos(angle) * factor * lambda;
	  result(0,2) = -gamma * (gamma - 1) * sin(angle) * cos(angle) * factor * lambda;
	  result(2,0) = result(0,2);
	  }
	return result;
    }

  // Get in-plane constraints
  double GetInPlaneConstraintObjective(VectorType r, VectorType r0, double angle, int gamma)
    {
	double result = cos(angle) * (r[0] - r0[0]) + sin(angle) * (r[2] - r0[2]);
	result = pow(result, gamma);
	return result;
    }
  VectorType GetInPlaneConstraintGradient(VectorType r, VectorType r0, double angle, int gamma)
    {
	VectorType result;
	result.Fill(0);
	if(gamma >= 1 || gamma < 0)
	  {
	  double factor = pow( cos(angle) * (r[0] - r0[0]) + sin(angle) * (r[2] - r0[2]), gamma - 1);
	  factor *= gamma;
	  result[0] = factor * cos(angle);
	  result[2] = factor * sin(angle);
	  }
	return result;
    }
  MatrixType GetInPlaneConstraintHessian(VectorType r, VectorType r0, double angle, double lambda, int gamma)
    {
	MatrixType result;
	result.Fill(0);
	if(gamma >= 2 || gamma < 0)
	  {
	  double factor = pow( cos(angle) * (r[0] - r0[0]) + sin(angle) * (r[2] - r0[2]), gamma - 2);
	  result(0,0) = gamma * (gamma - 1) * cos(angle) * cos(angle) * factor * lambda;
	  result(2,2) = gamma * (gamma - 1) * sin(angle) * sin(angle) * factor * lambda;
	  result(0,2) = gamma * (gamma - 1) * sin(angle) * cos(angle) * factor * lambda;
	  result(2,0) = result(0,2);
	  }
	return result;
    }
	
  // Get constraints along a specific dimension, usually SI, i.e. dim = 1
  double GetSingleDimensionConstraintObjective(VectorType r, VectorType r0, unsigned int dim, int gamma)
    {
	double result = r[dim] - r0[dim];
	result = pow(result, gamma);
	return result;
    }
  VectorType GetSingleDimensionConstraintGradient(VectorType r, VectorType r0, unsigned int dim, int gamma)
    {
	VectorType result;
	result.Fill(0);
	if(gamma >= 1 || gamma < 0)
	  result[dim] = gamma * pow(r[dim] - r0[dim],gamma - 1);
	return result;
    }
  MatrixType GetSingleDimensionConstraintHessian(VectorType r, VectorType r0, unsigned int dim, double lambda, int gamma)
    {
	MatrixType result;
	result.Fill(0);
	if(gamma >= 2 || gamma < 0)
	  result(1,1) = gamma * (gamma - 1) * lambda * pow(r[dim] - r0[dim],gamma - 2);
	return result;
    }
	
  /** Pointers to each subfilter */
  typename ForwardProjectionFilterType::Pointer	m_Rf, m_Rfx, m_Rfy, m_Rfz,
												m_Rfxx, m_Rfyy, m_Rfzz,
												m_Rfxy, m_Rfxz, m_Rfyz;
  typename SubtractFilterType::Pointer			m_s;
  typename MultiplyFilterType::Pointer			m_ZeroP,
												m_sRfx, m_sRfy, m_sRfz,
												m_RfxRfy, m_RfxRfz, m_RfyRfz,
												m_sRfxx, m_sRfyy, m_sRfzz,
												m_sRfxy, m_sRfxz, m_sRfyz;
  typename ImageStatisticsFilterType::Pointer	m_IS_s, m_IS_Rfx, m_IS_Rfy, m_IS_Rfz,
												m_IS_sRfx, m_IS_sRfy, m_IS_sRfz,
												m_IS_RfxRfy, m_IS_RfxRfz, m_IS_RfyRfz,
												m_IS_sRfxx, m_IS_sRfyy, m_IS_sRfzz,
												m_IS_sRfxy, m_IS_sRfxz, m_IS_sRfyz;
										
  
  /** Geometry object */
  ThreeDCircularProjectionGeometry::Pointer m_Geometry;

  /** Number of iterations */
  unsigned int m_NumberOfIterations;
  
  /** Stop iterations if change in x is smaller this value. Default = 0.01 mm */
  double m_StoppingCriterion;

  /** Regularization parameter Lambda */
  double m_LambdaInDepth, m_LambdaInPlane, m_LambdaSI;
  
  // The order of the regularization term Gamma
  int m_GammaInDepth, m_GammaInPlane, m_GammaSI;
  
  // Optimizer configuration
  int m_OptimizerConfiguration;
  
  /** c1 and c2 for line searching */
  double m_C1;
  double m_C2;
  
  /** The constrain angle */
  double m_ConstrainAngle;
  
  // Maximum step length of CG
  double m_MaxDisplacement;
  
  /** Verbose */
  bool m_Verbose;
  
  // Number of match points and histogram levels for histogram match
  unsigned int m_NumberOfMatchPoints;
  unsigned int m_NumberOfHistogramLevels;
  
  // Constraints on rigid registration
  double m_MaxRRShiftX, m_MaxRRShiftY;
  double m_MaxRRRotateAngle;
  
  // Rigid registation parameters
  RigidTransformType::ParametersType	m_RigidTransformParameters;
  
  // Vectors recording the residual and objective function and position vector in each iteration
  std::vector< double > m_ResidualVector;
  std::vector< double > m_ObjectiveVector;
  std::vector< VectorType > m_TargetOriginVector;
  
  // The processed projections p
  ImagePointerType  m_p;
  
  /** The image f and its derivatives */
  ImagePointerType	m_f, m_fx, m_fy, m_fz, m_fxx, m_fyy, m_fzz, m_fxy, m_fxz, m_fyz;
  
  /** The tumor-removed prior image */
  ImagePointerType m_PriorImage;
  
  // Output from m_Rf
  ImagePointerType m_RfImage;
  
  /** The current displacement value and a prior displacement value */
  VectorType	m_TargetOrigin, m_PriorOrigin;

  /** Time probes */
  itk::TimeProbe m_ForwardProjectionProbe;
  itk::TimeProbe m_ImageArithmeticProbe;
  itk::TimeProbe m_ImageStatisticsProbe;
  itk::TimeProbe m_ImageRegistrationProbe;

private:
  //purposely not implemented
  DirectConeBeamTracking(const Self&);
  void operator=(const Self&);
}; // end of class

} // end namespace rtk

#ifndef ITK_MANUAL_INSTANTIATION
#include "rtkDirectConeBeamTracking.txx"
#endif

#endif
