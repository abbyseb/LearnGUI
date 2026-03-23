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
 *	The Att geometry format basically inherits the Varian OBI structure.
 *	The Att XML file has the same format as the Varian XML file.
 *	However, in the att header, there are two additional fields,
 *	corresopnding to the x and y offsets.
 *	This enables one to work with both Varian and Elekta data in the form
 *	of Att geometry.
 *=========================================================================*/
 
#ifndef __rtkAttGeometryReader_h
#define __rtkAttGeometryReader_h

#include <itkLightProcessObject.h>
#include "rtkThreeDCircularProjectionGeometry.h"

namespace rtk
{

/** \class AttGeometryReader
 *
 * Creates a 3D circular geometry from Att format data. 
 *
 *
 * \author Simon Rit
 *
 * \ingroup IOFilters
 */
class IGT_EXPORT AttGeometryReader :
  public itk::LightProcessObject
{
public:
  /** Standard typedefs */
  typedef AttGeometryReader Self;
  typedef itk::LightProcessObject Superclass;
  typedef itk::SmartPointer<Self> Pointer;

  /** Convenient typedefs */
  typedef ThreeDCircularProjectionGeometry GeometryType;
  typedef std::vector<std::string>         FileNamesContainer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(AttGeometryReader, LightProcessObject);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Get the pointer to the generated geometry object. */
  itkGetMacro(Geometry, GeometryType::Pointer);

  /** Set the path to the Att xml file containing geometric information. */
  itkGetMacro(XMLFileName, std::string);
  itkSetMacro(XMLFileName, std::string);

  /** Set the vector of strings that contains the projection file names. Files
   * are processed in sequential order. */
  void SetProjectionsFileNames (const FileNamesContainer &name)
    {
    if ( m_ProjectionsFileNames != name)
      {
      m_ProjectionsFileNames = name;
      this->Modified();
      }
    }
  const FileNamesContainer & GetProjectionsFileNames() const
    {
    return m_ProjectionsFileNames;
    }

protected:
  AttGeometryReader();


private:
  //purposely not implemented
  AttGeometryReader(const Self&);
  void operator=(const Self&);

  virtual void GenerateData();

  GeometryType::Pointer m_Geometry;
  std::string           m_XMLFileName;
  FileNamesContainer    m_ProjectionsFileNames;
};

}
#endif
