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

#ifndef __rtkTotalVariationImageFilter_txx
#define __rtkTotalVariationImageFilter_txx
#include "rtkTotalVariationImageFilter.h"

#include "itkConstNeighborhoodIterator.h"
#include "itkProgressReporter.h"
#include <itkImageRegionConstIterator.h>

namespace rtk
{

using namespace itk;


template< typename TInputImage >
TotalVariationImageFilter< TInputImage >
::TotalVariationImageFilter()
{
  // first output is a copy of the image, DataObject created by
  // superclass
  m_SumOfSquareRoots.Fill(1);

  // allocate the data object for the output which is
  // just a decorator around real type
    typename RealObjectType::Pointer output =
      static_cast< RealObjectType * >( this->MakeOutput(1).GetPointer() );
    this->ProcessObject::SetNthOutput( 1, output.GetPointer() );

  this->GetTotalVariationOutput()->Set(NumericTraits< RealType >::Zero);
  
  m_UseImageSpacing = true;
  
  m_ScaleInputFilter = MultiplyFilterType::New();
  m_ScaleInputFilter->SetConstant( 1. );
  
  // default behaviour is to process all dimensions
  for (int dim = 0; dim < TInputImage::ImageDimension; dim++)
    {
    m_DimensionsProcessed[dim] = true;
    }
}

// This should be handled by an itkMacro, but it doesn't seem to work with pointer types
template< typename TInputImage >
void
TotalVariationImageFilter< TInputImage >
::SetDimensionsProcessed(bool* DimensionsProcessed)
{
  bool Modified=false;
  for (int dim=0; dim<TInputImage::ImageDimension; dim++)
    {
    if (m_DimensionsProcessed[dim] != DimensionsProcessed[dim])
      {
      m_DimensionsProcessed[dim] = DimensionsProcessed[dim];
      Modified = true;
      }
    }
  if(Modified) this->Modified();
}

template< typename TInputImage >
DataObject::Pointer
TotalVariationImageFilter< TInputImage >
::MakeOutput(DataObjectPointerArraySizeType output)
{
  switch ( output )
    {
    case 0:
      return TInputImage::New().GetPointer();
      break;
    case 1:
      return RealObjectType::New().GetPointer();
      break;
    default:
      // might as well make an image
      return TInputImage::New().GetPointer();
      break;
    }
}

template< typename TInputImage >
typename TotalVariationImageFilter< TInputImage >::RealObjectType *
TotalVariationImageFilter< TInputImage >
::GetTotalVariationOutput()
{
  return static_cast< RealObjectType * >( this->ProcessObject::GetOutput(1) );
}

template< typename TInputImage >
const typename TotalVariationImageFilter< TInputImage >::RealObjectType *
TotalVariationImageFilter< TInputImage >
::GetTotalVariationOutput() const
{
  return static_cast< const RealObjectType * >( this->ProcessObject::GetOutput(1) );
}

template< typename TInputImage >
void
TotalVariationImageFilter< TInputImage >
::GenerateInputRequestedRegion()
{
  Superclass::GenerateInputRequestedRegion();
  if ( this->GetInput() )
    {
    InputImagePointer image =
      const_cast< typename Superclass::InputImageType * >( this->GetInput() );
    image->SetRequestedRegionToLargestPossibleRegion();
    }
	
  // Check dimension of W if input
  if ( this->GetWeightingImage() )
    {	
	if (this->GetWeightingImage()->GetVectorLength() != TInputImage::ImageDimension)
	  {
	  itkGenericExceptionMacro(<< "The dimension of the adaptive weighting vector image does not match with the input image");
      return;
	  }
	
    for (unsigned int k = 0; k < TInputImage::ImageDimension; ++k)
	  {
	  if (this->GetInput(0)->GetLargestPossibleRegion().GetSize(k) != this->GetWeightingImage()->GetLargestPossibleRegion().GetSize(k))
	    {
		itkGenericExceptionMacro(<< "The dimension of the adaptive weighting vector image does not match with the input image");
		return;
		}
	  }
	}
}

template< typename TInputImage >
void
TotalVariationImageFilter< TInputImage >
::EnlargeOutputRequestedRegion(DataObject *data)
{
  Superclass::EnlargeOutputRequestedRegion(data);
  data->SetRequestedRegionToLargestPossibleRegion();
}

template< typename TInputImage >
void
TotalVariationImageFilter< TInputImage >
::AllocateOutputs()
{
  // Pass the input through as the output
  InputImagePointer image = const_cast< TInputImage * >( this->GetInput() );

  this->GraftOutput(image);

  // Nothing that needs to be allocated for the remaining outputs
}

template< typename TInputImage >
void
TotalVariationImageFilter< TInputImage >
::BeforeThreadedGenerateData()
{
  typename itk::ThreadIdType numberOfThreads = this->GetNumberOfThreads();

  // Resize the thread temporaries
  m_SumOfSquareRoots.SetSize(numberOfThreads);

  // Initialize the temporaries
  m_SumOfSquareRoots.Fill(NumericTraits< RealType >::Zero);
  
  m_SpacingCoeffs= this->GetInput()->GetSpacing();
  if (m_UseImageSpacing == false) m_SpacingCoeffs.Fill(1);

  if ( m_ScaleInputFilter->GetConstant() != 1 )
    {
	m_ScaleInputFilter->SetInput( this->GetInput(0) );
    m_ScaleInputFilter->Update();
	m_InputPointer = m_ScaleInputFilter->GetOutput();
	}
  else
	m_InputPointer = this->GetInput(0);
}

template< typename TInputImage >
void
TotalVariationImageFilter< TInputImage >
::AfterThreadedGenerateData()
{
  RealType totalVariation = 0;
  typename itk::ThreadIdType numberOfThreads = this->GetNumberOfThreads();

  // Add up the results from all threads
  for (typename itk::ThreadIdType i = 0; i < numberOfThreads; i++ )
    {
    totalVariation += m_SumOfSquareRoots[i];
    }
  totalVariation /= m_ScaleInputFilter->GetConstant();

  // Set the output
  this->GetTotalVariationOutput()->Set(totalVariation);
}

template< typename TInputImage >
void
TotalVariationImageFilter< TInputImage >
::ThreadedGenerateData(const RegionType & outputRegionForThread,
                       typename itk::ThreadIdType threadId)
{
  // Generate a list of indices of the dimensions to process
  std::vector<int> dimsToProcess;
  for (int dim = 0; dim < TInputImage::ImageDimension; dim++)
    {
    if(m_DimensionsProcessed[dim]) dimsToProcess.push_back(dim);
    }

  const SizeValueType size0 = outputRegionForThread.GetSize(0);
  if( size0 == 0)
    {
    return;
    }
  RealType sumOfSquareRoots = NumericTraits< RealType >::Zero;

  itk::Size<ImageDimension> radius;
  radius.Fill(1);

  itk::ConstNeighborhoodIterator<TInputImage> iit(radius, m_InputPointer, outputRegionForThread);
  iit.GoToBegin();
  itk::ZeroFluxNeumannBoundaryCondition<TInputImage> boundaryCondition;
  iit.OverrideBoundaryCondition(&boundaryCondition);

  SizeValueType c = (SizeValueType) (iit.Size() / 2); // get offset of center pixel
  SizeValueType strides[ImageDimension]; // get offsets to access neighboring pixels
  for (int dim=0; dim<ImageDimension; dim++)
    {
    strides[dim] = iit.GetStride(dim);
    }

  // Run through the image
  
  if ( m_WeightingImage )
    {
    itk::ImageRegionConstIterator<VectorImageType> wit(m_WeightingImage, outputRegionForThread);
    wit.GoToBegin();
    while(!iit.IsAtEnd())
      {
      // Compute the local differences around the central pixel
      float difference;
      float sumOfSquaredDifferences = 0;
      for (int k = 0; k < dimsToProcess.size(); k++)
        {
        difference = iit.GetPixel(c + strides[dimsToProcess[k]]) - iit.GetPixel(c);
		float w = wit.Get()[dimsToProcess[k]];
        sumOfSquaredDifferences += difference * difference * w * w;
        }
      sumOfSquareRoots += sqrt(sumOfSquaredDifferences);
      ++iit;
	  ++wit;
      }
	}
  else
    {
    while(!iit.IsAtEnd())
      {
      // Compute the local differences around the central pixel
      float difference;
      float sumOfSquaredDifferences = 0;
      for (int k = 0; k < dimsToProcess.size(); k++)
        {
        difference = iit.GetPixel(c + strides[dimsToProcess[k]]) - iit.GetPixel(c);
        sumOfSquaredDifferences += difference * difference / m_SpacingCoeffs[dimsToProcess[k]] / m_SpacingCoeffs[dimsToProcess[k]];
        }
      sumOfSquareRoots += sqrt(sumOfSquaredDifferences);
      ++iit;
      }
	}

  m_SumOfSquareRoots[threadId] = sumOfSquareRoots;
}

template< typename TInputImage >
void
TotalVariationImageFilter< TInputImage >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Total Variation: " << this->GetTotalVariation() << std::endl;
}
} // end namespace rtk and itk
#endif
