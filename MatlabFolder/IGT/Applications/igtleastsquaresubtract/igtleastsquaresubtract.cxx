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

#include "igtleastsquaresubtract_ggo.h"
#include "rtkGgoFunctions.h"
#include "igtConfiguration.h"

#include <igtLeastSquareSubtractionImageFilter.h>

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

/* Author: Andy Shieh 2016 */

int main(int argc, char * argv[])
{
	GGO(igtleastsquaresubtract, args_info);

	typedef float OutputPixelType;
	const unsigned int Dimension = 3;

	typedef itk::Image< OutputPixelType, Dimension >     OutputImageType;

	typedef itk::ImageFileReader<OutputImageType>		 ReaderType;
	ReaderType::Pointer reader1 = ReaderType::New();
	ReaderType::Pointer reader2 = ReaderType::New();
	reader1->SetFileName( args_info.input1_arg );
	reader2->SetFileName( args_info.input2_arg );

	typedef igt::LeastSquareSubtractionImageFilter<OutputImageType>	DifferenceFilterType;
	DifferenceFilterType::Pointer diffFilter = DifferenceFilterType::New();
	diffFilter->SetInput(0, reader1->GetOutput() );
	diffFilter->SetInput(1, reader2->GetOutput() );

	typedef itk::ImageFileWriter<OutputImageType>		WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName( args_info.output_arg );
	writer->SetInput( diffFilter->GetOutput() );

	TRY_AND_EXIT_ON_ITK_EXCEPTION( writer->Update() );

	if(args_info.verbose_flag)
	{
		std::cout << "a = " << diffFilter->Geta() << std::endl;
		std::cout << "b = " << diffFilter->Getb() << std::endl;
	}
  
    return EXIT_SUCCESS;
}