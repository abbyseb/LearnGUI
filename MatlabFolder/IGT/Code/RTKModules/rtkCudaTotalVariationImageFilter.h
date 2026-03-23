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

#ifndef __rtkCudaTotalVariationImageFilter_h
#define __rtkCudaTotalVariationImageFilter_h

#include "rtkTotalVariationImageFilter.h"
#include <itkCudaImageToImageFilter.h>
#include "igtWin32Header.h"
#include "itkVectorIndexSelectionCastImageFilter.h"
#include <itkStatisticsImageFilter.h>

namespace rtk
{

/** \class CudaTotalVariationImageFilter
 * \brief Implements the TotalVariationImageFilter on GPU (only for 3D image now)
 *
 *
 *
 * \author Andy Shieh
 *
 * \ingroup CudaImageToImageFilter
 */

  class ITK_EXPORT CudaTotalVariationImageFilter :
        public itk::CudaImageToImageFilter< itk::CudaImage<float,3>, itk::CudaImage<float,3>,
    TotalVariationImageFilter< itk::CudaImage<float,3> > >
{
public:
  /** Standard class typedefs. */
  typedef rtk::CudaTotalVariationImageFilter                               				Self;
  typedef itk::CudaImage<float,3>                                                       OutputImageType;
  typedef rtk::TotalVariationImageFilter< OutputImageType >			 	  				Superclass;
  typedef itk::SmartPointer<Self>                                                       Pointer;
  typedef itk::SmartPointer<const Self>                                                 ConstPointer;
  
  /** Image typedefs */
  typedef itk::CudaImage<float,3>		InputImageType;
  
  /** Vector image type for W */
  typedef itk::VectorImage< typename InputImageType::PixelType, 3 > VectorImageType;
  
  /** Vector image to scalar image */
  typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, InputImageType> IndexSelectionType;

  /** Image sum filters typedefs */
  typedef itk::StatisticsImageFilter< InputImageType > ImageSumFilterType;
  
  /** Standard New method. */
  itkNewMacro(Self)

  /** Runtime information support. */
  itkTypeMacro(CudaTotalVariationImageFilter, InterpolatorWithKnownWeightsImageFilter)
  
  /** Use the image spacing information in calculations. Use this option if you
   *  want derivatives in physical space. Default is UseImageSpacingOn. */
  void SetUseImageSpacingOn()
  { this->SetUseImageSpacing(true); }

  /** Ignore the image spacing. Use this option if you want derivatives in
      isotropic pixel space.  Default is UseImageSpacingOn. */
  void SetUseImageSpacingOff()
  { this->SetUseImageSpacing(false); }
  
  /** Set/Get the weighting image - Andy */
  itkSetMacro(WeightingImage, VectorImageType::Pointer);
  itkGetConstMacro(WeightingImage, VectorImageType::Pointer);
  
  /** Get the temporary TV sqrt value image */
  itkGetConstMacro(TVSqrtImage, InputImageType::Pointer);
	
  /** Set/Get whether or not the filter will use the spacing of the input
      image in its calculations */
  itkSetMacro(UseImageSpacing, bool);
  itkGetConstMacro(UseImageSpacing, bool);
  
  /** Set/Get scaling factor */
  itkSetMacro(ScalingFactor, float);
  itkGetMacro(ScalingFactor, float);

protected:
  igtcuda_EXPORT CudaTotalVariationImageFilter();
  ~CudaTotalVariationImageFilter(){
  }

  virtual void GPUGenerateData();

private:
  CudaTotalVariationImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&);         //purposely not implemented
  
  bool                              	m_UseImageSpacing;
  OutputImageType::SpacingType			m_SpacingCoeffs;
  
  // The adaptive weighting vector image W - Andy
  VectorImageType::Pointer 				m_WeightingImage; 
  
  float				m_ScalingFactor;

  ImageSumFilterType::Pointer		m_ImageSumFilter;
  
  // A temporary image to hold the TV sqrt values
  InputImageType::Pointer			m_TVSqrtImage;

}; // end of class

} // end namespace rtk

#endif
