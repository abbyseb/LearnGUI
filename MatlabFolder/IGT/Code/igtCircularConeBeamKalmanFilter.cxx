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

#ifndef __igtCircularConeBeamKalmanFilter_cxx
#define __igtCircularConeBeamKalmanFilter_cxx

#include "igtCircularConeBeamKalmanFilter.h"

namespace igt
{
	template<class TValueType>
	CircularConeBeamKalmanFilter<TValueType>::CircularConeBeamKalmanFilter()
	{
		this->SetNumberOfStateVariables(3);
		this->SetNumberOfMeasurements(3);
	}

	template<class TValueType>
	void CircularConeBeamKalmanFilter<TValueType>::IncorporateMeasurement()
	{
		this->SetH(this->CalculateH(this->GetGeometry(), m_xPredicted));

		MatrixType denominator = vnl_matrix_inverse<TValueType>(m_H * m_PPredicted * (m_H.transpose()) + m_R);
		m_K = m_PPredicted *(m_H.transpose()) * denominator;

		VectorType projectedPosition = this->GetGeometry()->CalculateProjSpaceCoordinates(m_xPredicted)[0];
		VectorType Hx = projectedPosition.extract(2, 0);
		m_x = m_xPredicted + m_K * (m_z - Hx);

		MatrixType identity(m_NumberOfStateVariables, m_NumberOfStateVariables, 0);
		identity.set_identity();
		m_P = (identity - m_K * m_H) * m_PPredicted;
	}

	template<class TValueType>
	typename CircularConeBeamKalmanFilter<TValueType>::MatrixType 
		CircularConeBeamKalmanFilter<TValueType>::CalculateH(
		rtk::ThreeDCircularProjectionGeometry::Pointer geometry, typename CircularConeBeamKalmanFilter<TValueType>::VectorType x)
	{
		MatrixType H(m_NumberOfMeasurements, m_NumberOfStateVariables, 0);
		
		const itk::Matrix<double,3,4> G = geometry->GetMatrices()[0];
		VectorType z = geometry->CalculateProjSpaceCoordinates(x)[0];
		double w = z(2);

		for (unsigned int i = 0; i != H.rows(); ++i)
		{
			for (unsigned int j = 0; j != H.columns(); ++j)
				H(i, j) = G(i, j) / w - z(i) * G(2, j) / w;
		}
		
		return H;
	}
} // end namespace igt

#endif // __igtCircularConeBeamKalmanFilter_cxx
