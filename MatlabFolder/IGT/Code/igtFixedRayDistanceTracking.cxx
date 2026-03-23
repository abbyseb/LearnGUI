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

#ifndef __igtFixedRayDistanceTracking_cxx
#define __igtFixedRayDistanceTracking_cxx

#include <igtFixedRayDistanceTracking.h>

namespace igt
{
	template<class ImageType>
	void FixedRayDistanceTracking<ImageType>::Estimate()
	{
		if (this->GetEstimator()->GetR().columns() != ImageDimension 
			|| this->GetEstimator()->GetR().rows() != ImageDimension)
			std::cerr << "The dimension of R must be " << ImageDimension << " by " << ImageDimension << "." << std::endl;

		// First set up measurement matrix
		MatrixType H(ImageDimension, ImageDimension, 0.);
		double rayScaleFactor = 0;
		for (unsigned int k_mat = 0; k_mat != ImageDimension; ++k_mat)
		{
			rayScaleFactor +=
				this->GetGeometry()->GetMatrices().back()(ImageDimension - 1, k_mat)
				* m_TargetCentroid(k_mat);
		}
		rayScaleFactor +=
			this->GetGeometry()->GetMatrices().back()(ImageDimension - 1, ImageDimension);

		for (unsigned i = 0; i != ImageDimension; ++i)
		{
			for (unsigned j = 0; j != ImageDimension; ++j)
			{
				H(i, j) = this->GetGeometry()->GetMatrices().back()(i, j);
				if (i < ImageDimension - 1)
					H(i, j) /= rayScaleFactor;
			}
		}

		this->GetEstimator()->SetH(H);
		// Estimate target centroid
		MatrixType H_sub = H.extract(ImageDimension - 1, ImageDimension, 0, 0);

		VectorType targetDRRCentroid = H_sub * m_TargetCentroid;
		targetDRRCentroid +=
			this->GetGeometry()->GetMatrices().back().GetVnlMatrix().get_column(3).extract(2, 0)
			/ rayScaleFactor;
		this->SetTargetDRRCentroid(targetDRRCentroid);

		// Target centroid
		this->GetEstimator()->Setx(m_TargetCentroid);

		// Measurement vector

		VectorType zVector = VectorType(ImageDimension, 0);
		zVector(0) =
			targetDRRCentroid(0) + this->GetDRRTranslationVector()(0) -
			this->GetGeometry()->GetMatrices().back().GetVnlMatrix()(0, 3) / rayScaleFactor;
		zVector(1) =
			targetDRRCentroid(1) + this->GetDRRTranslationVector()(1) -
			this->GetGeometry()->GetMatrices().back().GetVnlMatrix()(1, 3) / rayScaleFactor;
		zVector(2) = rayScaleFactor - this->GetGeometry()->GetMatrices().back()(ImageDimension - 1, ImageDimension);

		this->GetEstimator()->Setz(zVector);

		this->GetEstimator()->Compute();
		
		this->m_DRRTranslationVector = H_sub * (this->GetEstimator()->Getx()) - H_sub * (this->m_TargetCentroid);
		this->m_TargetDRRCentroid += this->m_DRRTranslationVector;

		this->SetTargetCentroid(m_Estimator->Getx());
	}

} // end namespace igt

#endif // __igtFixedRayDistanceTracking_cxx
