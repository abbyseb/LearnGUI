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

 #define HIS_HEADER_INFO_SIZE 68
 #define VARIAN_HEADER_INFO_SIZE 512

#ifndef __igtProjectionsReader_cxx
#define __igtProjectionsReader_cxx

#include "igtProjectionsReader.h"

#include <itkConfigure.h>
#include <itkTileImageFilter.h>
#include <itkCastImageFilter.h>

#include "rtkVarianObiRawImageFilter.h"
#include "rtkVarianObiRawHncImageFilter.h"
#include "igtVarianObiRawXimImageFilter.h"
#include "rtkElektaSynergyRawToAttenuationImageFilter.h"

namespace igt
{

template <class TOutputImage>
void ProjectionsReader<TOutputImage>
::ReadHncHndHeader(std::string filename)
{
  HncHnd_header header;
  FILE *     fp;

  fp = fopen (filename.c_str(), "rb");
  if (fp == NULL)
    itkGenericExceptionMacro(<< "Could not open file (for reading): " << filename);

  size_t nelements = 0;
  nelements += fread ( (void *) header.sFileType, sizeof(char), 32, fp);
  nelements += fread ( (void *) &header.FileLength, sizeof(itk::uint32_t), 1, fp);
  nelements += fread ( (void *) header.sChecksumSpec, sizeof(char), 4, fp);
  nelements += fread ( (void *) &header.nCheckSum, sizeof(itk::uint32_t), 1, fp);
  nelements += fread ( (void *) header.sCreationDate, sizeof(char), 8, fp);
  nelements += fread ( (void *) header.sCreationTime, sizeof(char), 8, fp);
  nelements += fread ( (void *) header.sPatientID, sizeof(char), 16, fp);
  nelements += fread ( (void *) &header.nPatientSer, sizeof(itk::uint32_t), 1, fp);
  nelements += fread ( (void *) header.sSeriesID, sizeof(char), 16, fp);
  nelements += fread ( (void *) &header.nSeriesSer, sizeof(itk::uint32_t), 1, fp);
  nelements += fread ( (void *) header.sSliceID, sizeof(char), 16, fp);
  nelements += fread ( (void *) &header.nSliceSer, sizeof(itk::uint32_t), 1, fp);
  nelements += fread ( (void *) &header.SizeX, sizeof(itk::uint32_t), 1, fp);
  nelements += fread ( (void *) &header.SizeY, sizeof(itk::uint32_t), 1, fp);
  nelements += fread ( (void *) &header.dSliceZPos, sizeof(double), 1, fp);
  nelements += fread ( (void *) header.sModality, sizeof(char), 16, fp);
  nelements += fread ( (void *) &header.nWindow, sizeof(itk::uint32_t), 1, fp);
  nelements += fread ( (void *) &header.nLevel, sizeof(itk::uint32_t), 1, fp);
  nelements += fread ( (void *) &header.nPixelOffset, sizeof(itk::uint32_t), 1, fp);
  nelements += fread ( (void *) header.sImageType, sizeof(char), 4, fp);
  nelements += fread ( (void *) &header.dGantryRtn, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dSAD, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dSFD, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dCollX1, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dCollX2, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dCollY1, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dCollY2, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dCollRtn, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dFieldX, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dFieldY, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dBladeX1, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dBladeX2, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dBladeY1, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dBladeY2, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dIDUPosLng, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dIDUPosLat, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dIDUPosVrt, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dIDUPosRtn, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dPatientSupportAngle, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dTableTopEccentricAngle, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dCouchVrt, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dCouchLng, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dCouchLat, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dIDUResolutionX, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dIDUResolutionY, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dImageResolutionX, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dImageResolutionY, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dEnergy, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dDoseRate, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dXRayKV, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dXRayMA, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dMetersetExposure, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dAcqAdjustment, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dCTProjectionAngle, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dCTNormChamber, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dGatingTimeTag, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dGating4DInfoX, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dGating4DInfoY, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dGating4DInfoZ, sizeof(double), 1, fp);
  nelements += fread ( (void *) &header.dGating4DInfoTime, sizeof(double), 1, fp);

  if(nelements != /*char*/120 + /*itk::uint32_t*/10 +  /*double*/41)
    itkGenericExceptionMacro(<< "Could not read header data in " << filename);

  if(fclose (fp) != 0)
    itkGenericExceptionMacro(<< "Could not close file: " << filename);

  m_Size[0] = header.SizeX;
  m_Size[1] = header.SizeY;
  if (header.dIDUResolutionX == 99999999)
	  m_Spacing[0] = 0.388;
  else
	  m_Spacing[0] = header.dIDUResolutionX;
  if (header.dIDUResolutionY == 99999999)
	  m_Spacing[1] = 0.388;
  else
	  m_Spacing[1] = header.dIDUResolutionY;
  m_Origin[0] = -0.5*(m_Size[0] - 1)*m_Spacing[0];
  m_Origin[1] = -0.5*(m_Size[1] - 1)*m_Spacing[1];

  m_AnglesDegree.push_back(header.dCTProjectionAngle);
  m_HeaderSize = VARIAN_HEADER_INFO_SIZE;

  ProjectionInfo projectionInfo;
  projectionInfo.width = header.SizeX;
  projectionInfo.height = header.SizeY;
  projectionInfo.spacingX = m_Spacing[0];
  projectionInfo.spacingY = m_Spacing[1];
  projectionInfo.sourceLat = 0;
  projectionInfo.sourceVrt = 0;
  projectionInfo.sourceLng = 0;
  projectionInfo.sourcePth = 0;
  projectionInfo.detectorLat = header.dIDUPosLat;
  projectionInfo.detectorVrt = header.dIDUPosVrt;
  projectionInfo.detectorLng = header.dIDUPosLng;
  projectionInfo.detectorPth = 0;
  projectionInfo.normChamberValue = header.dCTNormChamber;
  projectionInfo.timeTag = header.dGatingTimeTag;
  std::string fileExt(itksys::SystemTools::GetFilenameLastExtension(filename));
  projectionInfo.isCompressed = this->strcmpi(fileExt, ".hnd");
  projectionInfo.format = fileExt;
  projectionInfo.angle = header.dCTProjectionAngle;
  projectionInfo.headerSize = VARIAN_HEADER_INFO_SIZE;

  m_ProjectionInfos.push_back(projectionInfo);
}

template <class TOutputImage>
void ProjectionsReader<TOutputImage>
::ReadXimHeader(std::string filename)
{
	FILE *     fp;
	Xim_header ximHeader = Xim_header();

	fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
		itkGenericExceptionMacro(<< "Could not open file (for reading): " << filename);

	size_t nelements = 0;
	nelements += fread((void *)ximHeader.sFileFormatIdentifier, sizeof(char), 8, fp);
	nelements += fread((void *)&ximHeader.iFileFormatVersion, sizeof(int), 1, fp);
	nelements += fread((void *)&ximHeader.iImageWidth, sizeof(int), 1, fp);
	nelements += fread((void *)&ximHeader.iImageHeight, sizeof(int), 1, fp);
	nelements += fread((void *)&ximHeader.iBitsPerPixel, sizeof(int), 1, fp);
	nelements += fread((void *)&ximHeader.iBytesPerPixel, sizeof(int), 1, fp);
	nelements += fread((void *)&ximHeader.iCompressionIndicator, sizeof(int), 1, fp);
	nelements += fread((void *)&ximHeader.iLookupTableSize, sizeof(int), 1, fp);
	ximHeader.iLookupTablePosition = ftell(fp);

	if (ximHeader.iCompressionIndicator == 1)
	{
		fseek(fp, ximHeader.iLookupTableSize * sizeof(bool), SEEK_CUR);
		nelements += fread((void *)&ximHeader.iCompressedPixelBufferSize, sizeof(int), 1, fp);
		ximHeader.iImageBodyPosition = ftell(fp);
		fseek(fp, ximHeader.iCompressedPixelBufferSize, SEEK_CUR);
		nelements += fread((void *)&ximHeader.iUncompressedPixelBufferSize, sizeof(int), 1, fp);
	}
	else
	{
		nelements += fread((void *)&ximHeader.iUncompressedPixelBufferSize, sizeof(int), 1, fp);
		ximHeader.iImageBodyPosition = ftell(fp);
		fseek(fp, ximHeader.iUncompressedPixelBufferSize, SEEK_CUR);
	}

	// Histogram
	nelements += fread((void *)&ximHeader.iNumberOfBinsInHistogram, sizeof(int), 1, fp);
	if (ximHeader.iNumberOfBinsInHistogram > 0)
	{
		/*
		for (unsigned int k = 0; k != ximHeader.iNumberOfBinsInHistogram; ++k)
		{
		int histElement;
		nelements += fread((void *)&histElement, sizeof(int), 1, fp);
		ximHeader.viHistogramData.push_back(histElement);
		}
		*/
		// We don't really need to histogram
		// Maybe just skip it for now to save time
		fseek(fp, ximHeader.iNumberOfBinsInHistogram * sizeof(int), SEEK_CUR);
	}

	// Decode properties
	nelements += fread((void *)&ximHeader.iNumberOfProperties, sizeof(int), 1, fp);
	for (unsigned int k = 0; k != ximHeader.iNumberOfProperties; ++k)
	{
		int pNameLength;
		nelements += fread(&pNameLength, sizeof(int), 1, fp);
		char *cbuffer = (char*)malloc(pNameLength * sizeof(char));
		nelements += fread(cbuffer, sizeof(char), pNameLength, fp);
		std::string pName(cbuffer);
		pName = pName.substr(0, pNameLength);
		free(cbuffer);
		int pType;
		nelements += fread(&pType, sizeof(int), 1, fp);

		if (pType == 0) // Scalar int
		{
			int pValue_int;
			nelements += fread(&pValue_int, sizeof(int), 1, fp);
			ximHeader.vsScalarIntPropertyName.push_back(pName);
			ximHeader.viScalarIntPropertyValue.push_back(pValue_int);
			if (pName.compare("KVNormChamber") == 0)
				ximHeader.ikVNormChamber = pValue_int;
			else if (pName.compare("StartTime") == 0)
				ximHeader.iStartTime = pValue_int;
		}
		else if (pType == 1) // Scalar double
		{
			double pValue_double;
			nelements += fread(&pValue_double, sizeof(double), 1, fp);
			ximHeader.vsScalarDoublePropertyName.push_back(pName);
			ximHeader.vdScalarDoublePropertyValue.push_back(pValue_double);
			if (pName.compare("PixelWidth") == 0)
				ximHeader.dPixelWidth = pValue_double;
			else if (pName.compare("PixelHeight") == 0)
				ximHeader.dPixelHeight = pValue_double;
			else if (pName.compare("KVSourceRtn") == 0)
				ximHeader.dkVSourceRtn = pValue_double;
			else if (pName.compare("KVSourceVrt") == 0)
				ximHeader.dkVSourceVrt = pValue_double;
			else if (pName.compare("KVSourceLat") == 0)
				ximHeader.dkVSourceLat = pValue_double;
			else if (pName.compare("KVSourceLng") == 0)
				ximHeader.dkVSourceLng = pValue_double;
			else if (pName.compare("KVSourcePitch") == 0)
				ximHeader.dkVSourcePth = pValue_double;
			else if (pName.compare("KVDetectorRtn") == 0)
				ximHeader.dkVDetectorRtn = pValue_double;
			else if (pName.compare("KVDetectorVrt") == 0)
				ximHeader.dkVDetectorVrt = pValue_double;
			else if (pName.compare("KVDetectorLat") == 0)
				ximHeader.dkVDetectorLat = pValue_double;
			else if (pName.compare("KVDetectorLng") == 0)
				ximHeader.dkVDetectorLng = pValue_double;
			else if (pName.compare("KVDetectorPitch") == 0)
				ximHeader.dkVDetectorPth = pValue_double;
		}
		else if (pType == 2) // String
		{
			int pLength;
			nelements += fread(&pLength, sizeof(int), 1, fp);
			char *pbuffer = (char*)malloc(pLength * sizeof(char));
			nelements += fread(pbuffer, sizeof(char), pLength, fp);
			std::string pValue_str(pbuffer);
			pValue_str = pValue_str.substr(0, pLength);
			free(pbuffer);
			ximHeader.vsStringPropertyName.push_back(pName);
			ximHeader.vsStringPropertyValue.push_back(pValue_str);
		}
		else if (pType == 4) // Vector double
		{
			int pLength;
			nelements += fread(&pLength, sizeof(int), 1, fp);
			std::vector<double> pValue_vd;
			for (unsigned int kp = 0; kp != pLength / 8; ++kp)
			{
				double pElement;
				nelements += fread(&pElement, sizeof(double), 1, fp);
				pValue_vd.push_back(pElement);
			}
			ximHeader.vsVectorDoublePropertyName.push_back(pName);
			ximHeader.vvdVectorDoublePropertyValue.push_back(pValue_vd);
		}
		else if (pType == 5) // Vector int
		{
			int pLength;
			nelements += fread(&pLength, sizeof(int), 1, fp);
			std::vector<int> pValue_vi;
			for (unsigned int kp = 0; kp != pLength / 4; ++kp)
			{
				int pElement;
				nelements += fread(&pElement, sizeof(int), 1, fp);
				pValue_vi.push_back(pElement);
			}
			ximHeader.vsVectorIntPropertyName.push_back(pName);
			ximHeader.vviVectorIntPropertyValue.push_back(pValue_vi);
		}
		else
		{
			itkGenericExceptionMacro(<< "Property type " << pType << " not supported in " << filename);
		}
	}

	if (fclose(fp) != 0)
		itkGenericExceptionMacro(<< "Could not close file: " << filename);

	m_Size[0] = ximHeader.iImageWidth;
	m_Size[1] = ximHeader.iImageHeight;
	m_Spacing[0] = ximHeader.dPixelWidth * 10;
	m_Spacing[1] = ximHeader.dPixelHeight * 10;
	m_Origin[0] = -0.5*(ximHeader.iImageWidth - 1)*ximHeader.dPixelWidth;
	m_Origin[1] = -0.5*(ximHeader.iImageHeight - 1)*ximHeader.dPixelHeight;

	m_AnglesDegree.push_back(-1. * ximHeader.dkVSourceRtn + 180);
	m_HeaderSize = ximHeader.iImageBodyPosition;
	m_NormChamberValue = ximHeader.ikVNormChamber;
	m_isCompressed = ximHeader.iCompressionIndicator > 0;
	m_BytesPerPixel = ximHeader.iBytesPerPixel;
	m_LookupTableSize = ximHeader.iLookupTableSize;

	ProjectionInfo projectionInfo;
	projectionInfo.width = ximHeader.iImageWidth;
	projectionInfo.height = ximHeader.iImageHeight;
	projectionInfo.spacingX = ximHeader.dPixelWidth * 10;
	projectionInfo.spacingY = ximHeader.dPixelHeight * 10;
	projectionInfo.sourceLat = ximHeader.dkVSourceLat * 10;
	projectionInfo.sourceVrt = ximHeader.dkVSourceVrt * 10;
	projectionInfo.sourceLng = ximHeader.dkVSourceLng * 10;
	projectionInfo.sourcePth = ximHeader.dkVSourcePth;
	projectionInfo.detectorLat = ximHeader.dkVDetectorLat * 10;
	projectionInfo.detectorVrt = ximHeader.dkVDetectorVrt * 10;
	projectionInfo.detectorLng = ximHeader.dkVDetectorLng * 10;
	projectionInfo.detectorPth = ximHeader.dkVDetectorPth;
	projectionInfo.normChamberValue = ximHeader.ikVNormChamber;
	projectionInfo.timeTag = ximHeader.iStartTime;
	projectionInfo.isCompressed = ximHeader.iCompressionIndicator > 0;
	projectionInfo.format = ".xim";
	projectionInfo.angle = -1. * ximHeader.dkVSourceRtn + 180;
	projectionInfo.headerSize = ximHeader.iImageBodyPosition;

	m_ProjectionInfos.push_back(projectionInfo);
}

 template <class TOutputImage>
 void ProjectionsReader<TOutputImage>
::ReadHisHeader(std::string filename)
{
  // open file
  std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);

  if ( file.fail() )
    itkGenericExceptionMacro(<< "Could not open file (for reading): "
                             << filename);

  // read header
  char header[HIS_HEADER_INFO_SIZE];
  file.read(header, HIS_HEADER_INFO_SIZE);

  if (header[0]!=0 || header[1]!=112 || header[2]!=68 || header[3]!=0) {
    itkExceptionMacro(<< "rtk::HisImageIO::ReadImageInformation: file "
                      << filename
                      << " not in Heimann HIS format version 100");
    return;
    }

  int nrframes, type, ulx, uly, brx, bry;
  m_HeaderSize  = header[10] + (header[11]<<8);
  ulx           = header[12] + (header[13]<<8);
  uly           = header[14] + (header[15]<<8);
  brx           = header[16] + (header[17]<<8);
  bry           = header[18] + (header[19]<<8);
  nrframes      = header[20] + (header[21]<<8);
  type          = header[32] + (header[34]<<8);

  m_Size[0] = bry-uly+1;
  m_Size[1] = brx-ulx+1;

  m_Spacing[0] = 409.6/m_Size[0];
  m_Spacing[1] = 409.6/m_Size[1];

  m_Origin[0] = -0.5*(m_Size[0]-1.)*m_Spacing[0];
  m_Origin[1] = -0.5*(m_Size[1]-1.)*m_Spacing[1];

  ProjectionInfo projectionInfo;
  projectionInfo.width = m_Size[0];
  projectionInfo.height = m_Size[1];
  projectionInfo.spacingX = m_Spacing[0];
  projectionInfo.spacingY = m_Spacing[1];
  projectionInfo.sourceLat = 0;
  projectionInfo.sourceVrt = 0;
  projectionInfo.sourceLng = 0;
  projectionInfo.sourcePth = 0;
  projectionInfo.detectorLat = 0;
  projectionInfo.detectorVrt = 0;
  projectionInfo.detectorLng = 0;
  projectionInfo.detectorPth = 0;
  projectionInfo.normChamberValue = 0;
  projectionInfo.timeTag = 0;
  projectionInfo.isCompressed = false;
  projectionInfo.format = ".his";
  projectionInfo.angle = 0;
  projectionInfo.headerSize = HIS_HEADER_INFO_SIZE;

  m_ProjectionInfos.push_back(projectionInfo);
}

 template <class TOutputImage>
 void ProjectionsReader<TOutputImage>
::ReadHnc(std::string filename, void * buffer)
{
  FILE *     fp;

  fp = fopen (filename.c_str(), "rb");
  if (fp == NULL)
    {
    itkGenericExceptionMacro(<< "Could not open file (for reading): " << filename);
    return;
    }

  fseek(fp, 512, SEEK_SET);
  fread(buffer, sizeof(unsigned short int), m_Size[0]*m_Size[1], fp);

  fclose (fp);
  return;
}

 template <class TOutputImage>
 void ProjectionsReader<TOutputImage>
::ReadHnd(std::string filename, void * buffer)
{
  FILE *fp;

  itk::uint32_t *buf = (itk::uint32_t*)buffer;
  unsigned char *pt_lut;
  itk::uint32_t  a;
  unsigned char  v;
  int            lut_idx, lut_off;
  size_t         num_read;
  char           dc;
  short          ds;
  long           dl, diff=0;
  itk::uint32_t  i;

  fp = fopen (filename.c_str(), "rb");
  if (fp == NULL)
    itkGenericExceptionMacro(<< "Could not open file (for reading): " << filename);

  pt_lut = (unsigned char*) malloc (sizeof (unsigned char) * m_Size[0] * m_Size[1] );

  /* Read LUT */
  if(fseek (fp, 1024, SEEK_SET) != 0)
    itkGenericExceptionMacro(<< "Could not seek to image data in: " << filename);

  size_t nbytes = (m_Size[1]-1)*m_Size[0] / 4;
  if(nbytes != fread (pt_lut, sizeof(unsigned char), nbytes, fp))
    itkGenericExceptionMacro(<< "Could not read image LUT in: " << filename);

  /* Read first row */
  for (i = 0; i < m_Size[0]; i++) {
    if(1 != fread (&a, sizeof(itk::uint32_t), 1, fp))
      itkGenericExceptionMacro(<< "Could not read first row in: " << filename);
    buf[i] = a;
    }

  /* Read first pixel of second row */
  if(1 != fread (&a, sizeof(itk::uint32_t), 1, fp))
    itkGenericExceptionMacro(<< "Could not read first pixel of second row");
  buf[i++] = a;

  /* Decompress the rest */
  lut_idx = 0;
  lut_off = 0;
  while (i < m_Size[0] * m_Size[1] ) {
    itk::uint32_t r11, r12, r21;

    r11 = buf[i-m_Size[0]-1];
    r12 = buf[i-m_Size[0]];
    r21 = buf[i-1];
    v = pt_lut[lut_idx];
    switch (lut_off) {
      case 0:
        v = v & 0x03;
        lut_off++;
        break;
      case 1:
        v = (v & 0x0C) >> 2;
        lut_off++;
        break;
      case 2:
        v = (v & 0x30) >> 4;
        lut_off++;
        break;
      case 3:
        v = (v & 0xC0) >> 6;
        lut_off = 0;
        lut_idx++;
        break;
      }
    switch (v) {
      case 0:
        num_read = fread (&dc, sizeof(unsigned char), 1, fp);
        if (num_read != 1) goto read_error;
        diff = dc;
        break;
      case 1:
        num_read = fread (&ds, sizeof(unsigned short), 1, fp);
        if (num_read != 1) goto read_error;
        diff = ds;
        break;
      case 2:
        num_read = fread (&dl, sizeof(itk::uint32_t), 1, fp);
        if (num_read != 1) goto read_error;
        diff = dl;
        break;
      }
    buf[i] = r21 + r12 + diff - r11;
    i++;
    }

  /* Clean up */
  free (pt_lut);
  if(fclose (fp) != 0)
    itkGenericExceptionMacro(<< "Could not close file: " << filename);
  return;

  read_error:

  itkGenericExceptionMacro(<< "Error reading hnd file");
  free (pt_lut);
  if(fclose (fp) != 0)
    itkGenericExceptionMacro(<< "Could not close file: " << filename);
  return;
}

template <class TOutputImage>
void ProjectionsReader<TOutputImage>
::ReadXim(std::string filename, void * buffer)
{
	FILE *fp;

	fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
		itkGenericExceptionMacro(<< "Could not open file (for reading): " << filename);

	if (!m_isCompressed)
	{
		fseek(fp, m_HeaderSize, SEEK_SET);

		if (m_BytesPerPixel == 1)
			fread(buffer, sizeof(char), m_Size[0] * m_Size[1], fp);
		else if (m_BytesPerPixel == 2)
			fread(buffer, sizeof(short), m_Size[0] * m_Size[1], fp);
		else if (m_BytesPerPixel == 4)
			fread(buffer, sizeof(int), m_Size[0] * m_Size[1], fp);
		else
		{
			itkGenericExceptionMacro(<< "Number of bytes per pixel: " << m_BytesPerPixel << " not supported in " << filename);
		}
	}
	else
	{
		itk::int32_t *buf = (itk::int32_t*)buffer;
		unsigned char *pt_lut, v;
		unsigned int lut_idx = 0, lut_off = 0;
		unsigned int i;
		int a, diff = 0;
		char           dc;
		short          ds;

		pt_lut = (unsigned char*)malloc(sizeof (unsigned char)* m_LookupTableSize);
		fseek(fp, m_LookupTableSize, SEEK_SET);
		if (m_LookupTableSize != fread(pt_lut, sizeof (unsigned char), m_LookupTableSize, fp))
			itkGenericExceptionMacro(<< "Could not read image LUT in: " << filename);

		fseek(fp, m_HeaderSize, SEEK_SET);

		/* Read first row + first pixel of second row*/
		for (i = 0; i < m_Size[0] + 1; i++) {
			if (1 != fread(&a, sizeof(int), 1, fp))
				itkGenericExceptionMacro(<< "Could not read first row and first pixel of second row in: " << filename);
			buf[i] = a;
		}

		for (i = m_Size[0] + 1; i < m_Size[0] * m_Size[1]; ++i)
		{
			v = pt_lut[lut_idx];
			switch (lut_off) {
			case 0:
				v = v & 0x03;
				lut_off++;
				break;
			case 1:
				v = (v & 0x0C) >> 2;
				lut_off++;
				break;
			case 2:
				v = (v & 0x30) >> 4;
				lut_off++;
				break;
			case 3:
				v = (v & 0xC0) >> 6;
				lut_off = 0;
				lut_idx++;
				break;
			}

			if (v == 0)
			{
				if (1 != fread((void *)&dc, sizeof(char), 1, fp))
					itkGenericExceptionMacro(<< "Could not loop through a pixel of char type in: " << filename);
				diff = dc;
			}
			else if (v == 1)
			{
				if (1 != fread((void *)&ds, sizeof(short), 1, fp))
					itkGenericExceptionMacro(<< "Could not loop through a pixel of short type in: " << filename);
				diff = ds;
			}
			else
			{
				if (1 != fread((void *)&diff, sizeof(int), 1, fp))
					itkGenericExceptionMacro(<< "Could not loop through a pixel of int type in: " << filename);
			}
			buf[i] = diff + buf[i - 1] + buf[i - m_Size[0]] - buf[i - m_Size[0] - 1];
		}
	}

	if (fclose(fp) != 0)
		itkGenericExceptionMacro(<< "Could not close file: " << filename);
	return;
}

 template <class TOutputImage>
 void ProjectionsReader<TOutputImage>
::ReadHis(std::string filename, void * buffer)
{
  // open file
  std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);

  if ( file.fail() )
    itkGenericExceptionMacro(<< "Could not open file (for reading): " << filename);

  file.seekg(m_HeaderSize+HIS_HEADER_INFO_SIZE, std::ios::beg);
  if ( file.fail() )
    itkExceptionMacro(<<"File seek failed (His Read)");

  file.read( (char*)buffer, 2*m_Size[0]*m_Size[1] );
  if ( file.fail() )
    itkExceptionMacro(<<"Read failed: Wanted "
                      << 2*m_Size[0]*m_Size[1]
                      << " bytes, but read "
                      << file.gcount() << " bytes. The current state is: "
                      << file.rdstate() );
}

 template <class TOutputImage>
 void ProjectionsReader<TOutputImage>
::ReadAtt(std::string filename, void * buffer)
{
  FILE *     fp;

  fp = fopen (filename.c_str(), "rb");
  if (fp == NULL)
    {
    itkGenericExceptionMacro(<< "Could not open file (for reading): " << filename);
    return;
    }

  fseek(fp, 512, SEEK_SET);
  fread(buffer, sizeof(float), m_Size[0]*m_Size[1], fp);

  fclose (fp);
  return;
}

 template <class TOutputImage>
 void ProjectionsReader<TOutputImage>
::GenerateOutputInformation()
{
	m_AnglesDegree.clear();
	std::string filename(m_Filenames[0]);
	std::string fileExt( itksys::SystemTools::GetFilenameLastExtension(filename) );
	
	if (this->strcmpi(fileExt,".hnc"))
		this->ReadHncHndHeader(filename);
	else if (this->strcmpi(fileExt,".hnd"))
		this->ReadHncHndHeader(filename);
	else if (this->strcmpi(fileExt, ".xim"))
		this->ReadXimHeader(filename);
	else if (this->strcmpi(fileExt,".his"))
		this->ReadHisHeader(filename);
	else if (this->strcmpi(fileExt,".att"))
		this->ReadHncHndHeader(filename);
	else
	{
		itkGenericExceptionMacro(<< "Incompatible fileformat: " << filename);
		return;
	}

	m_Size[2] = m_Filenames.size();
	OutputImageIndexType index;
	index.Fill(0);
	OutputImageRegionType region(index, m_Size);
	this->GetOutput()->SetOrigin( m_Origin );
	this->GetOutput()->SetSpacing( m_Spacing );
	this->GetOutput()->SetLargestPossibleRegion( region );
}

 template <class TOutputImage>
 void ProjectionsReader<TOutputImage>
::GenerateData()
{
	m_AnglesDegree.clear();

	typename itk::ImageSource<TOutputImage>::Pointer rawToProjectionsFilter;

	OutputImageIndexType index;
	index.Fill(0);
	OutputImageSizeType sizePerSlice(m_Size);
	sizePerSlice[2] = 1;
	OutputImageRegionType regionPerSlice(index, sizePerSlice);

	itk::FixedArray< unsigned int, OutputImageDimension > layout;
	layout[0] = 1;
	layout[1] = 1;
	layout[2] = 0;

	std::string fileExt( itksys::SystemTools::GetFilenameLastExtension(m_Filenames[0]) );
	
	if (this->strcmpi(fileExt,".hnc"))
	{
		typedef itk::Image<unsigned short int, 3> InputImageType;
		typedef itk::TileImageFilter<InputImageType, InputImageType>	TileType;
		TileType::Pointer tileFilter = TileType::New();
		tileFilter->SetLayout( layout );
		for (unsigned int k = 0; k != m_Size[2]; ++k)
		{
			std::string filename(m_Filenames[k]);
			InputImageType::Pointer inputImg = InputImageType::New();
			inputImg->SetRegions( regionPerSlice );
			inputImg->Allocate();
			if (k > 0)
				this->ReadHncHndHeader(filename);
			this->ReadHnc(filename, inputImg->GetBufferPointer());
			tileFilter->SetInput( k, inputImg );
		}
		
      // Convert raw to Projections
	  if(!m_ProjectionConversion)
	    {
		typedef itk::CastImageFilter<InputImageType, OutputImageType>	CastType;
		CastType::Pointer castFilter = CastType::New();
		castFilter->SetInput(tileFilter->GetOutput());
		rawToProjectionsFilter = castFilter;
	    }
	  else
	    {
        typedef rtk::VarianObiRawHncImageFilter<InputImageType, OutputImageType> RawFilterType;
        typename RawFilterType::Pointer rawFilter = RawFilterType::New();
        rawFilter->SetInput( tileFilter->GetOutput() );
        rawToProjectionsFilter = rawFilter;
		tileFilter->ReleaseDataFlagOn();
		}
		tileFilter->Update();
	}
	else if (this->strcmpi(fileExt,".hnd"))
	{
		typedef itk::Image<unsigned int, 3> InputImageType;
		typedef itk::TileImageFilter<InputImageType, InputImageType>	TileType;
		TileType::Pointer tileFilter = TileType::New();
		tileFilter->SetLayout( layout );
		for (unsigned int k = 0; k != m_Size[2]; ++k)
		{
			std::string filename(m_Filenames[k]);
			InputImageType::Pointer inputImg = InputImageType::New();
			inputImg->SetRegions( regionPerSlice );
			inputImg->Allocate();
			if (k > 0)
				this->ReadHncHndHeader(filename);
			this->ReadHnd(filename, inputImg->GetBufferPointer());
			tileFilter->SetInput( k, inputImg );
		}

	  // Convert raw to Projections
	  if(!m_ProjectionConversion)
		{
		typedef itk::CastImageFilter<InputImageType, OutputImageType>	CastType;
		CastType::Pointer castFilter = CastType::New();
		castFilter->SetInput(tileFilter->GetOutput());
		rawToProjectionsFilter = castFilter;
	    }
	  else
	    {
        typedef rtk::VarianObiRawImageFilter<InputImageType, OutputImageType> RawFilterType;
        typename RawFilterType::Pointer rawFilter = RawFilterType::New();
        rawFilter->SetInput( tileFilter->GetOutput() );
        rawToProjectionsFilter = rawFilter;
		tileFilter->ReleaseDataFlagOn();
		}
		tileFilter->Update();
	}
	else if (this->strcmpi(fileExt, ".xim"))
	{
		typedef itk::Image<unsigned int, 3> InputImageType;
		typedef itk::TileImageFilter<InputImageType, InputImageType>	TileType;
		TileType::Pointer tileFilter = TileType::New();
		tileFilter->SetLayout(layout);
		for (unsigned int k = 0; k != m_Size[2]; ++k)
		{
			std::string filename(m_Filenames[k]);
			InputImageType::Pointer inputImg = InputImageType::New();
			inputImg->SetRegions(regionPerSlice);
			inputImg->Allocate();
			if (k > 0)
				this->ReadXimHeader(filename);
			this->ReadXim(filename, inputImg->GetBufferPointer());
			tileFilter->SetInput(k, inputImg);
		}

		// Convert raw to Projections
		if (!m_ProjectionConversion)
		{
			typedef itk::CastImageFilter<InputImageType, OutputImageType>	CastType;
			CastType::Pointer castFilter = CastType::New();
			castFilter->SetInput(tileFilter->GetOutput());
			rawToProjectionsFilter = castFilter;
		}
		else
		{
			typedef igt::VarianObiRawXimImageFilter<InputImageType, OutputImageType> RawFilterType;
			typename RawFilterType::Pointer rawFilter = RawFilterType::New();
			rawFilter->SetInput(tileFilter->GetOutput());
			rawFilter->SetNormChamberValue(m_NormChamberValue);
			rawToProjectionsFilter = rawFilter;
			tileFilter->ReleaseDataFlagOn();
		}
		tileFilter->Update();
	}
	else if (this->strcmpi(fileExt,".his"))
	{
		typedef itk::Image<unsigned short int, 3> InputImageType;
		typedef itk::TileImageFilter<InputImageType, InputImageType>	TileType;
		TileType::Pointer tileFilter = TileType::New();
		tileFilter->SetLayout( layout );
		for (unsigned int k = 0; k != m_Size[2]; ++k)
		{
			std::string filename(m_Filenames[k]);
			InputImageType::Pointer inputImg = InputImageType::New();
			inputImg->SetRegions( regionPerSlice );
			inputImg->Allocate();
			if (k > 0)
				this->ReadHisHeader(filename);
			this->ReadHis(filename, inputImg->GetBufferPointer());
			tileFilter->SetInput( k, inputImg );
		}

	  // Convert raw to Projections
	  if(!m_ProjectionConversion)
		{
		typedef itk::CastImageFilter<InputImageType, OutputImageType>	CastType;
		CastType::Pointer castFilter = CastType::New();
		castFilter->SetInput(tileFilter->GetOutput());
		rawToProjectionsFilter = castFilter;
	    }
	  else
	    {
        typedef rtk::ElektaSynergyRawToAttenuationImageFilter<InputImageType, OutputImageType> RawFilterType;
        typename RawFilterType::Pointer rawFilter = RawFilterType::New();
        rawFilter->SetInput( tileFilter->GetOutput() );
        rawToProjectionsFilter = rawFilter;
		tileFilter->ReleaseDataFlagOn();
		}
		tileFilter->Update();
	}
	else if (this->strcmpi(fileExt,".att"))
	{
		typedef itk::Image<float, 3> InputImageType;
		typedef itk::TileImageFilter<InputImageType, InputImageType>	TileType;
		TileType::Pointer tileFilter = TileType::New();
		tileFilter->SetLayout( layout );
		for (unsigned int k = 0; k != m_Size[2]; ++k)
		{
			std::string filename(m_Filenames[k]);
			InputImageType::Pointer inputImg = InputImageType::New();
			inputImg->SetRegions( regionPerSlice );
			inputImg->Allocate();
			if (k > 0)
				this->ReadHncHndHeader(filename);
			this->ReadAtt(filename, inputImg->GetBufferPointer());
			tileFilter->SetInput( k, inputImg );
		}
		typedef itk::CastImageFilter<InputImageType, OutputImageType>	CastType;
		CastType::Pointer castFilter = CastType::New();
		castFilter->SetInput(tileFilter->GetOutput());
		rawToProjectionsFilter = castFilter;
		tileFilter->Update();
	}
	else
	{
		itkGenericExceptionMacro(<< "Incompatible fileformat: " << m_Filenames[0]);
		return;
	}

	OutputImageRegionType region(index, m_Size);
	this->GetOutput()->SetLargestPossibleRegion( region );
	rawToProjectionsFilter->UpdateOutputInformation();
	rawToProjectionsFilter->GetOutput()->SetRequestedRegion( this->GetOutput()->GetRequestedRegion() );
	rawToProjectionsFilter->Update();
	this->GraftOutput( rawToProjectionsFilter->GetOutput() );
	this->GetOutput()->SetOrigin( m_Origin );
	this->GetOutput()->SetSpacing( m_Spacing );
	//this->GetOutput()->SetLargestPossibleRegion( region );
}


} //namespace igt

#endif // __igtProjectionsReader_cxx
