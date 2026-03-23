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

#ifndef __igtKalmanFilter_cxx
#define __igtKalmanFilter_cxx

#include "igtKalmanFilter.h"

namespace igt
{
	template<class TValueType>
	KalmanFilter<TValueType>::KalmanFilter()
	{
		this->SetNumberOfStateVariables(3);
		this->SetNumberOfMeasurements(3);
	}

	template<class TValueType>
	void KalmanFilter<TValueType>::SetNumberOfStateVariables(unsigned int n)
	{
		m_NumberOfStateVariables = n;
		m_x = VectorType(n, 0);
		m_u = VectorType(n, 0);
		m_F = MatrixType(n, n, 0);
		m_F.set_identity();
		m_B = MatrixType(n, n, 0);
		m_Q = MatrixType(n, n, 0);
		m_Q.set_identity();
		m_P = MatrixType(n, n, 0);
		m_P.set_identity();
	}

	template<class TValueType>
	void KalmanFilter<TValueType>::SetNumberOfMeasurements(unsigned int m)
	{
		m_NumberOfMeasurements = m;
		m_R = MatrixType(m, m, 0);
		m_R.set_identity();
	}

	template<class TValueType>
	void KalmanFilter<TValueType>::PredictState()
	{
		m_xPredicted = m_F * m_x + m_B * m_u;

		m_PPredicted = m_F * m_P * (m_F.transpose()) + m_Q;
	}

	template<class TValueType>
	void KalmanFilter<TValueType>::IncorporateMeasurement()
	{
		// Check if the measurement matrix has been set
		if (m_H.columns() != m_NumberOfStateVariables
			|| m_H.rows() != m_NumberOfMeasurements)
		{
			std::cerr << "The measurement matrix has not been set properly." << std::endl;
			return;
		}

		MatrixType denominator = vnl_matrix_inverse<TValueType>(m_H * m_PPredicted * (m_H.transpose()) + m_R);
		m_K = m_PPredicted *(m_H.transpose()) * denominator;

		m_x = m_xPredicted + m_K * (m_z - m_H * m_xPredicted);

		MatrixType identity(m_NumberOfStateVariables, m_NumberOfStateVariables, 0);
		identity.set_identity();
		m_P = (identity - m_K * m_H) * m_PPredicted;
	}

	template<class TValueType>
	void KalmanFilter<TValueType>::Compute()
	{
		this->PredictState();
		this->IncorporateMeasurement();
	}

	template<class TValueType>
	typename KalmanFilter<TValueType>::VectorType KalmanFilter<TValueType>::
		CalculateMean(std::vector<typename KalmanFilter<TValueType>::VectorType>& v)
	{
		VectorType mean(v[0].size(), 0);
		for (unsigned int k = 0; k != v.size(); ++k)
			mean += v[k] / v.size();
		return mean;
	}

	template<class TValueType>
	typename KalmanFilter<TValueType>::MatrixType KalmanFilter<TValueType>::
		CalculateCovariance(std::vector<typename KalmanFilter<TValueType>::VectorType>& v, 
		typename KalmanFilter<TValueType>::VectorType mu, 
		bool isSample = false)
	{
		double N;
		if (isSample)
			N = v.size() - 1;
		else
			N = v.size();

		MatrixType cov(mu.size(), mu.size(), 0);
		for (unsigned int i = 0; i != mu.size(); ++i)
		{
			for (unsigned int j = 0; j != mu.size(); ++j)
			{
				for (unsigned int k = 0; k != v.size(); ++k)
					cov(i, j) += (v[k](i) - mu(i)) * (v[k](j) - mu(j)) / N;
			}
		}

		return cov;
	}

} // end namespace igt

#endif // __igtKalmanFilter_cxx
