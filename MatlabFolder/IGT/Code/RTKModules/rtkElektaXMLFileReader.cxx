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

#include "rtkElektaXMLFileReader.h"
#include "itkMacro.h"

#include <itksys/SystemTools.hxx>
#include <itkMetaDataObject.h>

namespace rtk
{

int
ElektaXMLFileReader::
CanReadFile(const char *name)
{
  if(!itksys::SystemTools::FileExists(name) ||
     itksys::SystemTools::FileIsDirectory(name) ||
     itksys::SystemTools::FileLength(name) == 0)
    return 0;
  return 1;
}

void
ElektaXMLFileReader::
StartElement(const char * itkNotUsed(name),const char ** itkNotUsed(atts))
{
  m_CurCharacterData = "";
}

void
ElektaXMLFileReader::
EndElement(const char *name)
{
#define ENCAPLULATE_META_DATA_DOUBLE(metaName) \
  if(itksys::SystemTools::Strucmp(name, metaName) == 0) { \
    double d = atof(m_CurCharacterData.c_str() ); \
    itk::EncapsulateMetaData<double>(m_Dictionary, metaName, d); \
    } \
  else \
    itk::EncapsulateMetaData<double>(m_Dictionary, metaName, 0.);

#define ENCAPLULATE_META_DATA_STRING(metaName) \
  if(itksys::SystemTools::Strucmp(name, metaName) == 0) { \
    itk::EncapsulateMetaData<std::string>(m_Dictionary, metaName, m_CurCharacterData); \
    }

  ENCAPLULATE_META_DATA_DOUBLE("Width");
  ENCAPLULATE_META_DATA_DOUBLE("Height");
  ENCAPLULATE_META_DATA_DOUBLE("Depth");
  ENCAPLULATE_META_DATA_STRING("DicomUID");
  ENCAPLULATE_META_DATA_DOUBLE("AbsoluteTableLatPosIEC1217_MM");
  ENCAPLULATE_META_DATA_DOUBLE("AbsoluteTableLongPosIEC1217_MM");
  ENCAPLULATE_META_DATA_DOUBLE("AbsoluteTableVertPosIEC1217_MM");
  
  if(itksys::SystemTools::Strucmp(name, "SID") == 0)
    m_SID = atof(this->m_CurCharacterData.c_str() );

  if(itksys::SystemTools::Strucmp(name, "SDD") == 0)
    m_SDD = atof(this->m_CurCharacterData.c_str() );
  
  if(itksys::SystemTools::Strucmp(name, "GantryAngle") == 0)
    m_GantryAngles.push_back( atof(this->m_CurCharacterData.c_str() ) );
	
  if(itksys::SystemTools::Strucmp(name, "UCentre") == 0)
    m_UCenters.push_back( atof(this->m_CurCharacterData.c_str() ) );
		
  if(itksys::SystemTools::Strucmp(name, "VCentre") == 0)
    m_VCenters.push_back( atof(this->m_CurCharacterData.c_str() ) );
}

void
ElektaXMLFileReader::
CharacterDataHandler(const char *inData, int inLength)
{
  for(int i = 0; i < inLength; i++)
    m_CurCharacterData = m_CurCharacterData + inData[i];
}

}
