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


#include "igtVarianXimGeometryReader.h"
#include "igtVarianXimXMLFileReader.h"
#include "igtXimImageIOFactory.h"

#include <itkImageFileReader.h>
#include <itksys/SystemTools.hxx>

#ifndef M_PI
#define M_PI vnl_math::pi
#endif

igt::VarianXimGeometryReader
::VarianXimGeometryReader():
  m_Geometry(NULL)
{
	this->m_areAnglesOverriden = false;
	this->m_AnglesToOverride.clear();
}

void
igt::VarianXimGeometryReader
::GenerateData()
{
  // Create new RTK geometry object
  m_Geometry = GeometryType::New();

  // Read Varian XML file (for common geometric information)
  igt::VarianXimXMLFileReader::Pointer ximXmlReader;
  ximXmlReader = igt::VarianXimXMLFileReader::New();
  ximXmlReader->SetFilename(m_XMLFileName);
  ximXmlReader->GenerateOutputInformation();

  // Constants used to generate projection matrices
  itk::MetaDataDictionary &dic = *(ximXmlReader->GetOutputObject() );
  typedef itk::MetaDataObject< double > MetaDataDoubleType;
  double sdd = dynamic_cast<MetaDataDoubleType *>(dic["SID"].GetPointer() )->GetMetaDataObjectValue();
  double sid = dynamic_cast<MetaDataDoubleType *>(dic["SAD"].GetPointer() )->GetMetaDataObjectValue();

  double offsetx = dynamic_cast<MetaDataDoubleType *>(dic["ImagerLat"].GetPointer())->GetMetaDataObjectValue();
  double offsety = 0;

  typedef itk::MetaDataObject< std::string > MetaDataStringType;

  std::string fanType = dynamic_cast<const MetaDataStringType *>(dic["Fan"].GetPointer() )->GetMetaDataObjectValue();
  
  if(itksys::SystemTools::Strucmp(fanType.c_str(), "HALF") == 0)
	m_Geometry->SetIsShortScan( false );
  else
	m_Geometry->SetIsShortScan( true );

  // Projections reader (for angle)
  igt::XimImageIOFactory::RegisterOneFactory();

  // Projection matrices
  for(unsigned int noProj=0; noProj<m_ProjectionsFileNames.size(); noProj++)
    {
    typedef unsigned int                    InputPixelType;
    typedef itk::Image< InputPixelType, 2 > InputImageType;

    typedef itk::ImageFileReader< InputImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( m_ProjectionsFileNames[noProj] );
    reader->UpdateOutputInformation();

    double angle = 
      dynamic_cast<MetaDataDoubleType *>(reader->GetMetaDataDictionary()["dCTProjectionAngle"].GetPointer())->GetMetaDataObjectValue();

	if (m_areAnglesOverriden)
		angle = m_AnglesToOverride[noProj];

	//sid = 10. * dynamic_cast<MetaDataDoubleType *>(reader->GetMetaDataDictionary()["dkVSourceVrt"].GetPointer())->GetMetaDataObjectValue(); 
	//sdd = sid - 10. * dynamic_cast<MetaDataDoubleType *>(reader->GetMetaDataDictionary()["dkVSourceVrt"].GetPointer())->GetMetaDataObjectValue();
	offsetx = dynamic_cast<MetaDataDoubleType *>(reader->GetMetaDataDictionary()["dkVDetectorLat"].GetPointer())->GetMetaDataObjectValue();
	offsety = dynamic_cast<MetaDataDoubleType *>(reader->GetMetaDataDictionary()["dkVDetectorLng"].GetPointer())->GetMetaDataObjectValue();

	m_Geometry->AddProjection(sid, sdd, angle, offsetx, offsety);
    }
  m_Geometry->ChangeShortScanAngle();
}
