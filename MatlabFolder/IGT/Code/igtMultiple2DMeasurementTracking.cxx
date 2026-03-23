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

#ifndef __igtMultiple2DMeasurementTracking_cxx
#define __igtMultiple2DMeasurementTracking_cxx

#include <igtMultiple2DMeasurementTracking.h>

namespace igt
{
	template<class ImageType>
	void Multiple2DMeasurementTracking<ImageType>::Estimate()
	{
		if (m_MeasurementVectors.size() + 1 != m_NumberOf2DMeasurements)
			std::cerr << "Number of 2D measurements does not match with input." << std::endl;

		if (m_MultipleMeasurementGeometry.GetPointer() == NULL)
			std::cerr << "Geometry object for incoporating multiple measurements has not been set via SetMultipleMeasurementGeometry." << std::endl;

		if (m_NumberOf2DMeasurements != this->GetMultipleMeasurementGeometry()->GetGantryAngles().size())
			std::cerr << "Number of 2D measurements does not match with geometry information." << std::endl;

		// First set up measurement matrix
		MatrixType H((ImageDimension - 1)*m_NumberOf2DMeasurements, ImageDimension, 0.);
		std::vector<double> rayScaleFactors;
		for (unsigned int k = 0; k != m_NumberOf2DMeasurements; ++k)
		{
			rayScaleFactors.push_back(0);
			for (unsigned int k_mat = 0; k_mat != ImageDimension; ++k_mat)
			{
				rayScaleFactors[k] +=
					this->GetMultipleMeasurementGeometry()->GetMatrices()[k](ImageDimension - 1, k_mat)
					* m_TargetCentroid(k_mat);
			}
			rayScaleFactors[k] += 
				this->GetMultipleMeasurementGeometry()->GetMatrices()[k](ImageDimension - 1, ImageDimension);

			for (unsigned i = 0; i != ImageDimension - 1; ++i)
			{
				for (unsigned j = 0; j != ImageDimension; ++j)
				{
					H(i + (ImageDimension - 1) * k, j) = 
						this->GetMultipleMeasurementGeometry()->GetMatrices()[k](i, j);
					H(i + (ImageDimension - 1) * k, j) /= rayScaleFactors[k];
				}
			}
		}
		
		this->GetEstimator()->SetH(H);
		// Estimate target centroid using the matrix for the last projection
		MatrixType H_sub = H.extract(
			ImageDimension - 1, ImageDimension,
			(m_NumberOf2DMeasurements - 1) * (ImageDimension - 1), 0);

		VectorType targetDRRCentroid = H_sub * m_TargetCentroid;
		targetDRRCentroid +=
			this->GetMultipleMeasurementGeometry()->GetMatrices().back().GetVnlMatrix().get_column(3).extract(2, 0)
			/ rayScaleFactors.back();
		this->SetTargetDRRCentroid(targetDRRCentroid);

		// Target centroid
		this->GetEstimator()->Setx(m_TargetCentroid);

		// Measurement vector
		m_MeasurementVectors.push_back(this->GetTargetDRRCentroid() + this->GetDRRTranslationVector());

		VectorType zVector = VectorType(m_NumberOf2DMeasurements * (ImageDimension - 1),0);
		for (unsigned int k = 0; k != m_NumberOf2DMeasurements; ++k)
		{
			zVector(k * (ImageDimension - 1)) = 
				m_MeasurementVectors[k][0] - 
				this->GetMultipleMeasurementGeometry()->GetMatrices()[k].GetVnlMatrix()(0, 3) 
				/ rayScaleFactors[k];
			zVector(k * (ImageDimension - 1) + 1) = 
				m_MeasurementVectors[k][1] -
				this->GetMultipleMeasurementGeometry()->GetMatrices()[k].GetVnlMatrix()(1, 3) 
				/ rayScaleFactors[k];
		}

		this->GetEstimator()->Setz(zVector);

		this->GetEstimator()->Compute();
		
		this->m_DRRTranslationVector = H_sub * (this->GetEstimator()->Getx()) - H_sub * (this->m_TargetCentroid);
		this->m_TargetDRRCentroid += this->m_DRRTranslationVector;

		this->SetTargetCentroid(m_Estimator->Getx());
	}

} // end namespace igt

#endif // __igtMultiple2DMeasurementTracking_cxx
