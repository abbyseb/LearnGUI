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

#ifndef __igtKalmanFilter_h
#define __igtKalmanFilter_h

#include <itkProcessObject.h>
#include "vnl/vnl_vector.h"

namespace igt
{

/** \class KalmanFilter
 * \brief This is the base class of the Kalman filter,
 *		  which implements a basic Kalman filter for
 *		  linear system. This class is templated over
 *		  the number of state variabes n and number of
 *		  measurements m.
 * \author Andy Shieh 2016
 */

template<class TValueType = double>
class ITK_EXPORT KalmanFilter : public itk::ProcessObject
{
public:
	/** Standard class typedefs. */
	typedef KalmanFilter					Self;
	typedef itk::ProcessObject				Superclass;
	typedef itk::SmartPointer<Self>			Pointer;
	typedef itk::SmartPointer<const Self>	ConstPointer;

	/** Matrix and vector typedefs*/
	typedef vnl_vector<TValueType>			VectorType;
	typedef vnl_matrix<TValueType>			MatrixType;

	/* Standard New method. */
	itkNewMacro(Self);

	/** The actual workflows to be called by the user */
	virtual void PredictState();
	virtual void IncorporateMeasurement();
	virtual void Compute();

	/* Get number of state variables N and measurements M */
	itkGetMacro(NumberOfStateVariables, unsigned int);
	itkGetMacro(NumberOfMeasurements, unsigned int);

	/* Set number of state variables and measurements */
	virtual void SetNumberOfStateVariables(unsigned int n);
	virtual void SetNumberOfMeasurements(unsigned int m);

	/* State vector*/
	itkGetMacro(x, VectorType);
	itkSetMacro(x, VectorType);

	/* Predicted state vector*/
	itkGetMacro(xPredicted, VectorType);
	itkSetMacro(xPredicted, VectorType);

	/* Control vector*/
	itkGetMacro(u, VectorType);
	itkSetMacro(u, VectorType);

	/* Measurement vector*/
	itkGetMacro(z, VectorType);
	itkSetMacro(z, VectorType);

	/* State transform matrix*/
	itkGetMacro(F, MatrixType);
	itkSetMacro(F, MatrixType);

	/* Control matrix*/
	itkGetMacro(B, MatrixType);
	itkSetMacro(B, MatrixType);

	/*State uncertainty covariance matrix*/
	itkGetMacro(P, MatrixType);
	itkSetMacro(P, MatrixType);
	itkGetMacro(PPredicted, MatrixType);
	itkSetMacro(PPredicted, MatrixType);

	/* External uncertainty covaraince matrix*/
	itkGetMacro(Q, MatrixType);
	itkSetMacro(Q, MatrixType);

	/* Kalman gain*/
	itkGetMacro(K, MatrixType);
	itkSetMacro(K, MatrixType);

	/* Measurement matrix*/
	itkGetMacro(H, MatrixType);
	itkSetMacro(H, MatrixType);
	
	/* Measurement uncertainty covariacne matrix*/
	itkGetMacro(R, MatrixType);
	itkSetMacro(R, MatrixType);

	// Calculate mean and covariance for a set of state variables
	static VectorType CalculateMean(std::vector<VectorType>& v);
	static MatrixType CalculateCovariance(std::vector<VectorType>& v, VectorType mu, bool isSample);

protected:
	KalmanFilter();
	~KalmanFilter(){}

	VectorType m_x, m_xPredicted, m_u;
	VectorType m_z;
	MatrixType m_F, m_B, m_Q, m_P, m_PPredicted,
		m_K, m_H, m_R;

	unsigned int m_NumberOfStateVariables, m_NumberOfMeasurements;

private:
	//purposely not implemented
	KalmanFilter(const Self&);
	void operator=(const Self&);
}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtKalmanFilter.cxx"
#endif

#endif
