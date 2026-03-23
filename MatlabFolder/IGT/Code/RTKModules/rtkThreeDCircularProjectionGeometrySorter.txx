/*=========================================================================
 *
 *  Copyright RTK Consortium
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

#ifndef __rtkThreeDCircularProjectionGeometrySorter_txx
#define __rtkThreeDCircularProjectionGeometrySorter_txx

#include <itkMacro.h>

namespace rtk
{

ThreeDCircularProjectionGeometrySorter
::ThreeDCircularProjectionGeometrySorter()
{
	m_InputGeometry = NULL;
}

ThreeDCircularProjectionGeometrySorter
::~ThreeDCircularProjectionGeometrySorter()
{}

void
ThreeDCircularProjectionGeometrySorter
::SetInputGeometry(const GeometryPointer geometry)
{
	m_InputGeometry = geometry;
}

void
ThreeDCircularProjectionGeometrySorter
::SetBinNumberVector(const BinNumberVectorType binvector)
{
	m_BinNumberVector = binvector;
}

std::vector<ThreeDCircularProjectionGeometrySorter::GeometryPointer>
ThreeDCircularProjectionGeometrySorter
::GetOutput( )
{
	return m_OutputGeometryVector;
}

void
ThreeDCircularProjectionGeometrySorter
::Sort( )
{
	unsigned int nProj = m_InputGeometry->GetGantryAngles().size();
	
	if(nProj != m_BinNumberVector.size())
	{
      itkWarningMacro(<< "Number of projections does not match with bin number vector.");
	  return;
	}
	
	BinNumberType nBin = this->GetMaximumBinNumber();
	
	m_OutputGeometryVector.resize(nBin);
	
	for(unsigned int k = 0; k < nBin; ++k)
	  {
	  m_OutputGeometryVector[k] = GeometryType::New();
	  m_OutputGeometryVector[k]->SetIsShortScan( m_InputGeometry->GetIsShortScan() );
	  m_OutputGeometryVector[k]->SetShortScanAngle( m_InputGeometry->GetShortScanAngle() );
	  m_OutputGeometryVector[k]->SetIsDTS( m_InputGeometry->GetIsDTS() );
	  }
		
	std::vector<double> gantryAngles = m_InputGeometry->GetGantryAngles();
    std::vector<double> outOfPlaneAngles = m_InputGeometry->GetOutOfPlaneAngles();
    std::vector<double> inPlaneAngles = m_InputGeometry->GetInPlaneAngles();
	std::vector<double> sid = m_InputGeometry->GetSourceToIsocenterDistances();
	std::vector<double> sdd = m_InputGeometry->GetSourceToDetectorDistances();
	std::vector<double> sourceOffsetsX = m_InputGeometry->GetSourceOffsetsX();
	std::vector<double> sourceOffsetsY = m_InputGeometry->GetSourceOffsetsY();
	std::vector<double> projectionOffsetsX = m_InputGeometry->GetProjectionOffsetsX();
	std::vector<double> projectionOffsetsY = m_InputGeometry->GetProjectionOffsetsY();
	
	for(unsigned int k = 0; k < nProj; ++k)
	{
		if(m_BinNumberVector[k] > 0)
		{
			m_OutputGeometryVector[m_BinNumberVector[k]-1]
				->AddProjectionInRadians(sid[k],sdd[k],gantryAngles[k],
										 projectionOffsetsX[k],projectionOffsetsY[k],
										 outOfPlaneAngles[k],inPlaneAngles[k],
										sourceOffsetsX[k],sourceOffsetsY[k]);
		}
	}
	
	for(unsigned int k = 0; k < nBin; ++k)
	  m_OutputGeometryVector[k]->UpdateMeanAngle();

}

ThreeDCircularProjectionGeometrySorter::BinNumberType
ThreeDCircularProjectionGeometrySorter
::GetMaximumBinNumber( )
{
	BinNumberType maxvalue = 0;
	for(unsigned int k = 0; k < m_BinNumberVector.size(); ++k)
	{
		maxvalue = std::max( maxvalue, m_BinNumberVector[k] );
	}
	return maxvalue;
}

} // end namespace rtk

#endif
