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

#include "igttransformwithdvf_ggo.h"
#include "rtkGgoFunctions.h"
#include "igtConfiguration.h"

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkWarpImageFilter.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkTimeProbe.h>

/* Author: Andy Shieh 2017 */

int main(int argc, char * argv[])
{
	GGO(igttransformwithdvf, args_info);

	typedef float PixelType;
	const unsigned int Dimension = 3;

	typedef itk::Image<PixelType, Dimension>		ImageType;
	typedef itk::Vector<PixelType, Dimension>		VectorType;
	typedef itk::Image<VectorType, Dimension>		DVFType;
	typedef itk::Image<VectorType, Dimension + 1>	DVF4DType;
	typedef itk::ExtractImageFilter<DVF4DType, DVFType> ExtractDVFType;
	typedef itk::ImageFileReader<ImageType>			ImageReaderType;
	typedef itk::ImageFileReader<DVFType>			DVFReaderType;
	typedef itk::ImageFileReader<DVF4DType>			DVF4DReaderType;
	typedef itk::ImageFileWriter<ImageType>			ImageWriterType;
	typedef itk::WarpImageFilter<ImageType, ImageType, DVFType> WarpImageType;
	typedef itk::InvertIntensityImageFilter<DVFType, DVFType> InvertDVFType;
	typedef itk::LinearInterpolateImageFunction<ImageType, double> InterpolationType;

	itk::TimeProbe timeProbe;

	// Reading input image
	ImageReaderType::Pointer imgReader = ImageReaderType::New();
	imgReader->SetFileName(args_info.input_arg);

	if (args_info.verbose_flag)
		std::cout << "Reading input image... " << std::flush;
	timeProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION(imgReader->Update())
	timeProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << timeProbe.GetMean() << ' ' << timeProbe.GetUnit() << std::endl;
	timeProbe.Reset();

	// Reading DVF
	if (args_info.verbose_flag)
		std::cout << "Reading DVF... " << std::flush;
	timeProbe.Start();

	DVFReaderType::Pointer dvfReader = DVFReaderType::New();
	DVF4DReaderType::Pointer dvf4DReader = DVF4DReaderType::New();
	dvfReader->SetFileName(args_info.dvf_arg);
	dvf4DReader->SetFileName(args_info.dvf_arg);

	if (args_info.bin_given)
	{
		TRY_AND_EXIT_ON_ITK_EXCEPTION(dvf4DReader->Update())
	}
	else
	{
		TRY_AND_EXIT_ON_ITK_EXCEPTION(dvfReader->Update())
	}
	timeProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << timeProbe.GetMean() << ' ' << timeProbe.GetUnit() << std::endl;
	timeProbe.Reset();

	// Actual warping
	if (args_info.verbose_flag)
		std::cout << "Transforming image... " << std::flush;
	timeProbe.Start();

	ExtractDVFType::Pointer extracter = ExtractDVFType::New();
	if (args_info.bin_given)
	{
		extracter->SetInput(dvf4DReader->GetOutput());
		DVF4DType::RegionType region = dvf4DReader->GetOutput()->GetLargestPossibleRegion();
		DVF4DType::RegionType::SizeType size = region.GetSize();
		DVF4DType::RegionType::IndexType index = region.GetIndex();
		index[Dimension] = args_info.bin_arg - 1;
		size[Dimension] = 0;
		region.SetIndex(index);
		region.SetSize(size);
		extracter->SetExtractionRegion(region);
		extracter->SetDirectionCollapseToIdentity();
		TRY_AND_EXIT_ON_ITK_EXCEPTION(extracter->Update())
	}

	InterpolationType::Pointer interpolator = InterpolationType::New();
	WarpImageType::Pointer warper = WarpImageType::New();
	warper->SetInput(imgReader->GetOutput());
	warper->SetInterpolator(interpolator);
	warper->SetOutputOrigin(imgReader->GetOutput()->GetOrigin());
	warper->SetOutputSpacing(imgReader->GetOutput()->GetSpacing());
	warper->SetOutputDirection(imgReader->GetOutput()->GetDirection());
	InvertDVFType::Pointer inverter = InvertDVFType::New();
	if (args_info.neg_flag)
	{
		VectorType zeroVector = VectorType();
		zeroVector.Fill(0.);
		inverter->SetInPlace(true);
		if (args_info.bin_given)
			inverter->SetInput(extracter->GetOutput());
		else
			inverter->SetInput(dvfReader->GetOutput());
		inverter->SetMaximum(zeroVector);
		TRY_AND_EXIT_ON_ITK_EXCEPTION(inverter->Update());
		warper->SetDisplacementField(inverter->GetOutput());
	}
	else
	{
		if (args_info.bin_given)
			warper->SetDisplacementField(extracter->GetOutput());
		else
			warper->SetDisplacementField(dvfReader->GetOutput());
	}
		
	TRY_AND_EXIT_ON_ITK_EXCEPTION(warper->Update());

	timeProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << timeProbe.GetMean() << ' ' << timeProbe.GetUnit() << std::endl;
	timeProbe.Reset();

	// Write results
	ImageWriterType::Pointer writer = ImageWriterType::New();
	writer->SetInput(warper->GetOutput());
	writer->SetFileName(args_info.output_arg);
	
	if (args_info.verbose_flag)
		std::cout << "Writing result... " << std::flush;
	timeProbe.Start();

	TRY_AND_EXIT_ON_ITK_EXCEPTION(writer->Update());

	timeProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << timeProbe.GetMean() << ' ' << timeProbe.GetUnit() << std::endl;
	timeProbe.Reset();

    return EXIT_SUCCESS;
}