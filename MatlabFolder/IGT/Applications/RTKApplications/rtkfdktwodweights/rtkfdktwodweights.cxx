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

#include "rtkfdktwodweights_ggo.h"
#include "rtkGgoFunctions.h"

#include "rtkThreeDCircularProjectionGeometryXMLFile.h"
#include "rtkFDKWeightProjectionFilter.h"
#include "rtkThreeDCircularProjectionGeometrySorter.h"
#include "rtkBinNumberReader.h"

#include <itkImageFileWriter.h>

int main(int argc, char * argv[])
{
  GGO(rtkfdktwodweights, args_info);

  typedef float OutputPixelType;
  const unsigned int Dimension = 3;

  typedef itk::Image< OutputPixelType, Dimension > OutputImageType;

  // Read in bin number vector if csv file provided
  typedef rtk::BinNumberReader BinNumberReaderType;
  BinNumberReaderType::Pointer binReader = BinNumberReaderType::New();
  if(args_info.sortfile_given)
  {
	binReader->SetFileName( args_info.sortfile_arg );
	binReader->Parse();
  }

  // Projections reader
  typedef rtk::ProjectionsReader< OutputImageType > ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  if(args_info.sortfile_given)
	rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_rtkfdktwodweights>(reader, args_info, binReader->GetOutput() );
  else
	rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_rtkfdktwodweights>(reader, args_info);

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
  if(args_info.dts_flag)
    geometryReader->GetOutputObject()->SetDTSOn();

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

  // Weights filter
  typedef rtk::FDKWeightProjectionFilter< OutputImageType > WeightType;
  WeightType::Pointer wf = WeightType::New();
  wf->SetUseConstantAngularGap(args_info.constang_flag);
  wf->SetInput( reader->GetOutput() );
  wf->SetGeometry( geometryPointer );
  wf->SetNumberOfThreads(1);
  wf->InPlaceOff();

  // Write
  typedef itk::ImageFileWriter<  OutputImageType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( args_info.output_arg );
  writer->SetInput( wf->GetOutput() );
  writer->SetNumberOfStreamDivisions( args_info.divisions_arg );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( writer->Update() )

  return EXIT_SUCCESS;
}
