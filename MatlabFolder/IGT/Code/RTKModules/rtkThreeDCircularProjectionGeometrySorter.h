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

#ifndef __rtkThreeDCircularProjectionGeometrySorter_h
#define __rtkThreeDCircularProjectionGeometrySorter_h

#include "rtkThreeDCircularProjectionGeometry.h"

#include <itkLightObject.h>

#include <vector>

/** \class ThreeDCircularProjectionGeometrySorter
 * \brief Sort rtkThreeDCircularProjectionGeometry into different bins using
 *		  an input bin number vector. This object outputs a vector of 
 *		  rtkThreeDCircularProjectionGeometry objects.
 *
 * \author Andy Shieh
 *
 * \ingroup itkLightObject
 */
namespace rtk
{

class ITK_EXPORT ThreeDCircularProjectionGeometrySorter :
  public itk::LightObject
{
public:
  /** Standard class typedefs. */
  typedef ThreeDCircularProjectionGeometrySorter             Self;
  typedef itk::LightObject								     Superclass;
  typedef itk::SmartPointer<Self>                            Pointer;
  typedef itk::SmartPointer<const Self>                      ConstPointer;

  /** Some convenient typedefs. */
  typedef ThreeDCircularProjectionGeometry              	 GeometryType;
  typedef GeometryType::Pointer            					 GeometryPointer;
  typedef unsigned int                       				 BinNumberType;
  typedef std::vector<GeometryPointer>		 		 		 GeometryVectorType;
  typedef std::vector<BinNumberType>						 BinNumberVectorType;

  /** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(ThreeDCircularProjectionGeometrySorter, itk::LightObject);
  
  void SetInputGeometry(GeometryPointer geometry);
  void SetBinNumberVector(const BinNumberVectorType binvector);
  
  void Sort();
  
  std::vector<GeometryPointer> GetOutput();

protected:
  ThreeDCircularProjectionGeometrySorter();
  ~ThreeDCircularProjectionGeometrySorter();

private:
  //purposely not implemented
  ThreeDCircularProjectionGeometrySorter(const Self&);
  void operator=(const Self&);
  
  GeometryPointer				m_InputGeometry;
  BinNumberVectorType			m_BinNumberVector;
  GeometryVectorType			m_OutputGeometryVector;
  
  BinNumberType GetMaximumBinNumber();

}; // end of class

} // end namespace rtk

#ifndef ITK_MANUAL_INSTANTIATION
#include "rtkThreeDCircularProjectionGeometrySorter.txx"
#endif

#endif
