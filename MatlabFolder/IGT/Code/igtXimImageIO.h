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

#ifndef __igtXimImageIO_h
#define __igtXimImageIO_h

// itk include
#include <itkImageIOBase.h>

#if defined (_MSC_VER) && (_MSC_VER < 1600)
//SR: taken from
//#include "msinttypes/stdint.h"
#else
#include <stdint.h>
#endif

namespace igt {

/** \class XimImageIO
 * \brief Class for reading Xim Image file format
 *
 * Reads Xim files (file format used by Varian for Obi raw data).
 *
 * \author Andy Shieh
 *
 * \ingroup IOFilters
 */
class XimImageIO : public itk::ImageIOBase
{
public:
/** Standard class typedefs. */
  typedef XimImageIO              Self;
  typedef itk::ImageIOBase        Superclass;
  typedef itk::SmartPointer<Self> Pointer;
  typedef int					  PixelType;

  struct Xim_header {
	std::string sFileFormatIdentifier;
	int iFileFormatVersion;
	int iImageWidth;
	int iImageHeight;
	int iBitsPerPixel;
	int iBytesPerPixel;
	int iCompressionIndicator;
	int iLookupTableSize;
	int iLookupTablePosition;
	int iImageBodyPosition;
	int iCompressedPixelBufferSize;
	int iUncompressedPixelBufferSize;
	int iNumberOfBinsInHistogram;
	std::vector<int> viHistogramData;
	int iNumberOfProperties;
	
    double dMVCollimatorX1;
    double dMVDoseDelivered;
    int iMMSurrogateModelResultType1;
    double dMVDoseStop;
    double dKVDetectorVrt;
    double dMMPhase2;
    double dMVDetectorLat;
    double dControlPoint;
    double dKVCollimatorY1;
    double dKVCollimatorX1;
    double dMLCCarriageB;
    int iMMMotionModelResultType1;
    int iValid;
    int iStopTimeRemainder;
    double dMMPhase1;
    double dCouchVrt;
    double dFrameRate;
    double dKVDoseAreaProductStart;
    double dKVDoseAreaProductStop;
    int iMLCModel;
    std::string sKVCollimatorFoil;
    double dMMTrackingRemainderY;
    double dMVCollimatorY2;
    double dMVCollimatorX2;
    double dMVDetectorPitch;
    int iBeamState;
    int iNumberOfFrames;
    int iMMMotionModelResultType2;
    double dMVDetectorRtn;
    double dMMConformityIndexOverAreaDoserate;
    double dCouchLat;
    int iKVHardwareState;
    double dMVDetectorLng;
    double dCouchLng;
    int iImageSourceType;
    double dMMAmplitude1;
    std::string sImageSourceId;
    double dKVSourceLat;
    std::string sImageModeId;
    int iAcquisitionSystem;
    double dKVSourceVrt;
    double dKVDetectorLat;
    double dDataGain;
    int iStartTimeRemainder;
    double dKVSourceLng;
    double dKVDetectorPitch;
    int iKVBeamPulseCounter;
    std::string sKVCollimatorShape;
    int iKVNormChamber;
    double dKVAirKermaStart;
    int iImagePropertyVersionMinor;
    int iMMExtrapolationStartTime2Remainder;
    int iImagePropertyVersionMajor;
    std::string sMVEnergy;
    double dKVMilliAmperes;
    double dKVSourceRtn;
    std::vector<double> vdMLCLeafsA;
    std::vector<double> vdMLCLeafsB;
    int iHistogramBinSize;
    int iMMExtrapolationStartTime1Remainder;
    int iLastImage;
    int iExternalBeam;
    int iKVBeamHold;
    double dMMQuality2;
    double dPixelHeight;
    int iMotionCompensationHold;
    double dKVDetectorRtn;
    double dMLCCarriageA;
    double dMMConformityIndexUnderAreaRelative;
    double dMVCollimatorRtn;
    int iMMExtrapolationStartTime0Remainder;
    double dMMQualityAmp0;
    std::string sMMTrackingSourceId0;
    int iImageSourceIndex;
    double dKVDetectorLng;
    int iDataOffset;
    double dMVSourceVrt;
    double dGantryRtn;
    std::string sCalibrationSetId;
    double dMMQuality0;
    double dMMPhase0;
    double dCouchPit;
    double dMMConformityIndexOverAreaDoserateRelative;
    int iMMSegmentalAction;
    int iMMSurrogateModelResultType0;
    double dMMQuality1;
    double dKVSourcePitch;
    int iMMExtrapolationStartTime1;
    int iMMExtrapolationStartTime0;
    std::string sAcquisitionNotes;
    int iMMExtrapolationStartTime2;
    double dMVDoseRate;
    double dMMConformityIndexUnderAreaDoserate;
    int iMMMotionModelResultType0;
    std::string sKVFluoroLevelControl;
    int iKVAutoBrightnessControl;
    double dMMAmplitude2;
    std::string sMMTrackingSourceId1;
    double dMMTrackingRemainderX;
    int iMVBeamHold;
    std::string sAcquisitionModeId;
    std::string sKVFocalSpot;
    int iSequenceNumber;
    int iAcquisitionId;
    double dPixelWidth;
    int iNDIToolState;
    int iStopTime;
    int iMVBeamOn;
    int iMMSurrogateModelResultType2;
    double dMMConformityIndexUnderAreaDoserateRelative;
    double dCouchRtn;
    std::vector<int> viHistogramRoi;
    double dMVDetectorVrt;
    int iMMActionWindows;
    double dMMPhaseTarget;
    double dKVMilliSeconds;
    double dDoseConversionFactor;
    double dMMConformityIndexUnderArea;
    double dMMAmplitude0;
    double dMMQualityAmp2;
    std::string sNDIToolId;
    double dMMQualityAmp1;
    std::string sAcquisitionMode;
    double dMVDoseStart;
    std::string sAcquisitionSystemVersion;
    double dMMConformityIndexOverAreaRelative;
    std::string sImageMode;
    double dKVCollimatorY2;
    double dKVCollimatorX2;
    int iDataRescaleType;
    double dKVAirKermaStop;
    int iKVBeamOn;
    double dMMTrackingRemainderZ;
    std::string sMMTrackingSourceId2;
    int iMVBeamStateChange;
    double dCouchRol;
    int iDataShift;
    double dKVKiloVolts;
    int iImageFlipped;
    int iStartTime;
    double dUndefinedDouble;
    double dMMConformityIndexOverArea;
    double dMVCollimatorY1;
    std::string sGUID;
    };

  XimImageIO() : Superclass() {}

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(XimImageIO, itk::ImageIOBase);

  /*-------- This part of the interface deals with reading data. ------ */
  virtual void ReadImageInformation();

  virtual bool CanReadFile( const char* FileNameToRead );

  virtual void Read(void * buffer);

  /*-------- This part of the interfaces deals with writing data. ----- */
  virtual void WriteImageInformation(bool /*keepOfStream*/) { }

  virtual void WriteImageInformation() { WriteImageInformation(false); }

  virtual bool CanWriteFile(const char* filename);

  virtual void Write(const void* buffer);
  
  Xim_header GetHeader() { return m_Header; }

  double GetAngle() { return -1. * m_Header.dKVSourceRtn + 180; }
  double GetKVNormChamber() { return m_Header.iKVNormChamber; }
  double GetSID() { return m_Header.dKVSourceVrt * 10; }
  double GetIDUPosLng() { return m_Header.dKVDetectorLng * 10; }
  double GetIDUPosLat() { return m_Header.dKVDetectorLat * 10; }
  double GetIDUPosVrt() { return m_Header.dKVDetectorVrt * 10; }

private:

	Xim_header m_Header;

	unsigned char reverse_byte(unsigned char x);

}; // end class XimImageIO

} // end namespace

#endif /* end #define __igtXimImageIO_h */
