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

#ifndef __igtProjectionsReader_h
#define __igtProjectionsReader_h

#include <itkImageSource.h>

// Standard lib
#include <vector>
#include <string>

namespace igt
{

/** \class ProjectionsReader
 *		   
 *  \author Andy Shieh
 *
 * \ingroup ImageSource
 */
template <class TOutputImage>
class ITK_EXPORT ProjectionsReader : public itk::ImageSource<TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef ProjectionsReader              Self;
  typedef itk::ImageSource<TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>        Pointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(ProjectionsReader, itk::ImageSource);

  /** Some convenient typedefs. */
  typedef TOutputImage							OutputImageType;
  typedef typename OutputImageType::Pointer		OutputImagePointer;
  typedef typename OutputImageType::SizeType	OutputImageSizeType;
  typedef typename OutputImageType::IndexType	OutputImageIndexType;
  typedef typename OutputImageType::SpacingType OutputImageSpacingType;
  typedef typename OutputImageType::PointType   OutputImagePointType;
  typedef typename OutputImageType::RegionType	OutputImageRegionType;
  typedef typename OutputImageType::PixelType	OutputImagePixelType;

  typedef  std::vector<std::string> FileNamesContainer;

  /** ImageDimension constant */
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension);

  // Case insensitive string comparison
  bool strcmpi(const std::string& a, const std::string& b)
  {
	  unsigned int sz = a.size();
	  if (b.size() != sz)
		  return false;
	  for (unsigned int i = 0; i < sz; ++i)
	  if (tolower(a[i]) != tolower(b[i]))
		  return false;
	  return true;
  }

  /** Set the vector of strings that contains the file names. Files
   * are processed in sequential order. */
  void SetFileNames (const FileNamesContainer &name)
    {
    if ( m_Filenames != name)
      {
      m_Filenames = name;
      this->Modified();
      }
    }
  void SetFileNames(std::string name)
  {
	  m_Filenames.clear();
	  m_Filenames.push_back(name);
	  this->Modified();
  }
  const FileNamesContainer & GetFileNames() const
    {
    return m_Filenames;
    }

  /** Prepare the allocation of the output image during the first back
   * propagation of the pipeline. */
  virtual void GenerateOutputInformation(void);
  
  // Andy 2015: attenuation conversion option
  itkSetMacro(ProjectionConversion, bool);
  itkGetMacro(ProjectionConversion, bool);

  itkGetMacro(AnglesDegree, std::vector<double>);

  // Common header struct
  typedef struct ProjectionInfo {
	  unsigned int width;
	  unsigned int height;
	  double spacingX;
	  double spacingY;
	  double sourceLat;
	  double sourceVrt;
	  double sourceLng;
	  double sourcePth;
	  double detectorLat;
	  double detectorVrt;
	  double detectorLng;
	  double detectorPth;
	  double normChamberValue;
	  double timeTag;
	  bool isCompressed;
	  std::string format;
	  double angle;
	  unsigned int headerSize;
  } ProjectionInfo;

  // Varian header struct
  typedef struct HncHnd_header {
	  char sFileType[32];
	  unsigned int FileLength;
	  char sChecksumSpec[4];
	  unsigned int nCheckSum;
	  char sCreationDate[8];
	  char sCreationTime[8];
	  char sPatientID[16];
	  unsigned int nPatientSer;
	  char sSeriesID[16];
	  unsigned int nSeriesSer;
	  char sSliceID[16];
	  unsigned int nSliceSer;
	  unsigned int SizeX;
	  unsigned int SizeY;
	  double dSliceZPos;
	  char sModality[16];
	  unsigned int nWindow;
	  unsigned int nLevel;
	  unsigned int nPixelOffset;
	  char sImageType[4];
	  double dGantryRtn;
	  double dSAD;
	  double dSFD;
	  double dCollX1;
	  double dCollX2;
	  double dCollY1;
	  double dCollY2;
	  double dCollRtn;
	  double dFieldX;
	  double dFieldY;
	  double dBladeX1;
	  double dBladeX2;
	  double dBladeY1;
	  double dBladeY2;
	  double dIDUPosLng;
	  double dIDUPosLat;
	  double dIDUPosVrt;
	  double dIDUPosRtn;
	  double dPatientSupportAngle;
	  double dTableTopEccentricAngle;
	  double dCouchVrt;
	  double dCouchLng;
	  double dCouchLat;
	  double dIDUResolutionX;
	  double dIDUResolutionY;
	  double dImageResolutionX;
	  double dImageResolutionY;
	  double dEnergy;
	  double dDoseRate;
	  double dXRayKV;
	  double dXRayMA;
	  double dMetersetExposure;
	  double dAcqAdjustment;
	  double dCTProjectionAngle;
	  double dCTNormChamber;
	  double dGatingTimeTag;
	  double dGating4DInfoX;
	  double dGating4DInfoY;
	  double dGating4DInfoZ;
	  double dGating4DInfoTime;
  } HncHnd_header;

  typedef struct Xim_header {
	  char sFileFormatIdentifier[8];
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
	  std::vector<std::string> vsScalarIntPropertyName;
	  std::vector<int> viScalarIntPropertyValue;
	  std::vector<std::string> vsScalarDoublePropertyName;
	  std::vector<double> vdScalarDoublePropertyValue;
	  std::vector<std::string> vsStringPropertyName;
	  std::vector<std::string> vsStringPropertyValue;
	  std::vector<std::string> vsVectorIntPropertyName;
	  std::vector<std::vector<int>> vviVectorIntPropertyValue;
	  std::vector<std::string> vsVectorDoublePropertyName;
	  std::vector<std::vector<double>> vvdVectorDoublePropertyValue;
	  double dPixelWidth;
	  double dPixelHeight;
	  double ikVNormChamber;
	  double dkVSourceRtn;
	  double dkVSourceVrt;
	  double dkVSourceLat;
	  double dkVSourceLng;
	  double dkVSourcePth;
	  double dkVDetectorRtn;
	  double dkVDetectorVrt;
	  double dkVDetectorLat;
	  double dkVDetectorLng;
	  double dkVDetectorPth;
	  int iStartTime;
  } Xim_header;

  itkGetMacro(ProjectionInfos, std::vector<ProjectionInfo>);

protected:
	ProjectionsReader() {
		m_ProjectionConversion = true;
		m_Size.Fill(1);
		m_Spacing.Fill(1);
		m_Origin.Fill(0);
		m_HeaderSize = 0;
	};
  ~ProjectionsReader() {};

  /** Does the real work. */
  virtual void GenerateData();

  /** A list of filenames to be processed. */
  FileNamesContainer m_Filenames;

  void ReadHncHndHeader(std::string filename);

  void ReadXimHeader(std::string filename);

  void ReadHisHeader(std::string filename);

  void ReadHnc(std::string filename, void * buffer);

  void ReadHnd(std::string filename, void * buffer);

  void ReadHis(std::string filename, void * buffer);

  void ReadAtt(std::string filename, void * buffer);

  void ReadXim(std::string filename, void * buffer);

private:
  ProjectionsReader(const Self&); //purposely not implemented
  void operator=(const Self&);    //purposely not implemented
  
  // Andy 2015: Option to decide whether to convert to attenuation or not
  bool m_ProjectionConversion;

  OutputImageSizeType m_Size;
  OutputImageSpacingType m_Spacing;
  OutputImagePointType m_Origin;

  double m_NormChamberValue;

  bool m_isCompressed;

  unsigned int m_BytesPerPixel;

  unsigned int m_LookupTableSize;

  std::vector<double> m_AnglesDegree;

  unsigned int m_HeaderSize;

  std::vector<ProjectionInfo> m_ProjectionInfos;
};

} //namespace rtk

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtProjectionsReader.cxx"
#endif

#endif // __igtProjectionsReader_h
