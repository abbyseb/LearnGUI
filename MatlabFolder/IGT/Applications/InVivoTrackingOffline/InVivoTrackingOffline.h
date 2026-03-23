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

#ifndef InVivoTrackingOffline_H
#define InVivoTrackingOffline_H

#include <QMainWindow>
#include <qfile.h>
#include <qthread.h>
#include <TargetTracker.h>
#include <igtITKToQImage.h>
#include <rtkThreeDCircularProjectionGeometryXMLFile.h>

#include <itkBinaryThresholdImageFilter.h>
#include <itkBinaryContourImageFilter.h>
#include <itkBinaryDilateImageFilter.h>
#include <itkFlatStructuringElement.h>

#include <itkStatisticsImageFilter.h>

// Forward Qt class declarations
class Ui_InVivoTrackingOffline;

// The GUI class
class InVivoTrackingOffline : public QMainWindow
{
	Q_OBJECT
	QThread trackingThread;

public:

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

	/** Some convenient typedefs */
	typedef igt::ITKToQImage<ImageType>		ITKToQImageType;

	typedef TargetTracker::TrackingType::MatrixType	MatrixType;
	typedef TargetTracker::TrackingType::VectorType	VectorType;

	typedef itk::StatisticsImageFilter<ImageType> ImageStatisticsType;

	typedef itk::BinaryThresholdImageFilter<ImageType, ImageType>	BinaryThresholdImageType;
	typedef itk::BinaryContourImageFilter<ImageType, ImageType>		BinaryContourImageType;
	typedef itk::FlatStructuringElement< 3 >						StructuringElementType;
	typedef itk::BinaryDilateImageFilter<ImageType, ImageType, StructuringElementType> DilateImageType;

	// Constructor/Destructor
	InVivoTrackingOffline();
	~InVivoTrackingOffline();

public slots:

protected:

	// Check required inputs and enable start tracking button
	void checkStartRequirement();

	// Enable/Disable input fields
	void enableInputFields();
	void disableInputFields();

	// Save configuration
	void saveConfiguration(QString saveFile_QStr);

	// Write and plot results
	void writeResults(unsigned int k_frame, unsigned int currentBin, double angle);
	void plotResults(unsigned int k_frame, unsigned int currentBin, double angle);

	// Clear results
	void clearResults();

	// Save configuration and initiate result logging file before tracking starts
	void logFilesBeforeStart();

	// Read surrogate file
	void readSurrogateFile();

	// Read geometry file
	void readGeometryFile();

	// Calculate ground truth plot it
	void getGroundTruthLRSIAP();
	void getGroundTruthLateralInDepth();

protected slots:
	
	// Projection directory
	void on_projDirPushButton_released();
	void on_projDirTextEdit_textChanged();

	// Target model directory
	void on_targetDirPushButton_released();
	void on_targetDirTextEdit_textChanged();

	// RTK Geometry file
	void on_geometryFilePushButton_released();
	void on_geometryFileTextEdit_textChanged();

	// Sorting file
	void on_sortFilePushButton_released();
	void on_sortFileTextEdit_textChanged();

	// Output log directory
	void on_logDirPushButton_released();
	void on_logDirTextEdit_textChanged();

	// Write results or not
	void on_writeResultsCheckBox_stateChanged(int state);

	// Background model directory
	void on_bgDirPushButton_released();
	void on_bgDirTextEdit_textChanged();

	// Ground truth file
	void on_gtFilePushButton_released();
	void on_gtFileTextEdit_textChanged();

	// Surrogate signal file
	void on_surrogateFilePushButton_released();
	void on_surrogateFileTextEdit_textChanged();

	// Start/Stop tracking
	void on_startPushButton_released();
	void on_stopPushButton_released();

	// When tracking finishes
	void onTrackingDone(QString message);
	void onTrackingError(QString message);

	// Change program status
	void updateProgramStatus(QString message, QString style);
	void updateFrameNumber(unsigned int k);
	void updateProjAngle(double ang);
	void updateBinNumber(unsigned int k);
	void updateReadingTime(double t);
	void updateTrackingTime(double t);
	void updateTotalTime(double t);
	void updateStateCovariance(MatrixType p);
	void updateMatchingMetric(double value);
	void updateBGMatchScore(double value);

	// Load configuration
	void on_actionLoad_Configuration_triggered();

	// Save cofiguration
	void on_actionSave_Configuration_triggered();

	// Plot trajectory
	void processResults(unsigned int k_frame, unsigned int currentBin, double angle);

	// Set projection view pixmap
	void setProjViewPixmap();

	// Write data properties and tracking results
	void writeDataProperties();

signals:

	void startTrackingSignal();

private:

	// The actual tracking object
	TargetTracker *m_TargetTracker;

	// Designer form
	Ui_InVivoTrackingOffline *ui;

	// Geometry reader
	rtk::ThreeDCircularProjectionGeometryXMLFileReader::Pointer m_GeometryReader;

	// Display
	double m_ProjViewMin, m_ProjViewMax, m_DiffViewMin, m_DiffViewMax;

	unsigned int m_PlotLength;

	// Results
	QVector<double> m_GTFrameNoQVector, m_GTLRQVector, m_GTSIQVector, m_GTAPQVector,
		m_GTLateralQVector, m_GTInDepthQVector,
		m_PlotFrameNoQVector, m_TrackingLRQVector, m_TrackingSIQVector, m_TrackingAPQVector,
		m_TrackingLateralQVector, m_TrackingInDepthQVector,
		m_PriorLRQVector, m_PriorSIQVector, m_PriorAPQVector,
		m_PriorLateralQVector, m_PriorInDepthQVector,
		m_MatchingMetricValues, m_BGMatchScoreValues;
	MatrixType m_StateCovarianceMatrix;

	// Tracking display mask
	QVector<QRgb> m_TrackingMaskColorTable;

	/** Default path for selecting file and directory */
	QString m_DefaultPath;

	/** Output QFile */
	QFile *m_TrackingResultsFile;
	QString m_DateTime;
};

#endif // InVivoTrackingOffline_H
