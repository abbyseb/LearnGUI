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

#ifndef __rtkTotalVariationHessianOperatorImageFilter_txx
#define __rtkTotalVariationHessianOperatorImageFilter_txx
#include "rtkTotalVariationHessianOperatorImageFilter.h"

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
TotalVariationHessianOperatorImageFilter<TInputImage, TOutputImage>
::TotalVariationHessianOperatorImageFilter()
{
  // Only support 3D for now
  if (InputImageDimension != 3)
    {
	itkGenericExceptionMacro(<< "TotalVariationHessionOperatorImageFilter only supports 3D image for now");
	return;
    }

  // Input 0: The image based on which TV hessian matrix is calculated
  // Input 1: The v vector the matrix operates on
  this->SetNumberOfRequiredInputs(2);

  m_UseImageSpacing = true;
  
  m_ScaleInputFilter = MultiplyFilterType::New();
  m_ScaleInputFilter->SetConstant( 1. );
  
  m_ScaleInputVFilter = MultiplyFilterType::New();
  m_ScaleInputVFilter->SetConstant( 1. );
  
  // default behaviour is to process all dimensions
  for (int dim = 0; dim < InputImageType::ImageDimension; dim++)
    {
    m_DimensionsProcessed[dim] = true;
    }
}

template <class TInputImage, class TOutputImage>
TotalVariationHessianOperatorImageFilter<TInputImage, TOutputImage>
::~TotalVariationHessianOperatorImageFilter()
{
}

// This should be handled by an itkMacro, but it doesn't seem to work with pointer types
template <class TInputImage, class TOutputImage>
void
TotalVariationHessianOperatorImageFilter<TInputImage, TOutputImage>
::SetDimensionsProcessed(bool* DimensionsProcessed)
{
  bool Modified=false;
  for (int dim=0; dim<InputImageType::ImageDimension; dim++)
    {
    if (m_DimensionsProcessed[dim] != DimensionsProcessed[dim])
      {
      m_DimensionsProcessed[dim] = DimensionsProcessed[dim];
      Modified = true;
      }
    }
  if(Modified) this->Modified();
}

template <class TInputImage, class TOutputImage>
void
TotalVariationHessianOperatorImageFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();

  // get pointers to the input and output
  typename InputImageType::Pointer inputPtr0 = const_cast< InputImageType * >( this->GetInput(0) );
  typename InputImageType::Pointer inputPtr1 = const_cast< InputImageType * >( this->GetInput(1) );
  typename OutputImageType::Pointer outputPtr = this->GetOutput();

  if ( !inputPtr0 || !inputPtr1 || !outputPtr )
    {
    return;
    }
	
  // Check if input 0 and input 1 has the same dimension
  for (unsigned int k = 0; k < InputImageType::ImageDimension; ++k)
    {
	if (this->GetInput(0)->GetLargestPossibleRegion().GetSize(k) != this->GetInput(1)->GetLargestPossibleRegion().GetSize(k))
	  {
	  itkGenericExceptionMacro(<< "Both inputs must have the same size");
	  return;
	  }
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
	
  // get a copy of the input requested region (should equal the output
  // requested region)
  typename InputImageType::RegionType inputRequestedRegion;
  inputRequestedRegion = inputPtr0->GetRequestedRegion();

  // pad the input requested region by the operator radius
  inputRequestedRegion.PadByRadius(1);

  // crop the input requested region at the input's largest possible region
  if ( inputRequestedRegion.Crop( inputPtr0->GetLargestPossibleRegion() ) )
    {
    inputPtr0->SetRequestedRegion(inputRequestedRegion);
	inputPtr1->SetRequestedRegion(inputRequestedRegion);
    return;
    }
  else
    {
    // Couldn't crop the region (requested region is outside the largest
    // possible region).  Throw an exception.

    // store what we tried to request (prior to trying to crop)
    inputPtr0->SetRequestedRegion(inputRequestedRegion);
	inputPtr1->SetRequestedRegion(inputRequestedRegion);

    // build an exception
    itk::InvalidRequestedRegionError e(__FILE__, __LINE__);
    e.SetLocation(ITK_LOCATION);
    e.SetDescription("Requested region is (at least partially) outside the largest possible region.");
    e.SetDataObject(inputPtr0);
    throw e;
    }
}

template <class TInputImage, class TOutputImage>
void
TotalVariationHessianOperatorImageFilter<TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{
  m_SpacingCoeffs= this->GetInput()->GetSpacing();
  if (m_UseImageSpacing == false) m_SpacingCoeffs.Fill(1);
}

template <class TInputImage, class TOutputImage>
void
TotalVariationHessianOperatorImageFilter<TInputImage, TOutputImage>
::ThreadedGenerateData(const typename InputImageType::RegionType& outputRegionForThread, typename itk::ThreadIdType itkNotUsed(threadId))
{
  // Generate a list of indices of the dimensions to process
  std::vector<int> dimsToProcess;
  for (int dim = 0; dim < InputImageType::ImageDimension; dim++)
    {
    if(m_DimensionsProcessed[dim]) dimsToProcess.push_back(dim);
    }

  const unsigned int NDim = dimsToProcess.size();
	
  // Initialize weighting factors by spacing inverse square
  std::vector< std::vector<float> > w (NDim+1, std::vector<float>(NDim,1) );
  for (unsigned int k = 0; k < NDim+1; ++k)
	for (unsigned int l = 0; l < NDim; ++l)
	  w[k][l] = 1. / m_SpacingCoeffs[dimsToProcess[l]] / m_SpacingCoeffs[dimsToProcess[l]];

  typename OutputImageType::Pointer output = this->GetOutput();
  
  typename TInputImage::ConstPointer input;
  if ( m_ScaleInputFilter->GetConstant() != 1 )
    {
	m_ScaleInputFilter->SetInput( this->GetInput(0) );
    m_ScaleInputFilter->Update();
	input = m_ScaleInputFilter->GetOutput();
	}
  else
	input = this->GetInput(0);
	
  typename TInputImage::ConstPointer inputV;
  if ( m_ScaleInputVFilter->GetConstant() != 1 )
    {
	m_ScaleInputVFilter->SetInput( this->GetInput(1) );
    m_ScaleInputVFilter->Update();
	inputV = m_ScaleInputVFilter->GetOutput();
	}
  else
	inputV = this->GetInput(1);

  itk::ZeroFluxNeumannBoundaryCondition<InputImageType> zfnbc;

  itk::ImageRegionIterator<OutputImageType> oit(output, outputRegionForThread);
  oit.GoToBegin();

  itk::Size<InputImageDimension> radius;
  radius.Fill(1);

  itk::ConstNeighborhoodIterator<InputImageType> iit(radius, input, outputRegionForThread);
  iit.GoToBegin();
  iit.OverrideBoundaryCondition(&zfnbc);
  
  itk::ConstNeighborhoodIterator<InputImageType> iitV(radius, inputV, outputRegionForThread);
  iitV.GoToBegin();
  iitV.OverrideBoundaryCondition(&zfnbc);
  
  // FIXME:
  // If m_WeightingImage is not specified, create a dummy vector image for the sake of initializing wit
  typename VectorImageType::ConstPointer wPointer;
  if (m_WeightingImage)
	wPointer = m_WeightingImage;
  else
    {
	wPointer = VectorImageType::New();
	}
  itk::ConstNeighborhoodIterator<VectorImageType> wit(radius, wPointer, outputRegionForThread);
  if (m_WeightingImage)
    {
	wit.GoToBegin();
	}

  itk::SizeValueType c = (itk::SizeValueType) (iit.Size() / 2); // get offset of center pixel
  itk::SizeValueType strides[OutputImageType::ImageDimension]; // get offsets to access neighboring pixels
  for (int dim=0; dim<OutputImageType::ImageDimension; dim++)
    {
    strides[dim] = iit.GetStride(dim);
    }

  float tvhessian;
  std::vector< std::vector< float > > D ( NDim+1, std::vector<float> (NDim, 0) );
  std::vector< float > sv (NDim+1, 0);
  float cvalue;
  // Small value to prevent singularity
  const float eps = std::numeric_limits<float>::epsilon();
  while(!oit.IsAtEnd())
    {
	if (m_WeightingImage)
	  {
	  for (unsigned int l = 0; l < NDim; ++l)
		w[0][l] = wit.GetPixel(c)[l];
		
	  for (unsigned int k = 0; k < NDim; ++k)
	    {
		for (unsigned int l = 0; l < NDim; ++l)
		  w[k+1][l] = wit.GetPixel(c + strides[dimsToProcess[k]])[l];
		}
	  }
	
    // Compute normal backward differences, e.g. (x,y,z)-(x-1,y,z)
	cvalue = iit.GetPixel(c);
    for (unsigned int l = 0; l < dimsToProcess.size(); l++)
      {
	  D[0][l] = cvalue - iit.GetPixel(c - strides[dimsToProcess[l]]);
      }
	// Compute other differences
    for (unsigned int k = 0; k < dimsToProcess.size(); k++)
      {
	  cvalue = iit.GetPixel(c + strides[dimsToProcess[k]]);
	  for (unsigned int l = 0; l < dimsToProcess.size(); l++)
	    {
		D[k+1][l] = cvalue - iit.GetPixel(c + strides[dimsToProcess[k]] - strides[dimsToProcess[l]]);
	    }
      }
	// Compute square roots cubic
	for (unsigned int k = 0; k < dimsToProcess.size() + 1; k++)
	  {
	  sv[k] = 0;
	  for (unsigned int l = 0; l < dimsToProcess.size(); l++)
	    {
		sv[k] += D[k][l] * D[k][l] * w[k][l];
	    }
	  sv[k] += eps;
	  sv[k] *= sqrtf( sv[k] );
	  }
	  
	// Put everything together
	// Fix me: Generalize this to multi- and user-specified dimension
	
	// partial (x,y,z)(x,y,z)
	tvhessian = ( w[0][0]*w[0][1]*(D[0][0]-D[0][1])*(D[0][0]-D[0][1]) + w[0][0]*w[0][2]*(D[0][0]-D[0][2])*(D[0][0]-D[0][2]) + w[0][1]*w[0][2]*(D[0][1]-D[0][2])*(D[0][1]-D[0][2]) ) / sv[0]
		+ w[1][0] * ( w[1][1]*D[1][1]*D[1][1] + w[1][2]*D[1][2]*D[1][2] ) / sv[1]
		+ w[2][1] * ( w[2][0]*D[2][0]*D[2][0] + w[2][2]*D[2][2]*D[2][2] ) / sv[2]
		+ w[3][2] * ( w[3][0]*D[3][0]*D[3][0] + w[3][1]*D[3][1]*D[3][1] ) / sv[3];
	tvhessian *= iitV.GetPixel(c);
	
	// Partial (x,y,z) (x+-1,y,z) and other dimensions
	tvhessian += iitV.GetPixel(c - strides[0]) *
		w[0][0] * (w[0][1]*D[0][1]*(D[0][0] - D[0][1]) + w[0][2]*D[0][2]*(D[0][0] - D[0][2])) / sv[0];
	tvhessian += iitV.GetPixel(c - strides[1]) *
		w[0][1] * (w[0][0]*D[0][0]*(D[0][1] - D[0][0]) + w[0][2]*D[0][2]*(D[0][1] - D[0][2])) / sv[0];
	tvhessian += iitV.GetPixel(c - strides[2]) *
		w[0][2] * (w[0][0]*D[0][0]*(D[0][2] - D[0][0]) + w[0][1]*D[0][1]*(D[0][2] - D[0][1])) / sv[0];
	tvhessian += iitV.GetPixel(c + strides[0]) *
		w[1][0] * (w[1][1]*D[1][1]*(D[1][0] - D[1][1]) + w[1][2]*D[1][2]*(D[1][0] - D[1][2])) / sv[1];
	tvhessian += iitV.GetPixel(c + strides[1]) *
		w[2][1] * (w[2][0]*D[2][0]*(D[2][1] - D[2][0]) + w[2][2]*D[2][2]*(D[2][1] - D[2][2])) / sv[2];
	tvhessian += iitV.GetPixel(c + strides[2]) *
		w[3][2] * (w[3][0]*D[3][0]*(D[3][2] - D[3][0]) + w[3][1]*D[3][1]*(D[3][2] - D[3][1])) / sv[3];
		
	// Partial (x,y,z) (x+1,y-1,z) and other dimensions
	tvhessian -= iitV.GetPixel(c + strides[0] - strides[1]) *
		w[1][0] * w[1][1] * D[1][0] * D[1][1] / sv[1];
	tvhessian -= iitV.GetPixel(c + strides[0] - strides[2]) *
		w[1][0] * w[1][2] * D[1][0] * D[1][2] / sv[1];
	tvhessian -= iitV.GetPixel(c - strides[0] + strides[1]) *
		w[2][0] * w[2][1] * D[2][0] * D[2][1] / sv[2];
	tvhessian -= iitV.GetPixel(c + strides[1] - strides[2]) *
		w[2][1] * w[2][2] * D[2][1] * D[2][2] / sv[2];
	tvhessian -= iitV.GetPixel(c - strides[0] + strides[2]) *
		w[3][0] * w[3][2] * D[3][0] * D[3][2] / sv[3];
	tvhessian -= iitV.GetPixel(c - strides[1] + strides[2]) *
		w[3][1] * w[3][2] * D[3][1] * D[3][2] / sv[3];
	  
    oit.Set(tvhessian);
    ++oit;
    ++iit;
	++iitV;
	if (m_WeightingImage)
	  ++wit;
    }
}


} // end namespace rtk

#endif
