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

#ifndef __igtVarianXimGeometryReader_h
#define __igtVarianXimGeometryReader_h

#include <itkLightProcessObject.h>
#include "rtkThreeDCircularProjectionGeometry.h"

namespace igt
{

/** \class VarianXimGeometryReader
 *
 * Creates a 3D circular geometry from Varian XIM data (Truebeam). 
 *
 * \author Andy Shieh
 *
 * \ingroup IOFilters
 */
class IGT_EXPORT VarianXimGeometryReader :
  public itk::LightProcessObject
{
public:
  /** Standard typedefs */
  typedef VarianXimGeometryReader Self;
  typedef itk::LightProcessObject Superclass;
  typedef itk::SmartPointer<Self> Pointer;

  /** Convenient typedefs */
  typedef rtk::ThreeDCircularProjectionGeometry GeometryType;
  typedef std::vector<std::string>         FileNamesContainer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(VarianXimGeometryReader, LightProcessObject);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Get the pointer to the generated geometry object. */
  itkGetMacro(Geometry, GeometryType::Pointer);

  /** Set the path to the Varian OBI xml file containing geometric information. */
  itkGetMacro(XMLFileName, std::string);
  itkSetMacro(XMLFileName, std::string);

  /** Set angle values to override */
  void SetAnglesToOverride(std::vector<double> angles)
  {
	  m_AnglesToOverride = angles;
	  m_areAnglesOverriden = true;
  }

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
  VarianXimGeometryReader();


private:
  //purposely not implemented
  VarianXimGeometryReader(const Self&);
  void operator=(const Self&);

  virtual void GenerateData();

  GeometryType::Pointer m_Geometry;
  std::string           m_XMLFileName;
  FileNamesContainer    m_ProjectionsFileNames;
  bool					m_areAnglesOverriden;
  std::vector<double>	m_AnglesToOverride;
};

}
#endif
