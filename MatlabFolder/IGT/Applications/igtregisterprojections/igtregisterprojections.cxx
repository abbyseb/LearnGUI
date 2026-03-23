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

#include "igtregisterprojections_ggo.h"
#include "rtkGgoFunctions.h"
#include "igtConfiguration.h"

#include "igtRegisterProjectionsFilter.h"

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkTimeProbe.h>

/* Author: Andy Shieh 2016 */

int main(int argc, char * argv[])
{
	GGO(igtregisterprojections, args_info);

	typedef float PixelType;
	const unsigned int Dimension = 3;

	typedef itk::Image< PixelType, Dimension >     ImageType;
	typedef itk::Image< unsigned char, Dimension - 1 >	MaskType;

	// Reading
	typedef itk::ImageFileReader<ImageType>	ImageReaderType;
	typedef itk::ImageFileReader<MaskType>	MaskReaderType;
	ImageReaderType::Pointer movReader = ImageReaderType::New();
	ImageReaderType::Pointer fixReader = ImageReaderType::New();
	MaskReaderType::Pointer maskReader = MaskReaderType::New();
	movReader->SetFileName(args_info.moving_arg);
	fixReader->SetFileName(args_info.fixed_arg);
	if (args_info.mask_given)
		maskReader->SetFileName(args_info.mask_arg);

	itk::TimeProbe readerProbe;
	if (args_info.verbose_flag)
		std::cout << "Reading... " << std::flush;
	readerProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION(movReader->Update())
	TRY_AND_EXIT_ON_ITK_EXCEPTION(fixReader->Update())
	if (args_info.mask_given)
	{
		TRY_AND_EXIT_ON_ITK_EXCEPTION(maskReader->Update())
	}
	readerProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << readerProbe.GetMean() << ' ' << readerProbe.GetUnit() << std::endl;

	// Pass inputs to filter
	typedef igt::RegisterProjectionsFilter<ImageType, ImageType> RegisterProjectionsType;
	RegisterProjectionsType::Pointer registerProjectionsFilter = RegisterProjectionsType::New();
	registerProjectionsFilter->SetUseInverse(args_info.inverse_flag);
	registerProjectionsFilter->SetInput(0, movReader->GetOutput());
	registerProjectionsFilter->SetInput(1, fixReader->GetOutput());
	if (args_info.mask_given)
		registerProjectionsFilter->SetMask(maskReader->GetOutput());
	registerProjectionsFilter->SetVerbose(args_info.verbose_flag);
	
	switch (args_info.transform_arg)
	{
	case transform_arg_Rigid:
		registerProjectionsFilter->SetTransformOption(RegisterProjectionsType::TransformOption::Rigid);
		break;
	case transform_arg_Translation:
		registerProjectionsFilter->SetTransformOption(RegisterProjectionsType::TransformOption::Translation);
		break;
	}

	switch (args_info.metric_arg)
	{
	case metric_arg_MMI:
		registerProjectionsFilter->SetMetricOption(RegisterProjectionsType::MetricOption::MattesMutualInformation);
		break;
	case metric_arg_MS:
		registerProjectionsFilter->SetMetricOption(RegisterProjectionsType::MetricOption::MeanSquares);
		break;
	case metric_arg_NC:
		registerProjectionsFilter->SetMetricOption(RegisterProjectionsType::MetricOption::NormalizedCorrelation);
		break;
	}

	// Write results
	typedef itk::ImageFileWriter<ImageType> ImageWriterType;
	ImageWriterType::Pointer writer = ImageWriterType::New();
	writer->SetInput(registerProjectionsFilter->GetOutput());
	writer->SetFileName(args_info.output_arg);
	
	itk::TimeProbe writerProbe;
	if (args_info.verbose_flag)
		std::cout << "Calculating and writing... " << std::flush;
	writerProbe.Start();

	// Write image
	TRY_AND_EXIT_ON_ITK_EXCEPTION(writer->Update())

	// Write transform parameters
	std::ofstream parafile(args_info.outpara_arg);
	for (unsigned int k = 0; k < registerProjectionsFilter->GetTransformParameters().size(); k++)
	{
		for (unsigned int n = 0; n < registerProjectionsFilter->GetTransformParameters()[k].size(); n++)
		{
			parafile << registerProjectionsFilter->GetTransformParameters()[k][n];
			if (n < registerProjectionsFilter->GetTransformParameters()[k].size() - 1)
				parafile << ",";
		}
		if (k < registerProjectionsFilter->GetTransformParameters().size() - 1)
			parafile << std::endl;
	}
	parafile.close();

	writerProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << writerProbe.GetMean() << ' ' << writerProbe.GetUnit() << std::endl;

    return EXIT_SUCCESS;
}