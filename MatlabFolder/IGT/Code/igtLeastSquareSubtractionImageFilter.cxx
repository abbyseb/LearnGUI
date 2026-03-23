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

#ifndef __igtLeastSquareSubtractionImageFilter_cxx
#define __igtLeastSquareSubtractionImageFilter_cxx

#include "igtLeastSquareSubtractionImageFilter.h"

namespace igt
{
  template<class TOutputImage>
  LeastSquareSubtractionImageFilter<TOutputImage>
  ::LeastSquareSubtractionImageFilter()
  {
    this->SetNumberOfRequiredInputs(2);

    m_a = 1;
	m_b = 0;
	m_ScaledImage = 0;
  }

  template<class TOutputImage>
  void LeastSquareSubtractionImageFilter<TOutputImage>
  ::VerifyInputInformation()
  {
    ImageSizeType size1 = this->GetInput(0)->GetLargestPossibleRegion().GetSize();
	ImageSizeType size2 = this->GetInput(1)->GetLargestPossibleRegion().GetSize();
	if( size1 != size2 )
		itkGenericExceptionMacro(<< "The two input images must have the same size");
  }

  template<class TOutputImage>
  void LeastSquareSubtractionImageFilter<TOutputImage>
  ::GenerateData()
  {
	double xSquare, onesSquare, xOnes, yOnes, yX;

    MultiplyImageFilterType::Pointer multiplyFilter = MultiplyImageFilterType::New();
    SubtractImageFilterType::Pointer subtractFilter = SubtractImageFilterType::New();
	StatisticsImageFilterType::Pointer statisticsFilter = StatisticsImageFilterType::New();

	// Getting yOnes
	statisticsFilter->SetInput( this->GetInput(0) );
	statisticsFilter->Update();
	yOnes = statisticsFilter->GetSum();

	// Getting xOnes
	statisticsFilter->SetInput( this->GetInput(1) );
	statisticsFilter->Update();
	xOnes = statisticsFilter->GetSum();

	// For xSquare and yX
	statisticsFilter->SetInput( multiplyFilter->GetOutput() );

	// Getting xSquare
	multiplyFilter->SetInput(0, this->GetInput(1));
	multiplyFilter->SetInput(1, this->GetInput(1));
	multiplyFilter->Update();
	statisticsFilter->Update();
	xSquare = statisticsFilter->GetSum();

	// Getting yX
	multiplyFilter->SetInput(0, this->GetInput(0));
	multiplyFilter->SetInput(1, this->GetInput(1));
	multiplyFilter->Update();
	statisticsFilter->Update();
	yX = statisticsFilter->GetSum();

	// Getting onesSquare
	onesSquare = 1.;
	for (unsigned int k = 0; k < OutputImageType::ImageDimension; ++k)
		onesSquare *= this->GetInput(0)->GetLargestPossibleRegion().GetSize()[k];
    
	// Calcualte a and b
	m_a = onesSquare * yX - yOnes * xOnes;
	m_a /= xSquare * onesSquare - xOnes * xOnes;
	m_b = xSquare * yOnes - xOnes * yX;
	m_b /= xSquare * onesSquare - xOnes * xOnes;

	// Calculate y - (ax + b)
	SubtractImageFilterType::Pointer differenceImageFilter = SubtractImageFilterType::New();
	MultiplyImageFilterType::Pointer scaleImageFilter = MultiplyImageFilterType::New();
	AddImageFilterType::Pointer addConstantFilter = AddImageFilterType::New();
	scaleImageFilter->SetInput( this->GetInput(1) );
	scaleImageFilter->SetConstant( m_a );
	addConstantFilter->SetInput1( scaleImageFilter->GetOutput() );
	addConstantFilter->SetConstant2( m_b );
	differenceImageFilter->SetInput1( this->GetInput(0) );
	differenceImageFilter->SetInput2( addConstantFilter->GetOutput() );
	differenceImageFilter->Update();

	this->GraftOutput( differenceImageFilter->GetOutput() );
	m_ScaledImage = addConstantFilter->GetOutput();
	m_ScaledImage->DisconnectPipeline();

  }

} // end namespace igt

#endif // __igtLeastSquareSubtractionImageFilter_cxx
