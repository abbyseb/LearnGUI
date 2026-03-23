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

#ifndef __rtkVectorSorter_h
#define __rtkVectorSorter_h

#include <itkLightObject.h>

#include <vector>

/** \class VectorSorter
 * \brief Sort a vector to several sub-vectors based on an input bin number vector.
 *		  A bin number of zero indicates discarded element.
 *
 * \author Andy Shieh
 *
 * \ingroup itkLightObject
 */
namespace rtk
{

template<class TInput>
class ITK_EXPORT VectorSorter :
  public itk::LightObject
{
public:
  /** Standard class typedefs. */
  typedef VectorSorter                    				     Self;
  typedef itk::LightObject								     Superclass;
  typedef itk::SmartPointer<Self>                            Pointer;
  typedef itk::SmartPointer<const Self>                      ConstPointer;

  /** Some convenient typedefs. */
  typedef TInput                       						 InputType;
  typedef unsigned int                       				 BinNumberType;
  typedef std::vector<InputType>					 		 InputVectorType;
  typedef std::vector<BinNumberType>				 		 BinNumberVectorType;
  typedef std::vector<InputVectorType>				 		 OutputVectorType;

  /** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(VectorSorter, itk::LightObject);
  
  void SetInputVector(const InputVectorType input);
  void SetBinNumberVector(const BinNumberVectorType binvector);
  
  void Sort();
  
  const OutputVectorType & GetOutput();
  
  BinNumberType GetMaximumBinNumber();

protected:
  VectorSorter();
  ~VectorSorter();

private:
  //purposely not implemented
  VectorSorter(const Self&);
  void operator=(const Self&);
  
  InputVectorType				m_DataVector;
  BinNumberVectorType			m_BinNumberVector;
  OutputVectorType				m_OutputVector;

}; // end of class

} // end namespace rtk

#ifndef ITK_MANUAL_INSTANTIATION
#include "rtkVectorSorter.txx"
#endif

#endif
