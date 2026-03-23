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

#ifndef __igtRegisterProjectionsFilter_cxx
#define __igtRegisterProjectionsFilter_cxx

#include "igtRegisterProjectionsFilter.h"

namespace igt
{
	template<class TInputImage, class TOutputImage>
	RegisterProjectionsFilter<TInputImage, TOutputImage>
		::RegisterProjectionsFilter()
	{	
		// Input 0: moving image, input 1: fixed image
		this->SetNumberOfRequiredInputs(2);

		m_TransformOption = TransformOption::Rigid;
		m_MetricOption = MetricOption::MattesMutualInformation;
		m_Mask = 0;

		m_MaxShiftX = 20;
		m_MaxShiftY = 20;
		m_MaxRotation = itk::Math::pi / 9.;

		m_Verbose = false;
		m_UseInverse = false;
	}

	template<class TInputImage, class TOutputImage>
	void RegisterProjectionsFilter<TInputImage, TOutputImage>
		::GenerateOutputInformation()
	{
			if (this->m_Mask)
			{
				bool isMaskValid = true;
				isMaskValid &= (this->m_Mask->GetLargestPossibleRegion().GetSize()[0] == this->GetInput(1)->GetLargestPossibleRegion().GetSize()[0])
					&& (this->m_Mask->GetLargestPossibleRegion().GetSize()[1] == this->GetInput(1)->GetLargestPossibleRegion().GetSize()[1])
					&& (this->m_Mask->GetLargestPossibleRegion().GetIndex()[0] == this->GetInput(1)->GetLargestPossibleRegion().GetIndex()[0])
					&& (this->m_Mask->GetLargestPossibleRegion().GetIndex()[1] == this->GetInput(1)->GetLargestPossibleRegion().GetIndex()[1]);
				if (!isMaskValid)
					std::cerr << "The registration mask did not have the same dimension as the fixed image." << std::endl;
			}

			this->GetOutput()->SetOrigin(this->GetInput(1)->GetOrigin());
			this->GetOutput()->SetSpacing(this->GetInput(1)->GetSpacing());
			this->GetOutput()->SetLargestPossibleRegion(this->GetInput(1)->GetLargestPossibleRegion());
	}

	template<class TInputImage, class TOutputImage>
	void RegisterProjectionsFilter<TInputImage, TOutputImage>
	::GenerateData()
	{
		this->m_TransformParameters.clear();

		// Set up the tile filter to stack projections back later
		TileFilterType::Pointer	tileFilter = TileFilterType::New();
		TileFilterType::LayoutArrayType layout;
		layout.Fill(1);
		layout[ImageDimension - 1] = 0;
		tileFilter->SetLayout(layout);

		// Arrays of extract, least square match, and resample filters
		TInputImage::SizeType size = this->GetInput(1)->GetLargestPossibleRegion().GetSize();
		TInputImage::IndexType index = this->GetInput(1)->GetLargestPossibleRegion().GetIndex();
		const unsigned int NProj = size[ImageDimension - 1];
		size[ImageDimension - 1] = 0;

		// Registration region
		RegistrationImageType::RegionType registrationRegion;

		for(unsigned int k = 0; k != NProj; ++k)
	    {
			if (this->m_Verbose)
				std::cout << "Processing frame#" << (k + 1) << " ......" << std::endl;

			// Extract
			index[ImageDimension - 1] = k;
			TInputImage::RegionType region(index, size);
			ExtractFilterType::Pointer extractMov = ExtractFilterType::New();
			extractMov->SetInput(this->GetInput(0));
			extractMov->SetExtractionRegion(region);
			extractMov->SetDirectionCollapseToSubmatrix();
			ExtractFilterType::Pointer extractFix = ExtractFilterType::New();
			extractFix->SetInput(this->GetInput(1));
			extractFix->SetExtractionRegion(region);
			extractFix->SetDirectionCollapseToSubmatrix();
			extractMov->Update();
			extractFix->Update();

			// Registration
			ImageRegistrationType::Pointer registration = ImageRegistrationType::New();
			//MultiResRegistrationType::Pointer registration = MultiResRegistrationType::New();
			//registration->SetNumberOfLevels(3);
			if (m_UseInverse)
			{
				registration->SetFixedImage(extractMov->GetOutput());
				registration->SetMovingImage(extractFix->GetOutput());
			}
			else
			{
				registration->SetFixedImage(extractFix->GetOutput());
				registration->SetMovingImage(extractMov->GetOutput());
			}
			
			// Interpolator
			LinearInterpolatorType::Pointer	interpolator = LinearInterpolatorType::New();
			registration->SetInterpolator(interpolator);

			// Transform
			if (this->m_TransformOption == TransformOption::Translation)
			{
				TranslationTransformType::Pointer transform = TranslationTransformType::New();
				registration->SetTransform(transform);
				TranslationTransformType::ParametersType initialParameters(transform->GetNumberOfParameters());
				initialParameters.Fill(0.0);
				registration->SetInitialTransformParameters(initialParameters);
			}
			else if (this->m_TransformOption == TransformOption::Rigid)
			{
				RigidTransformType::Pointer transform = RigidTransformType::New();
				registration->SetTransform(RigidTransformType::New());
				RigidTransformType::ParametersType initialParameters(transform->GetNumberOfParameters());
				initialParameters.Fill(0.0);
				registration->SetInitialTransformParameters(initialParameters);
			}

			//Optimizer
			itk::LBFGSBOptimizer::Pointer optimizer = itk::LBFGSBOptimizer::New();
			itk::LBFGSBOptimizer::BoundSelectionType boundSelect(registration->GetTransform()->GetNumberOfParameters());
			itk::LBFGSBOptimizer::BoundValueType upperBound(registration->GetTransform()->GetNumberOfParameters());
			itk::LBFGSBOptimizer::BoundValueType lowerBound(registration->GetTransform()->GetNumberOfParameters());
			boundSelect.Fill(2);
			if (this->m_TransformOption == TransformOption::Translation)
			{
				upperBound[0] = m_MaxShiftX;
				lowerBound[0] = -m_MaxShiftX;
				upperBound[1] = m_MaxShiftY;
				lowerBound[1] = -m_MaxShiftY;
			}
			else if (this->m_TransformOption == TransformOption::Rigid)
			{
				upperBound[0] = m_MaxRotation;
				lowerBound[0] = -m_MaxRotation;
				upperBound[1] = m_MaxShiftX;
				lowerBound[1] = -m_MaxShiftX;
				upperBound[2] = m_MaxShiftY;
				lowerBound[2] = -m_MaxShiftY;
			}
			optimizer->SetBoundSelection(boundSelect);
			optimizer->SetUpperBound(upperBound);
			optimizer->SetLowerBound(lowerBound);
			optimizer->SetCostFunctionConvergenceFactor(1.e7);
			optimizer->SetProjectedGradientTolerance(1e-5);
			optimizer->SetMaximumNumberOfIterations(200);
			optimizer->SetMaximumNumberOfEvaluations(200);
			optimizer->SetMaximumNumberOfCorrections(10);
			/*
			itk::RegularStepGradientDescentOptimizer::Pointer optimizer = itk::RegularStepGradientDescentOptimizer::New();
			optimizer->SetMaximumStepLength(4.00);
			optimizer->SetMinimumStepLength(0.01);
			optimizer->SetNumberOfIterations(200);
			*/
			registration->SetOptimizer(optimizer);
			

			// Metric
			switch (m_MetricOption)
			{
			case MetricOption::MattesMutualInformation:
				registration->SetMetric(MMIMetricType::New());
				break;
			case MetricOption::MeanSquares:
				registration->SetMetric(MSMetricType::New());
				break;
			case MetricOption::NormalizedCorrelation:
				registration->SetMetric(NCMetricType::New());
				break;
			}

			// Registration region
			if (this->m_Mask)
			{
				if (k == 0)
				{
					BinaryThresholdType::Pointer thresholdMask = BinaryThresholdType::New();
					thresholdMask->SetInput(this->m_Mask);
					thresholdMask->SetLowerThreshold(0);
					thresholdMask->SetUpperThreshold(0);
					thresholdMask->SetInsideValue(0);
					thresholdMask->SetOutsideValue(1);
					thresholdMask->Update();
					MaskSpatialType::Pointer maskSpatialObject = MaskSpatialType::New();
					maskSpatialObject->SetImage(thresholdMask->GetOutput());
					registrationRegion = maskSpatialObject->GetAxisAlignedBoundingBoxRegion();
				}
				registration->SetFixedImageRegion(registrationRegion);
			}

			registration->Update();

			// Save transform parameters
			std::vector<double> thisParameter;
			for (unsigned int npara = 0; npara < registration->GetLastTransformParameters().GetSize(); npara++)
				thisParameter.push_back(registration->GetLastTransformParameters()[npara]);
			this->m_TransformParameters.push_back(thisParameter);

			// Resampler
			ResampleType::Pointer resampler = ResampleType::New();
			resampler->SetInput(extractMov->GetOutput());
			resampler->SetReferenceImage(extractFix->GetOutput());
			if (m_UseInverse)
				resampler->SetTransform(registration->GetOutput()->Get()->GetInverseTransform());
			else
				resampler->SetTransform(registration->GetOutput()->Get());
			resampler->UseReferenceImageOn();
			resampler->Update();

			// Pass results to tile filter
			RegistrationImageType::Pointer transformedImg = resampler->GetOutput();
			transformedImg->DisconnectPipeline();
			tileFilter->SetInput(k, transformedImg);
		}
		tileFilter->Update();
		tileFilter->GetOutput()->SetOrigin(this->GetInput(1)->GetOrigin());
		tileFilter->GetOutput()->SetSpacing(this->GetInput(1)->GetSpacing());
		this->GraftOutput(tileFilter->GetOutput());
	}

} // end namespace igt

#endif // __igtRegisterProjectionsFilter_cxx
