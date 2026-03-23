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

#ifndef __igtMultiple2DMeasurementTracking_h
#define __igtMultiple2DMeasurementTracking_h

#include <igtTargetTracking.h>

namespace igt
{

/** \class Multiple2DMeasurementTracking
 * \brief This class inherits from the base class 
 *		  TargetTracking, and uses multiple 2D
 *		  measurements in projection space to estimate
 *		  the 3D target position. The last 2D measurement
 *		  is always the one obtained from template matching.
 * \author Andy Shieh 2016
 */
template<class ImageType>
class ITK_EXPORT Multiple2DMeasurementTracking :
	public igt::TargetTracking<ImageType>
{
public:
	/** Standard class typedefs. */
	typedef Multiple2DMeasurementTracking	Self;
	typedef igt::TargetTracking<ImageType>	Superclass;
	typedef itk::SmartPointer<Self>			Pointer;
	typedef itk::SmartPointer<const Self>	ConstPointer;

	itkStaticConstMacro(ImageDimension, unsigned int, ImageType::ImageDimension);

	// Estimating step incorporting prior knowledge
	virtual void Estimate();

	/** Number of 2D measurement pairs */
	itkGetMacro(NumberOf2DMeasurements, unsigned int);
	void SetNumberOf2DMeasurements(unsigned int value)
	{
		m_NumberOf2DMeasurements = value;
		this->GetEstimator()->SetNumberOfMeasurements((ImageDimension - 1) * value);
	}

	/** The 2D measurement vectors */
	itkGetMacro(MeasurementVectors, std::vector<VectorType>&);
	void Add2DMeasurement(VectorType vector)
	{
		m_MeasurementVectors.push_back(vector);
	}
	void Clear2DMeasurements()
	{
		m_MeasurementVectors.clear();
	}

	/** Geometry object for using multiple measurements */
	itkGetMacro(MultipleMeasurementGeometry, rtk::ThreeDCircularProjectionGeometry::Pointer);
	itkSetMacro(MultipleMeasurementGeometry, rtk::ThreeDCircularProjectionGeometry::Pointer);

	/* Standard New method. */
	itkNewMacro(Self);

	/** Runtime information support. */
	itkTypeMacro(Multiple2DMeasurementTracking, igt::TargetTracking);

protected:
	Multiple2DMeasurementTracking(){
		this->SetNumberOf2DMeasurements(1);
	}
	~Multiple2DMeasurementTracking(){}

	/** Geometry object for using multiple measurements */
	rtk::ThreeDCircularProjectionGeometry::Pointer m_MultipleMeasurementGeometry;

	/** Number of 2D measurement pairs */
	unsigned int m_NumberOf2DMeasurements;

	/** Vector of 2D measurement vectors */
	std::vector<VectorType> m_MeasurementVectors;

private:
	//purposely not implemented
	Multiple2DMeasurementTracking(const Self&);
	void operator=(const Self&);
}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtMultiple2DMeasurementTracking.cxx"
#endif

#endif
