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

#ifndef __rtkTotalVariationHessianOperatorImageFilter_h
#define __rtkTotalVariationHessianOperatorImageFilter_h

#include <itkImageToImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkMultiplyImageFilter.h>
#include "itkVectorImage.h"

namespace rtk
{
/** \class TotalVariationHessianOperatorImageFilter
 * \brief Total variation Hessian Operator.
 *		  Support only 3D images for now, and ignoring dimToProcess.
 * \author Andy Shieh
 *
 * \ingroup IntensityImageFilters
 */

template <typename TInputImage, typename TOutputImage = TInputImage >
class TotalVariationHessianOperatorImageFilter :
        public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:
    /** Convenient typedefs for simplifying declarations. */
    typedef TInputImage 	InputImageType;
	typedef TOutputImage	OutputImageType;
	
    /** Image typedef support. */
    typedef typename InputImageType::PixelType  InputPixelType;
    typedef typename InputImageType::RegionType InputImageRegionType;
    typedef typename InputImageType::SizeType   InputSizeType;
	
    /** Extract dimension from input and output image. */
    itkStaticConstMacro(InputImageDimension, unsigned int,
                        InputImageType::ImageDimension);

    /** Standard class typedefs. */
    typedef TotalVariationHessianOperatorImageFilter               	  Self;
    typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
    typedef itk::SmartPointer<Self>                                   Pointer;
    typedef itk::SmartPointer<const Self>                             ConstPointer;

    /** Vector image type for W */
    typedef itk::VectorImage< InputPixelType, InputImageDimension > VectorImageType;
	typedef typename VectorImageType::Pointer						VectorImagePointer;
	
	/** Convenient typedefs. */
	typedef itk::MultiplyImageFilter< InputImageType, InputImageType , TOutputImage >	MultiplyFilterType;
	
    /** Method for creation through the object factory. */
    itkNewMacro(Self)

    /** Run-time type information (and related methods). */
    itkTypeMacro(TotalVariationHessianOperatorImageFilter, ImageToImageFilter)

	/** Set/Get the weighting image - Andy */
	itkSetMacro(WeightingImage, VectorImagePointer);
	itkGetConstMacro(WeightingImage, VectorImagePointer);
	
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

    /** Set/Get scaling factor to prevent loss in precision */	
	virtual void SetScalingFactor(float scalingFactor)
	{ m_ScaleInputFilter->SetConstant( scalingFactor ); m_ScaleInputVFilter->SetConstant( scalingFactor ); }
	virtual float GetScalingFactor()
	{ return m_ScaleInputFilter->GetConstant(); }

protected:
    TotalVariationHessianOperatorImageFilter();
    virtual ~TotalVariationHessianOperatorImageFilter();

    virtual void GenerateInputRequestedRegion();

    virtual void BeforeThreadedGenerateData();

    virtual void ThreadedGenerateData(const typename InputImageType::RegionType& outputRegionForThread, typename itk::ThreadIdType itkNotUsed(threadId));
	
private:
    TotalVariationHessianOperatorImageFilter(const Self&); //purposely not implemented
    void operator=(const Self&); //purposely not implemented

    bool                              	 m_UseImageSpacing;
    typename InputImageType::SpacingType m_SpacingCoeffs;
	
	typename MultiplyFilterType::Pointer		  m_ScaleInputFilter;
	typename MultiplyFilterType::Pointer		  m_ScaleInputVFilter;
	
    // The adaptive weighting vector image W - Andy
    VectorImagePointer						m_WeightingImage; 
	
    /** Set along which dimensions the gradient computation should be
        performed. The vector components at unprocessed dimensions are ignored */
	// Put it here for now in case it is used
    void SetDimensionsProcessed(bool* DimensionsProcessed);

    // list of the dimensions along which the divergence has
    // to be computed. The components on other dimensions
    // are ignored for performance, but the gradient filter
    // sets them to zero anyway
    bool m_DimensionsProcessed[InputImageType::ImageDimension];
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "rtkTotalVariationHessianOperatorImageFilter.txx"
#endif

#endif //__rtkTotalVariationHessianOperatorImageFilter__
