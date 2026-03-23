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

#ifndef __igtWarpBackProjectionImageFilter_h
#define __igtWarpBackProjectionImageFilter_h

#include "rtkBackProjectionImageFilter.h"

#include <itkBarrier.h>

#include "rtkCyclicDeformationImageFilter.h"

#include "itkVector.h"

#ifdef IGT_USE_CUDA
  #include "itkCudaImage.h"
#endif

namespace igt
{

/** \class WarpBackProjectionImageFilter
 * \brief Modified from rtk::FDKWarpBackProjectionImageFilter
 * [Rit et al, Med Phys, 2009].
 * 
 * \author Andy
 */
template <class TInputImage, class TOutputImage>
class ITK_EXPORT WarpBackProjectionImageFilter :
	public rtk::BackProjectionImageFilter<TInputImage,TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef WarpBackProjectionImageFilter                      	 Self;
  typedef rtk::BackProjectionImageFilter<TInputImage,TOutputImage> 	 Superclass;
  typedef itk::SmartPointer<Self>                                Pointer;
  typedef itk::SmartPointer<const Self>                          ConstPointer;
  typedef typename TInputImage::PixelType                        InputPixelType;
  typedef typename TOutputImage::RegionType                      OutputImageRegionType;

  typedef rtk::ProjectionGeometry<TOutputImage::ImageDimension>     GeometryType;
  typedef typename GeometryType::Pointer                            GeometryPointer;
  typedef typename GeometryType::MatrixType                         ProjectionMatrixType;
  typedef itk::Image<InputPixelType, TInputImage::ImageDimension-1> ProjectionImageType;
  typedef typename ProjectionImageType::Pointer                     ProjectionImagePointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(WarpBackProjectionImageFilter, BackProjectionImageFilter);

  /** Set the deformation. */
  virtual void SetDeformation(DeformationPointer def)
  {
	  m_Deformation = def;
  }

protected:
  WarpBackProjectionImageFilter():m_DeformationUpdateError(false) {};
  virtual ~WarpBackProjectionImageFilter() {};

  virtual void BeforeThreadedGenerateData();

  virtual void ThreadedGenerateData( const OutputImageRegionType& outputRegionForThread, typename itk::ThreadIdType threadId );

private:
  WarpBackProjectionImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&);                   //purposely not implemented

  DeformationPointer    m_Deformation;
  itk::Barrier::Pointer m_Barrier;
  bool                  m_DeformationUpdateError;
};

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtWarpBackProjectionImageFilter.txx"
#endif

#endif
