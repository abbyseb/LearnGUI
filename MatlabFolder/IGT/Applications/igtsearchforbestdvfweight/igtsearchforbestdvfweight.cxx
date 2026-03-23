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

#include "igtsearchforbestdvfweight_ggo.h"
#include "rtkGgoFunctions.h"
#include "igtConfiguration.h"

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkWarpImageFilter.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkExtractImageFilter.h>
#include <igtLinearCombinationFilter.h>
#include <itkMultiplyImageFilter.h>
#include <itkMattesMutualInformationImageToImageMetric.h>
#include <itkMutualInformationImageToImageMetric.h>
#include <itkMeanSquaresImageToImageMetric.h>
#include <itkNormalizedCorrelationImageToImageMetric.h>
#include <itkGradientDifferenceImageToImageMetric.h>
#include <itkMeanReciprocalSquareDifferenceImageToImageMetric.h>
#include <itkEuler3DTransform.h>
#include <itkTimeProbe.h>
#include <itkImageSpatialObject.h>

/* Author: Andy Shieh 2018 */

int main(int argc, char * argv[])
{
	GGO(igtsearchforbestdvfweight, args_info);
	
	typedef float PixelType;
	const unsigned int Dimension = 3;

	typedef itk::Image<PixelType, Dimension>		ImageType;
	typedef itk::Image<unsigned char, Dimension>	MaskType;
	typedef itk::Vector<PixelType, Dimension>		VectorType;
	typedef itk::Image<VectorType, Dimension>		DVFType;
	typedef itk::ImageFileReader<ImageType>			ImageReaderType;
	typedef itk::ImageFileReader<MaskType>			MaskReaderType;
	typedef itk::ImageFileReader<DVFType>			DVFReaderType;
	typedef itk::ImageFileWriter<ImageType>			ImageWriterType;
	typedef itk::ImageFileWriter<DVFType>			DVFWriterType;
	typedef itk::InvertIntensityImageFilter<DVFType, DVFType> InvertDVFType;
	typedef igt::LinearCombinationFilter<DVFType, DVFType> AddDVFType;
	typedef itk::LinearInterpolateImageFunction<ImageType, double> InterpolateImageType;
	typedef itk::WarpImageFilter<ImageType, ImageType, DVFType> WarpImageType;
	typedef itk::ImageSpatialObject<Dimension, unsigned char>	MaskSpatialType;

	itk::TimeProbe timeProbe;

	// Reading input and reference image
	if (args_info.verbose_flag)
		std::cout << "Reading input and reference images... " << std::flush;
	timeProbe.Start();

	ImageReaderType::Pointer inputReader = ImageReaderType::New();
	inputReader->SetFileName(args_info.input_arg);
	ImageReaderType::Pointer refReader = ImageReaderType::New();
	refReader->SetFileName(args_info.ref_arg);

	MaskReaderType::Pointer inputMaskReader;
	MaskReaderType::Pointer refMaskReader;
	MaskSpatialType::Pointer inputMaskSpatialObject;
	MaskSpatialType::Pointer refMaskSpatialObject;

	TRY_AND_EXIT_ON_ITK_EXCEPTION(inputReader->Update());
	TRY_AND_EXIT_ON_ITK_EXCEPTION(refReader->Update());

	if (args_info.imask_given)
	{
		inputMaskReader = MaskReaderType::New(); 
		inputMaskReader->SetNumberOfThreads(args_info.thread_arg);
		inputMaskReader->SetFileName(args_info.imask_arg);
		TRY_AND_EXIT_ON_ITK_EXCEPTION(inputMaskReader->Update());
		inputMaskSpatialObject = MaskSpatialType::New();
		inputMaskSpatialObject->SetImage(inputMaskReader->GetOutput());
		inputMaskSpatialObject->Update();
	}

	if (args_info.rmask_given)
	{
		refMaskReader = MaskReaderType::New();
		refMaskReader->SetNumberOfThreads(args_info.thread_arg);
		refMaskReader->SetFileName(args_info.rmask_arg);
		TRY_AND_EXIT_ON_ITK_EXCEPTION(refMaskReader->Update());
		refMaskSpatialObject = MaskSpatialType::New();
		refMaskSpatialObject->SetImage(refMaskReader->GetOutput());
		refMaskSpatialObject->Update();
	}

	timeProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << timeProbe.GetMean() << ' ' << timeProbe.GetUnit() << std::endl;
	timeProbe.Reset();

	// Reading DVF
	if (args_info.verbose_flag)
		std::cout << "Reading DVF... " << std::flush;
	timeProbe.Start();

	DVFReaderType::Pointer dvfReader = DVFReaderType::New();
	DVFReaderType::Pointer dvfInitReader = DVFReaderType::New();
	DVFType::Pointer dvfInit;
	dvfReader->SetFileName(args_info.dvf_arg);
	dvfReader->SetNumberOfThreads(args_info.thread_arg);
	TRY_AND_EXIT_ON_ITK_EXCEPTION(dvfReader->Update());
	if (args_info.dinit_given)
	{
		dvfInitReader->SetNumberOfThreads(args_info.thread_arg);
		dvfInitReader->SetFileName(args_info.dinit_arg);
		TRY_AND_EXIT_ON_ITK_EXCEPTION(dvfInitReader->Update());
		dvfInit = dvfInitReader->GetOutput();
		dvfInit->DisconnectPipeline();
	}
	else
	{
		itk::MultiplyImageFilter<DVFType>::Pointer zeroDVFFilter = itk::MultiplyImageFilter<DVFType>::New();
		zeroDVFFilter->SetNumberOfThreads(args_info.thread_arg);
		zeroDVFFilter->SetInPlace(false);
		zeroDVFFilter->SetInput(dvfReader->GetOutput());
		VectorType zeroVector = VectorType();
		zeroVector.Fill(0);
		zeroDVFFilter->SetConstant(zeroVector);
		TRY_AND_EXIT_ON_ITK_EXCEPTION(zeroDVFFilter->Update());
		dvfInit = zeroDVFFilter->GetOutput();
		dvfInit->DisconnectPipeline();
	}
	timeProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << timeProbe.GetMean() << ' ' << timeProbe.GetUnit() << std::endl;
	timeProbe.Reset();

	if (args_info.verbose_flag)
		std::cout << "Initializing search... " << std::flush;
	timeProbe.Start();

	// Set up warping pipeline
	InterpolateImageType::Pointer interpolator = InterpolateImageType::New();
	WarpImageType::Pointer warper = WarpImageType::New();
	warper->SetNumberOfThreads(args_info.thread_arg);
	AddDVFType::Pointer dvfAdd = AddDVFType::New();
	dvfAdd->SetNumberOfThreads(args_info.thread_arg);
	dvfAdd->AddInput(dvfInit.GetPointer(), 1);
	dvfAdd->AddInput(dvfReader->GetOutput(), 1);
	warper->SetInput(inputReader->GetOutput());
	warper->SetInterpolator(interpolator);
	warper->SetOutputOrigin(inputReader->GetOutput()->GetOrigin());
	warper->SetOutputSpacing(inputReader->GetOutput()->GetSpacing());
	warper->SetOutputDirection(inputReader->GetOutput()->GetDirection());
	InvertDVFType::Pointer inverter = InvertDVFType::New();
	if (args_info.neg_flag)
	{
		VectorType zeroVector = VectorType();
		zeroVector.Fill(0.);
		inverter->SetInPlace(true);
		inverter->SetInput(dvfAdd->GetOutput());
		inverter->SetMaximum(zeroVector);
		warper->SetDisplacementField(inverter->GetOutput());
	}
	else
		warper->SetDisplacementField(dvfAdd->GetOutput());

	// Initiate objects for calculating and storing results
	itk::MeanSquaresImageToImageMetric<ImageType, ImageType>::Pointer mse = itk::MeanSquaresImageToImageMetric<ImageType, ImageType>::New();
	itk::MutualInformationImageToImageMetric<ImageType, ImageType>::Pointer mi = itk::MutualInformationImageToImageMetric<ImageType, ImageType>::New();
	itk::MattesMutualInformationImageToImageMetric<ImageType, ImageType>::Pointer mmi = itk::MattesMutualInformationImageToImageMetric<ImageType, ImageType>::New();
	itk::NormalizedCorrelationImageToImageMetric<ImageType, ImageType>::Pointer nc = itk::NormalizedCorrelationImageToImageMetric<ImageType, ImageType>::New();
	itk::MeanReciprocalSquareDifferenceImageToImageMetric<ImageType, ImageType>::Pointer mrsd = itk::MeanReciprocalSquareDifferenceImageToImageMetric<ImageType, ImageType>::New();
	itk::GradientDifferenceImageToImageMetric<ImageType, ImageType>::Pointer gd = itk::GradientDifferenceImageToImageMetric<ImageType, ImageType>::New();
	itk::Euler3DTransform<double>::Pointer dummyTransform = itk::Euler3DTransform<double>::New();
	itk::Euler3DTransform<double>::ParametersType dummyTParameters(dummyTransform->GetNumberOfParameters());
	dummyTParameters.Fill(0);
	dummyTransform->SetParameters(dummyTParameters);
	itk::LinearInterpolateImageFunction<ImageType, double>::Pointer dummyInterpolator = itk::LinearInterpolateImageFunction<ImageType, double>::New();
	
	// Workflow to set up metric
#define SETUP_METRIC(f) \
	f->SetFixedImage(refReader->GetOutput()); \
	f->SetMovingImage(warper->GetOutput()); \
	f->SetInterpolator(dummyInterpolator); \
	f->SetTransform(dummyTransform); \
	f->SetFixedImageRegion(refReader->GetOutput()->GetLargestPossibleRegion()); \
	f->SetNumberOfThreads(args_info.thread_arg); \
	f->Initialize();
	
	if (args_info.metric_arg == 0) // MSE
	{
		SETUP_METRIC(mse);
		if (args_info.imask_given)
			mse->SetMovingImageMask(inputMaskSpatialObject);
		if (args_info.rmask_given)
			mse->SetFixedImageMask(refMaskSpatialObject);
	}
	else if (args_info.metric_arg == 1) // MI
	{
		SETUP_METRIC(mi);
		if (args_info.imask_given)
			mi->SetMovingImageMask(inputMaskSpatialObject);
		if (args_info.rmask_given)
			mi->SetFixedImageMask(refMaskSpatialObject);
	}
	else if (args_info.metric_arg == 2) // MMI
	{
		SETUP_METRIC(mmi);
		if (args_info.imask_given)
			mmi->SetMovingImageMask(inputMaskSpatialObject);
		if (args_info.rmask_given)
			mmi->SetFixedImageMask(refMaskSpatialObject);
	}
	else if (args_info.metric_arg == 3) // NC
	{
		SETUP_METRIC(nc);
		if (args_info.imask_given)
			nc->SetMovingImageMask(inputMaskSpatialObject);
		if (args_info.rmask_given)
			nc->SetFixedImageMask(refMaskSpatialObject);
	}
	else if (args_info.metric_arg == 4) // GD
	{
		SETUP_METRIC(gd);
		if (args_info.imask_given)
			gd->SetMovingImageMask(inputMaskSpatialObject);
		if (args_info.rmask_given)
			gd->SetFixedImageMask(refMaskSpatialObject);
	}
	else if (args_info.metric_arg == 5) // MRSD
	{
		SETUP_METRIC(mrsd);
		if (args_info.imask_given)
			mrsd->SetMovingImageMask(inputMaskSpatialObject);
		if (args_info.rmask_given)
			mrsd->SetFixedImageMask(refMaskSpatialObject);
	}
	else
		std::cerr << "ERROR: Unsupported metric type" << std::endl;

	std::vector<double> objValues, wValues;

	std::fstream resultWriter;
	resultWriter.open(args_info.result_arg, std::fstream::out);

	resultWriter << "Weights,ObjectiveValue,MetricType" << std::endl;

	timeProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << timeProbe.GetMean() << ' ' << timeProbe.GetUnit() << std::endl;
	timeProbe.Reset();


	// Search starts
	if (args_info.verbose_flag)
		std::cout << "Start searching for the optimal weight...... " << std::flush;
	timeProbe.Start();
	for (float w = args_info.low_arg; w <= args_info.up_arg; w += args_info.dw_arg)
	{
		wValues.push_back(w);
		dvfAdd->SetMultiplier(1, w);
		dvfAdd->Modified();
		TRY_AND_EXIT_ON_ITK_EXCEPTION(dvfAdd->Update());
		warper->Modified();
		TRY_AND_EXIT_ON_ITK_EXCEPTION(warper->Update());

		// Set up metric. "MSE","MI","MMI","NC","GD","MRSD"
		if (args_info.metric_arg == 0) // MSE
		{
			objValues.push_back(mse->GetValue(dummyTParameters));
			resultWriter << w << ",";
			resultWriter << objValues[objValues.size() - 1] << ",";
			resultWriter << "MSE";
			if (w <= args_info.up_arg - args_info.dw_arg)
				resultWriter << std::endl;
		}
		else if (args_info.metric_arg == 1) // MI
		{
			objValues.push_back(-1. * mi->GetValue(dummyTParameters));
			resultWriter << w << ",";
			resultWriter << objValues[objValues.size() - 1] << ",";
			resultWriter << "MI";
			if (w <= args_info.up_arg - args_info.dw_arg)
				resultWriter << std::endl;
		}
		else if (args_info.metric_arg == 2) // MMI
		{
			objValues.push_back(-1. * mmi->GetValue(dummyTParameters));
			resultWriter << w << ",";
			resultWriter << objValues[objValues.size() - 1] << ",";
			resultWriter << "MMI";
			if (w <= args_info.up_arg - args_info.dw_arg)
				resultWriter << std::endl;
		}
		else if (args_info.metric_arg == 3) // NC
		{
			objValues.push_back(-1. * nc->GetValue(dummyTParameters));
			resultWriter << w << ",";
			resultWriter << objValues[objValues.size() - 1] << ",";
			resultWriter << "NC";
			if (w <= args_info.up_arg - args_info.dw_arg)
				resultWriter << std::endl;
		}
		else if (args_info.metric_arg == 4) // GD
		{
			objValues.push_back(gd->GetValue(dummyTParameters));
			resultWriter << w << ",";
			resultWriter << objValues[objValues.size() - 1] << ",";
			resultWriter << "GD";
			if (w <= args_info.up_arg - args_info.dw_arg)
				resultWriter << std::endl;
		}
		else if (args_info.metric_arg == 5) // MRSD
		{
			objValues.push_back(mrsd->GetValue(dummyTParameters));
			resultWriter << w << ",";
			resultWriter << objValues[objValues.size() - 1] << ",";
			resultWriter << "MRSD";
			if (w <= args_info.up_arg - args_info.dw_arg)
				resultWriter << std::endl;
		}
		else
			std::cerr << "ERROR: Unsupported metric type" << std::endl;
	}
	resultWriter.close();
	timeProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << timeProbe.GetMean() << ' ' << timeProbe.GetUnit() << std::endl;
	timeProbe.Reset();

	// Find best w
	double maxVal = std::numeric_limits<double>::min();
	double minVal = std::numeric_limits<double>::max();
	int bestPos = -1;
	if (args_info.metric_arg == 0) // MSE
	{
		for (int k = 0; k != objValues.size(); ++k)
		{
			if (objValues[k] < minVal)
			{
				bestPos = k;
				minVal = objValues[k];
			}
		}
	}
	else if (args_info.metric_arg == 1 || args_info.metric_arg == 2 || args_info.metric_arg == 3 || args_info.metric_arg == 4 || args_info.metric_arg == 5) // MI, MMI, NC, GD, MRSE
	{
		for (int k = 0; k != objValues.size(); ++k)
		{
			if (objValues[k] > maxVal)
			{
				bestPos = k;
				maxVal = objValues[k];
			}
		}
	}
	else
		std::cerr << "ERROR: Unsupported metric type" << std::endl;
	double bestWeight = wValues[bestPos];
	dvfAdd->SetMultiplier(1, bestWeight);
	dvfAdd->Modified();
	TRY_AND_EXIT_ON_ITK_EXCEPTION(dvfAdd->Update());
	warper->Modified();
	TRY_AND_EXIT_ON_ITK_EXCEPTION(warper->Update());

	// Write results
	ImageWriterType::Pointer imgWriter = ImageWriterType::New();
	imgWriter->SetNumberOfThreads(args_info.thread_arg);
	DVFWriterType::Pointer dvfWriter = DVFWriterType::New();
	dvfWriter->SetNumberOfThreads(args_info.thread_arg);
	imgWriter->SetInput(warper->GetOutput());
	imgWriter->SetFileName(args_info.output_arg);
	dvfWriter->SetInput(dvfAdd->GetOutput());
	dvfWriter->SetFileName(args_info.outdvf_arg);
	
	if (args_info.verbose_flag)
		std::cout << "Writing result... " << std::flush;
	timeProbe.Start();

	TRY_AND_EXIT_ON_ITK_EXCEPTION(imgWriter->Update());
	TRY_AND_EXIT_ON_ITK_EXCEPTION(dvfWriter->Update());

	timeProbe.Stop();
	if (args_info.verbose_flag)
		std::cout << "It took " << timeProbe.GetMean() << ' ' << timeProbe.GetUnit() << std::endl;
	timeProbe.Reset();
	
    return EXIT_SUCCESS;
}