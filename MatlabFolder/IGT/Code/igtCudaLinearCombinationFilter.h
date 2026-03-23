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

#ifndef __igtCudaLinearCombinationFilter_h
#define __igtCudaLinearCombinationFilter_h

#include "igtLinearCombinationFilter.h"
#include "igtCudaLinearCombinationFilter.h"
#include <itkCudaImageToImageFilter.h>
#include "igtWin32Header.h"
#include "itkVectorIndexSelectionCastImageFilter.h"

namespace igt
{

/** \class CudaLinearCombinationFilter
 * \brief Implements the LinearCombinationFilter on GPU
 *
 * \author Andy Shieh
 */

  class ITK_EXPORT CudaLinearCombinationFilter :
        public itk::CudaImageToImageFilter< itk::CudaImage<float,3>, itk::CudaImage<float,3>,
		LinearCombinationFilter< itk::CudaImage<float,3>, itk::CudaImage<float,3> > >
{
public:
  /** Standard class typedefs. */
  typedef igt::CudaLinearCombinationFilter                 Self;
  typedef itk::CudaImage<float,3>                          ImageType;
  typedef igt::LinearCombinationFilter< OutputImageType >  Superclass;
  typedef itk::SmartPointer<Self>                          Pointer;
  typedef itk::SmartPointer<const Self>                    ConstPointer;
  
  /** Standard New method. */
  itkNewMacro(Self)

  /** Runtime information support. */
  itkTypeMacro(CudaLinearCombinationFilter, LinearCombinationFilter)

protected:
  igtcuda_EXPORT CudaLinearCombinationFilter();
  ~CudaLinearCombinationFilter(){
  }

  virtual void GPUGenerateData();

private:
  CudaLinearCombinationFilter(const Self&); //purposely not implemented
  void operator=(const Self&);         //purposely not implemented

}; // end of class

} // end namespace igt

#endif
