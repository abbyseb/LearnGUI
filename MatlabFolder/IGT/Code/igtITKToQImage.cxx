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

#ifndef __igtITKToQImage_cxx
#define __igtITKToQImage_cxx

#include "igtITKToQImage.h"
#include <itkLineConstIterator.h>

#include <QImage>

namespace igt
{

	template< class TInputImage >
	void
	ITKToQImage< TInputImage >
	::GenerateMonoScaleQImage(ITKImageConstPointerType itkImagePointer, QImage *qImagePointer, ITKImageRegionType region, ITKImagePixelType itkMin, ITKImagePixelType itkMax)
	{
		
		if (!itkImagePointer)
			itkGenericExceptionMacro(<< "ITK image pointer is not set");
		if (!qImagePointer)
			itkGenericExceptionMacro(<< "QImage pointer is not set");

		ITKImagePixelType minMaxGap = itkMax - itkMin;

		for (unsigned int k = 0; k != itkImagePointer->GetLargestPossibleRegion().GetSize()[1]; ++k)
		{
			uchar *qit = qImagePointer->scanLine(k);
			ITKImageType::IndexType corner1 = itkImagePointer->GetLargestPossibleRegion().GetIndex();
			corner1[1] = k;
			ITKImageType::IndexType corner2 = corner1;
			corner2[0] = itkImagePointer->GetLargestPossibleRegion().GetSize()[0] - 1;
			itk::LineConstIterator<ITKImageType> iit(itkImagePointer, corner1, corner2);
			iit.GoToBegin();
			while (!iit.IsAtEnd())
			{
				ITKImagePixelType inValue = iit.Get();
				if (inValue > itkMax)
					inValue = itkMax;
				else if (inValue < itkMin)
					inValue = itkMin;
				*qit = uchar(std::floor((inValue - itkMin) / minMaxGap * 255 + 0.5));
				++iit;
				++qit;
			}
		}
	}

	template< class TInputImage >
	void
	ITKToQImage< TInputImage >
	::GenerateBinaryMaskQImage(ITKImageConstPointerType itkImagePointer, QImage *qImagePointer, ITKImageRegionType region, ITKImagePixelType threshold, bool inverseMasking)
	{
		if (!itkImagePointer)
			itkGenericExceptionMacro(<< "ITK image pointer is not set");
		if (!qImagePointer)
			itkGenericExceptionMacro(<< "QImage pointer is not set");

		for (unsigned int k = 0; k != itkImagePointer->GetLargestPossibleRegion().GetSize()[1]; ++k)
		{
			uchar *qit = qImagePointer->scanLine(k);
			ITKImageType::IndexType corner1 = itkImagePointer->GetLargestPossibleRegion().GetIndex();
			corner1[1] = k;
			ITKImageType::IndexType corner2 = corner1;
			corner2[0] = itkImagePointer->GetLargestPossibleRegion().GetSize()[0] - 1;
			itk::LineConstIterator<ITKImageType> iit(itkImagePointer, corner1, corner2);
			iit.GoToBegin();
			while (!iit.IsAtEnd())
			{
				ITKImagePixelType inValue = iit.Get();
				if (inverseMasking)
				{
					if (inValue >= threshold)
						*qit = 0;
					else
						*qit = 1;
				}
				else
				{
					if (inValue > threshold)
						*qit = 1;
					else
						*qit = 0;
				}
				++iit;
				++qit;
			}
		}
	}

} // end namespace igt

#endif // __igtITKToQImage_cxx
