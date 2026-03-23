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

#include "rtkpiccs_ggo.h"
#include "rtkGgoFunctions.h"

#include "rtkThreeDCircularProjectionGeometryXMLFile.h"
#include "rtkPICCSConeBeamReconstructionFilter.h"
#include "rtkThreeDCircularProjectionGeometrySorter.h"
#include "rtkBinNumberReader.h"
#include "itkVectorImage.h"
#include "rtkCyclicDeformationImageFilter.h"
#include "itkTileImageFilter.h"

#ifdef IGT_USE_CUDA
  #include "itkCudaImage.h"
  #include "rtkCudaCyclicDeformationImageFilter.h"
#endif

#include <itkImageFileWriter.h>

int main(int argc, char * argv[])
{
  GGO(rtkpiccs, args_info);

  typedef float OutputPixelType;
  const unsigned int Dimension = 3;

  // Read in bin number vector if csv file provided
  typedef rtk::BinNumberReader BinNumberReaderType;
  BinNumberReaderType::Pointer binReader = BinNumberReaderType::New();
  if(args_info.sortfile_given)
  {
	binReader->SetFileName( args_info.sortfile_arg );
	binReader->Parse();
  }

#ifdef IGT_USE_CUDA
  typedef itk::CudaImage< OutputPixelType, Dimension > OutputImageType;
  typedef itk::CudaImage< OutputPixelType, Dimension-1 > ProjectionType;
#else
  typedef itk::Image< OutputPixelType, Dimension > OutputImageType;
  typedef itk::Image< OutputPixelType, Dimension - 1 > ProjectionType;
#endif

  // Projections reader
  typedef rtk::ProjectionsReader< OutputImageType > ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  if(args_info.sortfile_given)
	rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_rtkpiccs>(reader, args_info, binReader->GetOutput() );
  else
	rtk::SetProjectionsReaderFromGgo<ReaderType, args_info_rtkpiccs>(reader, args_info);

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

  // Create input: either an existing volume read from a file or a blank image
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
    // Create new empty volume
    typedef rtk::ConstantImageSource< OutputImageType > ConstantImageSourceType;
    ConstantImageSourceType::Pointer constantImageSource = ConstantImageSourceType::New();
    rtk::SetConstantImageSourceFromGgo<ConstantImageSourceType, args_info_rtkpiccs>(constantImageSource, args_info);
    inputFilter = constantImageSource;
    }
	
  // Read in prior image
  typedef itk::ImageFileReader<  OutputImageType > InputReaderType;
  InputReaderType::Pointer priorReader = InputReaderType::New();
  if(args_info.prior_given)
    priorReader->SetFileName( args_info.prior_arg );

  // PICCS reconstruction filter
  typedef rtk::PICCSConeBeamReconstructionFilter< OutputImageType >	PICCSReconType;
  PICCSReconType::Pointer piccs = PICCSReconType::New();

  // Motion-compensated objects for the compensation of a cyclic deformation
  typedef PICCSReconType::DVFImageType		DVFImageType;
  typedef PICCSReconType::DeformationType	DeformationType;
  typedef itk::ImageFileReader<DeformationType::InputImageType> DVFReaderType;
  DVFReaderType::Pointer dvfReader = DVFReaderType::New();
  DeformationType::Pointer def = DeformationType::New();
  def->SetInput(dvfReader->GetOutput());

#ifdef IGT_USE_CUDA
  typedef PICCSReconType::CudaDVFImageType		    CudaDVFImageType;
  typedef PICCSReconType::CudaDeformationType		CudaDeformationType;
  typedef itk::ImageFileReader<CudaDeformationType::InputImageType> CudaDVFReaderType;
  CudaDVFReaderType::Pointer dvfReader_cuda = CudaDVFReaderType::New();
  CudaDeformationType::Pointer def_cuda = CudaDeformationType::New();
  def_cuda->SetInput(dvfReader_cuda->GetOutput());
#endif

  // Set CPU/Cuda methods
  piccs->SetForwardProjectionFilter(args_info.fp_arg);
  if (args_info.signal_given && args_info.dvf_given)
  {
	  if (args_info.verbose_flag)
		  std::cout << "Reading DVF information from "
		  << args_info.signal_arg
		  << " and "
		  << args_info.dvf_arg
		  << "..."
		  << std::endl;
	  
	  if (args_info.bp_arg == 2)
	  {
#ifdef IGT_USE_CUDA
		  dvfReader_cuda->SetFileName(args_info.dvf_arg);
		  def_cuda->SetSignalFilename(args_info.signal_arg);
		  TRY_AND_EXIT_ON_ITK_EXCEPTION(def_cuda->Update());
		  piccs->SetCudaDeformation(def_cuda);
#else
		  std::cerr << "The program has not been compiled with cuda." << std::endl;
#endif
	  }
	  else
	  {
		  dvfReader->SetFileName(args_info.dvf_arg);
		  def->SetSignalFilename(args_info.signal_arg);
		  TRY_AND_EXIT_ON_ITK_EXCEPTION(def->Update());
		  piccs->SetDeformation(def);
	  }
	  if (args_info.bp_arg == 2)
		  piccs->SetBackProjectionFilter(5);
	  else
		  piccs->SetBackProjectionFilter(4);
  }
  else
	piccs->SetBackProjectionFilter(args_info.bp_arg);
  piccs->SetTVFilter(args_info.tv_arg);
  piccs->SetTVGradientFilter(args_info.tvgrad_arg);
  piccs->SetTVHessianFilter(args_info.tvhessian_arg);
  piccs->SetDisplacedDetectorFilter(args_info.dd_arg);
  
  // Set maximum allowed number of threads if given
  if (args_info.thread_given)
	piccs->SetMaxNumberOfThreads( args_info.thread_arg );
  
  typedef itk::VectorImage< OutputPixelType, Dimension > VectorImageType;
  typedef itk::ImageFileReader< VectorImageType > VectorImageReaderType;
  
  // Set adaptive TV weighting image if input
  if (args_info.wtv_given)
    {
	VectorImageReaderType::Pointer tvWeightingReader = VectorImageReaderType::New();
	tvWeightingReader->SetFileName( args_info.wtv_arg );
    if(args_info.verbose_flag)
      std::cout << "Reading adaptive TV weighting image... " << std::endl;
    TRY_AND_EXIT_ON_ITK_EXCEPTION( tvWeightingReader->Update() )
	piccs->SetTVWeightingImage( tvWeightingReader->GetOutput() );
	}
	
  // Set adaptive prior weighting image if input
  if (args_info.wprior_given)
    {
	VectorImageReaderType::Pointer priorWeightingReader = VectorImageReaderType::New();
	priorWeightingReader->SetFileName( args_info.wprior_arg );
    if(args_info.verbose_flag)
      std::cout << "Reading adaptive prior weighting image... " << std::endl;
    TRY_AND_EXIT_ON_ITK_EXCEPTION( priorWeightingReader->Update() )
	piccs->SetPriorWeightingImage( priorWeightingReader->GetOutput() );
	}

  // Set truncation calibration map
  typedef itk::TileImageFilter<ProjectionType, OutputImageType> TileType;
  typedef itk::ImageFileReader<ProjectionType> TwoDReaderType;
  if (args_info.truncation_given)
  {
	  if (args_info.verbose_flag)
		  std::cout << "Reading calibration map for truncation mismatch correction... " << std::endl;
	  itk::RegularExpressionSeriesFileNames::Pointer truncationMapNames = itk::RegularExpressionSeriesFileNames::New();
	  truncationMapNames->SetDirectory(args_info.truncation_arg);
	  truncationMapNames->SetRegularExpression(".mha");
	  truncationMapNames->SetNumericSort(args_info.nsort_flag);
	  truncationMapNames->SetSubMatch(args_info.submatch_arg);
	  TileType::Pointer tileTruncationMap = TileType::New();
	  itk::FixedArray< unsigned int, Dimension > layout;
	  layout[0] = 1;
	  layout[1] = 1;
	  layout[2] = 0;
	  tileTruncationMap->SetLayout(layout);
	  int mapNumber = 0;
	  for (int k = 0; k != truncationMapNames->GetFileNames().size(); ++k)
	  {
		  if (binReader->GetOutput()[k] != args_info.binnumber_arg)
			  continue;
		  TwoDReaderType::Pointer mapReader = TwoDReaderType::New();
		  mapReader->SetFileName(truncationMapNames->GetFileNames()[k]);
		  TRY_AND_EXIT_ON_ITK_EXCEPTION(mapReader->Update());
		  ProjectionType::Pointer tmpMap = mapReader->GetOutput();
		  tmpMap->DisconnectPipeline();
		  tileTruncationMap->SetInput(mapNumber, tmpMap);
	      mapNumber++;
	  }
	  TRY_AND_EXIT_ON_ITK_EXCEPTION(tileTruncationMap->Update());
	  tileTruncationMap->GetOutput()->SetOrigin(reader->GetOutput()->GetOrigin());

	  piccs->SetTruncationCalibrationMap(tileTruncationMap->GetOutput());
  }

  // Set other inputs
  piccs->SetVerbose( args_info.verbose_flag );
  piccs->SetInput( inputFilter->GetOutput() );
  piccs->SetInput(1, reader->GetOutput());
  
  piccs->SetGeometry( geometryPointer );
  piccs->SetNumberOfIterations( args_info.niterations_arg );
  piccs->SetStoppingCriterion( args_info.stop_arg );
  piccs->SetLambda( args_info.lambda_arg );
  if(args_info.prior_given)
    {
	piccs->SetInput(2, priorReader->GetOutput());
	piccs->SetAlpha( args_info.alpha_arg );
	}
  else
    {
	// Dummy prior
	piccs->SetInput(2, inputFilter->GetOutput());
	piccs->SetAlpha( 0 );
	}

  itk::TimeProbe totalTimeProbe;
  if(args_info.time_flag)
    {
    std::cout << "Recording elapsed time... " << std::endl << std::flush;
    totalTimeProbe.Start();
    }

  TRY_AND_EXIT_ON_ITK_EXCEPTION( piccs->Update() )

  /*
  if (args_info.verbose_flag)
    {
	std::cout << " Objective function values : " << std::endl;
    for (unsigned int k = 0; k <= args_info.niterations_arg; ++k)
	  std::cout << "     " << piccs->GetObjectiveVectors()[k] << std::endl;

    std::cout << " Residual values : " << std::endl;
    for (unsigned int k = 0; k <= args_info.niterations_arg; ++k)
	  std::cout << "     " << piccs->GetResidualVectors()[k] << std::endl;
    }
   */

  if(args_info.time_flag)
    {
    piccs->PrintTiming(std::cout);
    totalTimeProbe.Stop();
    std::cout << "It took...  " << totalTimeProbe.GetMean() << ' ' << totalTimeProbe.GetUnit() << std::endl;
    }

  // Write
  typedef itk::ImageFileWriter< OutputImageType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( args_info.output_arg );
  writer->SetInput( piccs->GetOutput() );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( writer->Update() )

  return EXIT_SUCCESS;
}
