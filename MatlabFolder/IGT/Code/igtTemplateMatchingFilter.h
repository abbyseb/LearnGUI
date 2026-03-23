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

#ifndef __igtTemplateMatchingFilter_h
#define __igtTemplateMatchingFilter_h

#include <itkImageToImageFilter.h>

#include "itkLinearInterpolateImageFunction.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkMattesMutualInformationImageToImageMetric.h"
#include "itkNormalizedCorrelationImageToImageMetric.h"
#include "itkImageToImageMetric.h"
#include "itkTranslationTransform.h"

namespace igt
{

/** \class igtTemplateMatchingFilter
 * \brief Perform template matching using an exhaustive search method.
 *		  Input 0: The template image.
 *		  Input 1: The image on which the template will be matched.
 * \author Andy Shieh
 */
template<class TOutputImage>
class ITK_EXPORT TemplateMatchingFilter :
	public itk::ImageToImageFilter<TOutputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef TemplateMatchingFilter							  Self;
  typedef ImageToImageFilter<TOutputImage, TOutputImage>	  Superclass;
  typedef itk::SmartPointer<Self>                             Pointer;
  typedef itk::SmartPointer<const Self>                       ConstPointer;

  /** Some convenient typedefs. */
  typedef TOutputImage ImageType;
  typedef typename ImageType::Pointer       ImagePointerType;
  typedef typename ImageType::PixelType		ImagePixelType;
  typedef typename ImageType::Pointer		ImagePointerType;
  typedef typename ImageType::RegionType 	ImageRegionType;
  typedef typename ImageType::SizeType 		ImageSizeType;
  typedef typename ImageType::IndexType 	ImageIndexType;
  typedef typename ImageType::SpacingType	ImageSpacingType;
  typedef typename ImageType::PointType		ImagePointType;
  typedef typename ImageType::DirectionType	ImageDirectionType;

  /** Typedefs for rigid registration */
  typedef itk::LinearInterpolateImageFunction< ImageType, double >				 InterpolatorType;
  typedef itk::MattesMutualInformationImageToImageMetric< ImageType, ImageType>	 MMIMetricType;
  typedef itk::MeanSquaresImageToImageMetric< ImageType, ImageType>				 MSMetricType;
  typedef itk::NormalizedCorrelationImageToImageMetric< ImageType, ImageType>	 NCCMetricType;
  typedef itk::ImageToImageMetric<ImageType, ImageType>							 MetricBaseType;
  typedef itk::TranslationTransform< double, ImageType::ImageDimension >		 TransformType;		
  typedef typename TransformType::ParametersType								 TransformParametersType;

  /** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(TemplateMatchingFilter, itk::ImageToImageFilter);

  itkGetMacro(SearchResolutions, std::vector<ImageSpacingType>);
  itkSetMacro(SearchResolutions, std::vector<ImageSpacingType>);

  itkGetMacro(SearchRadii, std::vector<ImageSpacingType>);
  itkSetMacro(SearchRadii, std::vector<ImageSpacingType>);

  // Metric. 0: MSE, 1: MMI, 2: NCC
  itkGetMacro(MetricOption, unsigned int);
  itkSetMacro(MetricOption, unsigned int);

  itkGetMacro(TransformParameters, TransformParametersType);

  itkGetMacro(FinalMetricValue, double);

  // If set to true, metric will be maximized instead of minimized.
  // Default behaviour is to minimize the metric.
  itkSetMacro(Maximize, bool);
  itkGetMacro(Maximize, bool);

protected:
  TemplateMatchingFilter();
  ~TemplateMatchingFilter(){}

  virtual void VerifyInputInformation();

  virtual void GenerateData();

  // Members

  unsigned int m_MetricOption;

  // Search resolutions and radii in mm
  std::vector<ImageSpacingType> m_SearchResolutions;
  std::vector<ImageSpacingType> m_SearchRadii;

  TransformParametersType m_TransformParameters;

  double m_FinalMetricValue;

  bool m_Maximize;

private:
  //purposely not implemented
  TemplateMatchingFilter(const Self&);
  void operator=(const Self&);

}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtTemplateMatchingFilter.cxx"
#endif

#endif
