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

#ifndef __igtRegisterProjectionsFilter_h
#define __igtRegisterProjectionsFilter_h

#include <itkImageToImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkTileImageFilter.h>
#include <itkImageRegistrationMethod.h>
#include <itkMultiResolutionImageRegistrationMethod.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkMeanSquaresImageToImageMetric.h>
#include <itkMattesMutualInformationImageToImageMetric.h>
#include <itkNormalizedCorrelationImageToImageMetric.h>
#include <itkRegularStepGradientDescentOptimizer.h>
#include <itkGradientDescentOptimizer.h>
#include <itkLBFGSBOptimizer.h>
#include <itkResampleImageFilter.h>
#include <itkRigid2DTransform.h>
#include <itkTranslationTransform.h>
#include <itkBSplineDeformableTransform.h>
#include <itkImageMaskSpatialObject.h>
#include <itkBinaryThresholdImageFilter.h>

namespace igt
{

/** \class igtRegisterProjectionsFilter
 * \brief Register projections frame by frame.
 * \author Andy Shieh
 */
template<class TInputImage, class TOutputImage>
class ITK_EXPORT RegisterProjectionsFilter :
	public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef RegisterProjectionsFilter						  Self;
  typedef ImageToImageFilter<TInputImage, TOutputImage>	  Superclass;
  typedef itk::SmartPointer<Self>                         Pointer;
  typedef itk::SmartPointer<const Self>                   ConstPointer;

  /** Enum for transform types */
  enum TransformOption {Translation, Rigid};
  enum MetricOption {MattesMutualInformation, MeanSquares, NormalizedCorrelation};

  /** Some convenient typedefs. */
  itkStaticConstMacro(ImageDimension, unsigned int, TInputImage::ImageDimension);

  // Typedefs for registration image
  typedef itk::Image< typename TInputImage::PixelType, ImageDimension - 1 >	RegistrationImageType;

  /** Typedefs for registration */
  typedef itk::ExtractImageFilter< TInputImage, RegistrationImageType >						 ExtractFilterType;
  typedef itk::TileImageFilter< RegistrationImageType, TOutputImage >						 TileFilterType;
  typedef itk::ImageRegistrationMethod< RegistrationImageType, RegistrationImageType >		 ImageRegistrationType;
  typedef itk::MultiResolutionImageRegistrationMethod< RegistrationImageType, RegistrationImageType > MultiResRegistrationType;
  typedef itk::LinearInterpolateImageFunction< RegistrationImageType, double >				 LinearInterpolatorType;
  typedef itk::MattesMutualInformationImageToImageMetric< RegistrationImageType, RegistrationImageType>
																							 MMIMetricType;
  typedef itk::MeanSquaresImageToImageMetric< RegistrationImageType, RegistrationImageType>  MSMetricType;
  typedef itk::NormalizedCorrelationImageToImageMetric<RegistrationImageType, RegistrationImageType> NCMetricType;
  typedef itk::Rigid2DTransform< double >									  				 RigidTransformType;
  typedef itk::TranslationTransform< double, ImageDimension - 1>							 TranslationTransformType;
  typedef itk::BSplineBaseTransform<double, ImageDimension - 1>								 BSplineDeformableTransformType;
  typedef itk::ResampleImageFilter< RegistrationImageType, RegistrationImageType >			 ResampleType;

  typedef itk::Image<unsigned char, ImageDimension - 1>	MaskImageType;
  typedef itk::BinaryThresholdImageFilter< MaskImageType, MaskImageType > BinaryThresholdType;
  typedef itk::ImageMaskSpatialObject<ImageDimension - 1> MaskSpatialType;

  /** Standard New method. */
  itkNewMacro(Self)

  /** Runtime information support. */
  itkTypeMacro(RegisterProjectionsFilter, itk::ImageToImageFilter)

  itkSetMacro(TransformOption, TransformOption)
  itkGetMacro(TransformOption, TransformOption)

  itkSetMacro(MetricOption, MetricOption)
  itkGetMacro(MetricOption, MetricOption)

  itkSetMacro(Mask, typename MaskImageType::Pointer)
  itkGetMacro(Mask, typename MaskImageType::Pointer)

  itkSetMacro(Verbose, bool)
  itkGetMacro(Verbose, bool)

  itkSetMacro(UseInverse, bool)
  itkGetMacro(UseInverse, bool)

  itkSetMacro(TransformParameters, std::vector< std::vector<double> >)
  itkGetMacro(TransformParameters, std::vector< std::vector<double> >)

  itkSetMacro(MaxShiftX, double)
  itkGetMacro(MaxShiftX, double)

  itkSetMacro(MaxShiftY, double)
  itkGetMacro(MaxShiftY, double)

  itkSetMacro(MaxRotation, double)
  itkGetMacro(MaxRotation, double)

protected:
 RegisterProjectionsFilter();
 ~RegisterProjectionsFilter(){}

  virtual void GenerateOutputInformation();
  virtual void GenerateData();

  TransformOption m_TransformOption;
  MetricOption m_MetricOption;

  typename MaskImageType::Pointer m_Mask;

  bool m_Verbose, m_UseInverse;

  std::vector< std::vector<double> > m_TransformParameters;

  double m_MaxShiftX, m_MaxShiftY, m_MaxRotation;

private:
  //purposely not implemented
  RegisterProjectionsFilter(const Self&);
  void operator=(const Self&);

}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtRegisterProjectionsFilter.cxx"
#endif

#endif
