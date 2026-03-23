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


#include "rtkElektaSynergyGeometryReader.h"
#include "rtkDbf.h"
#include <itksys/SystemTools.hxx>

rtk::ElektaSynergyGeometryReader
::ElektaSynergyGeometryReader():
  m_Geometry(NULL),
  m_DicomUID(""),
  m_ImageDbfFileName("IMAGE.DBF"),
  m_FrameDbfFileName("FRAME.DBF"),
  m_XMLFileName(""),
  m_SID(1000.),
  m_SDD(1536.),
  m_OffsetX0(0.),
  m_OffsetY0(0.),
  ANG_0(90.)
{
}

std::string
rtk::ElektaSynergyGeometryReader
::GetImageIDFromDicomUID()
{
  // Open frame database file
  rtk::DbfFile dbImage(m_ImageDbfFileName);
  if( !dbImage.is_open() )
    itkGenericExceptionMacro( << "Couldn't open " 
                              << m_ImageDbfFileName);

  // Search for correct record
  bool bReadOk;
  do {
    bReadOk = dbImage.ReadNextRecord();
    }
  while(bReadOk && std::string(m_DicomUID) != dbImage.GetFieldAsString("DICOM_UID") );

  // Error message if not found
  if(!bReadOk)
    {
    itkGenericExceptionMacro( << "Couldn't find acquisition with DICOM_UID "
                              << m_DicomUID
                              << " in table "
                              << m_ImageDbfFileName );
    }

  return dbImage.GetFieldAsString("DBID");
}

void
rtk::ElektaSynergyGeometryReader
::GetProjInfoFromDB(const std::string &imageID,
                    std::vector<float> &projAngle,
                    std::vector<float> &projFlexX,
                    std::vector<float> &projFlexY)
{
  // Open frame database file
  rtk::DbfFile dbFrame(m_FrameDbfFileName);
  if( !dbFrame.is_open() )
    itkGenericExceptionMacro( << "Couldn't open " 
                              << m_FrameDbfFileName);

  // Go through the database, select correct records and get data
  while( dbFrame.ReadNextRecord() )
    {
    if(dbFrame.GetFieldAsString("IMA_DBID") == imageID)
      {
      projAngle.push_back(dbFrame.GetFieldAsDouble("PROJ_ANG") );
      projFlexX.push_back(dbFrame.GetFieldAsDouble("U_CENTRE") );
      projFlexY.push_back(dbFrame.GetFieldAsDouble("V_CENTRE") );
      }
    }
}

void
rtk::ElektaSynergyGeometryReader
::GetGeoInfoFromXML( )
{
  // Read Att XML file (for common geometric information)
  rtk::AttXMLFileReader::Pointer xmlReader = rtk::AttXMLFileReader::New();
  xmlReader->SetFilename(m_XMLFileName);
  xmlReader->GenerateOutputInformation();

  // Constants used to generate projection matrices
  itk::MetaDataDictionary &dic = *(xmlReader->GetOutputObject() );
  typedef itk::MetaDataObject< double > MetaDataDoubleType;
  m_SDD = dynamic_cast<MetaDataDoubleType *>(dic["CalibratedSID"].GetPointer() )->GetMetaDataObjectValue();
  m_SID = dynamic_cast<MetaDataDoubleType *>(dic["CalibratedSAD"].GetPointer() )->GetMetaDataObjectValue();

  typedef itk::MetaDataObject< std::string > MetaDataStringType;
  std::string fanType = dynamic_cast<const MetaDataStringType *>(dic["FanType"].GetPointer() )->GetMetaDataObjectValue();
  if(itksys::SystemTools::Strucmp(fanType.c_str(), "HalfFan") == 0)
    {
	m_Geometry->SetIsShortScan( false );
    // Half Fan (offset detector), get lateral offset from XML file
    m_OffsetX0 =
      dynamic_cast<MetaDataDoubleType *>(dic["CalibratedDetectorOffsetX"].GetPointer() )->GetMetaDataObjectValue() +
      dynamic_cast<MetaDataDoubleType *>(dic["DetectorPosLat"].GetPointer() )->GetMetaDataObjectValue();
    }
  else
    {
	m_Geometry->SetIsShortScan( true );
    // Full Fan (centered detector)
    m_OffsetX0 =
      dynamic_cast<MetaDataDoubleType *>(dic["CalibratedDetectorOffsetX"].GetPointer() )->GetMetaDataObjectValue();
    }
  double m_OffsetY0 =
    dynamic_cast<MetaDataDoubleType *>(dic["CalibratedDetectorOffsetY"].GetPointer() )->GetMetaDataObjectValue();
}

void
rtk::ElektaSynergyGeometryReader
::GenerateData()
{
  // Create new RTK geometry object
  m_Geometry = GeometryType::New();

  // Get information from Synergy database
  std::vector<float> projAngle, projFlexX, projFlexY;
  GetProjInfoFromDB( GetImageIDFromDicomUID(), projAngle, projFlexX, projFlexY);
  
  // If XML file provided, get geometry information from it
  if(!m_XMLFileName.empty())
	this->GetGeoInfoFromXML();
	
  // Projection matrices
  for(unsigned int noProj=0; noProj<projAngle.size(); noProj++)
    {
	double angle = projAngle[noProj] + ANG_0;
	angle = angle - 360*floor(angle/360.); // between -360 and 360
    if(angle<0) angle += 360;         // between 0    and 360
    m_Geometry->AddProjection(m_SID,
                            m_SDD,
                            angle,
                            -projFlexX[noProj] + m_OffsetX0,
                            -projFlexY[noProj] + m_OffsetY0);
    }
  m_Geometry->ChangeShortScanAngle();
}
