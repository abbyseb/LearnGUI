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
 
 /*=========================================================================
 *	Author: Andy Shieh, School of Physics, The University of Sydney
 *	Purpose:
 *	Incorporate the IO of ATT projection format into RTK.
 *	The ATT format contains 512 byte of header information, 
 *	with the same structure as HNC header.
 *	The image pixel values are stored in terms of attenuation values, 
 *	with single (float) precision (4 bytes).
 *=========================================================================*/

#ifndef __rtkAttImageIOFactory_h
#define __rtkAttImageIOFactory_h

#include "rtkAttImageIO.h"

// itk include
#include "itkImageIOBase.h"
#include "itkObjectFactoryBase.h"
#include "itkVersion.h"

namespace rtk
{

/** \class AttImageIOFactory
 *
 * Factory for Att files
 *(512 header like in hnc, and image consists of attenuation values in single precision).
 *
 * \author Andy Shieh, School of Physics, The University of Sydney
 */
class AttImageIOFactory : public itk::ObjectFactoryBase
{
public:
  /** Standard class typedefs. */
  typedef AttImageIOFactory             Self;
  typedef itk::ObjectFactoryBase        Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Class methods used to interface with the registered factories. */
  const char* GetITKSourceVersion(void) const {
    return ITK_SOURCE_VERSION;
  }

  const char* GetDescription(void) const {
    return "Att ImageIO Factory, allows the loading of Att images into insight";
  }

  /** Method for class instantiation. */
  itkFactorylessNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(AttImageIOFactory, ObjectFactoryBase);

  /** Register one factory of this type  */
  static void RegisterOneFactory(void) {
    ObjectFactoryBase::RegisterFactory( Self::New() );
  }

protected:
  AttImageIOFactory();
  ~AttImageIOFactory() {};
  typedef AttImageIOFactory myProductType;
  const myProductType* m_MyProduct;

private:
  AttImageIOFactory(const Self&); //purposely not implemented
  void operator=(const Self&);    //purposely not implemented

};

} // end namespace

#endif // __rtkAttImageIOFactory_h
