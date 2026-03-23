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

#ifndef __igtLeastSquareSubtractionImageFilter_h
#define __igtLeastSquareSubtractionImageFilter_h

#include <itkImageToImageFilter.h>
#include <itkMultiplyImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkAddImageFilter.h>
#include <itkStatisticsImageFilter.h>

namespace igt
{

/** \class igtLeastSquareSubtractionImageFilter
 * \brief Subtract image 2 from image 1 in the least square sense.
 *		  Image 1 is input(0) and image 2 is input(1).
 *		i.e., image 2 is first scaled by ax + b before the subtraction
 *			  such that norm(image1 - image2) is minimum.
 *	      The solutions of a and b to minimise norm( y - (ax+b) ) is, 
 *		  a = [ |Ones|^2 * y^T*x - (y^T*Ones)*(x^T*Ones) ] / [ |x|^2 * |ones|^2 - (x^T*Ones)^2 ]
 *		  b = [ |x|^2 * y^T*Ones - (x^T*Ones) * (y^T)*x] / [ |x|^2*|Ones|^2 - (x^T*Ones)^2 ]
 *		  with y being input(0) and x being input(1), Ones being the one vector with the same size of x and y
 *
 *
 * \author Andy Shieh
 */
template<class TOutputImage>
class ITK_EXPORT LeastSquareSubtractionImageFilter :
  public itk::ImageToImageFilter<TOutputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef LeastSquareSubtractionImageFilter					  Self;
  typedef itk::ImageToImageFilter<TOutputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>                             Pointer;
  typedef itk::SmartPointer<const Self>                       ConstPointer;

  /** Convenient typedefs */
  typedef TOutputImage							OutputImageType;
  typedef typename OutputImageType::SizeType	ImageSizeType;
  typedef typename OutputImageType::Pointer		ImagePointerType;

  /** Typedefs of each subfilter of this composite filter */
  typedef itk::MultiplyImageFilter<OutputImageType>			  MultiplyImageFilterType;
  typedef itk::SubtractImageFilter<OutputImageType>			  SubtractImageFilterType;
  typedef itk::AddImageFilter<OutputImageType>				  AddImageFilterType;
  typedef itk::StatisticsImageFilter<OutputImageType>		  StatisticsImageFilterType;

  /** Standard New method. */
  itkNewMacro(Self)

  /** Runtime information support. */
  itkTypeMacro(LeastSquareSubtractionImageFilter, itk::ImageToImageFilter)

  // Get a and b
  itkGetMacro(a, double);
  itkGetMacro(b, double);

  // Get scaled image
  itkGetMacro(ScaledImage, ImagePointerType);

protected:
  LeastSquareSubtractionImageFilter();
  ~LeastSquareSubtractionImageFilter(){}

  virtual void VerifyInputInformation();

  virtual void GenerateData();

  // Members
  
  // Scaling factor a and b
  double m_a, m_b;

  ImagePointerType m_ScaledImage;

private:
  //purposely not implemented
  LeastSquareSubtractionImageFilter(const Self&);
  void operator=(const Self&);

}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtLeastSquareSubtractionImageFilter.cxx"
#endif

#endif
