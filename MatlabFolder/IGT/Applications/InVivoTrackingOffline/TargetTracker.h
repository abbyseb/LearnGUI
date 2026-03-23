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

#ifndef TargetTracker_H
#define TargetTracker_H

#include <QObject>

#include <rtkVectorSorter.h>

#include <itkImageMomentsCalculator.h>
#include <itkImageFileReader.h>
#include <itkRegularExpressionSeriesFileNames.h>
#include <itkLaplacianSharpeningImageFilter.h>

#include <itkImageFileWriter.h>

#include <igtProjectionsReader.h>
#include <igtTargetTracking.h>
#include <igtMultiple2DMeasurementTracking.h>
#include <igtFixedRayDistanceTracking.h>
#include <igtProjectionAlignmentFilter.h>

#ifdef IGT_USE_CUDA
	#include <itkCudaImage.h>
#endif

#ifndef M_PI
	#define M_PI vnl_math::pi
#endif

// The class for the actual tracking computation
class TargetTracker : public QObject
{
	Q_OBJECT

public:

	// Constructor/Destructor
	TargetTracker();
	~TargetTracker();

	// Image typedefs
	typedef float						   PixelType;
	typedef itk::Image< PixelType, 3 >     CPUImageType;
	typedef itk::Image< PixelType, 2 >     CPUTwoDImageType;
#ifdef IGT_USE_CUDA
	typedef itk::CudaImage< PixelType, 3 > ImageType;
	typedef itk::CudaImage< PixelType, 2 > TwoDImageType;
#else
	typedef CPUImageType				   ImageType;
	typedef CPUTwoDImageType			   TwoDImageType;
#endif

	// Typedefs for readers
	typedef igt::ProjectionsReader<ImageType>				ProjReaderType;
	typedef itk::ImageFileReader<ImageType>					ImageReaderType;

	// Image writer for testing purpose
	typedef itk::ImageFileWriter<TwoDImageType>				Image2DWriterType;
	typedef itk::ImageFileWriter<ImageType>					ImageWriterType;

	// Other objects
	typedef itk::ImageMomentsCalculator<ImageType>			ImageCentroidType;
	typedef itk::LaplacianSharpeningImageFilter<ImageType, ImageType>	SharpenImageType;
	typedef rtk::VectorSorter<std::string>					FileNameSorterType;
	typedef igt::TargetTracking<ImageType>					TrackingType;
	typedef igt::ProjectionAlignmentFilter<ImageType>		ProjectionAlignmentType;

	// Light objects type
	typedef TrackingType::VectorType		VectorType;
	typedef TrackingType::MatrixType		MatrixType;

	// Macros to get/set members
	itk::RegularExpressionSeriesFileNames::Pointer GetProjFileNames(){ return m_ProjFileNames; }
	itk::RegularExpressionSeriesFileNames::Pointer GetTargetFileNames(){ return m_TargetFileNames; }
	itk::RegularExpressionSeriesFileNames::Pointer GetBGFileNames(){ return m_BGFileNames; }

	std::string GetGeometryFileName(){ return m_GeometryFileName; }
	void SetGeometryFileName(std::string str){ m_GeometryFileName = str; }

	std::string GetSortFileName(){ return m_SortFileName; }
	void SetSortFileName(std::string str){ m_SortFileName = str; }

	std::string GetOutputDir(){ return m_OutputDir; }
	void SetOutputDir(std::string str){ m_OutputDir = str; }

	std::string GetGPUOption(){ return m_GPUOption; }
	void SetGPUOption(std::string str){ m_GPUOption = str; }

	unsigned int GetNProj(){ return m_NProj; }
	void SetNProj(unsigned int n){ m_NProj = n; }

	unsigned int GetNBin(){ return m_NBin; }
	void SetNBin(unsigned int n){ m_NBin = n; }

	bool GetUseBGSubtraction(){ return m_UseBGSubtraction; }
	void SetUseBGSubtraction(bool tag){ m_UseBGSubtraction = tag; }

	bool GetStopNow(){ return m_StopNow; }
	void SetStopNow(bool tag){ m_StopNow = tag; }

	bool GetGTCheck(){ return m_GTCheck; }
	void SetGTCheck(bool tag){ m_GTCheck = tag; }

	unsigned int GetFirstFrame(){ return m_FirstFrame; }
	void SetFirstFrame(unsigned int n){ m_FirstFrame = n; }

	unsigned int GetEndFrame(){ return m_EndFrame; }
	void SetEndFrame(unsigned int n){ m_EndFrame = n; }

	unsigned int GetFrameStep(){ return m_FrameStep; }
	void SetFrameStep(unsigned int n){ m_FrameStep = n; }

	double GetMatchingThreshold(){ return m_MatchingThreshold; }
	void SetMatchingThreshold(double value){ m_MatchingThreshold = value; }

	VectorType GetSurrogacySlope(){ return m_SurrogacySlope; }
	void SetSurrogacySlope(unsigned int k, double v){ m_SurrogacySlope[k] = v; }

	VectorType GetSurrogacyOffset(){ return m_SurrogacyOffset; }
	void SetSurrogacyOffset(unsigned int k, double v){ m_SurrogacyOffset[k] = v; }

	unsigned int GetTemplateMatchingMetricOption(){ return m_TemplateMatchingMetricOption; }
	void SetTemplateMatchingMetricOption(unsigned int n){ m_TemplateMatchingMetricOption = n; }

	const std::vector<unsigned int> &GetFrameNumbers(){ return m_FrameNumbers; }
	const std::vector<double> &GetMatchingMetricValues(){ return m_MatchingMetricValues; }
	const std::vector<double> &GetBGMatchScoreValues(){ return m_BGMatchScoreValues; }
	const std::vector<VectorType> &GetTrackingTrj(){ return m_TrackingTrj; }
	const std::vector<VectorType> &GetPriorTrj(){ return m_PriorTrj; }
	const std::vector<VectorType> &GetProjTranslationVector(){ return m_ProjTranslationVector; }
	const std::vector<ProjectionAlignmentType::TranslationTransformType::ParametersType> & GetBG2DShifts() { return m_BG2DShifts; }
	std::vector<double> &GetSurrogateSignal(){ return m_SurrogateSignal; }

	ImageType::SizeType GetModelSize(){ return m_ModelSize; }
	ImageType::SizeType GetProjSize(){ return m_ProjSize; }
	ImageType::SpacingType GetModelSpacing(){ return m_ModelSpacing; }
	ImageType::SpacingType GetProjSpacing(){ return m_ProjSpacing; }
	std::vector< VectorType > GetPriorCentroids(){ return m_PriorCentroids; }

	ImageType::ConstPointer GetProjImage(){ return m_ProjImage; }
	ImageType::ConstPointer GetDiffProjImage(){ return m_DiffProjImage; }
	ImageType::ConstPointer GetTargetDRRImage(){ return m_TargetDRRImage; }
	ImageType::RegionType	GetTargetDRRROI(){ return m_TargetDRRROI; }

	MatrixType GetStateCovarianceMatrix(){ return m_StateCovarianceMatrix; }
	MatrixType GetMeasurementCovarianceMatrix(){ return m_MeasurementCovarianceMatrix; }
	MatrixType GetKalmanGain(){ return m_KalmanGain; }

	void SetGroundTruthTrj(std::vector<VectorType> &trj){ m_GroundTruthTrj = trj; }
	const std::vector<VectorType> &GetGroundTruthTrj(){ return m_GroundTruthTrj; }
	void SetGroundTruthFrames(std::vector<unsigned int> &n){ m_GroundTruthFrames = n; }
	const std::vector<unsigned int> &GetGroundTruthFrames(){ return m_GroundTruthFrames; }

public slots:
	
// The actual computaitonal work for tracking
	void doTracking();

signals:

	void trackingDoneSignal(QString message);
	void trackingErrorSignal(QString message);

	void updateProgramStatusSignal(QString message, QString style);
	void updateFrameNumberSignal(unsigned int k);
	void updateProjAngleSignal(double ang);
	void updateBinNumberSignal(unsigned int k);
	void updateReadingTimeSignal(double t);
	void updateTrackingTimeSignal(double t);
	void updateTotalTimeSignal(double t);
	void updateStateCovarianceSignal(MatrixType p);
	void updateMatchingMetricSignal(double value);
	void updateBGMatchScoreSignal(double value);

	void resultReadySignal(unsigned int k_frame, unsigned int currentBin, double angle);
	void projViewReadySignal();
	void writeDataPropertiesSignal();

protected:

private:

	MatrixType estimateMeasurementUncertainties(double matchingMetric, double lowerBound, double upperBound, double bgScore);

	itk::RegularExpressionSeriesFileNames::Pointer m_ProjFileNames;
	itk::RegularExpressionSeriesFileNames::Pointer m_TargetFileNames;
	itk::RegularExpressionSeriesFileNames::Pointer m_BGFileNames;

	std::string m_GeometryFileName, m_SortFileName, m_OutputDir;

	std::string m_GPUOption;

	ImageType::SizeType m_ModelSize, m_ProjSize;
	ImageType::SpacingType m_ModelSpacing, m_ProjSpacing;
	std::vector< VectorType > m_PriorCentroids;

	TrackingType::DRRImageSpacingType m_DRRSearchRadius, m_BGMaxShift;
	TrackingType::DRRImageSpacingType m_DRRROIRadius;

	unsigned int m_NProj, m_NBin;

	bool m_UseBGSubtraction;

	bool m_StopNow;

	bool m_GTCheck;

	unsigned int m_FirstFrame, m_FrameStep, m_EndFrame;

	unsigned int m_TemplateMatchingMetricOption;

	double m_MatchingThreshold;

	double m_MSEBGThreshold, m_MMIBGThreshold, m_NCBGThreshold;

	unsigned int m_NumberOfPastPoints;

	// Triangulation angle. Not used currently.
	double m_TriangulationAngle;

	std::vector<unsigned int> m_FrameNumbers;
	std::vector<double>	m_MatchingMetricValues, m_BGMatchScoreValues;
	std::vector<VectorType> m_TrackingTrj, m_PriorTrj;
	std::vector<VectorType> m_ProjTranslationVector, m_TargetDRRCentroids;
	std::vector<ProjectionAlignmentType::TranslationTransformType::ParametersType> m_BG2DShifts;

	VectorType m_SurrogacySlope, m_SurrogacyOffset;
	std::vector<double> m_SurrogateSignal;

	ImageType::ConstPointer	m_ProjImage, m_DiffProjImage, m_TargetDRRImage;
	ImageType::RegionType	m_TargetDRRROI;

	MatrixType	m_StateCovarianceMatrix, m_MeasurementCovarianceMatrix,
		m_KalmanGain;

	// A backdoor to investigate whether the ground truth is appropriate
	std::vector<VectorType> m_GroundTruthTrj;
	std::vector<unsigned int> m_GroundTruthFrames;
};
#endif // TargetTracker_H