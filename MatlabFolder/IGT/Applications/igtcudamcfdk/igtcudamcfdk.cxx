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

#include "igtcudamcfdk_ggo.h"
#include "rtkGgoFunctions.h"
#include "igtConfiguration.h"
#include "itkTimeProbe.h"

#include "rtkThreeDCircularProjectionGeometryXMLFile.h"
#include "rtkDisplacedDetectorImageFilter.h"
#include "rtkCudaDisplacedDetectorImageFilter.h"
#include "rtkCudaParkerShortScanImageFilter.h"
#include "rtkCudaFFTRampImageFilter.h"
#include "rtkCudaFDKWeightProjectionFilter.h"
#include "rtkThreeDCircularProjectionGeometrySorter.h"
#include "rtkBinNumberReader.h"
#include "rtkCudaFDKWeightProjectionFilter.h"
#include "itkCudaImage.h"
#include "rtkCudaCyclicDeformationImageFilter.h"
//#include "rtkCudaWarpBackProjectionImageFilter.h"
#include "rtkFDKWarpBackProjectionImageFilter.h"

#include <itkExtractImageFilter.h>
#include <itkImageFileWriter.h>
#include <itkTileImageFilter.h>

int main(int argc, char * argv[])
{
  GGO(igtcudamcfdk, args_info);

  typedef float OutputPixelType;
  const unsigned int Dimension = 3;

  typedef itk::CudaImage< OutputPixelType, Dimension > OutputImageType;

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
	  rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_igtcudamcfdk>(reader, args_info, binReader->GetOutput());
  else
	  rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_igtcudamcfdk>(reader, args_info);

  itk::TimeProbe readerProbe;
  if (args_info.verbose_flag)
	  std::cout << "Reading... " << std::flush;
  readerProbe.Start();
  TRY_AND_EXIT_ON_ITK_EXCEPTION(reader->Update())
	  readerProbe.Stop();
  if (args_info.verbose_flag)
	  std::cout << "It took " << readerProbe.GetMean() << ' ' << readerProbe.GetUnit() << std::endl;
  int nProj = reader->GetOutput()->GetLargestPossibleRegion().GetSize()[Dimension-1];

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

  // DVF
  if (args_info.verbose_flag)
	  std::cout << "Reading DVF ... ";
  itk::TimeProbe dvfTimeProbe;
  dvfTimeProbe.Start();
  typedef itk::Vector<float, 3> DVFPixelType;
  typedef itk::Image< DVFPixelType, 3 > DVFImageType;
  typedef rtk::CyclicDeformationImageFilter< DVFImageType > DeformationType;
  typedef itk::ImageFileReader<DeformationType::InputImageType> DVFReaderType;
  DVFReaderType::Pointer dvfReader = DVFReaderType::New();
  DeformationType::Pointer def = DeformationType::New();
  def->SetInput(dvfReader->GetOutput());
  dvfReader->SetFileName(args_info.dvf_arg);
  def->SetSignalFilename(args_info.signal_arg);
  TRY_AND_EXIT_ON_ITK_EXCEPTION(dvfReader->Update());
  TRY_AND_EXIT_ON_ITK_EXCEPTION(def->Update());
  dvfTimeProbe.Stop();
  if (args_info.verbose_flag)
	  std::cout << "It took " << dvfTimeProbe.GetMean() << ' ' << dvfTimeProbe.GetUnit() << std::endl;

  if (args_info.verbose_flag)
	  std::cout << "Reconstructing ... ";
  itk::TimeProbe reconTimeProbe;
  reconTimeProbe.Start();

  // Displaced detector weighting
  typedef rtk::DisplacedDetectorImageFilter< OutputImageType > DDFCPUType;
  typedef rtk::CudaDisplacedDetectorImageFilter DDFType;
  DDFCPUType::Pointer ddf;
  if (args_info.dd_given)
  {
	  if (!strcmp(args_info.dd_arg, "cpu"))
		  ddf = DDFCPUType::New();
	  else
		  ddf = DDFType::New();
  }
  else
	  ddf = DDFType::New();
  ddf->SetInput( reader->GetOutput() );
  ddf->SetGeometry( geometryPointer );
  ddf->InPlaceOn();

  // Short scan image filter
  typedef rtk::CudaParkerShortScanImageFilter PSSFType;
  PSSFType::Pointer pssf = PSSFType::New();
  pssf->SetInput(ddf->GetOutput());
  pssf->SetGeometry( geometryPointer );
  pssf->InPlaceOn();
  pssf->UpdateOutputInformation();
  //TRY_AND_EXIT_ON_ITK_EXCEPTION(pssf->Update());

  // Extract filter type
  typedef itk::ExtractImageFilter<OutputImageType, OutputImageType> ExtractType;
  ExtractType::Pointer extractProj = ExtractType::New();
  extractProj->SetInput(pssf->GetOutput());
  ExtractType::InputImageRegionType subsetRegion;
  subsetRegion = pssf->GetOutput()->GetLargestPossibleRegion();
  subsetRegion.SetIndex(Dimension - 1, 0);
  subsetRegion.SetSize(Dimension - 1, std::min(args_info.subsetsize_arg, nProj));
  extractProj->SetExtractionRegion(subsetRegion);

  // FDK 2D weight
  typedef rtk::CudaFDKWeightProjectionFilter FDKWeightType;
  FDKWeightType::Pointer fdkweight = FDKWeightType::New();
  fdkweight->SetInput(extractProj->GetOutput());
  fdkweight->SetGeometry(geometryPointer);
  fdkweight->InPlaceOn();
  //TRY_AND_EXIT_ON_ITK_EXCEPTION(fdkweight->Update());

  // Ramp filter
  typedef rtk::CudaFFTRampImageFilter RampType;
  RampType::Pointer ramp = RampType::New();
  ramp->SetInput(fdkweight->GetOutput());
  ramp->SetTruncationCorrection(args_info.pad_arg);
  ramp->SetHannCutFrequency(args_info.hann_arg);
  ramp->SetHannCutFrequencyY(args_info.hannY_arg);

  /*
  if (args_info.verbose_flag)
	  std::cout << "Processing projections ... ";
  itk::TimeProbe projTimeProbe;
  projTimeProbe.Start();
  TRY_AND_EXIT_ON_ITK_EXCEPTION(ramp->Update());
  projTimeProbe.Stop();
  if (args_info.verbose_flag)
	  std::cout << "It took " << projTimeProbe.GetMean() << ' ' << projTimeProbe.GetUnit() << std::endl;
  */

  // Create reconstructed image
  typedef rtk::ConstantImageSource< OutputImageType > ConstantImageSourceType;
  ConstantImageSourceType::Pointer constantImageSource = ConstantImageSourceType::New();
  rtk::SetConstantImageSourceFromGgo<ConstantImageSourceType, args_info_igtcudamcfdk>(constantImageSource, args_info);

  // Back projection
  typedef rtk::FDKWarpBackProjectionImageFilter<OutputImageType, OutputImageType, DeformationType> BPType;
  BPType::Pointer bp = BPType::New();
  bp->InPlaceOn();
  bp->SetTranspose(false);
  //bp->SetCudaDeformation(def);
  bp->SetDeformation(def);
  bp->SetGeometry(&(*(geometryPointer)));
  bp->SetInput(0, constantImageSource->GetOutput());
  bp->SetInput(1, ramp->GetOutput());
  bp->UpdateOutputInformation();
  bp->GetOutput()->SetRequestedRegion(bp->GetOutput()->GetRequestedRegion());
  bp->GetOutput()->PropagateRequestedRegion();
  for (int i = 0; i < nProj; i += args_info.subsetsize_arg)
  {
	  // Change projection subset
	  subsetRegion.SetIndex(Dimension - 1, i);
	  subsetRegion.SetSize(Dimension - 1, std::min(args_info.subsetsize_arg, nProj - i));
	  extractProj->SetExtractionRegion(subsetRegion);

	  if (i)
	  {
		  OutputImageType::Pointer pimg = bp->GetOutput();
		  pimg->DisconnectPipeline();
		  bp->SetInput(0, pimg);

		  // This is required to reset the full pipeline
		  bp->GetOutput()->UpdateOutputInformation();
		  bp->GetOutput()->PropagateRequestedRegion();
	  }
	  TRY_AND_EXIT_ON_ITK_EXCEPTION(bp->Update());
  }
  reconTimeProbe.Stop();
  if (args_info.verbose_flag)
	  std::cout << "It took " << reconTimeProbe.GetMean() << ' ' << reconTimeProbe.GetUnit() << std::endl;

  if (args_info.verbose_flag)
	  std::cout << "Writing ... ";
  itk::TimeProbe writerTimeProbe;
  writerTimeProbe.Start();
  typedef itk::ImageFileWriter<OutputImageType> WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetInput(bp->GetOutput());
  writer->SetFileName(args_info.output_arg);
  TRY_AND_EXIT_ON_ITK_EXCEPTION(writer->Update());
  writerTimeProbe.Stop();
  if (args_info.verbose_flag)
	  std::cout << "It took " << writerTimeProbe.GetMean() << ' ' << writerTimeProbe.GetUnit() << std::endl;

  return EXIT_SUCCESS;
}
