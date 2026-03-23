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

#ifndef __rtkDirectConeBeamNCCTracking_h
#define __rtkDirectConeBeamNCCTracking_h

#include "rtkDirectConeBeamNCCTracking.h"

namespace rtk
{

/** \class DirectConeBeamNCCTracking
 * \brief Direct tumor tracking method by minimizing the objective function
 *   f_obj = -NCC + constraints
 *
 * \author Andy Shieh 2015
 *
 * \ingroup ReconstructionAlgorithm
 */
template<class TInputImage, class TOutputImage=TInputImage>
class ITK_EXPORT DirectConeBeamNCCTracking :
  public rtk::DirectConeBeamTracking<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef DirectConeBeamNCCTracking                               		   Self;
  typedef DirectConeBeamTracking<TInputImage, TOutputImage> 			   Superclass;
  typedef itk::SmartPointer<Self>                                          Pointer;
  typedef itk::SmartPointer<const Self>                                    ConstPointer;
  
  /* Typedefs for matrix operation */
  typedef itk::Matrix<double, 3, 3>			MatrixType;
  typedef itk::Vector<double, 3>			VectorType;  
  
/** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(DirectConeBeamNCCTracking, DirectConeBeamTracking);
  
  virtual void PrintTiming(std::ostream& os) const;

protected:
  DirectConeBeamNCCTracking();
  ~DirectConeBeamNCCTracking(){}

  virtual void GenerateInputRequestedRegion();

  virtual void GenerateOutputInformation();

  virtual void GenerateData();

  /** The two inputs should not be in the same space so there is nothing
   * to verify. */
  virtual void VerifyInputInformation() {}
  
  // Pointers to each subfilter
  typename Superclass::MultiplyFilterType::Pointer			m_pRf, m_pRfx, m_pRfy, m_pRfz,
												m_pRfxx, m_pRfyy, m_pRfzz,
												m_pRfxy, m_pRfxz, m_pRfyz;
  typename Superclass::ImageStatisticsFilterType::Pointer	m_IS_p, m_IS_Rf, m_IS_pRf,
												m_IS_pRfx, m_IS_pRfy, m_IS_pRfz,
												m_IS_pRfxx, m_IS_pRfyy, m_IS_pRfzz,
												m_IS_pRfxy, m_IS_pRfxz, m_IS_pRfyz;

private:
  //purposely not implemented
  DirectConeBeamNCCTracking(const Self&);
  void operator=(const Self&);
}; // end of class

} // end namespace rtk

#ifndef ITK_MANUAL_INSTANTIATION
#include "rtkDirectConeBeamNCCTracking.txx"
#endif

#endif
