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

#ifndef __igtTemplateMatchingFilter_cxx
#define __igtTemplateMatchingFilter_cxx

#include "igtTemplateMatchingFilter.h"

namespace igt
{
	template<class TOutputImage>
	TemplateMatchingFilter<TOutputImage>
	::TemplateMatchingFilter()
	{
		// Input 0: The template image.
		// Input 1 : The image on which the template will be matched.
		this->SetNumberOfRequiredInputs(2);

		m_MetricOption = 0; // Default is MS

		ImageSpacingType defaultResolution;
		defaultResolution.Fill(1.);
		m_SearchResolutions.push_back(defaultResolution);

		ImageSpacingType defaultRadius;
		defaultRadius.Fill(10.);
		m_SearchRadii.push_back(defaultRadius);

		TransformType::Pointer transformTmp = TransformType::New();
		m_TransformParameters = TransformParametersType(transformTmp->GetNumberOfParameters());
		m_TransformParameters.Fill(0.);

		m_FinalMetricValue = 0;
		
		m_Maximize = false;
	}

	template<class TOutputImage>
	void TemplateMatchingFilter<TOutputImage>
	::VerifyInputInformation()
	{
		if (m_SearchRadii.size() != m_SearchResolutions.size())
			std::cerr << "Number of multi search radii and search resolutions must be the same." << std::endl;
	}

	template<class TOutputImage>
	void TemplateMatchingFilter<TOutputImage>
	::GenerateData()
	{
		const unsigned int Dim = ImageType::ImageDimension;

		unsigned int nSearch = m_SearchRadii.size();

		// Loop through search resolution layers
		for (unsigned int kres = 0; kres != nSearch; ++kres)
		{
			// Calculate the number of points that need to be looped through
			ImageSizeType nPoints;
			for (unsigned int kdim = 0; kdim != Dim; ++kdim)
				nPoints[kdim] = 1 +
				2 * std::round(m_SearchRadii[kres][kdim] / m_SearchResolutions[kres][kdim]);

			// For convenience. nProd(0) = 1. nProd(1) = Nx. nProd(2) = Nx*Ny. etc
			ImageSizeType nProd;
			unsigned int nTotal = 1;
			nProd.Fill(1);
			for (unsigned int kprod = 0; kprod != Dim; ++kprod)
			{
				nTotal *= nPoints[kprod];
				for (unsigned int kdim = 0; kdim != kprod; ++kdim)
					nProd[kprod] *= nPoints[kdim];
			}

			// Initializing for the loop process
			std::vector<TransformParametersType> parameters;
			std::vector<double>	metricValues;
			unsigned int k_final = 0;
			double finalMetricValue;
			if (m_Maximize)
				finalMetricValue = std::numeric_limits<double>::min();
			else
				finalMetricValue = std::numeric_limits<double>::max();

			// The actual loop-search process
			for (unsigned int ks = 0; ks != nTotal; ++ks)
			{
				TransformType::Pointer transform = TransformType::New();

				// First calculate index and transform parameters
				// ks = x + y*Nx + z*Nx*Ny
				ImageIndexType index;
				index.Fill(0);
				TransformParametersType parameterTmp = TransformParametersType(transform->GetNumberOfParameters());
				parameterTmp.Fill(0.);
				for (unsigned int kdim = 0; kdim != Dim; ++kdim)
				{
					unsigned int ktmp = ks;
					for (unsigned int ksub = 0; ksub < kdim; ++ksub)
						ktmp -= index[Dim - ksub - 1] * nProd[Dim - ksub - 1];
					index[Dim - kdim - 1] = std::floor(ktmp / nProd[Dim - kdim - 1]);
					parameterTmp[Dim - kdim - 1] = m_TransformParameters[Dim - kdim - 1] +
						m_SearchResolutions[kres][Dim - kdim - 1] * 
						(double(index[Dim - kdim - 1]) - double(std::floor(nPoints[Dim - kdim - 1] / 2)));
				}
				parameters.push_back(parameterTmp);

				// Then apply this to metric
				MSMetricType::Pointer metricMS = MSMetricType::New();
				MMIMetricType::Pointer metricMMI = MMIMetricType::New();
				NCCMetricType::Pointer metricNCC = NCCMetricType::New();
				InterpolatorType::Pointer interpolator = InterpolatorType::New();
				transform->SetParameters(parameterTmp);
				
				// Cannot have a single pointer for all metrics, so using define is the best way I can think of
#define COMPUTE_METRIC(metric) \
	metric->SetFixedImage(this->GetInput(0)); \
	metric->SetMovingImage(this->GetInput(1)); \
	metric->SetFixedImageRegion(this->GetInput(0)->GetLargestPossibleRegion()); \
	metric->SetInterpolator(interpolator); \
	metric->SetTransform(transform); \
	metric->Initialize(); \
	metricValues.push_back(metric->GetValue(parameterTmp))

				switch (m_MetricOption)
				{
				case 0:
					COMPUTE_METRIC(metricMS);
					break;
				case 1:
					COMPUTE_METRIC(metricMMI);
					break;
				case 2:
					COMPUTE_METRIC(metricNCC);
					break;
				default:
					std::cerr << "Unsupported metric option." << std::endl;
				}

				
				if (m_Maximize)
				{
					if (metricValues.back() > finalMetricValue)
					{
						finalMetricValue = metricValues.back();
						k_final = ks;
					}
				}
				else
				{
					if (metricValues.back() < finalMetricValue) 
					{
						finalMetricValue = metricValues.back();
						k_final = ks;
					}
				}
			}
			m_FinalMetricValue = finalMetricValue;
			m_TransformParameters = parameters[k_final];
		}

		ImagePointerType output = ImageType::New();
		output->Graft(this->GetInput(0));
		ImagePointType newOrigin = this->GetInput(0)->GetOrigin();
		for (unsigned int kdim = 0; kdim != Dim; ++kdim)
			newOrigin[kdim] += m_TransformParameters[kdim];
		output->SetOrigin(newOrigin);

		this->ProcessObject::SetNthOutput(0, output);
	}

} // end namespace igt

#endif // __igtTemplateMatchingFilter_cxx
