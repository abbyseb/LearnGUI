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

#ifndef __igtLinearCombinationFilter_txx
#define __igtLinearCombinationFilter_txx

#include "igtLinearCombinationFilter.h"

namespace igt
{
	template<class TInputImage, class TOutputImage>
	void LinearCombinationFilter<TInputImage, TOutputImage>::GenerateInputRequestedRegion()
	{
	}

	template<class TInputImage, class TOutputImage>
	void LinearCombinationFilter<TInputImage, TOutputImage>::GenerateOutputInformation()
	{
		if (this->m_InputImages.size() < 1)
			std::cerr << "No input images have been assigned." << std::endl;
		
		if (!this->m_InputImages[0])
			std::cerr << "The first input image pointer must not be null." << std::endl;
		
		// Input check	
		TInputImage::RegionType region0 = this->m_InputImages[0]->GetLargestPossibleRegion();
		for (unsigned int k = 1; k != this->m_InputImages.size(); ++k)
		{
			if (!this->m_InputImages[k])
				continue;
				
			TInputImage::RegionType region = this->m_InputImages[k]->GetLargestPossibleRegion();
			for (unsigned int dim = 0; dim != TInputImage::ImageDimension; dim++)
			{
				if ( region.GetSize()[dim] != region0.GetSize()[dim]
				  || region.GetIndex()[dim] != region0.GetIndex()[dim])
				  std::cerr << "The dimension of input image #" << k << " does not match with that of input image 0." << std::endl;
			}
		}
		
		// Initialize output with the first input
		this->GetOutput()->SetLargestPossibleRegion(this->m_InputImages[0]->GetLargestPossibleRegion());
		this->GetOutput()->SetOrigin(this->m_InputImages[0]->GetOrigin());
		this->GetOutput()->SetDirection(this->m_InputImages[0]->GetDirection());
		this->GetOutput()->SetSpacing(this->m_InputImages[0]->GetSpacing());
	}

	template<class TInputImage, class TOutputImage>
	void LinearCombinationFilter<TInputImage, TOutputImage>::BeforeThreadedGenerateData()
	{
	}
	
	template<class TInputImage, class TOutputImage>
	void LinearCombinationFilter<TInputImage, TOutputImage>::
		ThreadedGenerateData(const typename TInputImage::RegionType& outputRegionForThread, typename itk::ThreadIdType itkNotUsed(threadID))
	{
		IteratorType it_out(this->GetOutput(), outputRegionForThread);
		
		for (unsigned int k = 0; k != this->m_InputImages.size(); ++k)
		{
			if (this->m_InputImages[k])
			{
				ConstIteratorType it(this->m_InputImages[k], outputRegionForThread);
				it_out.GoToBegin();
				it.GoToBegin();
				while (!it_out.IsAtEnd())
				{
					while (!it_out.IsAtEndOfLine())
					{
						if (k == 0)
							it_out.Set( it.Get() * this->m_Multipliers[k] );
						else
							it_out.Set( it_out.Get() + it.Get() * this->m_Multipliers[k] );
						++it_out;
						++it;
					}
					it_out.NextLine();
					it.NextLine();
				}
			}
			else
			{
				it_out.GoToBegin();
				while (!it_out.IsAtEnd())
				{
					while (!it_out.IsAtEndOfLine())
					{
						it_out.Set( it_out.Get() + this->m_Multipliers[k] );
						++it_out;
					}
					it_out.NextLine();
				}
			}
		}
	}

} // end namespace igt

#endif // __igtLinearCombinationFilter_txx
