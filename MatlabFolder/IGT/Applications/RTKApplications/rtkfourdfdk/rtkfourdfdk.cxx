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

#include "rtkfourdfdk_ggo.h"
#include "rtkGgoFunctions.h"
#include "igtConfiguration.h"

#include "rtkThreeDCircularProjectionGeometryXMLFile.h"
#include "rtkDisplacedDetectorImageFilter.h"
#include "rtkParkerShortScanImageFilter.h"
#include "rtkFDKConeBeamReconstructionFilter.h"
#ifdef IGT_USE_CUDA
#  include "rtkCudaDisplacedDetectorImageFilter.h"
#  include "rtkCudaParkerShortScanImageFilter.h"
#  include "rtkCudaFDKConeBeamReconstructionFilter.h"
#endif
#ifdef IGT_USE_OPENCL
#  include "rtkOpenCLFDKConeBeamReconstructionFilter.h"
#endif
#include "rtkThreeDCircularProjectionGeometrySorter.h"
#include "rtkBinNumberReader.h"

#include <itkStreamingImageFilter.h>
#include <itkImageFileWriter.h>

#include <iostream>
#include <iomanip>
#include <sstream>

int main(int argc, char * argv[])
{
  GGO(rtkfourdfdk, args_info);
  
  // Check on hardware parameter
#ifndef IGT_USE_CUDA
  if(!strcmp(args_info.hardware_arg, "cuda") )
    {
    std::cerr << "The program has not been compiled with cuda option" << std::endl;
    return EXIT_FAILURE;
    }
#endif
#ifndef IGT_USE_OPENCL
   if(!strcmp(args_info.hardware_arg, "opencl") )
     {
     std::cerr << "The program has not been compiled with opencl option" << std::endl;
     return EXIT_FAILURE;
    }
#endif

  typedef float OutputPixelType;
  const unsigned int Dimension = 3;
  
  // Number of digits for bin numbers in output file names
  const unsigned int n_Digits = 5;
  
  // Record total time
  double time_total = 0;

  typedef itk::Image< OutputPixelType, Dimension >     CPUOutputImageType;
#ifdef IGT_USE_CUDA
  typedef itk::CudaImage< OutputPixelType, Dimension > OutputImageType;
#else
  typedef CPUOutputImageType                           OutputImageType;
#endif

  // Convert output prefix from char* to string for easier processing
  std::string outPrefix( args_info.output_arg );

  // Projections reader
  typedef rtk::ProjectionsReader< OutputImageType > ReaderType;
  
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
    
  // Generate file names
  itk::RegularExpressionSeriesFileNames::Pointer names = itk::RegularExpressionSeriesFileNames::New();
  names->SetDirectory(args_info.path_arg);
  names->SetNumericSort(args_info.nsort_flag);
  names->SetRegularExpression(args_info.regexp_arg);
  names->SetSubMatch(args_info.submatch_arg);
  if(args_info.verbose_flag)
    std::cout << "Regular expression matches "
              << names->GetFileNames().size()
              << " projection file(s) in total..."
              << std::endl;
			  
  // Read in bin number vector
  typedef rtk::BinNumberReader BinNumberReaderType;
  BinNumberReaderType::Pointer binReader = BinNumberReaderType::New();
  if(args_info.sortfile_given)
  {
	binReader->SetFileName( args_info.sortfile_arg );
	binReader->Parse();
  }
			  
  // Sort file names
  rtk::VectorSorter<std::string>::Pointer sorter = rtk::VectorSorter<std::string>::New();
  sorter->SetInputVector( names->GetFileNames() );
  sorter->SetBinNumberVector( binReader->GetOutput() );
  sorter->Sort();
  unsigned int NumberOfBins = sorter->GetMaximumBinNumber();
  if(args_info.verbose_flag)
  std::cout << "Reconstructing "
            << NumberOfBins
            << " bins in total according to the projection sorting file "
			<< args_info.sortfile_arg
			<< "..."
            << std::endl;
  
  // Sort geometries
  rtk::ThreeDCircularProjectionGeometrySorter::Pointer geometrySorter = rtk::ThreeDCircularProjectionGeometrySorter::New();
  geometrySorter->SetInputGeometry( geometryReader->GetOutputObject() );
  geometrySorter->SetBinNumberVector( binReader->GetOutput() );
  geometrySorter->Sort();
  
  // Displaced detector weighting
  typedef rtk::DisplacedDetectorImageFilter< OutputImageType > DDFCPUType;
#ifdef IGT_USE_CUDA
  typedef rtk::CudaDisplacedDetectorImageFilter DDFType;
#else
  typedef rtk::DisplacedDetectorImageFilter< OutputImageType > DDFType;
#endif
	
  // Short scan image filter
  typedef rtk::ParkerShortScanImageFilter< OutputImageType > PSSFCPUType;
#ifdef IGT_USE_CUDA
  typedef rtk::CudaParkerShortScanImageFilter PSSFType;
#else
  typedef rtk::ParkerShortScanImageFilter< OutputImageType > PSSFType;
#endif
  
  // Create reconstructed image
  typedef rtk::ConstantImageSource< OutputImageType > ConstantImageSourceType;

  // Streaming depending on streaming capability of writer
  typedef itk::StreamingImageFilter<CPUOutputImageType, CPUOutputImageType> StreamerType;
  
  // Write
  typedef itk::ImageFileWriter<CPUOutputImageType> WriterType;
  
  if(args_info.verbose_flag)
	std::cout << "====================" << std::endl;
  for(unsigned int n = 0; n < NumberOfBins; ++n)
  {
  	if(args_info.verbose_flag)
		std::cout << "Bin #" << n+1 << std::endl;
    // Skip if a bin is allocated zero projection
	if( sorter->GetOutput()[n].size()==0 )
	{
		std::cout << "	No projections were allocated to bin #" << n+1 << std::endl;
		continue;
	}
	
    // Time probes
    itk::TimeProbe readerProbe;
    itk::TimeProbe writerProbe;
		
	// Read projections
	ReaderType::Pointer reader = ReaderType::New();
	reader->SetFileNames( sorter->GetOutput()[n] );
	// TRY_AND_EXIT_ON_ITK_EXCEPTION( reader->UpdateOutputInformation() );
	if(!args_info.lowmem_flag)
	  {
		if(args_info.verbose_flag)
			std::cout << "	Reading " << sorter->GetOutput()[n].size() << " projections... " << std::flush;
		readerProbe.Reset();
		readerProbe.Start();
		// TRY_AND_EXIT_ON_ITK_EXCEPTION( reader->UpdateLargestPossibleRegion() )
		TRY_AND_EXIT_ON_ITK_EXCEPTION( reader->Update() )
		readerProbe.Stop();
		time_total += readerProbe.GetMean();
		if(args_info.verbose_flag)
			std::cout << "	It took " << readerProbe.GetMean() << ' ' << readerProbe.GetUnit() << std::endl;
	  }
	  
    DDFCPUType::Pointer ddf;
    if(!strcmp(args_info.hardware_arg, "cuda")) 
      {
  	  if(args_info.dd_given)
	    {
	    if(!strcmp(args_info.dd_arg, "cuda"))
		  ddf = DDFType::New();
	    else
	      ddf = DDFCPUType::New();
	    }
	  else
	    ddf = DDFType::New();
	  }
    else
      {
	  if(args_info.dd_given)
	    {
	    if(!strcmp(args_info.dd_arg, "cuda"))
		  ddf = DDFType::New();
	    else
	      ddf = DDFCPUType::New();
	    }
	  else
	    ddf = DDFCPUType::New();
	  }
	  
    PSSFCPUType::Pointer pssf;
    if(!strcmp(args_info.hardware_arg, "cuda") )
      pssf = PSSFType::New();
    else
      pssf = PSSFCPUType::New();
    pssf->SetInput( ddf->GetOutput() );
    pssf->InPlaceOff();
	  
    ConstantImageSourceType::Pointer constantImageSource = ConstantImageSourceType::New();
    rtk::SetConstantImageSourceFromGgo<ConstantImageSourceType, args_info_rtkfourdfdk>(constantImageSource, args_info);
  
    // This macro sets options for fdk filter which I can not see how to do better
    // because TFFTPrecision is not the same, e.g. for CPU and CUDA (SR)
    #define SET_FELDKAMP_OPTIONS(f) \
    f->SetInput( 0, constantImageSource->GetOutput() ); \
    f->SetInput( 1, pssf->GetOutput() ); \
    f->GetRampFilter()->SetTruncationCorrection(args_info.pad_arg); \
    f->GetRampFilter()->SetHannCutFrequency(args_info.hann_arg); \
    f->GetRampFilter()->SetHannCutFrequencyY(args_info.hannY_arg); \
    f->SetProjectionSubsetSize(args_info.subsetsize_arg)
	
    // FDK reconstruction filtering
    typedef rtk::FDKConeBeamReconstructionFilter< OutputImageType > FDKCPUType;
    FDKCPUType::Pointer feldkamp;
  #ifdef IGT_USE_OPENCL
    typedef rtk::OpenCLFDKConeBeamReconstructionFilter FDKOPENCLType;
    FDKOPENCLType::Pointer feldkampOCL;
  #endif
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
  #ifdef IGT_USE_OPENCL
    else if(!strcmp(args_info.hardware_arg, "opencl") )
      {
      feldkampOCL = FDKOPENCLType::New();
      SET_FELDKAMP_OPTIONS( feldkampOCL );
      pfeldkamp = feldkampOCL->GetOutput();
      }
  #endif
	  
	// Set filter inputs and geometry
	ddf->SetInput( reader->GetOutput() );
    ddf->SetGeometry( geometrySorter->GetOutput()[n] );
    pssf->SetGeometry( geometrySorter->GetOutput()[n] );
    if(!strcmp(args_info.hardware_arg, "cpu") )
    {
		feldkamp->SetGeometry( geometrySorter->GetOutput()[n] );
    }
	#ifdef IGT_USE_CUDA
	else if(!strcmp(args_info.hardware_arg, "cuda") )
    {
		feldkampCUDA->SetGeometry( geometrySorter->GetOutput()[n] );
    }
	#endif
	#ifdef IGT_USE_OPENCL
	else if(!strcmp(args_info.hardware_arg, "opencl") )
    {
		feldkampOCL->SetGeometry( geometrySorter->GetOutput()[n] );
    }
	#endif
 	
    StreamerType::Pointer streamerBP = StreamerType::New();
    streamerBP->SetInput( pfeldkamp );
    streamerBP->SetNumberOfStreamDivisions( args_info.divisions_arg );
    WriterType::Pointer writer = WriterType::New();
    writer->SetInput( streamerBP->GetOutput() );
	
	// Set file name for this bin
    std::ostringstream os;
	os << std::setfill('0') << std::setw(n_Digits) << (n+1);
	std::string fileNumber = os.str();
	std::string fileName = outPrefix + fileNumber + ".mha";
	writer->SetFileName( fileName );
	
	// Write
	if(args_info.verbose_flag)
		std::cout << "	Reconstructing and writing... " << std::flush;
	writerProbe.Reset();
	writerProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( writer->Update() );
	writerProbe.Stop();
	time_total += writerProbe.GetMean();
  
	if(args_info.verbose_flag)
    {
		std::cout << "	It took " << writerProbe.GetMean() << ' ' << readerProbe.GetUnit() << std::endl;
		if(!strcmp(args_info.hardware_arg, "cpu") )
			feldkamp->PrintTiming(std::cout);
		#ifdef IGT_USE_CUDA
		else if(!strcmp(args_info.hardware_arg, "cuda") )
			feldkampCUDA->PrintTiming(std::cout);
		#endif
		#ifdef IGT_USE_OPENCL
		else if(!strcmp(args_info.hardware_arg, "opencl") )
			feldkampOCL->PrintTiming(std::cout);
		#endif
		std::cout << "====================" << std::endl;
    }
	if(!strcmp(args_info.hardware_arg, "cpu") )
		feldkamp->ResetTimeProbes();
	#ifdef IGT_USE_CUDA
	else if(!strcmp(args_info.hardware_arg, "cuda") )
		feldkampCUDA->ResetTimeProbes();
	#endif
	#ifdef IGT_USE_OPENCL
	else if(!strcmp(args_info.hardware_arg, "opencl") )
		feldkampOCL->ResetTimeProbes();
	#endif
  }
  
  itk::TimeProbe tmpProbe;
  if(args_info.verbose_flag)
	std::cout << "4D FDK reconstruction completed using " 
			  << time_total << ' ' << tmpProbe.GetUnit() << std::endl;

  return EXIT_SUCCESS;
}
