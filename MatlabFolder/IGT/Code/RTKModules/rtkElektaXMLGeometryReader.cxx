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


#include "rtkElektaXMLGeometryReader.h"
#include "rtkElektaXMLFileReader.h"
#include <itksys/SystemTools.hxx>

rtk::ElektaXMLGeometryReader
::ElektaXMLGeometryReader():
  m_Geometry(NULL),
  m_DicomUID(""),
  m_XMLFileName(""),
  m_SID(1000.),
  m_SDD(1536.),
  m_OffsetX0(0.),
  m_OffsetY0(0.),
  ANG_0(90.)
{
}

void
rtk::ElektaXMLGeometryReader
::GenerateData()
{
  // Create new RTK geometry object
  m_Geometry = GeometryType::New();

  // Read Elekta XML file
  rtk::ElektaXMLFileReader::Pointer xmlReader;
  xmlReader = rtk::ElektaXMLFileReader::New();
  xmlReader->SetFilename(m_XMLFileName);
  xmlReader->GenerateOutputInformation();
  
  unsigned int NProj = xmlReader->GetGantryAngles().size();
  
  // Andy Shieh 2015
  m_SID = xmlReader->GetSID();
  m_SDD = xmlReader->GetSDD();
  
  // Short scan or not
  if( std::fabs(xmlReader->GetUCenters()[0]) > 10)
    m_Geometry->SetIsShortScan( false );
	
  // Projection matrices
  for(unsigned int noProj=0; noProj<NProj; noProj++)
    {
	double angle = xmlReader->GetGantryAngles()[noProj] + ANG_0;
	angle = angle - 360*floor(angle/360.); // between -360 and 360
    if(angle<0) angle += 360;         // between 0    and 360
    m_Geometry->AddProjection(m_SID,
                              m_SDD,
                              angle,
                              m_OffsetX0 - xmlReader->GetUCenters()[noProj],
                              m_OffsetY0 + xmlReader->GetVCenters()[noProj]);
    }
  m_Geometry->ChangeShortScanAngle();
}
