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


#include "rtkVarianObiGeometryReader.h"
#include "rtkVarianObiXMLFileReader.h"
#include "rtkHndImageIOFactory.h"
#include "rtkHncImageIOFactory.h"

#include <itkImageFileReader.h>
#include <itksys/SystemTools.hxx>

#ifndef M_PI
#define M_PI vnl_math::pi
#endif

rtk::VarianObiGeometryReader
::VarianObiGeometryReader():
  m_Geometry(NULL)
{
}

void
rtk::VarianObiGeometryReader
::GenerateData()
{
  // Create new RTK geometry object
  m_Geometry = GeometryType::New();

  // Read Varian XML file (for common geometric information)
  rtk::VarianObiXMLFileReader::Pointer obiXmlReader;
  obiXmlReader = rtk::VarianObiXMLFileReader::New();
  obiXmlReader->SetFilename(m_XMLFileName);
  obiXmlReader->GenerateOutputInformation();

  // Constants used to generate projection matrices
  itk::MetaDataDictionary &dic = *(obiXmlReader->GetOutputObject() );
  typedef itk::MetaDataObject< double > MetaDataDoubleType;
  const double sdd = dynamic_cast<MetaDataDoubleType *>(dic["CalibratedSID"].GetPointer() )->GetMetaDataObjectValue();
  const double sid = dynamic_cast<MetaDataDoubleType *>(dic["CalibratedSAD"].GetPointer() )->GetMetaDataObjectValue();
  
  // Andy Shieh 2016 - To include detector sag correction
  double kVOffsetX1 = 0, kVOffsetX2 = 0, kVOffsetX3 = 0, kVOffsetY1 = 0, kVOffsetY2 = 0, kVOffsetY3 = 0;
  if (dic.HasKey("kVOffsetX1"))
	  kVOffsetX1 = dynamic_cast<MetaDataDoubleType *>(dic["kVOffsetX1"].GetPointer())->GetMetaDataObjectValue();
  if (dic.HasKey("kVOffsetX2"))
	  kVOffsetX1 = dynamic_cast<MetaDataDoubleType *>(dic["kVOffsetX2"].GetPointer())->GetMetaDataObjectValue();
  if (dic.HasKey("kVOffsetX3"))
	  kVOffsetX1 = dynamic_cast<MetaDataDoubleType *>(dic["kVOffsetX3"].GetPointer())->GetMetaDataObjectValue();
  if (dic.HasKey("kVOffsetY1"))
	  kVOffsetX1 = dynamic_cast<MetaDataDoubleType *>(dic["kVOffsetY1"].GetPointer())->GetMetaDataObjectValue();
  if (dic.HasKey("kVOffsetY2"))
	  kVOffsetX1 = dynamic_cast<MetaDataDoubleType *>(dic["kVOffsetY2"].GetPointer())->GetMetaDataObjectValue();
  if (dic.HasKey("kVOffsetY3"))
	  kVOffsetX1 = dynamic_cast<MetaDataDoubleType *>(dic["kVOffsetY3"].GetPointer())->GetMetaDataObjectValue();

  typedef itk::MetaDataObject< std::string > MetaDataStringType;
  double offsetx;
  std::string fanType = dynamic_cast<const MetaDataStringType *>(dic["FanType"].GetPointer() )->GetMetaDataObjectValue();
  if(itksys::SystemTools::Strucmp(fanType.c_str(), "HalfFan") == 0)
    {
	m_Geometry->SetIsShortScan( false );
    // Half Fan (offset detector), get lateral offset from XML file
    offsetx =
      dynamic_cast<MetaDataDoubleType *>(dic["CalibratedDetectorOffsetX"].GetPointer() )->GetMetaDataObjectValue() +
	  dynamic_cast<MetaDataDoubleType *>(dic["DetectorPosLat"].GetPointer())->GetMetaDataObjectValue();
    }
  else
    {
	m_Geometry->SetIsShortScan( true );
    // Full Fan (centered detector)
    offsetx =
      dynamic_cast<MetaDataDoubleType *>(dic["CalibratedDetectorOffsetX"].GetPointer() )->GetMetaDataObjectValue();
    }
  const double offsety =
    dynamic_cast<MetaDataDoubleType *>(dic["CalibratedDetectorOffsetY"].GetPointer() )->GetMetaDataObjectValue();

  // Projections reader (for angle)
  rtk::HndImageIOFactory::RegisterOneFactory();
  rtk::HncImageIOFactory::RegisterOneFactory();

  // Projection matrices
  for(unsigned int noProj=0; noProj<m_ProjectionsFileNames.size(); noProj++)
    {
    typedef unsigned int                    InputPixelType;
    typedef itk::Image< InputPixelType, 2 > InputImageType;

    typedef itk::ImageFileReader< InputImageType > ReaderType;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName( m_ProjectionsFileNames[noProj] );
    reader->UpdateOutputInformation();

    const double angle =
      dynamic_cast<MetaDataDoubleType *>(reader->GetMetaDataDictionary()["dCTProjectionAngle"].GetPointer())->GetMetaDataObjectValue();

	double offsetx_sag, offsety_sag;
	offsetx_sag = offsetx + 
		kVOffsetX1 + kVOffsetX2 * sin((angle + kVOffsetX3) * M_PI / 180.);
	offsety_sag = offsety +
		kVOffsetY1 + kVOffsetY2 * sin((angle + kVOffsetY3) * M_PI / 180.);

	m_Geometry->AddProjection(sid, sdd, angle, offsetx_sag, offsety_sag);
    }
  m_Geometry->ChangeShortScanAngle();
}
