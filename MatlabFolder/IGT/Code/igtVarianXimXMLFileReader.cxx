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

#include "igtVarianXimXMLFileReader.h"
#include "itkMacro.h"

#include <itksys/SystemTools.hxx>
#include <itkMetaDataObject.h>

namespace igt
{

int
VarianXimXMLFileReader::
CanReadFile(const char *name)
{
  if(!itksys::SystemTools::FileExists(name) ||
     itksys::SystemTools::FileIsDirectory(name) ||
     itksys::SystemTools::FileLength(name) == 0)
    return 0;
  return 1;
}

void
VarianXimXMLFileReader::
StartElement(const char * itkNotUsed(name),const char ** itkNotUsed(atts))
{
  m_CurCharacterData = "";
}

void
VarianXimXMLFileReader::
EndElement(const char *name)
{
  std::string name_str(name);
  encaplulateMetadataDouble("StartAngle", name_str);
  encaplulateMetadataDouble("StopAngle", name_str);
  encaplulateMetadataDouble("Velocity", name_str);
  encaplulateMetadataDouble("FrameRate", name_str);
  encaplulateMetadataDouble("SID", name_str);
  encaplulateMetadataDouble("SAD", name_str);
  encaplulateMetadataDouble("ImagerLat", name_str);
  encaplulateMetadataDouble("ImagerSizeX", name_str);
  encaplulateMetadataDouble("ImagerSizeY", name_str);
  encaplulateMetadataDouble("ImagerResX", name_str);
  encaplulateMetadataDouble("ImagerResY", name_str);
  encaplulateMetadataString("Fan", name_str);
  encaplulateMetadataString("Trajectory Type", name_str);
}

void
VarianXimXMLFileReader::
encaplulateMetadataDouble(std::string metaName, std::string name_str)
{
	if (name_str.compare(metaName) == 0)
	{
		double d = atof(m_CurCharacterData.c_str());
		itk::EncapsulateMetaData<double>(m_Dictionary, metaName, d);
	}
}

void
VarianXimXMLFileReader::
encaplulateMetadataString(std::string metaName, std::string name_str)
{
	if (name_str.compare(metaName) == 0)
	{
		itk::EncapsulateMetaData<std::string>(m_Dictionary, metaName, m_CurCharacterData);
	}
}


void
VarianXimXMLFileReader::
CharacterDataHandler(const char *inData, int inLength)
{
  for(int i = 0; i < inLength; i++)
    m_CurCharacterData = m_CurCharacterData + inData[i];
}

}
