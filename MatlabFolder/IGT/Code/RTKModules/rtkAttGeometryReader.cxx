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


#include "rtkAttGeometryReader.h"
#include "rtkAttXMLFileReader.h"
#include "rtkAttImageIOFactory.h"

#include <itkImageFileReader.h>
#include <itksys/SystemTools.hxx>

rtk::AttGeometryReader
::AttGeometryReader():
  m_Geometry(NULL)
{
}

void
rtk::AttGeometryReader
::GenerateData()
{
  // Create new RTK geometry object
  m_Geometry = GeometryType::New();

  // Read Att XML file (for common geometric information)
  rtk::AttXMLFileReader::Pointer attXmlReader;
  attXmlReader = rtk::AttXMLFileReader::New();
  attXmlReader->SetFilename(m_XMLFileName);
  attXmlReader->GenerateOutputInformation();

  // Constants used to generate projection matrices
  itk::MetaDataDictionary &dic = *(attXmlReader->GetOutputObject() );
  typedef itk::MetaDataObject< double > MetaDataDoubleType;
  const double sdd = dynamic_cast<MetaDataDoubleType *>(dic["CalibratedSID"].GetPointer() )->GetMetaDataObjectValue();
  const double sid = dynamic_cast<MetaDataDoubleType *>(dic["CalibratedSAD"].GetPointer() )->GetMetaDataObjectValue();

  typedef itk::MetaDataObject< std::string > MetaDataStringType;
  double offsetx;
  std::string fanType = dynamic_cast<const MetaDataStringType *>(dic["FanType"].GetPointer() )->GetMetaDataObjectValue();
  if(itksys::SystemTools::Strucmp(fanType.c_str(), "HalfFan") == 0)
    {
    // Half Fan (offset detector), get lateral offset from XML file
    offsetx =
      dynamic_cast<MetaDataDoubleType *>(dic["CalibratedDetectorOffsetX"].GetPointer() )->GetMetaDataObjectValue() +
      dynamic_cast<MetaDataDoubleType *>(dic["DetectorPosLat"].GetPointer() )->GetMetaDataObjectValue();
    }
  else
    {
    // Full Fan (centered detector)
    offsetx =
      dynamic_cast<MetaDataDoubleType *>(dic["CalibratedDetectorOffsetX"].GetPointer() )->GetMetaDataObjectValue();
    }
  double offsety =
    dynamic_cast<MetaDataDoubleType *>(dic["CalibratedDetectorOffsetY"].GetPointer() )->GetMetaDataObjectValue();

  // Projections reader (for angle)
  rtk::AttImageIOFactory::RegisterOneFactory();

  // Projection matrices
  for(unsigned int noProj=0; noProj<m_ProjectionsFileNames.size(); noProj++)
    {
    typedef float                    InputPixelType;
    typedef itk::Image< InputPixelType, 2 > InputImageType;

    typedef itk::ImageFileReader< InputImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( m_ProjectionsFileNames[noProj] );
    reader->UpdateOutputInformation();

    const double angle =
      dynamic_cast<MetaDataDoubleType *>(reader->GetMetaDataDictionary()["dCTProjectionAngle"].GetPointer())->GetMetaDataObjectValue();
	  
	// Also reads in the OffsetX and OffsetY recorded in each projection image.
	// This allows one to work with varying offset values.
	// The offset values read from the projection files are on top of the values recorded in the xml file.
	
	double offsetx_proj =
      dynamic_cast<MetaDataDoubleType *>(reader->GetMetaDataDictionary()["dOffsetX"].GetPointer())->GetMetaDataObjectValue();
	  
	double offsety_proj =
      dynamic_cast<MetaDataDoubleType *>(reader->GetMetaDataDictionary()["dOffsetY"].GetPointer())->GetMetaDataObjectValue();

	offsetx = offsetx + offsetx_proj;
	offsety = offsety + offsetx_proj;
	  
    m_Geometry->AddProjection(sid, sdd, angle, offsetx, offsety);
    }
	m_Geometry->ChangeShortScanAngle();
}
