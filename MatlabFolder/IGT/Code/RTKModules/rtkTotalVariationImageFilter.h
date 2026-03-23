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

#ifndef __rtkTotalVariationImageFilter_h
#define __rtkTotalVariationImageFilter_h

#include "itkImageToImageFilter.h"
#include "itkNumericTraits.h"
#include "itkArray.h"
#include "itkSimpleDataObjectDecorator.h"
#include <itkMultiplyImageFilter.h>
#include "itkVectorImage.h"

namespace rtk
{
/** \class TotalVariationImageFilter
 * \brief Compute the total variation of an Image.
 *
 * TotalVariationImageFilter computes the total variation, defined
 * as the L1 norm of the image of the L2 norm of the gradient,
 * of an image. The filter needs all of its input image.  It
 * behaves as a filter with an input and output. Thus it can be inserted
 * in a pipeline with other filters and the total variation will only be
 * recomputed if a downstream filter changes.
 *
 * The filter passes its input through unmodified. The filter is
 * threaded.
 *
 * Andy: Added in vector image W to adaptively weight the TV term.
 *
 */
 using namespace itk;

template< typename TInputImage >
class TotalVariationImageFilter:
  public itk::ImageToImageFilter< TInputImage, TInputImage >
{
public:
  /** Standard Self typedef */
  typedef TotalVariationImageFilter                      	  Self;
  typedef itk::ImageToImageFilter< TInputImage, TInputImage > Superclass;
  typedef itk::SmartPointer< Self >                           Pointer;
  typedef itk::SmartPointer< const Self >                     ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(TotalVariationImageFilter, ImageToImageFilter);

  /** Image related typedefs. */
  typedef typename TInputImage::Pointer InputImagePointer;

  typedef typename TInputImage::RegionType RegionType;
  typedef typename TInputImage::SizeType   SizeType;
  typedef typename TInputImage::IndexType  IndexType;
  typedef typename TInputImage::PixelType  PixelType;
  
  /** Vector image type for W */
  typedef itk::VectorImage< PixelType, TInputImage::ImageDimension > VectorImageType;
  typedef typename VectorImageType::Pointer VectorImagePointer;
  
  /** Convenient typedefs. */
  typedef itk::MultiplyImageFilter< TInputImage, TInputImage , TInputImage >	MultiplyFilterType;  
  
  /** Image related typedefs. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TInputImage::ImageDimension);

  /** Type to use for computations. */
  typedef typename NumericTraits< PixelType >::RealType RealType;

  /** Smart Pointer type to a DataObject. */
  typedef typename DataObject::Pointer DataObjectPointer;

  /** Type of DataObjects used for scalar outputs */
  typedef SimpleDataObjectDecorator< RealType >  RealObjectType;
//  typedef SimpleDataObjectDecorator< PixelType > PixelObjectType;

  /** Return the computed Minimum. */
  RealType GetTotalVariation() const
  { return this->GetTotalVariationOutput()->Get(); }
  RealObjectType * GetTotalVariationOutput();

  const RealObjectType * GetTotalVariationOutput() const;

  /** Make a DataObject of the correct type to be used as the specified
   * output. */
  typedef ProcessObject::DataObjectPointerArraySizeType DataObjectPointerArraySizeType;
  using Superclass::MakeOutput;
  virtual DataObjectPointer MakeOutput(DataObjectPointerArraySizeType idx);
  
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

#ifdef ITK_USE_CONCEPT_CHECKING
  // Begin concept checking
  itkConceptMacro( InputHasNumericTraitsCheck,
                   ( Concept::HasNumericTraits< PixelType > ) );
  // End concept checking
#endif

protected:
  TotalVariationImageFilter();
  ~TotalVariationImageFilter(){}
  void PrintSelf(std::ostream & os, Indent indent) const;

  /** Pass the input through unmodified. Do this by Grafting in the
   *  AllocateOutputs method.
   */
  void AllocateOutputs();

  /** Initialize some accumulators before the threads run. */
  void BeforeThreadedGenerateData();

  /** Do final mean and variance computation from data accumulated in threads.
   */
  void AfterThreadedGenerateData();

  /** Multi-thread version GenerateData. */
  void  ThreadedGenerateData(const RegionType &
                             outputRegionForThread,
                             typename itk::ThreadIdType threadId);

  // Override since the filter needs all the data for the algorithm
  void GenerateInputRequestedRegion();

  // Override since the filter produces all of its output
  void EnlargeOutputRequestedRegion(DataObject *data);

private:
  TotalVariationImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &);        //purposely not implemented
  
  bool                              m_UseImageSpacing;
  typename TInputImage::SpacingType m_SpacingCoeffs;
  
  typename MultiplyFilterType::Pointer		  m_ScaleInputFilter;
  
  typename TInputImage::ConstPointer  m_InputPointer;
  
  // list of the dimensions along which the divergence has
  // to be computed. The components on other dimensions
  // are ignored for performance, but the gradient filter
  // sets them to zero anyway
  bool m_DimensionsProcessed[TInputImage::ImageDimension];
  
  // The adaptive weighting vector image W - Andy
  VectorImagePointer 	m_WeightingImage; 
  
  Array< RealType >       m_SumOfSquareRoots;
}; // end of class
} // end namespace rtk and itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "rtkTotalVariationImageFilter.txx"
#endif

#endif