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

#include "rtkmkb_ggo.h"
#include "rtkGgoFunctions.h"
#include "igtConfiguration.h"

#include "rtkThreeDCircularProjectionGeometryXMLFile.h"
#include "rtkDisplacedDetectorImageFilter.h"
#include "rtkParkerShortScanImageFilter.h"
#include "rtkFDKConeBeamReconstructionFilter.h"
#include "rtkMKBConeBeamReconstructionFilter.h"
#ifdef IGT_USE_CUDA
#  include "rtkCudaDisplacedDetectorImageFilter.h"
#  include "rtkCudaParkerShortScanImageFilter.h"
#  include "rtkCudaFDKConeBeamReconstructionFilter.h"
#endif
#include "rtkThreeDCircularProjectionGeometrySorter.h"
#include "rtkBinNumberReader.h"

#include <itkStreamingImageFilter.h>
#include <itkImageFileWriter.h>

int main(int argc, char * argv[])
{
  GGO(rtkmkb, args_info);

  // Check on hardware parameter
#ifndef IGT_USE_CUDA
  if(!strcmp(args_info.hardware_arg, "cuda") | args_info.fp_arg == 2)
    {
    std::cerr << "The program has not been compiled with cuda option" << std::endl;
    return EXIT_FAILURE;
    }
#endif

  typedef float OutputPixelType;
  const unsigned int Dimension = 3;

  typedef itk::Image< OutputPixelType, Dimension >     CPUOutputImageType;
#ifdef IGT_USE_CUDA
  typedef itk::CudaImage< OutputPixelType, Dimension > OutputImageType;
#else
  typedef CPUOutputImageType                           OutputImageType;
#endif

  // Projection reader
  typedef rtk::ProjectionsReader< OutputImageType > ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  itk::TimeProbe readerProbe;
  
  // Writer
  typedef itk::ImageFileWriter<CPUOutputImageType> WriterType;
  WriterType::Pointer writer = WriterType::New();
  itk::TimeProbe writerProbe;

  // Read in bin number vector if csv file provided
  typedef rtk::BinNumberReader BinNumberReaderType;
  BinNumberReaderType::Pointer binReader = BinNumberReaderType::New();
  if(args_info.sortfile_given)
  {
    if(args_info.verbose_flag)
      std::cout << "Reading projection sorting information from "
                << args_info.sortfile_arg
                << "..."
                << std::endl;
	binReader->SetFileName( args_info.sortfile_arg );
	binReader->Parse();
  }

  // Geometry
  if(args_info.verbose_flag)
    std::cout << "Reading geometry information from "
              << args_info.geometry_arg
              << "..."
              << std::endl;
  rtk::ThreeDCircularProjectionGeometryXMLFileReader::Pointer geometryReader;
  geometryReader = rtk::ThreeDCircularProjectionGeometryXMLFileReader::New();
  geometryReader->SetFilename(args_info.geometry_arg);
  TRY_AND_EXIT_ON_ITK_EXCEPTION( geometryReader->GenerateOutputInformation() );

  // Create input: either an existing volume read from a file or reconstruct FDK_3D by using all projections
  itk::ImageSource< OutputImageType >::Pointer inputFilter;
  if(args_info.input_given)
    {
    // Read an existing image to initialize the volume
    typedef itk::ImageFileReader<  OutputImageType > InputReaderType;
    InputReaderType::Pointer inputReader = InputReaderType::New();
    inputReader->SetFileName( args_info.input_arg );
    inputFilter = inputReader;
    }
  else
    {
    // Reconstruct FDK_3D by using all projections
    // Projection reading
	rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_rtkmkb>(reader, args_info);
	if(!args_info.lowmem_flag)
		{
		if(args_info.verbose_flag)
		std::cout << "Reading projections for reconstruction of initial 3D FDK image... " << std::flush;
		readerProbe.Start();
		TRY_AND_EXIT_ON_ITK_EXCEPTION( reader->Update() )
		readerProbe.Stop();
		if(args_info.verbose_flag)
			std::cout << "It took " << readerProbe.GetMean() << ' ' << readerProbe.GetUnit() << std::endl;
		}

	// Displaced detector weighting
	typedef rtk::DisplacedDetectorImageFilter< OutputImageType > DDFCPUType;
	#ifdef IGT_USE_CUDA
		typedef rtk::CudaDisplacedDetectorImageFilter DDFType;
	#else
		typedef rtk::DisplacedDetectorImageFilter< OutputImageType > DDFType;
	#endif
	DDFCPUType::Pointer ddf;
	if(!strcmp(args_info.hardware_arg, "cuda") )
		ddf = DDFType::New();
	else
		ddf = DDFCPUType::New();
	ddf->SetInput( reader->GetOutput() );
	ddf->SetGeometry( geometryReader->GetOutputObject() );

	// Short scan image filter
	typedef rtk::ParkerShortScanImageFilter< OutputImageType > PSSFCPUType;
	#ifdef IGT_USE_CUDA
		typedef rtk::CudaParkerShortScanImageFilter PSSFType;
	#else
		typedef rtk::ParkerShortScanImageFilter< OutputImageType > PSSFType;
	#endif
	 PSSFCPUType::Pointer pssf;
	if(!strcmp(args_info.hardware_arg, "cuda") )
		pssf = PSSFType::New();
	else
		pssf = PSSFCPUType::New();
	pssf->SetInput( ddf->GetOutput() );
	pssf->SetGeometry( geometryReader->GetOutputObject() );
	pssf->InPlaceOff();

	// Create reconstructed image
	typedef rtk::ConstantImageSource< OutputImageType > ConstantImageSourceType;
	ConstantImageSourceType::Pointer constantImageSource = ConstantImageSourceType::New();
	rtk::SetConstantImageSourceFromGgo<ConstantImageSourceType, args_info_rtkmkb>(constantImageSource, args_info);

	// This macro sets options for fdk filter which I can not see how to do better
	// because TFFTPrecision is not the same, e.g. for CPU and CUDA (SR)
	#define SET_FELDKAMP_OPTIONS(f) \
		f->SetInput( 0, constantImageSource->GetOutput() ); \
		f->SetInput( 1, pssf->GetOutput() ); \
		f->SetGeometry( geometryReader->GetOutputObject() ); \
		f->GetRampFilter()->SetTruncationCorrection(args_info.pad_arg); \
		f->GetRampFilter()->SetHannCutFrequency(args_info.hann_arg); \
		f->GetRampFilter()->SetHannCutFrequencyY(args_info.hannY_arg); \
		f->SetProjectionSubsetSize(args_info.subsetsize_arg)

	// FDK reconstruction filtering
	typedef rtk::FDKConeBeamReconstructionFilter< OutputImageType > FDKCPUType;
	FDKCPUType::Pointer feldkamp;
	#ifdef IGT_USE_CUDA
		typedef rtk::CudaFDKConeBeamReconstructionFilter FDKCUDAType;
		FDKCUDAType::Pointer feldkampCUDA;
	#endif
	itk::Image< OutputPixelType, Dimension > *pfeldkamp = NULL;
	if(!strcmp(args_info.hardware_arg, "cpu") )
		{
		feldkamp = FDKCPUType::New();
		SET_FELDKAMP_OPTIONS( feldkamp );
		pfeldkamp = feldkamp->GetOutput();
		}
	#ifdef IGT_USE_CUDA
	else if(!strcmp(args_info.hardware_arg, "cuda") )
		{
		feldkampCUDA = FDKCUDAType::New();
		SET_FELDKAMP_OPTIONS( feldkampCUDA );
		pfeldkamp = feldkampCUDA->GetOutput();
		}
	#endif

    // Streaming depending on streaming capability of writer
    typedef itk::StreamingImageFilter<CPUOutputImageType, CPUOutputImageType> StreamerType;
    StreamerType::Pointer streamerBP = StreamerType::New();
	streamerBP->SetInput( pfeldkamp );
	streamerBP->SetNumberOfStreamDivisions( args_info.divisions_arg );

	writer->SetFileName( args_info.initial_arg );
	writer->SetInput( streamerBP->GetOutput() );

	if(args_info.verbose_flag)
		std::cout << "Reconstructing and writing the initial 3D FDK image... " << std::flush;

	writerProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( writer->Update() );
	writerProbe.Stop();

	if(args_info.verbose_flag)
		{
		std::cout << "It took " << writerProbe.GetMean() << ' ' << writerProbe.GetUnit() << std::endl;
		}

	// Set to inputFilter
    // Read an existing image to initialize the volume
    typedef itk::ImageFileReader<  OutputImageType > InputReaderType;
    InputReaderType::Pointer inputReader = InputReaderType::New();
    inputReader->SetFileName( args_info.initial_arg );
    inputFilter = inputReader;
    }

  // Select a subset of projection geometries
  rtk::ThreeDCircularProjectionGeometrySorter::GeometryPointer geometryPointer;
  if(args_info.sortfile_given)
  {
	  rtk::ThreeDCircularProjectionGeometrySorter::Pointer geometrySorter = rtk::ThreeDCircularProjectionGeometrySorter::New();
	  geometrySorter->SetInputGeometry( geometryReader->GetOutputObject() );
	  geometrySorter->SetBinNumberVector( binReader->GetOutput() );
	  geometrySorter->Sort();
	  geometryPointer = geometrySorter->GetOutput()[args_info.binnumber_arg - 1];
  }
  else
  {
	  geometryPointer = geometryReader->GetOutputObject();
  }

// Read projections
  if(args_info.sortfile_given)
	rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_rtkmkb>(reader, args_info, binReader->GetOutput() );
  else
	rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_rtkmkb>(reader, args_info);
  reader->UpdateLargestPossibleRegion();
  if(!args_info.lowmem_flag)
	{
		if(args_info.verbose_flag)
		std::cout << "Reading projections for MKB reconstruction... " << std::flush;
		readerProbe.Reset();
		readerProbe.Start();
		TRY_AND_EXIT_ON_ITK_EXCEPTION( reader->Update() )
		readerProbe.Stop();
		if(args_info.verbose_flag)
		std::cout << "It took " << readerProbe.GetMean() << ' ' << readerProbe.GetUnit() << std::endl;
    }

  typedef rtk::MKBConeBeamReconstructionFilter< OutputImageType, OutputImageType > MKBReconType;
  MKBReconType::Pointer mkb = MKBReconType::New();
  mkb->SetGeometry( geometryPointer );
  mkb->SetForwardProjectionFilter(args_info.fp_arg);
  mkb->SetInput(0, inputFilter->GetOutput() );
  mkb->SetInput(1, reader->GetOutput());
  if (args_info.threshold_flag)
	mkb->SetPositivityThresholdOn();
  else
	mkb->SetPositivityThresholdOff();
  mkb->SetScalingFactor( args_info.step_arg );

  // Instantiate displaced detector filter, parker weighting filter
  if(!strcmp(args_info.hardware_arg, "cpu") )
  {
	  mkb->InstantiateDisplacedDetectorFilter();
	  mkb->InstantiateParkerWeightingFilter();
  }
#ifdef IGT_USE_CUDA
  else if(!strcmp(args_info.hardware_arg, "cuda") )
  {
	  mkb->InstantiateDisplacedDetectorCUDAFilter();
	  mkb->InstantiateParkerWeightingCUDAFilter();
  }
#endif

  // Set FDK filter
  mkb->SetFDKReconstructionFilter( args_info.hardware_arg );
  
  #define SET_MKB_FDK_OPTIONS(f) \
		f->GetRampFilter()->SetTruncationCorrection(args_info.pad_arg); \
		f->GetRampFilter()->SetHannCutFrequency(args_info.hann_arg); \
		f->GetRampFilter()->SetHannCutFrequencyY(args_info.hannY_arg); \
		f->SetProjectionSubsetSize(args_info.subsetsize_arg)

  if(!strcmp(args_info.hardware_arg, "cpu") )
	{SET_MKB_FDK_OPTIONS( mkb->GetFDKReconstructionCPUFilter() );}
#ifdef IGT_USE_CUDA
  else if(!strcmp(args_info.hardware_arg, "cuda") )
	{SET_MKB_FDK_OPTIONS( mkb->GetFDKReconstructionCUDAFilter() );}
#endif

  // Write
  writer->SetFileName( args_info.output_arg );
  writer->SetInput( mkb->GetOutput() );

  if(args_info.verbose_flag)
    std::cout << "MKB Reconstructing and writing... " << std::flush;
  
  writerProbe.Reset();
  writerProbe.Start();
  TRY_AND_EXIT_ON_ITK_EXCEPTION( writer->Update() );
  writerProbe.Stop();

  if(args_info.verbose_flag)
    {
		std::cout << "It took " << writerProbe.GetMean() << ' ' << writerProbe.GetUnit() << std::endl;
		mkb->PrintTiming(std::cout);
    }

  return EXIT_SUCCESS;
}
