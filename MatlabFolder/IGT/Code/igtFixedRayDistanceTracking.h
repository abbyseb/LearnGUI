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

#ifndef __igtFixedRayDistanceTracking_h
#define __igtFixedRayDistanceTracking_h

#include <igtTargetTracking.h>

namespace igt
{

/** \class FixedRayDistanceTracking
 * \brief This class inherits from the base class 
 *		  TargetTracking, and assumes a fixed ray
 *		  distance from the target to the CBCT
 *		  imager to constrain tracking in the imaging
 *		  beam direction.
 * \author Andy Shieh 2016
 */
template<class ImageType>
class ITK_EXPORT FixedRayDistanceTracking :
	public igt::TargetTracking<ImageType>
{
public:
	/** Standard class typedefs. */
	typedef FixedRayDistanceTracking	Self;
	typedef igt::TargetTracking<ImageType>	Superclass;
	typedef itk::SmartPointer<Self>			Pointer;
	typedef itk::SmartPointer<const Self>	ConstPointer;

	itkStaticConstMacro(ImageDimension, unsigned int, ImageType::ImageDimension);

	// Estimating step incorporting prior knowledge
	virtual void Estimate();

	/* Standard New method. */
	itkNewMacro(Self);

	/** Runtime information support. */
	itkTypeMacro(FixedRayDistanceTracking, igt::TargetTracking);

protected:
	FixedRayDistanceTracking(){ this->GetEstimator()->SetNumberOfMeasurements(ImageDimension); }
	~FixedRayDistanceTracking(){}

private:
	//purposely not implemented
	FixedRayDistanceTracking(const Self&);
	void operator=(const Self&);
}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtFixedRayDistanceTracking.cxx"
#endif

#endif
