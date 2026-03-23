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

#include "igtmhatodicomseries_ggo.h"
#include "rtkGgoFunctions.h"
#include "igtConfiguration.h"

#include <itkImageFileReader.h>
#include <itkImageSeriesWriter.h>
#include <itkPermuteAxesImageFilter.h>
#include <itkGDCMImageIO.h>

#include <iostream>
#include <iomanip>
#include <sstream>

/* Author: Andy Shieh 2017 */

int main(int argc, char * argv[])
{
	GGO(igtmhatodicomseries, args_info);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	static const std::string slash = "\\";
#else
	static const std::string slash = "/";
#endif

	typedef short PixelType;
	const unsigned int Dimension = 3;
	typedef itk::Image<PixelType, Dimension> VolumeType;
	typedef itk::Image<PixelType, Dimension-1> ImageType;

	typedef itk::ImageFileReader<VolumeType> ReaderType;
	ReaderType::Pointer reader = ReaderType::New();
	reader->SetFileName(args_info.input_arg);
	TRY_AND_EXIT_ON_ITK_EXCEPTION(reader->Update());

	typedef itk::PermuteAxesImageFilter<VolumeType> PermuteType;
	PermuteType::Pointer permuter = PermuteType::New();
	permuter->SetInput(reader->GetOutput());
	itk::FixedArray<unsigned int, 3> order;
	for (unsigned int dim = 0; dim < 3; ++dim)
		order[dim] = args_info.permutation_arg[dim];
	permuter->SetOrder(order);
	TRY_AND_EXIT_ON_ITK_EXCEPTION(permuter->Update());

	typedef itk::ImageSeriesWriter<VolumeType, ImageType> WriterType;
	typedef itk::GDCMImageIO                        ImageIOType;

	ImageIOType::Pointer gdcmIO = ImageIOType::New();

	WriterType::Pointer writer = WriterType::New();
	writer->SetImageIO(gdcmIO);
	writer->SetInput(permuter->GetOutput());

	WriterType::FileNamesContainer fileNames;
	for (unsigned int k = 0; k < permuter->GetOutput()->GetLargestPossibleRegion().GetSize()[Dimension - 1]; ++k)
	{
		std::ostringstream os;
		os << std::setfill('0') << std::setw(5) << (k);
		std::string fileNumber = os.str();
		std::string fileName = args_info.output_arg + slash + fileNumber + ".dcm";
		fileNames.push_back(fileName);
	}

	writer->SetFileNames(fileNames);
	TRY_AND_EXIT_ON_ITK_EXCEPTION(writer->Update());

    return EXIT_SUCCESS;
}