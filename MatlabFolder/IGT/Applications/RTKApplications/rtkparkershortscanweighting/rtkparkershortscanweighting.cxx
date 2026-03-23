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

#include "rtkparkershortscanweighting_ggo.h"
#include "rtkGgoFunctions.h"

#include "rtkThreeDCircularProjectionGeometryXMLFile.h"
#include "rtkParkerShortScanImageFilter.h"
#ifdef IGT_USE_CUDA
#  include "rtkCudaParkerShortScanImageFilter.h"
#endif
#include "rtkThreeDCircularProjectionGeometrySorter.h"
#include "rtkBinNumberReader.h"

#include <itkImageFileWriter.h>

int main(int argc, char * argv[])
{
  GGO(rtkparkershortscanweighting, args_info);

  // Read in bin number vector if csv file provided
  typedef rtk::BinNumberReader BinNumberReaderType;
  BinNumberReaderType::Pointer binReader = BinNumberReaderType::New();
  if(args_info.sortfile_given)
  {
	binReader->SetFileName( args_info.sortfile_arg );
	binReader->Parse();
  }

  // Check on hardware parameter
#ifndef IGT_USE_CUDA
  if(!strcmp(args_info.hardware_arg, "cuda") )
    {
    std::cerr << "The program has not been compiled with cuda option" << std::endl;
    return EXIT_FAILURE;
    }
#endif

  typedef float OutputPixelType;
  const unsigned int Dimension = 3;

#ifdef IGT_USE_CUDA
  typedef itk::CudaImage< OutputPixelType, Dimension > OutputImageType;
#else
  typedef itk::Image< OutputPixelType, Dimension > OutputImageType;
#endif

  // Projections reader
  typedef rtk::ProjectionsReader< OutputImageType > ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  if(args_info.sortfile_given)
	rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_rtkparkershortscanweighting>(reader, args_info, binReader->GetOutput() );
  else
	rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_rtkparkershortscanweighting>(reader, args_info);

  // Geometry
  if(args_info.verbose_flag)
    std::cout << "Reading geometry information from "
              << args_info.geometry_arg
              << "..."
              << std::endl;
  rtk::ThreeDCircularProjectionGeometryXMLFileReader::Pointer geometryReader;
  geometryReader = rtk::ThreeDCircularProjectionGeometryXMLFileReader::New();
  geometryReader->SetFilename(args_info.geometry_arg);
  TRY_AND_EXIT_ON_ITK_EXCEPTION( geometryReader->GenerateOutputInformation() )

  // Geometry pointer type
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
  pssf->SetInput( reader->GetOutput() );
  pssf->SetGeometry( geometryPointer );
  pssf->InPlaceOff();

  // Write
  typedef itk::ImageFileWriter<  OutputImageType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( args_info.output_arg );
  writer->SetInput( pssf->GetOutput() );
  writer->SetNumberOfStreamDivisions( args_info.divisions_arg );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( writer->Update() )

  return EXIT_SUCCESS;
}
