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

#ifndef __igtCircularConeBeamKalmanFilter_h
#define __igtCircularConeBeamKalmanFilter_h

#include <itkProcessObject.h>
#include <igtKalmanFilter.h>
#include <rtkThreeDCircularProjectionGeometry.h>

namespace igt
{

/** \class CircularConeBeamKalmanFilter
 * \brief This is the dereived class for estimating 3D position
 *		  from 2D cone-beam projection position.
 * \author Andy Shieh 2016
 */

template<class TValueType = double>
class ITK_EXPORT CircularConeBeamKalmanFilter : public igt::KalmanFilter<TValueType>
{
public:
	/** Standard class typedefs. */
	typedef CircularConeBeamKalmanFilter	Self;
	typedef igt::KalmanFilter<TValueType>	Superclass;
	typedef itk::SmartPointer<Self>			Pointer;
	typedef itk::SmartPointer<const Self>	ConstPointer;

	/* Standard New method. */
	itkNewMacro(Self);

	/** The actual workflows to be called by the user */
	virtual void IncorporateMeasurement();

	/** Get / Set the object pointer to projection geometry */
	itkGetMacro(Geometry, rtk::ThreeDCircularProjectionGeometry::Pointer);
	virtual void SetGeometry(rtk::ThreeDCircularProjectionGeometry::Pointer geometry)
	{
		m_Geometry = geometry;
	}

	MatrixType CalculateH(rtk::ThreeDCircularProjectionGeometry::Pointer geometry, VectorType x);

protected:
	CircularConeBeamKalmanFilter();
	~CircularConeBeamKalmanFilter(){}

	rtk::ThreeDCircularProjectionGeometry::Pointer m_Geometry;

private:
	//purposely not implemented
	CircularConeBeamKalmanFilter(const Self&);
	void operator=(const Self&);
}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtCircularConeBeamKalmanFilter.cxx"
#endif

#endif
