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

// std include
#include <stdio.h>

#include "igtXimImageIO.h"
#include <itkMetaDataObject.h>

//--------------------------------------------------------------------
// Read Image Information
void igt::XimImageIO::ReadImageInformation()
{
  FILE *     fp;

  fp = fopen (m_FileName.c_str(), "rb");
  if (fp == NULL)
    itkGenericExceptionMacro(<< "Could not open file (for reading): " << m_FileName);

  size_t nelements = 0;
		  
  {
	char *sbuffer = (char*)malloc(8 * sizeof(char));
	nelements += fread ( (void *) sbuffer, sizeof(char), 8, fp);
	std::string fileformat_str(sbuffer);
	m_Header.sFileFormatIdentifier = fileformat_str.substr(0, 8);
	free(sbuffer);
  }

  nelements += fread ( (void *) &m_Header.iFileFormatVersion, sizeof(int), 1, fp);
  nelements += fread ( (void *) &m_Header.iImageWidth, sizeof(int), 1, fp);
  nelements += fread ( (void *) &m_Header.iImageHeight, sizeof(int), 1, fp);
  nelements += fread ( (void *) &m_Header.iBitsPerPixel, sizeof(int), 1, fp);
  nelements += fread ( (void *) &m_Header.iBytesPerPixel, sizeof(int), 1, fp);
  nelements += fread ( (void *) &m_Header.iCompressionIndicator, sizeof(int), 1, fp);
  nelements += fread ( (void *) &m_Header.iLookupTableSize, sizeof(int), 1, fp);
  m_Header.iLookupTablePosition = ftell(fp);

  if (m_Header.iCompressionIndicator == 1)
  {
	  fseek(fp, m_Header.iLookupTableSize * sizeof(bool), SEEK_CUR);
	  nelements += fread((void *)&m_Header.iCompressedPixelBufferSize, sizeof(int), 1, fp);
	  m_Header.iImageBodyPosition = ftell(fp);
	  fseek(fp, m_Header.iCompressedPixelBufferSize, SEEK_CUR);
	  nelements += fread((void *)&m_Header.iUncompressedPixelBufferSize, sizeof(int), 1, fp);
  }
  else
  {
	  nelements += fread((void *)&m_Header.iUncompressedPixelBufferSize, sizeof(int), 1, fp);
	  m_Header.iImageBodyPosition = ftell(fp);
	  fseek(fp, m_Header.iUncompressedPixelBufferSize, SEEK_CUR);
  }
  
  // Histogram
  nelements += fread((void *)&m_Header.iNumberOfBinsInHistogram, sizeof(int), 1, fp);
  if (m_Header.iNumberOfBinsInHistogram > 0)
  {
	  /*
	  for (unsigned int k = 0; k != m_Header.iNumberOfBinsInHistogram; ++k)
	  {
		  int histElement;
		  nelements += fread((void *)&histElement, sizeof(int), 1, fp);
		  m_Header.viHistogramData.push_back(histElement);
	  }
	  */
	  // We don't really need to histogram
	  // Maybe just skip it for now to save time
	  fseek(fp, m_Header.iNumberOfBinsInHistogram * sizeof(int), SEEK_CUR);
  }

  // Decode properties
  nelements += fread((void *)&m_Header.iNumberOfProperties, sizeof(int), 1, fp);
  for (unsigned int k = 0; k != m_Header.iNumberOfProperties; ++k)
  {
	  int pNameLength;
	  nelements += fread(&pNameLength, sizeof(int), 1, fp);
	  char *cbuffer = (char*) malloc(pNameLength * sizeof(char));
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
		  if (pName.compare("MMSurrogateModelResultType1") == 0)
			  m_Header.iMMSurrogateModelResultType1 = pValue_int;
		  else if (pName.compare("MMMotionModelResultType1") == 0)
			  m_Header.iMMMotionModelResultType1 = pValue_int;
		  else if (pName.compare("Valid") == 0)
			  m_Header.iValid = pValue_int;
		  else if (pName.compare("StopTimeRemainder") == 0)
			  m_Header.iStopTimeRemainder = pValue_int;
		  else if (pName.compare("MLCModel") == 0)
			  m_Header.iMLCModel = pValue_int;
		  else if (pName.compare("BeamState") == 0)
			  m_Header.iBeamState = pValue_int;
		  else if (pName.compare("NumberOfFrames") == 0)
			  m_Header.iNumberOfFrames = pValue_int;
		  else if (pName.compare("MMMotionModelResultType2") == 0)
			  m_Header.iMMMotionModelResultType2 = pValue_int;
		  else if (pName.compare("KVHardwareState") == 0)
			  m_Header.iKVHardwareState = pValue_int;
		  else if (pName.compare("ImageSourceType") == 0)
			  m_Header.iImageSourceType = pValue_int;
		  else if (pName.compare("AcquisitionSystem") == 0)
			  m_Header.iAcquisitionSystem = pValue_int;
		  else if (pName.compare("StartTimeRemainder") == 0)
			  m_Header.iStartTimeRemainder = pValue_int;
		  else if (pName.compare("KVBeamPulseCounter") == 0)
			  m_Header.iKVBeamPulseCounter = pValue_int;
		  else if (pName.compare("KVNormChamber") == 0)
			  m_Header.iKVNormChamber = pValue_int;
		  else if (pName.compare("ImagePropertyVersionMinor") == 0)
			  m_Header.iImagePropertyVersionMinor = pValue_int;
		  else if (pName.compare("MMExtrapolationStartTime2Remainder") == 0)
			  m_Header.iMMExtrapolationStartTime2Remainder = pValue_int;
		  else if (pName.compare("ImagePropertyVersionMajor") == 0)
			  m_Header.iImagePropertyVersionMajor = pValue_int;
		  else if (pName.compare("HistogramBinSize") == 0)
			  m_Header.iHistogramBinSize = pValue_int;
		  else if (pName.compare("MMExtrapolationStartTime1Remainder") == 0)
			  m_Header.iMMExtrapolationStartTime1Remainder = pValue_int;
		  else if (pName.compare("LastImage") == 0)
			  m_Header.iLastImage = pValue_int;
		  else if (pName.compare("ExternalBeam") == 0)
			  m_Header.iExternalBeam = pValue_int;
		  else if (pName.compare("KVBeamHold") == 0)
			  m_Header.iKVBeamHold = pValue_int;
		  else if (pName.compare("MotionCompensationHold") == 0)
			  m_Header.iMotionCompensationHold = pValue_int;
		  else if (pName.compare("MMExtrapolationStartTime0Remainder") == 0)
			  m_Header.iMMExtrapolationStartTime0Remainder = pValue_int;
		  else if (pName.compare("ImageSourceIndex") == 0)
			  m_Header.iImageSourceIndex = pValue_int;
		  else if (pName.compare("DataOffset") == 0)
			  m_Header.iDataOffset = pValue_int;
		  else if (pName.compare("MMSegmentalAction") == 0)
			  m_Header.iMMSegmentalAction = pValue_int;
		  else if (pName.compare("MMSurrogateModelResultType0") == 0)
			  m_Header.iMMSurrogateModelResultType0 = pValue_int;
		  else if (pName.compare("MMExtrapolationStartTime1") == 0)
			  m_Header.iMMExtrapolationStartTime1 = pValue_int;
		  else if (pName.compare("MMExtrapolationStartTime0") == 0)
			  m_Header.iMMExtrapolationStartTime0 = pValue_int;
		  else if (pName.compare("MMExtrapolationStartTime2") == 0)
			  m_Header.iMMExtrapolationStartTime2 = pValue_int;
		  else if (pName.compare("MMMotionModelResultType0") == 0)
			  m_Header.iMMMotionModelResultType0 = pValue_int;
		  else if (pName.compare("KVAutoBrightnessControl") == 0)
			  m_Header.iKVAutoBrightnessControl = pValue_int;
		  else if (pName.compare("MVBeamHold") == 0)
			  m_Header.iMVBeamHold = pValue_int;
		  else if (pName.compare("SequenceNumber") == 0)
			  m_Header.iSequenceNumber = pValue_int;
		  else if (pName.compare("AcquisitionId") == 0)
			  m_Header.iAcquisitionId = pValue_int;
		  else if (pName.compare("NDIToolState") == 0)
			  m_Header.iNDIToolState = pValue_int;
		  else if (pName.compare("StopTime") == 0)
			  m_Header.iStopTime = pValue_int;
		  else if (pName.compare("MVBeamOn") == 0)
			  m_Header.iMVBeamOn = pValue_int;
		  else if (pName.compare("MMSurrogateModelResultType2") == 0)
			  m_Header.iMMSurrogateModelResultType2 = pValue_int;
		  else if (pName.compare("MMActionWindows") == 0)
			  m_Header.iMMActionWindows = pValue_int;
		  else if (pName.compare("DataRescaleType") == 0)
			  m_Header.iDataRescaleType = pValue_int;
		  else if (pName.compare("KVBeamOn") == 0)
			  m_Header.iKVBeamOn = pValue_int;
		  else if (pName.compare("MVBeamStateChange") == 0)
			  m_Header.iMVBeamStateChange = pValue_int;
		  else if (pName.compare("DataShift") == 0)
			  m_Header.iDataShift = pValue_int;
		  else if (pName.compare("ImageFlipped") == 0)
			  m_Header.iImageFlipped = pValue_int;
		  else if (pName.compare("StartTime") == 0)
			  m_Header.iStartTime = pValue_int;
	  }
	  else if (pType == 1) // Scalar double
	  {
		  double pValue_double;
		  nelements += fread(&pValue_double, sizeof(double), 1, fp);
		  if (pName.compare("MVCollimatorX1") == 0)
			  m_Header.dMVCollimatorX1 = pValue_double;
		  else if (pName.compare("MVDoseDelivered") == 0)
			  m_Header.dMVDoseDelivered = pValue_double;
		  else if (pName.compare("MVDoseStop") == 0)
			  m_Header.dMVDoseStop = pValue_double;
		  else if (pName.compare("KVDetectorVrt") == 0)
			  m_Header.dKVDetectorVrt = pValue_double;
		  else if (pName.compare("MMPhase2") == 0)
			  m_Header.dMMPhase2 = pValue_double;
		  else if (pName.compare("MVDetectorLat") == 0)
			  m_Header.dMVDetectorLat = pValue_double;
		  else if (pName.compare("ControlPoint") == 0)
			  m_Header.dControlPoint = pValue_double;
		  else if (pName.compare("KVCollimatorY1") == 0)
			  m_Header.dKVCollimatorY1 = pValue_double;
		  else if (pName.compare("KVCollimatorX1") == 0)
			  m_Header.dKVCollimatorX1 = pValue_double;
		  else if (pName.compare("MLCCarriageB") == 0)
			  m_Header.dMLCCarriageB = pValue_double;
		  else if (pName.compare("MMPhase1") == 0)
			  m_Header.dMMPhase1 = pValue_double;
		  else if (pName.compare("CouchVrt") == 0)
			  m_Header.dCouchVrt = pValue_double;
		  else if (pName.compare("FrameRate") == 0)
			  m_Header.dFrameRate = pValue_double;
		  else if (pName.compare("KVDoseAreaProductStart") == 0)
			  m_Header.dKVDoseAreaProductStart = pValue_double;
		  else if (pName.compare("KVDoseAreaProductStop") == 0)
			  m_Header.dKVDoseAreaProductStop = pValue_double;
		  else if (pName.compare("MMTrackingRemainderY") == 0)
			  m_Header.dMMTrackingRemainderY = pValue_double;
		  else if (pName.compare("MVCollimatorY2") == 0)
			  m_Header.dMVCollimatorY2 = pValue_double;
		  else if (pName.compare("MVCollimatorX2") == 0)
			  m_Header.dMVCollimatorX2 = pValue_double;
		  else if (pName.compare("MVDetectorPitch") == 0)
			  m_Header.dMVDetectorPitch = pValue_double;
		  else if (pName.compare("MVDetectorRtn") == 0)
			  m_Header.dMVDetectorRtn = pValue_double;
		  else if (pName.compare("MMConformityIndexOverAreaDoserate") == 0)
			  m_Header.dMMConformityIndexOverAreaDoserate = pValue_double;
		  else if (pName.compare("CouchLat") == 0)
			  m_Header.dCouchLat = pValue_double;
		  else if (pName.compare("MVDetectorLng") == 0)
			  m_Header.dMVDetectorLng = pValue_double;
		  else if (pName.compare("CouchLng") == 0)
			  m_Header.dCouchLng = pValue_double;
		  else if (pName.compare("MMAmplitude1") == 0)
			  m_Header.dMMAmplitude1 = pValue_double;
		  else if (pName.compare("KVSourceLat") == 0)
			  m_Header.dKVSourceLat = pValue_double;
		  else if (pName.compare("KVSourceVrt") == 0)
			  m_Header.dKVSourceVrt = pValue_double;
		  else if (pName.compare("KVDetectorLat") == 0)
			  m_Header.dKVDetectorLat = pValue_double;
		  else if (pName.compare("DataGain") == 0)
			  m_Header.dDataGain = pValue_double;
		  else if (pName.compare("KVSourceLng") == 0)
			  m_Header.dKVSourceLng = pValue_double;
		  else if (pName.compare("KVDetectorPitch") == 0)
			  m_Header.dKVDetectorPitch = pValue_double;
		  else if (pName.compare("KVAirKermaStart") == 0)
			  m_Header.dKVAirKermaStart = pValue_double;
		  else if (pName.compare("KVMilliAmperes") == 0)
			  m_Header.dKVMilliAmperes = pValue_double;
		  else if (pName.compare("KVSourceRtn") == 0)
			  m_Header.dKVSourceRtn = pValue_double;
		  else if (pName.compare("MMQuality2") == 0)
			  m_Header.dMMQuality2 = pValue_double;
		  else if (pName.compare("PixelHeight") == 0)
			  m_Header.dPixelHeight = pValue_double;
		  else if (pName.compare("KVDetectorRtn") == 0)
			  m_Header.dKVDetectorRtn = pValue_double;
		  else if (pName.compare("MLCCarriageA") == 0)
			  m_Header.dMLCCarriageA = pValue_double;
		  else if (pName.compare("MMConformityIndexUnderAreaRelative") == 0)
			  m_Header.dMMConformityIndexUnderAreaRelative = pValue_double;
		  else if (pName.compare("MVCollimatorRtn") == 0)
			  m_Header.dMVCollimatorRtn = pValue_double;
		  else if (pName.compare("MMQualityAmp0") == 0)
			  m_Header.dMMQualityAmp0 = pValue_double;
		  else if (pName.compare("KVDetectorLng") == 0)
			  m_Header.dKVDetectorLng = pValue_double;
		  else if (pName.compare("MVSourceVrt") == 0)
			  m_Header.dMVSourceVrt = pValue_double;
		  else if (pName.compare("GantryRtn") == 0)
			  m_Header.dGantryRtn = pValue_double;
		  else if (pName.compare("MMQuality0") == 0)
			  m_Header.dMMQuality0 = pValue_double;
		  else if (pName.compare("MMPhase0") == 0)
			  m_Header.dMMPhase0 = pValue_double;
		  else if (pName.compare("CouchPit") == 0)
			  m_Header.dCouchPit = pValue_double;
		  else if (pName.compare("MMConformityIndexOverAreaDoserateRelative") == 0)
			  m_Header.dMMConformityIndexOverAreaDoserateRelative = pValue_double;
		  else if (pName.compare("MMQuality1") == 0)
			  m_Header.dMMQuality1 = pValue_double;
		  else if (pName.compare("KVSourcePitch") == 0)
			  m_Header.dKVSourcePitch = pValue_double;
		  else if (pName.compare("MVDoseRate") == 0)
			  m_Header.dMVDoseRate = pValue_double;
		  else if (pName.compare("MMConformityIndexUnderAreaDoserate") == 0)
			  m_Header.dMMConformityIndexUnderAreaDoserate = pValue_double;
		  else if (pName.compare("MMAmplitude2") == 0)
			  m_Header.dMMAmplitude2 = pValue_double;
		  else if (pName.compare("MMTrackingRemainderX") == 0)
			  m_Header.dMMTrackingRemainderX = pValue_double;
		  else if (pName.compare("PixelWidth") == 0)
			  m_Header.dPixelWidth = pValue_double;
		  else if (pName.compare("MMConformityIndexUnderAreaDoserateRelative") == 0)
			  m_Header.dMMConformityIndexUnderAreaDoserateRelative = pValue_double;
		  else if (pName.compare("CouchRtn") == 0)
			  m_Header.dCouchRtn = pValue_double;
		  else if (pName.compare("MVDetectorVrt") == 0)
			  m_Header.dMVDetectorVrt = pValue_double;
		  else if (pName.compare("MMPhaseTarget") == 0)
			  m_Header.dMMPhaseTarget = pValue_double;
		  else if (pName.compare("KVMilliSeconds") == 0)
			  m_Header.dKVMilliSeconds = pValue_double;
		  else if (pName.compare("DoseConversionFactor") == 0)
			  m_Header.dDoseConversionFactor = pValue_double;
		  else if (pName.compare("MMConformityIndexUnderArea") == 0)
			  m_Header.dMMConformityIndexUnderArea = pValue_double;
		  else if (pName.compare("MMAmplitude0") == 0)
			  m_Header.dMMAmplitude0 = pValue_double;
		  else if (pName.compare("MMQualityAmp2") == 0)
			  m_Header.dMMQualityAmp2 = pValue_double;
		  else if (pName.compare("MMQualityAmp1") == 0)
			  m_Header.dMMQualityAmp1 = pValue_double;
		  else if (pName.compare("MVDoseStart") == 0)
			  m_Header.dMVDoseStart = pValue_double;
		  else if (pName.compare("MMConformityIndexOverAreaRelative") == 0)
			  m_Header.dMMConformityIndexOverAreaRelative = pValue_double;
		  else if (pName.compare("KVCollimatorY2") == 0)
			  m_Header.dKVCollimatorY2 = pValue_double;
		  else if (pName.compare("KVCollimatorX2") == 0)
			  m_Header.dKVCollimatorX2 = pValue_double;
		  else if (pName.compare("KVAirKermaStop") == 0)
			  m_Header.dKVAirKermaStop = pValue_double;
		  else if (pName.compare("MMTrackingRemainderZ") == 0)
			  m_Header.dMMTrackingRemainderZ = pValue_double;
		  else if (pName.compare("CouchRol") == 0)
			  m_Header.dCouchRol = pValue_double;
		  else if (pName.compare("KVKiloVolts") == 0)
			  m_Header.dKVKiloVolts = pValue_double;
		  else if (pName.compare("UndefinedDouble") == 0)
			  m_Header.dUndefinedDouble = pValue_double;
		  else if (pName.compare("MMConformityIndexOverArea") == 0)
			  m_Header.dMMConformityIndexOverArea = pValue_double;
		  else if (pName.compare("MVCollimatorY1") == 0)
			  m_Header.dMVCollimatorY1 = pValue_double;
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
		  if (pName.compare("KVCollimatorFoil") == 0)
			  m_Header.sKVCollimatorFoil = pValue_str;
		  else if (pName.compare("ImageSourceId") == 0)
			  m_Header.sImageSourceId = pValue_str;
		  else if (pName.compare("ImageModeId") == 0)
			  m_Header.sImageModeId = pValue_str;
		  else if (pName.compare("KVCollimatorShape") == 0)
			  m_Header.sKVCollimatorShape = pValue_str;
		  else if (pName.compare("MVEnergy") == 0)
			  m_Header.sMVEnergy = pValue_str;
		  else if (pName.compare("MMTrackingSourceId0") == 0)
			  m_Header.sMMTrackingSourceId0 = pValue_str;
		  else if (pName.compare("CalibrationSetId") == 0)
			  m_Header.sCalibrationSetId = pValue_str;
		  else if (pName.compare("AcquisitionNotes") == 0)
			  m_Header.sAcquisitionNotes = pValue_str;
		  else if (pName.compare("KVFluoroLevelControl") == 0)
			  m_Header.sKVFluoroLevelControl = pValue_str;
		  else if (pName.compare("MMTrackingSourceId1") == 0)
			  m_Header.sMMTrackingSourceId1 = pValue_str;
		  else if (pName.compare("AcquisitionModeId") == 0)
			  m_Header.sAcquisitionModeId = pValue_str;
		  else if (pName.compare("KVFocalSpot") == 0)
			  m_Header.sKVFocalSpot = pValue_str;
		  else if (pName.compare("NDIToolId") == 0)
			  m_Header.sNDIToolId = pValue_str;
		  else if (pName.compare("AcquisitionMode") == 0)
			  m_Header.sAcquisitionMode = pValue_str;
		  else if (pName.compare("AcquisitionSystemVersion") == 0)
			  m_Header.sAcquisitionSystemVersion = pValue_str;
		  else if (pName.compare("ImageMode") == 0)
			  m_Header.sImageMode = pValue_str;
		  else if (pName.compare("MMTrackingSourceId2") == 0)
			  m_Header.sMMTrackingSourceId2 = pValue_str;
		  else if (pName.compare("GUID") == 0)
			  m_Header.sGUID = pValue_str;
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
		  if (pName.compare("MLCLeafsA") == 0)
			  m_Header.vdMLCLeafsA = pValue_vd;
		  else if (pName.compare("MLCLeafsB") == 0)
			  m_Header.vdMLCLeafsB = pValue_vd;
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
		  if (pName.compare("HistogramRoi") == 0)
			  m_Header.viHistogramRoi = pValue_vi;
	  }
	  else
	  {
		  itkGenericExceptionMacro(<< "Property type " << pType << " not supported in " << m_FileName);
	  }
  }

  if(fclose (fp) != 0)
    itkGenericExceptionMacro(<< "Could not close file: " << m_FileName);

  /* Convert xim to ITK image information */
  SetNumberOfDimensions(2);
  SetDimensions(0, m_Header.iImageWidth);
  SetDimensions(1, m_Header.iImageHeight);
  SetSpacing(0, m_Header.dPixelWidth * 10);
  SetSpacing(1, m_Header.dPixelHeight * 10);
  SetOrigin(0, -0.5*(m_Header.iImageWidth - 1)*m_Header.dPixelWidth); //SR: assumed centered
  SetOrigin(1, -0.5*(m_Header.iImageHeight - 1)*m_Header.dPixelHeight); //SR: assumed centered
  if (m_Header.iBytesPerPixel == 1)
	  SetComponentType(itk::ImageIOBase::CHAR);
  else if (m_Header.iBytesPerPixel == 2)
	  SetComponentType(itk::ImageIOBase::SHORT);
  else if (m_Header.iBytesPerPixel == 4)
	  SetComponentType(itk::ImageIOBase::INT);
  else
  {
	  itkGenericExceptionMacro(<< "Number of bytes per pixel: " << m_Header.iBytesPerPixel << " not supported in " << m_FileName);
  }

  /* Store important meta information in the meta data dictionary */
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dCTProjectionAngle", -1. * m_Header.dKVSourceRtn + 180);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "ikVNormChamber", m_Header.iKVNormChamber);
  itk::EncapsulateMetaData<bool>(this->GetMetaDataDictionary(), "bIsCompressed", m_Header.iCompressionIndicator > 0);
  itk::EncapsulateMetaData<int>(this->GetMetaDataDictionary(), "iImageBodyPosition", m_Header.iImageBodyPosition);

  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dkVSourceRtn", m_Header.dKVSourceRtn);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dkVSourceVrt", m_Header.dKVSourceVrt * 10);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dkVSourceLat", m_Header.dKVSourceLat * 10);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dkVSourceLng", m_Header.dKVSourceLng * 10);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dkVSourcePth", m_Header.dKVSourcePitch);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dkVDetectorRtn", m_Header.dKVDetectorRtn);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dkVDetectorVrt", m_Header.dKVDetectorVrt * 10);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dkVDetectorLat", m_Header.dKVDetectorLat * 10);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dkVDetectorLng", m_Header.dKVDetectorLng * 10);
  itk::EncapsulateMetaData<double>(this->GetMetaDataDictionary(), "dkVDetectorPth", m_Header.dKVDetectorPitch);
}

//--------------------------------------------------------------------
bool igt::XimImageIO::CanReadFile(const char* FileNameToRead)
{
  std::string                  filename(FileNameToRead);
  const std::string::size_type it = filename.find_last_of( "." );
  std::string                  fileExt( filename, it+1, filename.length() );

  if (fileExt != std::string("xim") && fileExt != std::string("XIM"))
    return false;
  return true;
}

//--------------------------------------------------------------------
// Read Image Content
void igt::XimImageIO::Read(void * buffer)
{
  FILE *fp;

  fp = fopen(m_FileName.c_str(), "rb");
  if (fp == NULL)
	  itkGenericExceptionMacro(<< "Could not open file (for reading): " << m_FileName);

  if (m_Header.iCompressionIndicator == 0)
  {
	  fseek(fp, m_Header.iImageBodyPosition, SEEK_SET);
	  
	  if (m_Header.iBytesPerPixel == 1)
		  fread(buffer, sizeof(char), GetDimensions(0) * GetDimensions(1), fp);
	  else if (m_Header.iBytesPerPixel == 2)
		  fread(buffer, sizeof(short), GetDimensions(0) * GetDimensions(1), fp);
	  else if (m_Header.iBytesPerPixel == 4)
		  fread(buffer, sizeof(int), GetDimensions(0) * GetDimensions(1), fp);
	  else
	  {
		  itkGenericExceptionMacro(<< "Number of bytes per pixel: " << m_Header.iBytesPerPixel << " not supported in " << m_FileName);
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

	  pt_lut = (unsigned char*)malloc(sizeof (unsigned char)* m_Header.iLookupTableSize);
	  fseek(fp, m_Header.iLookupTablePosition, SEEK_SET);
	  if (m_Header.iLookupTableSize != fread(pt_lut, sizeof (unsigned char), m_Header.iLookupTableSize, fp))
		  itkGenericExceptionMacro(<< "Could not read image LUT in: " << m_FileName);

	  fseek(fp, m_Header.iImageBodyPosition, SEEK_SET);

	  /* Read first row + first pixel of second row*/
	  for (i = 0; i < GetDimensions(0) + 1; i++) {
		  if (1 != fread(&a, sizeof(int), 1, fp))
			  itkGenericExceptionMacro(<< "Could not read first row and first pixel of second row in: " << m_FileName);
		  buf[i] = a;
	  }

	  for (i = GetDimensions(0) + 1; i < GetDimensions(0) * GetDimensions(1); ++i)
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
				  itkGenericExceptionMacro(<< "Could not loop through a pixel of char type in: " << m_FileName);
			  diff = dc;
		  }
		  else if (v == 1)
		  {
			  if (1 != fread((void *)&ds, sizeof(short), 1, fp))
				  itkGenericExceptionMacro(<< "Could not loop through a pixel of short type in: " << m_FileName);
			  diff = ds;
		  }
		  else
		  {
			  if (1 != fread((void *)&diff, sizeof(int), 1, fp))
				  itkGenericExceptionMacro(<< "Could not loop through a pixel of int type in: " << m_FileName);
		  }
		  buf[i] = diff + buf[i - 1] + buf[i - m_Header.iImageWidth] - buf[i - m_Header.iImageWidth - 1];
	  }
  }

  if(fclose (fp) != 0)
    itkGenericExceptionMacro(<< "Could not close file: " << m_FileName);
  return;
}

//--------------------------------------------------------------------
bool igt::XimImageIO::CanWriteFile(const char* itkNotUsed(FileNameToWrite))
{
  return false;
}

//--------------------------------------------------------------------
// Write Image
void igt::XimImageIO::Write(const void* itkNotUsed(buffer))
{
  //TODO?
}

unsigned char igt::XimImageIO::reverse_byte(unsigned char x)
{
	static const unsigned char table[] = {
		0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
		0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
		0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
		0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
		0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
		0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
		0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
		0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
		0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
		0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
		0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
		0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
		0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
		0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
		0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
		0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
		0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
		0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
		0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
		0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
		0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
		0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
		0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
		0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
		0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
		0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
		0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
		0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
		0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
		0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
		0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
		0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
	};
	return table[x];
}