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

#ifndef __igtITKToQImage_h
#define __igtITKToQImage_h

#include <itkProcessObject.h>

#include "igtConfiguration.h"

#include <QImage>
 
namespace igt
{

/** \class igtITKToQImage
 * \brief  Class with static methods to convert ITK image to QImage.
 *		   Expect 2D ITK image input.
 *
 *
 * \author Andy Shieh
 */
 
template<class TInputImage>
class ITK_EXPORT ITKToQImage:
	public itk::ProcessObject
{
public:
    /** Standard class typedefs. */
    typedef ITKToQImage                       	Self;
	typedef itk::ProcessObject					Superclass;
    typedef itk::SmartPointer<Self>				Pointer;
    typedef itk::SmartPointer<const Self>		ConstPointer;
	
	typedef TInputImage							ITKImageType;
	typedef typename ITKImageType::IndexType	ITKImageIndexType;
	typedef typename ITKImageType::ConstPointer	ITKImageConstPointerType;
	typedef typename ITKImageType::PixelType	ITKImagePixelType;
	typedef typename ITKImageType::RegionType   ITKImageRegionType;

	/** Method for creation through the object factory. */
	itkNewMacro(Self);

	/** Run-time type information (and related methods). */
	itkTypeMacro(ITKToQImage, ProcessObject);

	static void GenerateMonoScaleQImage(ITKImageConstPointerType itkImagePointer, QImage *qImagePointer, ITKImageRegionType region, ITKImagePixelType itkMin, ITKImagePixelType itkMax);
	
	static void GenerateBinaryMaskQImage(ITKImageConstPointerType itkImagePointer, QImage *qImagePointer, ITKImageRegionType region, ITKImagePixelType threshold, bool inverseMasking);

protected:
  ITKToQImage(){};
  virtual ~ITKToQImage() {};

private:
  ITKToQImage(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

}; // end of class

} // end namespace igt
#ifndef ITK_MANUAL_INSTANTIATION
#include "igtITKToQImage.cxx"
#endif

#endif
