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

#include "TargetTracker.h"

#include <itkTimeProbe.h>
#include <rtkThreeDCircularProjectionGeometryXMLFile.h>
#include <rtkThreeDCircularProjectionGeometrySorter.h>
#include <rtkBinNumberReader.h>

// Constructor
TargetTracker::TargetTracker()
{
	m_StopNow = false;

	m_NProj = 0;
	m_NBin = 0;
	m_UseBGSubtraction = false;

	m_GPUOption = "None";

	m_TemplateMatchingMetricOption = 0;

	m_DRRSearchRadius[0] = 10;
	m_DRRSearchRadius[1] = 20;

	m_BGMaxShift[0] = 10;
	m_BGMaxShift[1] = 20;

	m_DRRROIRadius[0] = 100;
	m_DRRROIRadius[1] = 100;

	m_MatchingThreshold = 0.4;

	m_MSEBGThreshold = 0.1;
	m_MMIBGThreshold = 1.4;
	m_NCBGThreshold = 0.95;

	m_ProjImage = 0;
	m_DiffProjImage = 0;
	m_TargetDRRImage = 0;

	m_SurrogacyOffset = VectorType(3, 0);
	m_SurrogacySlope = VectorType(3, 0);

	// Projection file names
	m_ProjFileNames = itk::RegularExpressionSeriesFileNames::New();
	m_ProjFileNames->NumericSortOn();
	m_ProjFileNames->SetRegularExpression(".[hnc|hnd|his|att]");

	// Target model file names
	m_TargetFileNames = itk::RegularExpressionSeriesFileNames::New();
	m_TargetFileNames->NumericSortOn();
	m_TargetFileNames->SetRegularExpression(".[mha]");

	// BG model file names
	m_BGFileNames = itk::RegularExpressionSeriesFileNames::New();
	m_BGFileNames->NumericSortOn();
	m_BGFileNames->SetRegularExpression(".[mha]");

	m_NumberOfPastPoints = 50;

	m_GTCheck = false;
};

TargetTracker::~TargetTracker()
{

}

// Estimate measurement uncertainties
TargetTracker::MatrixType TargetTracker::
	estimateMeasurementUncertainties(double matchingMetric, double lowerBound, double upperBound, double bgScore)
{
	//MatrixType R(3, 3, 0);
	MatrixType R(2, 2, 0);

	if (m_TemplateMatchingMetricOption == 0) // MSE
	{
		matchingMetric = (matchingMetric <= 0) ? std::numeric_limits<double>::epsilon() : matchingMetric;
		double threshold = (m_MatchingThreshold <= 0) ? std::numeric_limits<double>::epsilon() : m_MatchingThreshold;
		R.fill_diagonal(
			(-1. / (1. + pow(matchingMetric / threshold, 20)) + 1) * (upperBound - lowerBound) + lowerBound);
	}
	else if (m_TemplateMatchingMetricOption == 1 || m_TemplateMatchingMetricOption == 2) // MMI and NC
	{	
		double invMetric = (1. - matchingMetric > 1)? 0 : 1 - matchingMetric;
		invMetric = (invMetric <= 0) ? std::numeric_limits<double>::epsilon() : invMetric;
		double invThreshold = (1. - m_MatchingThreshold > 1) ? 0 : 1 - m_MatchingThreshold;
		invThreshold = (invThreshold <= 0) ? std::numeric_limits<double>::epsilon() : invThreshold;
		R.fill_diagonal(
			(-1. / (1. + pow(invMetric / invThreshold, 20)) + 1) * (upperBound - lowerBound) + lowerBound);
	}

	if ((m_TemplateMatchingMetricOption == 0 && bgScore > m_MSEBGThreshold) ||
		(m_TemplateMatchingMetricOption == 1 && bgScore < m_MMIBGThreshold) ||
		(m_TemplateMatchingMetricOption == 2 && bgScore < m_NCBGThreshold))
		R(0, 0) = upperBound;

	return R;
}

void TargetTracker::doTracking()
{
	// Clear results
	m_TrackingTrj.clear();
	m_PriorTrj.clear();
	m_PriorCentroids.clear();
	m_FrameNumbers.clear();
	m_ProjTranslationVector.clear();
	m_TargetDRRCentroids.clear();
	m_MatchingMetricValues.clear();
	m_BG2DShifts.clear();

	// Read sorting information
	rtk::BinNumberReader::Pointer sortInfoReader = rtk::BinNumberReader::New();
	emit updateProgramStatusSignal("Reading sorting information...",
		"QLabel{ color: green; }");
	sortInfoReader->SetFileName(m_SortFileName);
	sortInfoReader->Parse();

	if (sortInfoReader->GetOutput().size() != m_NProj)
	{
		emit trackingErrorSignal(QString("Number of entries in sorting information file does not match with number of projections."));
		return;
	}

	if (sortInfoReader->GetMaxBinNumber() != m_NBin)
	{
		emit trackingErrorSignal(QString("Number of bins in sorting information file does not match with number of target models."));
		return;
	}

	// Read geometry
	emit updateProgramStatusSignal("Reading geometry information...",
		"QLabel{ color: green; }");
	rtk::ThreeDCircularProjectionGeometryXMLFileReader::Pointer geometryReader
		= rtk::ThreeDCircularProjectionGeometryXMLFileReader::New();
	geometryReader->SetFilename(m_GeometryFileName);
	geometryReader->GenerateOutputInformation();
	// Need to enforce full-scan geometry even if it's not for the small angular range of DTS
	geometryReader->GetOutputObject()->SetShortScanOff();
	geometryReader->GetOutputObject()->SetDTSOn();

	if (geometryReader->GetOutputObject()->GetGantryAngles().size() != m_NProj)
	{
		emit trackingErrorSignal(QString("Number of entries in geometry information file does not match with number of projections."));
		return;
	}

	// Read target models
	emit updateProgramStatusSignal("Reading target models...",
		"QLabel{ color: green; }");
	std::vector< ImageReaderType::Pointer > targetReaders;
	std::vector< ImageType::Pointer > targetModels;
	std::vector<VectorType> pastPoints;
	m_PriorCentroids.clear();
	for (unsigned int k_Bin = 0; k_Bin != m_NBin; ++k_Bin)
	{
		targetReaders.push_back(ImageReaderType::New());
		targetReaders[k_Bin]->SetFileName(m_TargetFileNames->GetFileNames()[k_Bin]);
		targetReaders[k_Bin]->Update();
		targetModels.push_back(targetReaders[k_Bin]->GetOutput());
		targetModels[k_Bin]->DisconnectPipeline();

		// Calculate target centroid
		ImageCentroidType::Pointer centroidCalculator = ImageCentroidType::New();
		centroidCalculator->SetImage(targetModels[k_Bin]);
		centroidCalculator->Compute();
		m_PriorCentroids.push_back(centroidCalculator->GetCenterOfGravity().GetVnlVector());
		pastPoints.push_back(centroidCalculator->GetCenterOfGravity().GetVnlVector());
	}
	m_ModelSize = targetModels[0]->GetLargestPossibleRegion().GetSize();
	m_ModelSpacing = targetModels[0]->GetSpacing();
	VectorType meanPriorCentroids = TrackingType::EstimatorType::CalculateMean(m_PriorCentroids);
	MatrixType covPriorCentroids = TrackingType::EstimatorType::CalculateCovariance(m_PriorCentroids, meanPriorCentroids, false);

	// Set up prediction uncertainty for each phase bin
	// Estimate initial prediction uncertainty using variance in prior centroids
	std::vector<MatrixType> predictionUncertanties4D;
	for (unsigned int k_Bin = 0; k_Bin != m_NBin; ++k_Bin)
		predictionUncertanties4D.push_back(covPriorCentroids);

	// Read background models
	std::vector< ImageReaderType::Pointer > bgReaders;
	std::vector< ImageType::Pointer > bgModels;
	if (m_UseBGSubtraction)
	{
		emit updateProgramStatusSignal("Reading background models...",
			"QLabel{ color: green; }");

		for (unsigned int k_Bin = 0; k_Bin != m_NBin; ++k_Bin)
		{
			bgReaders.push_back(ImageReaderType::New());
			bgReaders[k_Bin]->SetFileName(m_BGFileNames->GetFileNames()[k_Bin]);
			bgReaders[k_Bin]->Update();
			bgModels.push_back(bgReaders[k_Bin]->GetOutput());
			bgModels[k_Bin]->DisconnectPipeline();
		}
	}

	// Read one projection so we can write data properties beforehand
	emit updateProgramStatusSignal("Collecting projection data properties...",
		"QLabel{ color: green; }");
	ProjReaderType::Pointer projReaderTmp = ProjReaderType::New();
	projReaderTmp->SetFileNames(m_ProjFileNames->GetFileNames()[0]);
	projReaderTmp->Update();
	m_ProjSize = projReaderTmp->GetOutput()->GetLargestPossibleRegion().GetSize();
	m_ProjSpacing = projReaderTmp->GetOutput()->GetSpacing();
	
	emit writeDataPropertiesSignal();

	// Initiate tracking filter
	TrackingType::Pointer trackingFilter = TrackingType::New();
	trackingFilter->SetTemplateMatchingMetricOption(m_TemplateMatchingMetricOption);
	if (m_GPUOption.compare("CUDA") == 0)
		trackingFilter->SetUseCUDA(true);

	// Set the uncertainties for the prediction step
	trackingFilter->GetEstimator()->SetP(MatrixType(3,3,0));

	// Vector of angular values
	std::vector<double> angles;

	// previous and current bin
	unsigned int currentBin = sortInfoReader->GetOutput()[m_FirstFrame], prevReliableBin = currentBin;
	VectorType prevReliableCentroid = m_PriorCentroids[currentBin - 1];

	// Begins
	for (unsigned int k_frame = m_FirstFrame; k_frame <= m_EndFrame; k_frame += m_FrameStep)
	{
		// Bin number read from sorting file
		currentBin = sortInfoReader->GetOutput()[k_frame];
		unsigned int k_count = (k_frame - m_FirstFrame) / m_FrameStep;

		itk::TimeProbe totalTimeProbe;
		totalTimeProbe.Start();

		emit updateFrameNumberSignal(k_frame+1);
		emit updateBinNumberSignal(currentBin);

		m_FrameNumbers.push_back(k_frame + 1);

		// Get the sorting vector for this frame
		std::vector< unsigned int > sortVector(m_ProjFileNames->GetFileNames().size(), 0);
		sortVector[k_frame] = 1;

		// Measure computation time for reading and other miscellaneous process
		itk::TimeProbe readingTimeProbe;
		readingTimeProbe.Start();

		// Sort geometry
		rtk::ThreeDCircularProjectionGeometrySorter::Pointer geometrySorter
			= rtk::ThreeDCircularProjectionGeometrySorter::New();
		geometrySorter->SetInputGeometry(geometryReader->GetOutputObject());
		geometrySorter->SetBinNumberVector(sortVector);
		geometrySorter->Sort();
		angles.push_back(geometrySorter->GetOutput()[0]->GetGantryAngles().back() * 180. / M_PI);
		emit updateProjAngleSignal(angles.back());

		// Read projections
		emit updateProgramStatusSignal(QString("Reading projections for frame ") + QString::number(k_frame + 1) + QString("..."),
			"QLabel{ color: green; }");
		FileNameSorterType::Pointer projNameSorter = FileNameSorterType::New();
		projNameSorter->SetInputVector(m_ProjFileNames->GetFileNames());
		projNameSorter->SetBinNumberVector(sortVector);
		projNameSorter->Sort();
		ProjReaderType::Pointer projReader = ProjReaderType::New();
		projReader->SetFileNames(projNameSorter->GetOutput()[0]);
		projReader->Update();
		m_ProjImage = projReader->GetOutput();

		readingTimeProbe.Stop();
		emit updateReadingTimeSignal(readingTimeProbe.GetMeanTime());

		// Tracking time probe starts from here
		emit updateProgramStatusSignal(QString("Tracking target for frame ") + QString::number(k_frame + 1) + QString("..."),
			"QLabel{ color: green; }");
		itk::TimeProbe trackingTimeProbe;
		trackingTimeProbe.Start();

		// Sharpen the projections
		ImageType::Pointer sharpenedProj = projReader->GetOutput();
		SharpenImageType::Pointer sharpenProjFilter = SharpenImageType::New();
		sharpenProjFilter->SetInput(projReader->GetOutput());
		//sharpenProjFilter->Update();
		//sharpenedProj = sharpenProjFilter->GetOutput();
		sharpenedProj = projReader->GetOutput();

		// Estimate current position to be previous position + difference between current and previous bin
		VectorType estimatedCentroid = 
			prevReliableCentroid + (m_PriorCentroids[currentBin - 1] - m_PriorCentroids[prevReliableBin - 1]);
		
		// Backdoor check using ground truth
		if (m_GTCheck)
		{
			for (unsigned int k_gt = 0; k_gt != m_GroundTruthFrames.size(); ++k_gt)
			{
				if (m_GroundTruthFrames[k_gt] == k_frame)
				{
					estimatedCentroid = m_GroundTruthTrj[k_gt];
					break;
				}
			}
			m_DRRSearchRadius.Fill(0);
			trackingFilter->SetNumberOfIterations(0);
		}

		// Estimate ROI of DRR
		VectorType estimatedDRRCentroid = geometrySorter->GetOutput()[0]->CalculateProjSpaceCoordinates(estimatedCentroid)[0];
		ProjectionAlignmentType::RegistrationImageIndexType drrROIIndex, drrROIEndIndex;
		drrROIIndex[0] = estimatedDRRCentroid[0] / (projReader->GetOutput()->GetSpacing()[0])
			+ 0.5 * (projReader->GetOutput()->GetLargestPossibleRegion().GetSize()[0] - 1)
			- m_DRRROIRadius[0] / (projReader->GetOutput()->GetSpacing()[0]);
		drrROIIndex[1] = estimatedDRRCentroid[1] / (projReader->GetOutput()->GetSpacing()[1])
			+ 0.5 * (projReader->GetOutput()->GetLargestPossibleRegion().GetSize()[1] - 1)
			- m_DRRROIRadius[1] / (projReader->GetOutput()->GetSpacing()[1]);
		drrROIIndex[0] = (drrROIIndex[0] < 0) ? 0 : drrROIIndex[0];
		drrROIIndex[1] = (drrROIIndex[1] < 0) ? 0 : drrROIIndex[1];
		drrROIEndIndex[0] = estimatedDRRCentroid[0] / (projReader->GetOutput()->GetSpacing()[0])
			+ 0.5 * (projReader->GetOutput()->GetLargestPossibleRegion().GetSize()[0] - 1)
			+ m_DRRROIRadius[0] / (projReader->GetOutput()->GetSpacing()[0]);
		drrROIEndIndex[1] = estimatedDRRCentroid[1] / (projReader->GetOutput()->GetSpacing()[1])
			+ 0.5 * (projReader->GetOutput()->GetLargestPossibleRegion().GetSize()[1] - 1)
			+ m_DRRROIRadius[1] / (projReader->GetOutput()->GetSpacing()[1]);
		drrROIEndIndex[0] = (drrROIEndIndex[0] > (projReader->GetOutput()->GetLargestPossibleRegion().GetSize()[0] - 1)) ?
			(projReader->GetOutput()->GetLargestPossibleRegion().GetSize()[0] - 1) : drrROIEndIndex[0];
		drrROIEndIndex[1] = (drrROIEndIndex[1] > (projReader->GetOutput()->GetLargestPossibleRegion().GetSize()[1] - 1)) ?
			(projReader->GetOutput()->GetLargestPossibleRegion().GetSize()[1] - 1) : drrROIEndIndex[1];
		ProjectionAlignmentType::RegistrationImageSizeType drrROISize;
		drrROISize[0] = drrROIEndIndex[0] - drrROIIndex[0] + 1;
		drrROISize[1] = drrROIEndIndex[1] - drrROIIndex[1] + 1;
		ProjectionAlignmentType::RegistrationImageRegionType drrROI(drrROIIndex, drrROISize);

		// Set up tracking filter
		trackingFilter->SetSharpenDRR(false);
		trackingFilter->SetDRRSearchRadius(m_DRRSearchRadius);
		trackingFilter->SetTargetImage(targetModels[currentBin - 1]);
		trackingFilter->SetTargetCentroid(estimatedCentroid);
		trackingFilter->SetGeometry(geometrySorter->GetOutput()[0]);
		// Track first with the original projection
		trackingFilter->SetReferenceProjectionImage(sharpenedProj);

		// Set prediction uncertainty
		if (pastPoints.size() < m_NumberOfPastPoints)
			predictionUncertanties4D[currentBin - 1] =
				TrackingType::EstimatorType::CalculateCovariance(m_PriorCentroids, meanPriorCentroids, false);
		else
		{
			VectorType pastPoints_mean = TrackingType::EstimatorType::CalculateMean(pastPoints);
			predictionUncertanties4D[currentBin - 1] =
				TrackingType::EstimatorType::CalculateCovariance(pastPoints, pastPoints_mean, false);
		}


		trackingFilter->GetEstimator()->SetQ(predictionUncertanties4D[currentBin - 1]);

		// First, do the measurement, i.e. template matching
		trackingFilter->MeasurePosition();

		// If matching score is too bad, use background subtracted projections
		double matchingMetric = trackingFilter->GetMatchingMetric();
		double bgMatchScore = 0;
		if (
			m_UseBGSubtraction && !m_GTCheck &&
			((m_TemplateMatchingMetricOption == 0 && matchingMetric > m_MSEBGThreshold) || 
			(m_TemplateMatchingMetricOption == 1 && matchingMetric < m_MMIBGThreshold) ||
			(m_TemplateMatchingMetricOption == 2 && matchingMetric < m_NCBGThreshold))
			)
		{
			// Background subtraction
			ProjectionAlignmentType::Pointer projAlignmentFilter = ProjectionAlignmentType::New();
			projAlignmentFilter->SetSharpenDRR(false);
			projAlignmentFilter->SetMatchMetricOption(m_TemplateMatchingMetricOption);
			if (m_TemplateMatchingMetricOption == 0)
				projAlignmentFilter->SetMatchThreshold(m_MSEBGThreshold);
			else if (m_TemplateMatchingMetricOption == 1)
				projAlignmentFilter->SetMatchThreshold(m_MMIBGThreshold);
			else if (m_TemplateMatchingMetricOption == 2)
				projAlignmentFilter->SetMatchThreshold(m_NCBGThreshold);
			projAlignmentFilter->SetRegistrationRegion(drrROI);
			projAlignmentFilter->SetIsRegistrationRegionUsed(true);
			projAlignmentFilter->SetMaxShift(m_BGMaxShift);
			projAlignmentFilter->SetGeometry(geometrySorter->GetOutput()[0]);
			projAlignmentFilter->SetInputVolume(bgModels[currentBin - 1]);
			projAlignmentFilter->SetAuxInputVolume(targetModels[currentBin - 1]);
			projAlignmentFilter->SetInput(sharpenedProj);
			if (m_GPUOption.compare("CUDA") == 0)
				projAlignmentFilter->SetForwardProjectionFilter(2);
			else
				projAlignmentFilter->SetForwardProjectionFilter(0);
			projAlignmentFilter->Update();
			bgMatchScore = projAlignmentFilter->GetMatchScore();
			m_DiffProjImage = projAlignmentFilter->GetDifferenceProjection();

			trackingFilter->SetReferenceProjectionImage(projAlignmentFilter->GetDifferenceProjection());
			trackingFilter->SetTargetCentroid(estimatedCentroid);
			if ((m_TemplateMatchingMetricOption == 0 && bgMatchScore > m_MSEBGThreshold) ||
				(m_TemplateMatchingMetricOption == 1 && bgMatchScore < m_MMIBGThreshold) ||
				(m_TemplateMatchingMetricOption == 2 && bgMatchScore < m_NCBGThreshold))
			{
				TrackingType::DRRImageSpacingType drrSearchRadius = m_DRRSearchRadius;
				drrSearchRadius[0] *= 0.5;
				trackingFilter->SetDRRSearchRadius(drrSearchRadius);
			}
			trackingFilter->MeasurePosition();
			matchingMetric = trackingFilter->GetMatchingMetric();
			m_BG2DShifts.push_back(projAlignmentFilter->GetTransformParameters());
		}
		else
		{
			m_DiffProjImage = sharpenedProj;
			ProjectionAlignmentType::TranslationTransformType::ParametersType zeroParameter(2);
			zeroParameter.Fill(0);
			m_BG2DShifts.push_back(zeroParameter);
		}
		
		// Combine prediction and measurement
		double upperBound = m_BGMaxShift[1] * m_BGMaxShift[1];
		double lowerBound = targetModels[currentBin - 1]->GetSpacing()[0] + 
			targetModels[currentBin - 1]->GetSpacing()[1] +
			targetModels[currentBin - 1]->GetSpacing()[2];
		lowerBound = lowerBound / 3 *
			geometrySorter->GetOutput()[0]->GetSourceToDetectorDistances()[0] / 
			geometrySorter->GetOutput()[0]->GetSourceToIsocenterDistances()[0];
		lowerBound = 0.5 * projReader->GetOutput()->GetSpacing()[0]
			+ 0.5 * projReader->GetOutput()->GetSpacing()[1];
		lowerBound *= lowerBound;

		trackingFilter->GetEstimator()->SetR(this->
			estimateMeasurementUncertainties(matchingMetric, lowerBound, upperBound, bgMatchScore));
		trackingFilter->Estimate();

		/*
		if ((m_TemplateMatchingMetricOption == 0 && matchingMetric <= m_MatchingThreshold) ||
			((m_TemplateMatchingMetricOption == 1 || m_TemplateMatchingMetricOption == 2) && matchingMetric >= m_MatchingThreshold))
		{
			prevReliableCentroid = trackingFilter->GetTargetCentroid();
			prevReliableBin = currentBin;
			pastPoints.push_back(trackingFilter->GetTargetCentroid());
		}
		*/

		prevReliableCentroid = trackingFilter->GetTargetCentroid();
		prevReliableBin = currentBin;
		pastPoints.push_back(trackingFilter->GetTargetCentroid());

		while (pastPoints.size() > m_NumberOfPastPoints)
			pastPoints.erase(pastPoints.begin());

		m_BGMatchScoreValues.push_back(bgMatchScore);
		m_MatchingMetricValues.push_back(matchingMetric);

		trackingTimeProbe.Stop();
		emit updateTrackingTimeSignal(trackingTimeProbe.GetMeanTime());

		m_TargetDRRImage = trackingFilter->GetTargetDRRImage();
		m_TargetDRRROI = trackingFilter->GetTargetDRRROI();

		m_StateCovarianceMatrix = trackingFilter->GetEstimator()->GetP();
		m_MeasurementCovarianceMatrix = trackingFilter->GetEstimator()->GetR();
		m_KalmanGain = trackingFilter->GetEstimator()->GetK();

		m_ProjTranslationVector.push_back(trackingFilter->GetDRRTranslationVector());
		m_TargetDRRCentroids.push_back(trackingFilter->GetTargetDRRCentroid());
		m_TrackingTrj.push_back(trackingFilter->GetTargetCentroid());
		m_PriorTrj.push_back(m_PriorCentroids[currentBin-1]);

		emit updateMatchingMetricSignal(matchingMetric);
		emit updateBGMatchScoreSignal(bgMatchScore);
		emit updateStateCovarianceSignal(trackingFilter->GetEstimator()->GetP());
		emit projViewReadySignal();
		emit resultReadySignal(k_frame, currentBin, angles.back() * M_PI / 180.);

		totalTimeProbe.Stop();
		emit updateTotalTimeSignal(totalTimeProbe.GetMeanTime());

		if (m_StopNow)
		{
			m_StopNow = false;
			emit trackingDoneSignal("Tracking has been stopped.");
			return;
		}
	}

	emit trackingDoneSignal("InVivoTrackingOffline has finished tracking.");
}