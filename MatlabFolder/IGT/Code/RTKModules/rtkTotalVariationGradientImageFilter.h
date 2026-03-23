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

#ifndef __rtkTotalVariationGradientImageFilter_h
#define __rtkTotalVariationGradientImageFilter_h

#include <itkImageToImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkMultiplyImageFilter.h>
#include "itkVectorImage.h"

namespace rtk
{
/** \class TotalVariationGradientImageFilter
 * \brief Computes the gradient of total variation of an image
 * \author Andy Shieh
 *
 * \ingroup IntensityImageFilters
 */

template <typename TInputImage, typename TOutputImage = TInputImage >
class TotalVariationGradientImageFilter :
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
    typedef TotalVariationGradientImageFilter                	   Self;
    typedef itk::ImageToImageFilter< InputImageType, TOutputImage> Superclass;
    typedef itk::SmartPointer<Self>                                Pointer;
    typedef itk::SmartPointer<const Self>                          ConstPointer;

    /** Image typedef support. */
    typedef typename InputImageType::PixelType  InputPixelType;
    typedef typename InputImageType::RegionType InputImageRegionType;
    typedef typename InputImageType::SizeType   InputSizeType;
	
    /** Vector image type for W */
    typedef itk::VectorImage< InputPixelType, InputImageDimension > VectorImageType;
	typedef typename VectorImageType::Pointer						VectorImagePointer;
	
	/** Convenient typedefs. */
	typedef itk::MultiplyImageFilter< InputImageType, InputImageType , TOutputImage >	MultiplyFilterType;
	
    /** Method for creation through the object factory. */
    itkNewMacro(Self)

    /** Run-time type information (and related methods). */
    itkTypeMacro(TotalVariationGradientImageFilter, ImageToImageFilter)

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
	
	/** Set/Get the weighting image - Andy */
	itkSetMacro(WeightingImage, VectorImagePointer);
	itkGetConstMacro(WeightingImage, VectorImagePointer);
	
    /** Set/Get scaling factor to prevent loss in precision */	
	virtual void SetScalingFactor(float scalingFactor)
	{ m_ScaleInputFilter->SetConstant( scalingFactor ); }
	virtual float GetScalingFactor()
	{ return m_ScaleInputFilter->GetConstant(); }

    /** Set along which dimensions the gradient computation should be
        performed. The vector components at unprocessed dimensions are ignored */
    void SetDimensionsProcessed(bool* DimensionsProcessed);

protected:
    TotalVariationGradientImageFilter();
    virtual ~TotalVariationGradientImageFilter();

    virtual void GenerateInputRequestedRegion();

    virtual void BeforeThreadedGenerateData();

    virtual void ThreadedGenerateData(const typename InputImageType::RegionType& outputRegionForThread, typename itk::ThreadIdType itkNotUsed(threadId));
	
private:
    TotalVariationGradientImageFilter(const Self&); //purposely not implemented
    void operator=(const Self&); //purposely not implemented

    bool                              m_UseImageSpacing;
    typename TInputImage::SpacingType m_SpacingCoeffs;
	
	typename MultiplyFilterType::Pointer		  m_ScaleInputFilter;
	
    // The adaptive weighting vector image W - Andy
    VectorImagePointer 		m_WeightingImage; 

    // list of the dimensions along which the divergence has
    // to be computed. The components on other dimensions
    // are ignored for performance, but the gradient filter
    // sets them to zero anyway
    bool m_DimensionsProcessed[TInputImage::ImageDimension];
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "rtkTotalVariationGradientImageFilter.txx"
#endif

#endif //__rtkTotalVariationGradientImageFilter__
