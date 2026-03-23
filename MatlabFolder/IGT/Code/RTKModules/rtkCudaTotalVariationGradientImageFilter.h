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

#ifndef __rtkCudaTotalVariationGradientImageFilter_h
#define __rtkCudaTotalVariationGradientImageFilter_h

#include "rtkTotalVariationGradientImageFilter.h"
#include <itkCudaImageToImageFilter.h>
#include "igtWin32Header.h"
#include "itkVectorIndexSelectionCastImageFilter.h"

namespace rtk
{

/** \class CudaTotalVariationGradientImageFilter
 * \brief Implements the TotalVariationGradientImageFilter on GPU (only for 3D image now)
 *
 *
 *
 * \author Andy Shieh
 *
 * \ingroup CudaImageToImageFilter
 */

  class ITK_EXPORT CudaTotalVariationGradientImageFilter :
        public itk::CudaImageToImageFilter< itk::CudaImage<float,3>, itk::CudaImage<float,3>,
    TotalVariationGradientImageFilter< itk::CudaImage<float,3>, itk::CudaImage<float,3> > >
{
public:
  /** Standard class typedefs. */
  typedef rtk::CudaTotalVariationGradientImageFilter                               		Self;
  typedef itk::CudaImage<float,3>                                                       OutputImageType;
  typedef rtk::TotalVariationGradientImageFilter< OutputImageType >			 	  		Superclass;
  typedef itk::SmartPointer<Self>                                                       Pointer;
  typedef itk::SmartPointer<const Self>                                                 ConstPointer;
  
  /** Image typedefs */
  typedef itk::CudaImage<float,3>		InputImageType;
  
  /** Vector image type for W */
  typedef itk::VectorImage< typename InputImageType::PixelType, 3 > VectorImageType;
  
  /** Vector image to scalar image */
  typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, InputImageType> IndexSelectionType;
  
  /** Standard New method. */
  itkNewMacro(Self)

  /** Runtime information support. */
  itkTypeMacro(CudaTotalVariationGradientImageFilter, InterpolatorWithKnownWeightsImageFilter)
  
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
	
  /** Set/Get whether or not the filter will use the spacing of the input
      image in its calculations */
  itkSetMacro(UseImageSpacing, bool);
  itkGetConstMacro(UseImageSpacing, bool);
  
  /** Set/Get scaling factor */
  itkSetMacro(ScalingFactor, float);
  itkGetMacro(ScalingFactor, float);

protected:
  igtcuda_EXPORT CudaTotalVariationGradientImageFilter();
  ~CudaTotalVariationGradientImageFilter(){
  }

  virtual void GPUGenerateData();

private:
  CudaTotalVariationGradientImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&);         //purposely not implemented
  
  bool                              	m_UseImageSpacing;
  OutputImageType::SpacingType			m_SpacingCoeffs;
  
  // The adaptive weighting vector image W - Andy
  VectorImageType::Pointer 				m_WeightingImage; 
  
  float				m_ScalingFactor;

}; // end of class

} // end namespace rtk

#endif
