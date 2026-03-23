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

#ifndef __rtkPartialDerivativeImageFilter_h
#define __rtkPartialDerivativeImageFilter_h

#include <itkImageToImageFilter.h>
#include <itkCastImageFilter.h>

namespace rtk
{
/** \class PartialDerivativeImageFilter
 * \brief Computes the gradient of total variation of an image
 * \author Andy Shieh
 *
 * \ingroup IntensityImageFilters
 */

template <typename TInputImage, typename TOutputImage = TInputImage >
class PartialDerivativeImageFilter :
        public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:
    /** Extract dimension from input and output image. */
    itkStaticConstMacro(InputImageDimension, unsigned int,
                        TInputImage::ImageDimension);

    /** Convenient typedefs for simplifying declarations. */
    typedef TInputImage InputImageType;
	typedef TOutputImage OutputImageType;

    /** Standard class typedefs. */
    typedef PartialDerivativeImageFilter                	   	   Self;
    typedef itk::ImageToImageFilter< InputImageType, TOutputImage> Superclass;
    typedef itk::SmartPointer<Self>                                Pointer;
    typedef itk::SmartPointer<const Self>                          ConstPointer;

    /** Image typedef support. */
    typedef typename InputImageType::PixelType  InputPixelType;
    typedef typename InputImageType::RegionType InputImageRegionType;
    typedef typename InputImageType::SizeType   InputSizeType;
	
    /** Method for creation through the object factory. */
    itkNewMacro(Self)

    /** Run-time type information (and related methods). */
    itkTypeMacro(PartialDerivativeImageFilter, ImageToImageFilter)

    /** Use the image spacing information in calculations. Use this option if you
     *  want derivatives in physical space. Default is UseImageSpacingOn. */
    void SetUseImageSpacingOn()
    { this->SetUseImageSpacing(true); }

    /** Ignore the image spacing. Use this option if you want derivatives in
        isotropic pixel space.  Default is UseImageSpacingOn. */
    void SetUseImageSpacingOff()
    { this->SetUseImageSpacing(false); }

    /** Set/Get whether or not the filter will use the spacing of the input
        image in its calculations */
    itkSetMacro(UseImageSpacing, bool);
    itkGetConstMacro(UseImageSpacing, bool);
	
	/** Set/Get derivative option (default = 0, i.e. f_x) */
	itkSetMacro(DerivativeOption, unsigned int);
	itkGetConstMacro(DerivativeOption, unsigned int);

protected:
    PartialDerivativeImageFilter();
    virtual ~PartialDerivativeImageFilter();

    virtual void GenerateInputRequestedRegion();

    virtual void BeforeThreadedGenerateData();

    virtual void ThreadedGenerateData(const typename InputImageType::RegionType& outputRegionForThread, typename itk::ThreadIdType itkNotUsed(threadId));
	
private:
    PartialDerivativeImageFilter(const Self&); //purposely not implemented
    void operator=(const Self&); //purposely not implemented

    bool                              m_UseImageSpacing;
    typename TInputImage::SpacingType m_SpacingCoeffs;
	
	// Derivative option, default = 0
	// 0: fx, 1: fy, 2: fz, 3: fxx, 4: fyy, 5: fzz
	// 6: fxy, 7: fxz, 8: fyz
	unsigned int m_DerivativeOption;
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "rtkPartialDerivativeImageFilter.txx"
#endif

#endif //__rtkPartialDerivativeImageFilter__
