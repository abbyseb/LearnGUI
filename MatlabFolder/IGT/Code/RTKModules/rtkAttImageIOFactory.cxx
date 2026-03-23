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

#include "rtkAttImageIOFactory.h"

#include <fstream>

//====================================================================
rtk::AttImageIOFactory::AttImageIOFactory()
{
  this->RegisterOverride("itkImageIOBase",
                         "AttImageIO",
                         "Htt Image IO",
                         1,
                         itk::CreateObjectFunction<AttImageIO>::New() );
}
