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

#ifndef __rtkPartialDerivativeImageFilter_txx
#define __rtkPartialDerivativeImageFilter_txx
#include "rtkPartialDerivativeImageFilter.h"

#include <itkConstShapedNeighborhoodIterator.h>
#include <itkNeighborhoodInnerProduct.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>
#include <itkNeighborhoodAlgorithm.h>
#include <itkZeroFluxNeumannBoundaryCondition.h>
#include <itkOffset.h>
#include <itkProgressReporter.h>

#include <math.h>

namespace rtk
{

template <class TInputImage, class TOutputImage>
PartialDerivativeImageFilter<TInputImage, TOutputImage>
::PartialDerivativeImageFilter()
{
  m_UseImageSpacing = true;
  
  m_DerivativeOption = 0;
}

template <class TInputImage, class TOutputImage>
PartialDerivativeImageFilter<TInputImage, TOutputImage>
::~PartialDerivativeImageFilter()
{
}

template <class TInputImage, class TOutputImage>
void
PartialDerivativeImageFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();

  // get pointers to the input and output
  typename TInputImage::Pointer inputPtr = const_cast< TInputImage * >( this->GetInput() );
  typename TOutputImage::Pointer outputPtr = this->GetOutput();

  if ( !inputPtr || !outputPtr )
    {
    return;
    }
	
  if(m_DerivativeOption > 8)
    {
    itkGenericExceptionMacro(<< "Unsupported derivative option");
    }

  // get a copy of the input requested region (should equal the output
  // requested region)
  typename TInputImage::RegionType inputRequestedRegion;
  inputRequestedRegion = inputPtr->GetRequestedRegion();

  // pad the input requested region by the operator radius
  inputRequestedRegion.PadByRadius(1);

  // crop the input requested region at the input's largest possible region
  if ( inputRequestedRegion.Crop( inputPtr->GetLargestPossibleRegion() ) )
    {
    inputPtr->SetRequestedRegion(inputRequestedRegion);
    return;
    }
  else
    {
    // Couldn't crop the region (requested region is outside the largest
    // possible region).  Throw an exception.

    // store what we tried to request (prior to trying to crop)
    inputPtr->SetRequestedRegion(inputRequestedRegion);

    // build an exception
    itk::InvalidRequestedRegionError e(__FILE__, __LINE__);
    e.SetLocation(ITK_LOCATION);
    e.SetDescription("Requested region is (at least partially) outside the largest possible region.");
    e.SetDataObject(inputPtr);
    throw e;
    }
}

template <class TInputImage, class TOutputImage>
void
PartialDerivativeImageFilter< TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{
  m_SpacingCoeffs= this->GetInput()->GetSpacing();
  if (m_UseImageSpacing == false) m_SpacingCoeffs.Fill(1);
}

template <class TInputImage, class TOutputImage>
void
PartialDerivativeImageFilter< TInputImage, TOutputImage>
::ThreadedGenerateData(const typename TInputImage::RegionType& outputRegionForThread, typename itk::ThreadIdType itkNotUsed(threadID))
{
  typename TOutputImage::Pointer output = this->GetOutput();
  typename TInputImage::ConstPointer input = this->GetInput(0);

  itk::ZeroFluxNeumannBoundaryCondition<InputImageType> zfnbc;

  itk::ImageRegionIterator<TOutputImage> oit(output, outputRegionForThread);
  oit.GoToBegin();

  itk::Size<InputImageDimension> radius;
  radius.Fill(1);
  if( m_DerivativeOption < 6 && m_DerivativeOption > 2)
    radius[m_DerivativeOption - 3] = 2;

  itk::ConstNeighborhoodIterator<TInputImage> iit(radius, input, outputRegionForThread);
  iit.GoToBegin();
  iit.OverrideBoundaryCondition(&zfnbc);
	
  itk::SizeValueType c = (itk::SizeValueType) (iit.Size() / 2); // get offset of center pixel
  itk::SizeValueType strides[TOutputImage::ImageDimension]; // get offsets to access neighboring pixels
  for (int dim=0; dim<TOutputImage::ImageDimension; dim++)
    {
    strides[dim] = iit.GetStride(dim);
    }
  
  while(!oit.IsAtEnd())
    {
	if(m_DerivativeOption < 3) // fx,fy,fz
	  oit.Set( (iit.GetPixel(c - strides[m_DerivativeOption]) 
			  - iit.GetPixel(c + strides[m_DerivativeOption]))
			  / m_SpacingCoeffs[m_DerivativeOption] / 2. );
	else if (m_DerivativeOption < 6) // fxx,fyy,fzz
	  oit.Set( (iit.GetPixel(c - 2*strides[m_DerivativeOption-3])
	          - 2 * iit.GetPixel(c)
			  + iit.GetPixel(c + 2*strides[m_DerivativeOption-3]))
			  / m_SpacingCoeffs[m_DerivativeOption-3] 
			  / m_SpacingCoeffs[m_DerivativeOption-3] 
			  / 4. );
	else if (m_DerivativeOption == 6) // fxy
	  oit.Set( (iit.GetPixel(c - strides[0] - strides[1])
	          - iit.GetPixel(c - strides[0] + strides[1])
			  - iit.GetPixel(c + strides[0] - strides[1])
			  + iit.GetPixel(c + strides[0] + strides[1]))
			  / m_SpacingCoeffs[0] 
			  / m_SpacingCoeffs[1] 
			  / 4. );
	else if (m_DerivativeOption == 7) // fxz
	  oit.Set( (iit.GetPixel(c - strides[0] - strides[2])
	          - iit.GetPixel(c - strides[0] + strides[2])
			  - iit.GetPixel(c + strides[0] - strides[2])
			  + iit.GetPixel(c + strides[0] + strides[2]))
			  / m_SpacingCoeffs[0] 
			  / m_SpacingCoeffs[2] 
			  / 4. );
	else if (m_DerivativeOption == 8) // fyz
	  oit.Set( (iit.GetPixel(c - strides[1] - strides[2])
	          - iit.GetPixel(c - strides[1] + strides[2])
			  - iit.GetPixel(c + strides[1] - strides[2])
			  + iit.GetPixel(c + strides[1] + strides[2]))
			  / m_SpacingCoeffs[1] 
			  / m_SpacingCoeffs[2] 
			  / 4. );
    
    ++oit;
    ++iit;
    }
}


} // end namespace rtk

#endif
