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

#ifndef __igtProjectionAlignmentFilter_cxx
#define __igtProjectionAlignmentFilter_cxx

#include "igtProjectionAlignmentFilter.h"

namespace igt
{
	template<class TOutputImage>
	ProjectionAlignmentFilter<TOutputImage>
	::ProjectionAlignmentFilter()
	{
		this->SetNumberOfRequiredInputs(1);

		m_DifferenceProjection = 0;
		m_Geometry = 0;
		m_InputVolume = 0;
		m_AuxInputVolume = 0;

		m_SharpenDRR = false;

		m_MaxShift.Fill(20.);
		m_MaxRotateAngle = itk::Math::pi / 36.;

		m_TransformParameters.Fill(0.);

		m_NumberOfIterations = 100;

		m_IsRegistrationRegionUsed = false;

		m_MatchScore = 0;

		m_MatchThreshold = 0.5;

		m_MatchMetricOption = 0;

		m_IndependentShiftCap.Fill(20.);
	}

	template<class TOutputImage>
	void
	ProjectionAlignmentFilter<TOutputImage>
	::SetForwardProjectionFilter (int _arg)
	{
		if( _arg != this->GetForwardProjectionFilter() )
		{
			Superclass::SetForwardProjectionFilter( _arg );
			this->m_CurrentForwardProjectionConfiguration = _arg;
		}
	}

	template<class TOutputImage>
	void ProjectionAlignmentFilter<TOutputImage>
	::GenerateData()
	{
		if(this->GetGeometry().GetPointer() == NULL)
		{
			itkGenericExceptionMacro(<< "The geometry object has not been set");
		}

		if(!m_InputVolume)
		{
			itkGenericExceptionMacro(<< "The input volume has not been set");
		}
		const unsigned int Dimension = TOutputImage::ImageDimension;
		// First, forward project volume
		ImageMultiplyType::Pointer zeroProjFilter = ImageMultiplyType::New();
		zeroProjFilter->SetInput(this->GetInput());
		zeroProjFilter->SetConstant(0);
		zeroProjFilter->Update();
		ForwardProjectionType::Pointer forwardProjector =
				this->InstantiateForwardProjectionFilter( this->GetForwardProjectionFilter() );
		forwardProjector->InPlaceOff();
		forwardProjector->SetGeometry( this->GetGeometry() );
		forwardProjector->SetInput(0, zeroProjFilter->GetOutput() );
		forwardProjector->SetInput(1, m_InputVolume);
		forwardProjector->Update();

		// Also forward project auxillary input volume if provided
		ImageMultiplyType::Pointer zeroProjFilter_Aux = ImageMultiplyType::New();
		zeroProjFilter_Aux->SetInput(this->GetInput());
		zeroProjFilter_Aux->SetConstant(0);
		zeroProjFilter_Aux->Update();
		ForwardProjectionType::Pointer forwardProjector_Aux =
			this->InstantiateForwardProjectionFilter(this->GetForwardProjectionFilter());
		forwardProjector_Aux->InPlaceOff();
		forwardProjector_Aux->SetGeometry(this->GetGeometry());
		forwardProjector_Aux->SetInput(0, zeroProjFilter_Aux->GetOutput());
		forwardProjector_Aux->SetInput(1, m_AuxInputVolume);

		// Add the forward-projected input and auxillary volume together if AuxInputVolume is used
		AddImageType::Pointer addImageFilter = AddImageType::New();
		if (m_AuxInputVolume)
		{
			forwardProjector_Aux->Update();
			addImageFilter->SetInput1(forwardProjector->GetOutput());
			addImageFilter->SetInput2(forwardProjector_Aux->GetOutput());
			addImageFilter->Update();
		}

		// Then rigid registration on a projection by projection basis
		// Set up the tile filter to stack projections back later
		TileFilterType::Pointer	tileDRRFilter = TileFilterType::New();
		TileFilterType::LayoutArrayType layout;
		layout.Fill(1);
		layout[ImageDimension - 1] = 0;
		tileDRRFilter->SetLayout( layout );

		// Also need tile filter to tile up the histogram matched projection images
		TileFilterType::Pointer	tileProjectionFilter = TileFilterType::New();
		tileProjectionFilter->SetLayout(layout);

		// Arrays of extract, least square match, and resample filters
		std::vector< ExtractFilterType::Pointer >	extractProjectionFilters, extractDRRFilters, extractDRRFilters_Align;
		std::vector< SharpenImageType::Pointer >	sharpenDRRFilters;
		std::vector< LeastSquareSubtractionType::Pointer >  leastSquareSubtractionFilters;
		std::vector< HistogramMatchType::Pointer >  histogramMatchFilters;
		std::vector< ResampleType::Pointer >		resamplers;
		ImageSizeType size = this->GetInput()->GetLargestPossibleRegion().GetSize();
		ImageIndexType index = this->GetInput()->GetLargestPossibleRegion().GetIndex();
		const unsigned int NProj = size[2];
		size[Dimension-1] = 0;

		for(unsigned int k = 0; k != NProj; ++k)
	    {
			// Extract
			index[Dimension-1] = k;
			ImageRegionType region(index, size);
			extractProjectionFilters.push_back( ExtractFilterType::New() );
			extractProjectionFilters[k]->SetInput( this->GetInput() );
			extractProjectionFilters[k]->SetExtractionRegion( region );
			extractProjectionFilters[k]->SetDirectionCollapseToSubmatrix();
			extractDRRFilters.push_back( ExtractFilterType::New() );
			extractDRRFilters[k]->SetInput( forwardProjector->GetOutput() );
			extractDRRFilters[k]->SetExtractionRegion( region );
			extractDRRFilters[k]->SetDirectionCollapseToSubmatrix();
			extractProjectionFilters[k]->Update();
			extractDRRFilters[k]->Update();

			extractDRRFilters_Align.push_back(ExtractFilterType::New());
			extractDRRFilters_Align[k]->SetInput(addImageFilter->GetOutput());
			extractDRRFilters_Align[k]->SetExtractionRegion(region);
			extractDRRFilters_Align[k]->SetDirectionCollapseToSubmatrix();
			if (m_AuxInputVolume)
				extractDRRFilters_Align[k]->Update();

			// Sharpen DRR
			RegistrationImageType::Pointer processedDRR;
			if (m_AuxInputVolume)
				processedDRR = extractDRRFilters_Align[k]->GetOutput();
			else
				processedDRR = extractDRRFilters[k]->GetOutput();
			if (m_SharpenDRR)
			{
				sharpenDRRFilters.push_back(SharpenImageType::New());
				if (m_AuxInputVolume)
					sharpenDRRFilters[k]->SetInput(extractDRRFilters_Align[k]->GetOutput());
				else
					sharpenDRRFilters[k]->SetInput(extractDRRFilters[k]->GetOutput());
				sharpenDRRFilters[k]->Update();
				processedDRR = sharpenDRRFilters[k]->GetOutput();
			}
			processedDRR->DisconnectPipeline();
				
			histogramMatchFilters.push_back(HistogramMatchType::New());
			histogramMatchFilters[k]->SetInput(extractProjectionFilters[k]->GetOutput());
			if (m_AuxInputVolume)
				histogramMatchFilters[k]->SetReferenceImage(extractDRRFilters_Align[k]->GetOutput());
			else
				histogramMatchFilters[k]->SetReferenceImage(extractDRRFilters[k]->GetOutput());
			histogramMatchFilters[k]->SetNumberOfHistogramLevels(10);
			histogramMatchFilters[k]->SetNumberOfMatchPoints(10);
			histogramMatchFilters[k]->Update();
			RegistrationImageType::Pointer scaledImg = histogramMatchFilters[k]->GetOutput();
			
			/*
			leastSquareSubtractionFilters.push_back( LeastSquareSubtractionType::New() );
			leastSquareSubtractionFilters[k]->SetInput(0, processedDRR);
			leastSquareSubtractionFilters[k]->SetInput(1, extractProjectionFilters[k]->GetOutput());
			leastSquareSubtractionFilters[k]->Update();
			RegistrationImageType::Pointer scaledImg = leastSquareSubtractionFilters[k]->GetScaledImage();
			*/

			ImageRegistrationType::Pointer	registration = ImageRegistrationType::New();
			LinearInterpolatorType::Pointer	interpolator = LinearInterpolatorType::New();
			TranslationTransformType::Pointer		transform = TranslationTransformType::New();
			TranslationTransformType::ParametersType	initialParameters(transform->GetNumberOfParameters());
			initialParameters.Fill(0.0);
			transform->SetParameters(initialParameters);

			//Optimizer
			itk::LBFGSBOptimizer::Pointer optimizer = itk::LBFGSBOptimizer::New();
			itk::LBFGSBOptimizer::BoundSelectionType boundSelect(transform->GetNumberOfParameters());
			itk::LBFGSBOptimizer::BoundValueType upperBound(transform->GetNumberOfParameters());
			itk::LBFGSBOptimizer::BoundValueType lowerBound(transform->GetNumberOfParameters());
			boundSelect.Fill(2);
			for (unsigned int kdim = 0; kdim != ImageDimension - 1; ++kdim)
			{
				upperBound[kdim] = this->GetMaxShift()[kdim];
				lowerBound[kdim] = -1. * this->GetMaxShift()[kdim];
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
			switch (m_MatchMetricOption)
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

			registration->SetOptimizer( optimizer );
			registration->SetTransform( transform );
			registration->SetInitialTransformParameters( transform->GetParameters() );
			registration->SetInterpolator( interpolator );
			registration->SetFixedImage(scaledImg);
			registration->SetMovingImage(processedDRR);
			if (m_IsRegistrationRegionUsed)
				registration->SetFixedImageRegion(m_RegistrationRegion);
			else
				registration->SetFixedImageRegion(scaledImg->GetLargestPossibleRegion());
			registration->SetFixedImageRegion(scaledImg->GetLargestPossibleRegion());
			registration->Update();

			// Constrain the transformation
			m_TransformParameters = registration->GetLastTransformParameters();

			bool isReliable = true;

			/*
			for (unsigned int kdim = 0; kdim != ImageDimension - 1; ++kdim)
				isReliable = isReliable &&
					(std::fabs(m_TransformParameters[kdim]) <= this->GetMaxShift()[kdim]);
			*/
			
			switch (m_MatchMetricOption)
			{
			case 0:
				m_MatchScore = optimizer->GetValue();
				isReliable = isReliable && (m_MatchScore <= m_MatchThreshold);
				break;
			case 1:
				m_MatchScore = -1. * optimizer->GetValue();
				isReliable = isReliable && (m_MatchScore >= m_MatchThreshold);
				break;
			case 2:
				m_MatchScore = -1. * optimizer->GetValue();
				isReliable = isReliable && (m_MatchScore >= m_MatchThreshold);
				break;
			default:
				std::cerr << "Unsupported template matching metric option." << std::endl;
			}
			
			if (!isReliable)
			{
			    // m_TransformParameters.Fill(0.0);
				// SI is usually fine. Just constrain lateral
				if (std::fabs(m_TransformParameters[0]) > 0.5 * this->GetMaxShift()[0])
					m_TransformParameters[0] = 0;
			}
			
			for (unsigned int kdim = 0; kdim != ImageDimension - 1; ++kdim)
			{
				if (std::fabs(m_TransformParameters[kdim]) > this->GetIndependentShiftCap()[kdim])
					m_TransformParameters[kdim] = 0;
			}
			
			TranslationTransformType::Pointer finalTransform = TranslationTransformType::New();
			finalTransform->SetParameters(m_TransformParameters);

			// Resampler
			resamplers.push_back( ResampleType::New() );
			resamplers[k]->SetInput(extractDRRFilters[k]->GetOutput());
			resamplers[k]->SetReferenceImage(scaledImg);
			resamplers[k]->SetTransform( finalTransform );
			resamplers[k]->UseReferenceImageOn();
			resamplers[k]->Update();
			// Pass results to tile filter
			tileDRRFilter->SetInput(k, resamplers[k]->GetOutput());
			tileProjectionFilter->SetInput(k, scaledImg);
		}
	tileDRRFilter->Update();
	tileDRRFilter->GetOutput()->SetOrigin( this->GetInput()->GetOrigin() );
	tileProjectionFilter->Update();
	tileProjectionFilter->GetOutput()->SetOrigin(this->GetInput()->GetOrigin());
	this->GraftOutput( tileDRRFilter->GetOutput() );

	// Calculate difference projections
	ImageSubtractionType::Pointer subtractionFilter = ImageSubtractionType::New();
	subtractionFilter->SetInput(0, tileProjectionFilter->GetOutput());
	subtractionFilter->SetInput(1, tileDRRFilter->GetOutput());
	subtractionFilter->Update();
	m_DifferenceProjection = subtractionFilter->GetOutput();
	m_DifferenceProjection->DisconnectPipeline();
	}

} // end namespace igt

#endif // __igtProjectionAlignmentFilter_cxx
