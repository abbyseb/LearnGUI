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

#ifndef __igtProjectionAlignmentFilter_h
#define __igtProjectionAlignmentFilter_h

#include "rtkIterativeConeBeamReconstructionFilter.h"
#include "rtkForwardProjectionImageFilter.h"
#include "rtkThreeDCircularProjectionGeometry.h"

#include <itkExtractImageFilter.h>
#include <itkTileImageFilter.h>
#include <itkHistogramMatchingImageFilter.h>
#include "itkImageRegistrationMethod.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkMattesMutualInformationImageToImageMetric.h"
#include "itkNormalizedCorrelationImageToImageMetric.h"
#include "itkRegularStepGradientDescentOptimizer.h"
#include <itkGradientDescentOptimizer.h>
#include <itkLBFGSBOptimizer.h>
#include "itkResampleImageFilter.h"
#include "itkEuler3DTransform.h"
#include "itkRigid2DTransform.h"
#include "itkTranslationTransform.h"
#include "itkMultiplyImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkStatisticsImageFilter.h"
#include <itkLaplacianSharpeningImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkAddImageFilter.h>

#include "igtLeastSquareSubtractionImageFilter.h"

namespace igt
{

/** \class igtProjectionAlignmentFilter
 * \brief Generate DRRs from input 1 (volume) and rigidly
 *		align the DRRs with the acquired projections
 *		(input 0) on a frame by frame basis.
 *		The output is the aligned DRRs.
 *		The difference projections are also available via
 *		GetDifferenceProjections()
 * \author Andy Shieh
 */
template<class TOutputImage>
class ITK_EXPORT ProjectionAlignmentFilter :
	public rtk::IterativeConeBeamReconstructionFilter<TOutputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef ProjectionAlignmentFilter							  Self;
  typedef IterativeConeBeamReconstructionFilter<TOutputImage, TOutputImage> 
															  Superclass;
  typedef itk::SmartPointer<Self>                             Pointer;
  typedef itk::SmartPointer<const Self>                       ConstPointer;

  /** Some convenient typedefs. */
  typedef TOutputImage OutputImageType;
  typedef typename OutputImageType::Pointer         OutputImagePointer;
  typedef typename OutputImageType::PixelType		ImagePixelType;
  typedef typename OutputImageType::Pointer			ImagePointerType;
  typedef typename OutputImageType::RegionType 		ImageRegionType;
  typedef typename OutputImageType::SizeType 		ImageSizeType;
  typedef typename OutputImageType::IndexType 		ImageIndexType;
  typedef typename OutputImageType::SpacingType		ImageSpacingType;
  typedef typename OutputImageType::PointType		ImagePointType;
  typedef typename OutputImageType::DirectionType	ImageDirectionType;

  typedef rtk::ForwardProjectionImageFilter<OutputImageType, OutputImageType>				ForwardProjectionType;
  typedef itk::MultiplyImageFilter<OutputImageType>											ImageMultiplyType;
  typedef itk::SubtractImageFilter<OutputImageType>											ImageSubtractionType;
  typedef rtk::ThreeDCircularProjectionGeometry												ThreeDCircularProjectionGeometry;

  itkStaticConstMacro(ImageDimension, unsigned int, OutputImageType::ImageDimension);

  // Typedefs for registration image
  typedef itk::Image< ImagePixelType, TOutputImage::ImageDimension - 1 >	RegistrationImageType;
  typedef typename RegistrationImageType::Pointer			RegistrationImagePointer;
  typedef typename RegistrationImageType::PixelType			RegistrationImagePixelType;
  typedef typename RegistrationImageType::Pointer			RegistrationImagePointerType;
  typedef typename RegistrationImageType::RegionType 		RegistrationImageRegionType;
  typedef typename RegistrationImageType::SizeType 			RegistrationImageSizeType;
  typedef typename RegistrationImageType::IndexType 		RegistrationImageIndexType;
  typedef typename RegistrationImageType::SpacingType		RegistrationImageSpacingType;
  typedef typename RegistrationImageType::PointType			RegistrationImagePointType;
  typedef typename RegistrationImageType::DirectionType		RegistrationImageDirectionType;

  /** Typedefs for rigid registration */
  typedef itk::ExtractImageFilter< OutputImageType, RegistrationImageType >					 ExtractFilterType;
  typedef itk::TileImageFilter< RegistrationImageType, OutputImageType >					 TileFilterType;
  typedef itk::ImageRegistrationMethod< RegistrationImageType, RegistrationImageType >		 ImageRegistrationType;
  typedef itk::LinearInterpolateImageFunction< RegistrationImageType, double >				 LinearInterpolatorType;
  typedef itk::MattesMutualInformationImageToImageMetric< RegistrationImageType, RegistrationImageType>
																							 MMIMetricType;
  typedef itk::MeanSquaresImageToImageMetric< RegistrationImageType, RegistrationImageType>  MSMetricType;
  typedef itk::NormalizedCorrelationImageToImageMetric<RegistrationImageType, RegistrationImageType> NCMetricType;
  typedef itk::Rigid2DTransform< double >									  				 RigidTransformType;
  typedef itk::TranslationTransform< double, TOutputImage::ImageDimension - 1>				 TranslationTransformType;
  typedef itk::ResampleImageFilter< RegistrationImageType, RegistrationImageType >			 ResampleType;

  // Typedefs for subtractions and additions
  typedef igt::LeastSquareSubtractionImageFilter<RegistrationImageType>						 LeastSquareSubtractionType;
  typedef itk::SubtractImageFilter<RegistrationImageType, RegistrationImageType>			 SubtractImageType;
  typedef itk::HistogramMatchingImageFilter<RegistrationImageType, RegistrationImageType>	 HistogramMatchType;
  typedef itk::AddImageFilter<OutputImageType, OutputImageType>								 AddImageType;

  // Typedefs for image statistics
  typedef itk::StatisticsImageFilter<OutputImageType>										 StatisticsImageFilterType;
  typedef itk::RegionOfInterestImageFilter<OutputImageType, OutputImageType>				 ImageROIType;

  // Typedefs for image sharpening
  typedef itk::LaplacianSharpeningImageFilter<RegistrationImageType, RegistrationImageType>
	  SharpenImageType;

  /** Standard New method. */
  itkNewMacro(Self)

  /** Runtime information support. */
  itkTypeMacro(ProjectionAlignmentFilter, rtk::IterativeConeBeamReconstructionFilter)

  // Set forward projection option
  virtual void SetForwardProjectionFilter(int _arg);

  // Geometry
  itkGetMacro(Geometry, ThreeDCircularProjectionGeometry::Pointer);
  itkSetMacro(Geometry, ThreeDCircularProjectionGeometry::Pointer);

  // Difference Projection
  itkGetMacro(DifferenceProjection, ImagePointerType);

  // Input volume
  itkGetMacro(InputVolume, ImagePointerType);
  itkSetMacro(InputVolume, ImagePointerType);

  // The auxillary input volume used for alignment
  itkGetMacro(AuxInputVolume, ImagePointerType);
  itkSetMacro(AuxInputVolume, ImagePointerType);

  // Get / Set rigid registration constraints
  itkGetMacro(MaxShift, RegistrationImageSpacingType);
  itkSetMacro(MaxShift, RegistrationImageSpacingType);
  itkGetMacro(MaxRotateAngle, double);
  itkSetMacro(MaxRotateAngle, double);
  itkGetMacro(IndependentShiftCap, RegistrationImageSpacingType);
  itkSetMacro(IndependentShiftCap, RegistrationImageSpacingType);

  // Get / Set the boolean of whether or not to sharpen DRR
  itkGetMacro(SharpenDRR, bool);
  itkSetMacro(SharpenDRR, bool);

  // Get / Set parameters for the optimizer
  itkGetMacro(NumberOfIterations, unsigned int);
  itkSetMacro(NumberOfIterations, unsigned int);

  // Get the final transform parameters
  itkGetMacro(TransformParameters, typename TranslationTransformType::ParametersType);
  
  // Get / Set parameters for user-specified registration region
  itkGetMacro(RegistrationRegion, RegistrationImageRegionType);
  itkSetMacro(RegistrationRegion, RegistrationImageRegionType);
  itkGetMacro(IsRegistrationRegionUsed, bool);
  itkSetMacro(IsRegistrationRegionUsed, bool);

  itkGetMacro(MatchScore, double);

  itkGetMacro(MatchMetricOption, unsigned int);
  itkSetMacro(MatchMetricOption, unsigned int);

  itkGetMacro(MatchThreshold, double);
  itkSetMacro(MatchThreshold, double);

protected:
  ProjectionAlignmentFilter();
  ~ProjectionAlignmentFilter(){}

  virtual void VerifyInputInformation(){};

  virtual void GenerateData();

  // Members

  ThreeDCircularProjectionGeometry::Pointer m_Geometry;

  ImagePointerType m_DifferenceProjection, m_InputVolume, m_AuxInputVolume;

  // Constraints on rigid registration
  RegistrationImageSpacingType m_MaxShift;
  RegistrationImageSpacingType m_IndependentShiftCap;
  double m_MaxRotateAngle;

  typename TranslationTransformType::ParametersType m_TransformParameters;

  // Whether or not to sharpen DRR
  bool m_SharpenDRR;

  // Parameters for the optimizer
  unsigned int m_NumberOfIterations;

  // User-specified region for registration
  RegistrationImageRegionType m_RegistrationRegion;
  bool m_IsRegistrationRegionUsed;

  double m_MatchScore;

  double m_MatchThreshold;

  //0: MS
  //1: MMI
  //2: NC
  unsigned int m_MatchMetricOption;

private:
  //purposely not implemented
  ProjectionAlignmentFilter(const Self&);
  void operator=(const Self&);

}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtProjectionAlignmentFilter.cxx"
#endif

#endif
