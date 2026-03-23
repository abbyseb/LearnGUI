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

#ifndef __rtkVectorSorter_txx
#define __rtkVectorSorter_txx

#include <itkMacro.h>

namespace rtk
{

template <class TInput>
VectorSorter<TInput>
::VectorSorter()
{}

template <class TInput>
VectorSorter<TInput>
::~VectorSorter()
{}

template <class TInput>
void
VectorSorter<TInput>
::SetInputVector(const InputVectorType input)
{
	m_DataVector = input;
}

template <class TInput>
void
VectorSorter<TInput>
::SetBinNumberVector(const BinNumberVectorType binvector)
{
	m_BinNumberVector = binvector;
}

template <class TInput>
const std::vector< std::vector<TInput> > &
VectorSorter<TInput>
::GetOutput( )
{
	return m_OutputVector;
}

template <class TInput>
void
VectorSorter<TInput>
::Sort( )
{
	if(m_DataVector.size() != m_BinNumberVector.size())
	{
      itkWarningMacro(<< "Data vector does not match with bin number vector.");
	  return;
	}
	
	BinNumberType nBin = this->GetMaximumBinNumber();
	
	m_OutputVector.resize(nBin);
	
	for(unsigned int k = 0; k < m_DataVector.size(); ++k)
	{
		if(m_BinNumberVector[k] > 0)
		{
			m_OutputVector[m_BinNumberVector[k]-1].push_back(m_DataVector[k]);
		}
	}
	
}

template <class TInput>
typename VectorSorter<TInput>::BinNumberType
VectorSorter<TInput>
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
