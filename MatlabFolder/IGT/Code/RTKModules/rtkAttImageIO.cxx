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

// std include
#include <stdio.h>

#include "rtkAttImageIO.h"
#include "itkMetaDataObject.h"

static std::string
GetExtension(const std::string & filename)
{
  std::string fileExt( itksys::SystemTools::GetFilenameLastExtension(filename) );

  //Check that a valid extension was found.
  if ( fileExt != ".att" && fileExt != ".Att" && fileExt != ".ATT" )
    {
    return ( "" );
    }
  return ( fileExt );
}

//--------------------------------------------------------------------
// Read Image Information
void rtk::AttImageIO::ReadImageInformation()
{
  FILE *     fp;

  fp = fopen (m_FileName.c_str(), "rb");
  if (fp == NULL)
    itkGenericExceptionMacro(<< "Could not open file (for reading): " << m_FileName);

  const std::string fileExt = GetExtension(m_FileName);
  
  if ( fileExt == ".att" || fileExt == ".Att" || fileExt == ".ATT" )
    {
	  fread ( (void *) m_Header.sFileType, sizeof(char), 32, fp);
	  fread ( (void *) &m_Header.FileLength, sizeof(unsigned int), 1, fp);
	  fread ( (void *) m_Header.sChecksumSpec, sizeof(char), 4, fp);
	  fread ( (void *) &m_Header.nCheckSum, sizeof(unsigned int), 1, fp);
	  fread ( (void *) m_Header.sCreationDate, sizeof(char), 8, fp);
	  fread ( (void *) m_Header.sCreationTime, sizeof(char), 8, fp);
	  fread ( (void *) m_Header.sPatientID, sizeof(char), 16, fp);
	  fread ( (void *) &m_Header.nPatientSer, sizeof(unsigned int), 1, fp);
	  fread ( (void *) m_Header.sSeriesID, sizeof(char), 16, fp);
	  fread ( (void *) &m_Header.nSeriesSer, sizeof(unsigned int), 1, fp);
	  fread ( (void *) m_Header.sSliceID, sizeof(char), 16, fp);
	  fread ( (void *) &m_Header.nSliceSer, sizeof(unsigned int), 1, fp);
	  fread ( (void *) &m_Header.SizeX, sizeof(unsigned int), 1, fp);
	  fread ( (void *) &m_Header.SizeY, sizeof(unsigned int), 1, fp);
	  fread ( (void *) &m_Header.dSliceZPos, sizeof(double), 1, fp);
	  fread ( (void *) m_Header.sModality, sizeof(char), 16, fp);
	  fread ( (void *) &m_Header.nWindow, sizeof(unsigned int), 1, fp);
	  fread ( (void *) &m_Header.nLevel, sizeof(unsigned int), 1, fp);
	  fread ( (void *) &m_Header.nPixelOffset, sizeof(unsigned int), 1, fp);
	  fread ( (void *) m_Header.sImageType, sizeof(char), 4, fp);
	  fread ( (void *) &m_Header.dGantryRtn, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dSAD, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dSFD, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dCollX1, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dCollX2, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dCollY1, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dCollY2, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dCollRtn, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dFieldX, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dFieldY, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dBladeX1, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dBladeX2, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dBladeY1, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dBladeY2, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dIDUPosLng, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dIDUPosLat, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dIDUPosVrt, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dIDUPosRtn, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dPatientSupportAngle, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dTableTopEccentricAngle, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dCouchVrt, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dCouchLng, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dCouchLat, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dIDUResolutionX, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dIDUResolutionY, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dImageResolutionX, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dImageResolutionY, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dEnergy, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dDoseRate, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dXRayKV, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dXRayMA, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dMetersetExposure, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dAcqAdjustment, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dCTProjectionAngle, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dCTNormChamber, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dGatingTimeTag, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dGating4DInfoX, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dGating4DInfoY, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dGating4DInfoZ, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dGating4DInfoTime, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dOffsetX, sizeof(double), 1, fp);
	  fread ( (void *) &m_Header.dOffsetY, sizeof(double), 1, fp);
	  fclose (fp);
    }
  else
    itkGenericExceptionMacro(<< "Could not read file: " << m_FileName);
	
  /* Convert att to ITK image information */
  SetNumberOfDimensions(2);
  SetDimensions(0, m_Header.SizeX);
  SetDimensions(1, m_Header.SizeY);
  SetSpacing(0, m_Header.dIDUResolutionX);
  SetSpacing(1, m_Header.dIDUResolutionY);
  SetOrigin(0, -0.5*(m_Header.SizeX-1)*m_Header.dIDUResolutionX); //SR: assumed centered
  SetOrigin(1, -0.5*(m_Header.SizeY-1)*m_Header.dIDUResolutionY); //SR: assumed centered
  SetComponentType(itk::ImageIOBase::FLOAT);

  /* Store important meta information in the meta data dictionary */
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dCTProjectionAngle", m_Header.dCTProjectionAngle);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dOffsetX", m_Header.dOffsetX);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dOffsetY", m_Header.dOffsetY);
}

//--------------------------------------------------------------------
bool rtk::AttImageIO::CanReadFile(const char* FileNameToRead)
{
  std::string filename(FileNameToRead);
  std::string fileExt( itksys::SystemTools::GetFilenameLastExtension(filename) );

  //Check that a valid extension was found.
  if ( fileExt != ".att" && fileExt != ".Att" && fileExt != ".ATT" )
    {
    return false;
    }

  return true;
}

//--------------------------------------------------------------------
// Read Image Content
void rtk::AttImageIO::Read(void * buffer)
{
  FILE *     fp;

  fp = fopen (m_FileName.c_str(), "rb");
  if (fp == NULL)
    {
    itkGenericExceptionMacro(<< "Could not open file (for reading): " << m_FileName);
    return;
    }
	
  const std::string fileExt = GetExtension(m_FileName);
  
  if ( fileExt == ".att" || fileExt == ".Att" || fileExt == ".ATT" )
    {
	  fseek(fp, 512, SEEK_SET);
	  fread(buffer, sizeof(float), GetDimensions(0)*GetDimensions(1), fp);
    }

/*  file.seekg(512, std::ios::beg);
  if( !this->ReadBufferAsBinary( file, buffer, sizeof(unsigned short int) * GetDimensions(0) * GetDimensions(1)) )
    {
    itkExceptionMacro(<< "Could not read file: " << m_FileName);
    file.close();
    return;
    }
*/
  fclose (fp);
  return;

}

//--------------------------------------------------------------------
bool rtk::AttImageIO::CanWriteFile(const char* FileNameToWrite)
{
  return false;
}

//--------------------------------------------------------------------
// Write Image
void rtk::AttImageIO::Write(const void* buffer)
{
  //TODO?
}
