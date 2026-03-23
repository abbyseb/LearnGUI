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

#ifndef __igtTargetTracking_cxx
#define __igtTargetTracking_cxx

#include <igtTargetTracking.h>

namespace igt
{
	template<class ImageType>
	TargetTracking<ImageType>::TargetTracking()
	{
		m_TargetDRRCentroid = vnl_vector<double>(ImageDimension - 1, 0);
		m_DRRTranslationVector = vnl_vector<double>(ImageDimension - 1, 0);
		m_TargetOrigin = vnl_vector<double>(ImageDimension, 0);
		m_TargetCentroid = vnl_vector<double>(ImageDimension, 0);

		m_SharpenDRR = false;

		m_TemplateMatchingMetricOption = 0;

		m_Estimator = EstimatorType::New();
		m_Estimator->SetNumberOfStateVariables(3);
		m_Estimator->SetNumberOfMeasurements(2);
		MatrixType q = m_Estimator->GetQ();
		q.fill_diagonal(5 * 5);

		m_NumberOfIterations = 100;

		m_UseCUDA = false;

		for (unsigned int kdim = 0; kdim != ImageDimension - 1; ++kdim)
			m_DRRSearchRadius[kdim] = 20;
	}

	template<class ImageType>
	void TargetTracking<ImageType>::VerifyInputInformation()
	{
		// Check geometry
		if (this->GetGeometry().GetPointer() == NULL)
			std::cerr << "The geometry object has not been set" << std::endl;

		// Check input images
		if (!this->GetTargetImage())
			std::cerr << "The target image has not been set" << std::endl;
		if (!this->GetReferenceProjectionImage())
			std::cerr << "The reference projection image has not been set" << std::endl;
	}

	template<class ImageType>
	void TargetTracking<ImageType>::GenerateTargetDRR()
	{
		// Prepare constant zero image for forward projection
		ConstantImageType::Pointer zeroProjSource = ConstantImageType::New();
		zeroProjSource->SetOrigin(this->GetReferenceProjectionImage()->GetOrigin());
		zeroProjSource->SetSpacing(this->GetReferenceProjectionImage()->GetSpacing());
		zeroProjSource->SetDirection(this->GetReferenceProjectionImage()->GetDirection());
		zeroProjSource->SetSize(this->GetReferenceProjectionImage()->GetLargestPossibleRegion().GetSize());
		zeroProjSource->SetConstant(0.);
		zeroProjSource->UpdateOutputInformation();

		// Forward project target model
		ForwardProjectionType::Pointer forwardProjector;
		if (m_UseCUDA)
		{
#ifdef IGT_USE_CUDA
			forwardProjector = CudaForwardProjectionType::New();
#else
			std::cerr << "The program has not been compiled with CUDA." << std::endl;
			return;
#endif
		}
		else
			forwardProjector = JosephForwardProjectionType::New();
		forwardProjector->SetInput(zeroProjSource->GetOutput());
		forwardProjector->SetInput(1, this->GetTargetImage());
		forwardProjector->SetGeometry(this->GetGeometry());
		forwardProjector->Update();

		// Extract target DRR to ROI, 
		// i.e. smallest bounding box containing all non-zero elements
		ThresholdToMaskType::Pointer thresholdMask = ThresholdToMaskType::New();
		thresholdMask->SetInput(forwardProjector->GetOutput());
		thresholdMask->SetLowerThreshold(0);
		thresholdMask->SetUpperThreshold(0);
		thresholdMask->SetInsideValue(0);
		thresholdMask->SetOutsideValue(1);
		thresholdMask->Update();
		MaskSpatialObjectType::Pointer maskSpatialObject = MaskSpatialObjectType::New();
		maskSpatialObject->SetImage(thresholdMask->GetOutput());
		m_TargetDRRROI = maskSpatialObject->GetAxisAlignedBoundingBoxRegion();
		ImageIndexType drrMargin;
		drrMargin[0] = std::floor(m_TargetDRRROI.GetSize()[0] * 0.04 + 0.5);
		drrMargin[1] = std::floor(m_TargetDRRROI.GetSize()[1] * 0.04 + 0.5);
		ImageSizeType drrSize = m_TargetDRRROI.GetSize();
		drrSize[0] = drrSize[0] + 2 * drrMargin[0];
		drrSize[1] = drrSize[1] + 2 * drrMargin[1];
		ImageIndexType drrIndex = m_TargetDRRROI.GetIndex();
		drrIndex[0] = (drrIndex[0] - drrMargin[0] >= 0) ? drrIndex[0] - drrMargin[0] : 0;
		drrIndex[1] = (drrIndex[1] - drrMargin[1] >= 0) ? drrIndex[1] - drrMargin[1] : 0;
		drrSize[0] = (drrIndex[0] + drrSize[0] < forwardProjector->GetOutput()->GetLargestPossibleRegion().GetSize()[0]) ?
			drrSize[0] : drrIndex[0] + drrSize[0] - forwardProjector->GetOutput()->GetLargestPossibleRegion().GetSize()[0];
		drrSize[1] = (drrIndex[1] + drrSize[1] < forwardProjector->GetOutput()->GetLargestPossibleRegion().GetSize()[1]) ?
			drrSize[1] : drrIndex[1] + drrSize[1] - forwardProjector->GetOutput()->GetLargestPossibleRegion().GetSize()[1];
		m_TargetDRRROI.SetIndex(drrIndex);
		m_TargetDRRROI.SetSize(drrSize);
		
		ImageROIType::Pointer targetROIFilter = ImageROIType::New();
		targetROIFilter->SetRegionOfInterest(m_TargetDRRROI);
		targetROIFilter->SetInput(forwardProjector->GetOutput());
		targetROIFilter->Update();

		this->SetTargetDRRImage(targetROIFilter->GetOutput());
	}

	template<class ImageType>
	void TargetTracking<ImageType>::ProjectionTemplateMatching()
	{
		// Collapse to 2D images to make things quicker
		ExtractImageType::Pointer extractReference = ExtractImageType::New();
		ExtractImageType::Pointer extractTarget = ExtractImageType::New();
		ImageType::RegionType projRegion = this->GetReferenceProjectionImage()->GetLargestPossibleRegion();
		ImageType::RegionType drrRegion = this->GetTargetDRRImage()->GetLargestPossibleRegion();
		projRegion.SetSize(2, 0);
		drrRegion.SetSize(2, 0);
		extractReference->SetExtractionRegion(projRegion);
		extractTarget->SetExtractionRegion(drrRegion);
		extractReference->SetInput(this->GetReferenceProjectionImage());
		extractTarget->SetInput(this->GetTargetDRRImage());
		extractReference->SetDirectionCollapseToIdentity();
		extractTarget->SetDirectionCollapseToIdentity();
		extractReference->Update();
		extractTarget->Update();

		// Sharpen DRR if option is turned on
		DRRImageType::Pointer processedDRR = extractTarget->GetOutput();
		if (m_SharpenDRR)
		{
			SharpenImageType::Pointer sharpenDRRFilter = SharpenImageType::New();
			sharpenDRRFilter->SetInput(extractTarget->GetOutput());
			sharpenDRRFilter->Update();
			processedDRR = sharpenDRRFilter->GetOutput();
			processedDRR->DisconnectPipeline();
		}
		
		/*
		// Set up template matching filter
		TemplateMatchingType::Pointer templateMatcher = TemplateMatchingType::New();
		templateMatcher->SetInput(0, processedDRR);
		templateMatcher->SetInput(1, extractReference->GetOutput());
		templateMatcher->SetMetricOption(m_TemplateMatchingMetricOption);
		// Set up multiresolution search
		DRRImageSpacingType initialResolution, secondResolution, finalResolution;
		for (unsigned int kdim = 0; kdim != ImageDimension - 1; ++kdim)
		{
			initialResolution[kdim] = processedDRR->GetSpacing()[kdim] * 4;
			secondResolution[kdim] = processedDRR->GetSpacing()[kdim] * 2;
			finalResolution[kdim] = processedDRR->GetSpacing()[kdim] * 1;
		}
		std::vector<DRRImageSpacingType> searchRadii;
		//searchRadii.push_back(secondResolution); // Final search radius is the same as second search resolution
		//searchRadii.push_back(initialResolution); // Second search radius is the same as initial search resolution
		searchRadii.push_back(m_DRRSearchRadius);
		templateMatcher->SetSearchRadii(searchRadii);
		std::vector<DRRImageSpacingType> searchResolutions;
		searchResolutions.push_back(finalResolution); // Final search resolution is the same as image spacing
		//searchResolutions.push_back(secondResolution);
		//searchResolutions.push_back(initialResolution);
		templateMatcher->SetSearchResolutions(searchResolutions);

		templateMatcher->Update();
		
		for (unsigned int kdim = 0; kdim != ImageDimension - 1; ++kdim)
			m_DRRTranslationVector(kdim) = templateMatcher->GetTransformParameters()[kdim];
		m_MatchingMetric = templateMatcher->GetFinalMetricValue();
		*/

		
		// Use registration for template matching
		ImageRegistrationType::Pointer		registration = ImageRegistrationType::New();
		
		// Interpolator
		LinearInterpolatorType::Pointer		interpolator = LinearInterpolatorType::New();

		// Transform
		TranslationTransformType::Pointer	transform = TranslationTransformType::New();
		TranslationTransformType::ParametersType	initialParameters(transform->GetNumberOfParameters());
		initialParameters.Fill(0.0);
		transform->SetParameters(initialParameters);

		// Optimizer
		itk::LBFGSBOptimizer::Pointer optimizer = itk::LBFGSBOptimizer::New();
		itk::LBFGSBOptimizer::BoundSelectionType boundSelect(transform->GetNumberOfParameters());
		itk::LBFGSBOptimizer::BoundValueType upperBound(transform->GetNumberOfParameters());
		itk::LBFGSBOptimizer::BoundValueType lowerBound(transform->GetNumberOfParameters());
		boundSelect.Fill(2);
		for (unsigned int kdim = 0; kdim != ImageDimension - 1; ++kdim)
		{
			upperBound[kdim] = m_DRRSearchRadius[kdim];
			lowerBound[kdim] = -1. * m_DRRSearchRadius[kdim];
		}
		optimizer->SetBoundSelection(boundSelect);
		optimizer->SetUpperBound(upperBound);
		optimizer->SetLowerBound(lowerBound);
		optimizer->SetCostFunctionConvergenceFactor(1.e7);
		optimizer->SetProjectedGradientTolerance(1e-5);
		optimizer->SetMaximumNumberOfIterations(m_NumberOfIterations);
		optimizer->SetMaximumNumberOfEvaluations(m_NumberOfIterations);
		optimizer->SetMaximumNumberOfCorrections(7);

		// Metric
		MSMetricType::Pointer				metricMS = MSMetricType::New();
		MMIMetricType::Pointer				metricMMI = MMIMetricType::New();
		NCMetricType::Pointer				metricNC = NCMetricType::New();
		switch (m_TemplateMatchingMetricOption)
		{
		case 0:
			registration->SetMetric(metricMS);
			break;
		case 1:
			registration->SetMetric(metricMMI);
			break;
		case 2:
			registration->SetMetric(metricNC);
			break;
		default:
			std::cerr << "Unsupported template matching metric option." << std::endl;
		}

		// Set up registration and update
		registration->SetOptimizer(optimizer);
		registration->SetTransform(transform);
		registration->SetInitialTransformParameters(transform->GetParameters());
		registration->SetInterpolator(interpolator);
		registration->SetFixedImage(processedDRR);
		registration->SetMovingImage(extractReference->GetOutput());
		registration->SetFixedImageRegion(registration->GetFixedImage()->GetLargestPossibleRegion());
		registration->Update();

		// Get final transform parameters
		for (unsigned int kdim = 0; kdim != ImageDimension - 1; ++kdim)
			m_DRRTranslationVector(kdim) = registration->GetLastTransformParameters()[kdim];
		m_MatchingMetric = optimizer->GetValue();

		// Constrain with search radius
		bool withinMaxShift = true;
		for (unsigned int kdim = 0; kdim != ImageDimension - 1; ++kdim)
			withinMaxShift = withinMaxShift &&
			(std::fabs(m_DRRTranslationVector(kdim)) < m_DRRSearchRadius[kdim]);
		if (!withinMaxShift)
			m_DRRTranslationVector.fill(0);

		// Convert the metric value to relevant format
		if (m_TemplateMatchingMetricOption == 0) // MSE
		{
			ImageStatsType::Pointer targetDRRStats = ImageStatsType::New();
			targetDRRStats->SetInput(processedDRR);
			targetDRRStats->Update();
			double targetDRRMS = targetDRRStats->GetVariance() +
				(targetDRRStats->GetMean()) * (targetDRRStats->GetMean());
			m_MatchingMetric = m_MatchingMetric / targetDRRMS;
		}
		else if (m_TemplateMatchingMetricOption == 1) // MMI
			m_MatchingMetric *= -1.;
		else if (m_TemplateMatchingMetricOption == 2) // NC
			m_MatchingMetric *= -1.;
		else
			std::cerr << "Unsupported template matching metric option." << std::endl;
	}

	template<class ImageType>
	void TargetTracking<ImageType>::Estimate()
	{
		m_Estimator->SetGeometry(this->GetGeometry());

		m_TargetDRRCentroid = this->GetGeometry()->CalculateProjSpaceCoordinates(m_TargetCentroid)[0].extract(2,0);

		// Target centroid
		m_Estimator->Setx(m_TargetCentroid);

		// Measurement vector
		m_Estimator->Setz(m_TargetDRRCentroid + m_DRRTranslationVector);

		m_Estimator->Compute();

		m_DRRTranslationVector = 
			this->GetGeometry()->CalculateProjSpaceCoordinates(m_Estimator->Getx())[0].extract(2, 0) - m_TargetDRRCentroid;
		m_TargetDRRCentroid += m_DRRTranslationVector;

		this->SetTargetCentroid(m_Estimator->Getx());
	}

	template<class ImageType>
	void TargetTracking<ImageType>::Compute()
	{
		this->MeasurePosition();

		this->Estimate();
	}

	template<class ImageType>
	void TargetTracking<ImageType>::MeasurePosition()
	{
		this->VerifyInputInformation();

		this->GenerateTargetDRR();

		this->ProjectionTemplateMatching();
	}
} // end namespace igt

#endif // __igtTargetTracking_cxx
