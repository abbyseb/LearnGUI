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

#include "rtktargettracking_ggo.h"
#include "rtkGgoFunctions.h"
#include "igtConfiguration.h"

#include "rtkThreeDCircularProjectionGeometryXMLFile.h"
#include "rtkDirectConeBeamTracking.h"
#include "rtkDirectConeBeamNCCTracking.h"
#include "rtkThreeDCircularProjectionGeometrySorter.h"
#include "rtkBinNumberReader.h"
#include "rtkPartialDerivativeImageFilter.h"

#include <itkExtractImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkImageFileWriter.h>
#include <itkExtractImageFilter.h>
#include <itkRegularExpressionSeriesFileNames.h>
#include <itkCastImageFilter.h>
#include <itkMatrix.h>
#include <itkVector.h>
#include <itkCSVNumericObjectFileWriter.h>

#include <math.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vnl/vnl_matrix.h>

#ifndef M_PI
#define M_PI vnl_math::pi
#endif

/* Author: Andy Shieh 2015 */

int main(int argc, char * argv[])
{
  GGO(rtktargettracking, args_info);
  
  /* Check CUDA availability */
#ifndef IGT_USE_CUDA
  if(args_info.fp_arg == 2)
    {
    std::cerr << "The program has not been compiled with cuda option" << std::endl;
    return EXIT_FAILURE;
    }
#endif

  if(args_info.prearc_arg < args_info.arcsize_arg)
    {
	std::cerr << "Input prearc must be larger or equal to arcsize" << std::endl;
	return EXIT_FAILURE;
    }

  /* Define output and display image types */
  typedef float OutputPixelType;
  const unsigned int Dimension = 3;
  
  typedef itk::Image< OutputPixelType, Dimension >     CPUOutputImageType;
#ifdef IGT_USE_CUDA
  typedef itk::CudaImage< OutputPixelType, Dimension > OutputImageType;
#else
  typedef CPUOutputImageType                           OutputImageType;
#endif

  typedef itk::Image< OutputPixelType, Dimension-1 >	DisplayImageType;

  /* Declare time probes */
  itk::TimeProbe ioProbe;
  itk::TimeProbe derivativeProbe;

  /* Define projection and prior reader type */
  typedef rtk::ProjectionsReader< CPUOutputImageType > ProjectionReaderType;
  typedef itk::ImageFileReader< CPUOutputImageType >   ImageReaderType;
  
  /* Define and declare image casting filters to change from CPU to CUDA if needed */
  typedef itk::CastImageFilter< CPUOutputImageType, OutputImageType >  CastInputType;
  typedef itk::CastImageFilter< OutputImageType, CPUOutputImageType >  CastOutputType;
  
  // Partial derivative image filter type
  typedef rtk::PartialDerivativeImageFilter<OutputImageType, OutputImageType> PartialDerivativeType;
  
  /* Define tracking filter type */
  typedef rtk::DirectConeBeamTracking< OutputImageType, OutputImageType > TrackingType;
  typedef rtk::DirectConeBeamNCCTracking< OutputImageType, OutputImageType > NCCTrackingType;
  
  // Define extract filter type
  typedef itk::RegionOfInterestImageFilter< OutputImageType, OutputImageType > ExtractProjType;
  typedef itk::ExtractImageFilter< CPUOutputImageType, DisplayImageType > ExtractDisplayType;
  
  /* Read in bin number vector */
  typedef rtk::BinNumberReader BinNumberReaderType;
  BinNumberReaderType::Pointer binReader = BinNumberReaderType::New();
  if(args_info.verbose_flag)
    std::cout << "Reading projection sorting information from "
              << args_info.sortfile_arg
              << "..."
              << std::flush;
  binReader->SetFileName( args_info.sortfile_arg );
  ioProbe.Start();
  TRY_AND_EXIT_ON_ITK_EXCEPTION(binReader->Parse() )
  ioProbe.Stop();
  if(args_info.verbose_flag)
	std::cout << "It took " << ioProbe.GetMean() << ' ' << ioProbe.GetUnit() << std::endl;
  
  /* Calculate number of non-zero elements and number of bins in bin vector */
  /* Also calculate the projection index vector */
  unsigned int nProj = 0;
  unsigned int nBin = 0;
  std::vector< unsigned int > projIndices;
  for (unsigned int k = 0; k < binReader->GetOutput().size(); ++k)
    {
	if ( binReader->GetOutput()[k] > 0 )
	  {
	  ++nProj;
	  projIndices.push_back( k );
	  }
	nBin = (binReader->GetOutput()[k] > nBin)?(binReader->GetOutput()[k]):nBin;
	}
  const unsigned int NProj = nProj;
  const unsigned int NBin = nBin;
	
  /* First and last frame */
  unsigned int firstFrame = args_info.first_arg;
  if( firstFrame < 1 )
	firstFrame = 1;
  unsigned int lastFrame = NProj;
  if( args_info.last_given )
	lastFrame = args_info.last_arg;
  if( lastFrame > NProj )
    lastFrame = NProj;
	
  /* Read target models */
  std::vector< ImageReaderType::Pointer > modelReaderVector;
  std::vector< CastInputType::Pointer > modelCastVector;
  std::vector< OutputImageType::Pointer > fVector;
  for(unsigned int k = 0; k < NBin; ++k)
   {
    modelReaderVector.push_back( ImageReaderType::New() );
	modelCastVector.push_back( CastInputType::New() );
   }
  itk::RegularExpressionSeriesFileNames::Pointer modelNames = itk::RegularExpressionSeriesFileNames::New();
  modelNames->SetDirectory(args_info.modelpath_arg);
  modelNames->SetNumericSort(args_info.modelnsort_flag);
  modelNames->SetRegularExpression(args_info.modelexp_arg);
  if(args_info.verbose_flag)
    std::cout << "Regular expression matches "
              << modelNames->GetFileNames().size()
              << " model image file(s)..."
              << std::endl;
  if( modelNames->GetFileNames().size() != NBin )
    {
    std::cerr << "The number of bins from the sort file and the model images do not match" << std::endl;
    return EXIT_FAILURE;
	}
  if(args_info.verbose_flag)
    std::cout << "Reading target model images from "
              << args_info.modelpath_arg
              << "..."
              << std::flush;	
  ioProbe.Reset();
  ioProbe.Start();	
  for(unsigned int k = 0; k < NBin; ++k)
    {
    modelReaderVector[k]->SetFileName( modelNames->GetFileNames()[k] );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( modelReaderVector[k]->Update() );
	modelCastVector[k]->SetInput( modelReaderVector[k]->GetOutput() );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( modelCastVector[k]->Update() );
	fVector.push_back( modelCastVector[k]->GetOutput() );
	fVector[k]->DisconnectPipeline();
    }
  ioProbe.Stop();
  if(args_info.verbose_flag)
	std::cout << "It took " << ioProbe.GetMean() << ' ' << ioProbe.GetUnit() << std::endl;
	
  std::vector< itk::Vector<double, 3> > priorOrigins;
  for(unsigned int bin = 0; bin < NBin; ++bin)
    {
	itk::Vector<double, 3> priorOrigin;
    for(unsigned int k = 0; k < 3; ++k)
	  priorOrigin[k] = fVector[bin]->GetOrigin()[k];
	priorOrigins.push_back( priorOrigin );
	}
	
  // Calculate partial derivatives of the target model
  std::vector< PartialDerivativeType::Pointer > fxFilter, fyFilter, fzFilter,
												fxxFilter, fyyFilter, fzzFilter,
												fxyFilter, fxzFilter, fyzFilter;
  std::vector< OutputImageType::Pointer > 		fxVector, fyVector, fzVector,
												fxxVector, fyyVector, fzzVector,
												fxyVector, fxzVector, fyzVector;
  if(args_info.verbose_flag)
    std::cout << "Calculating partial derivatives of target models..."
              << std::flush;	
  derivativeProbe.Reset();
  derivativeProbe.Start();	
  for(unsigned int k = 0; k < NBin; ++k)
   {
    fxFilter.push_back( PartialDerivativeType::New() );
	fxFilter[k]->SetInput( fVector[k] );
	fxFilter[k]->SetDerivativeOption( 0 );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( fxFilter[k]->Update() );
	fxVector.push_back( fxFilter[k]->GetOutput() );
	fxVector[k]->DisconnectPipeline();
	
    fyFilter.push_back( PartialDerivativeType::New() );
	fyFilter[k]->SetInput( fVector[k] );
	fyFilter[k]->SetDerivativeOption( 1 );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( fyFilter[k]->Update() );
	fyVector.push_back( fyFilter[k]->GetOutput() );
	fyVector[k]->DisconnectPipeline();
	
    fzFilter.push_back( PartialDerivativeType::New() );
	fzFilter[k]->SetInput( fVector[k] );
	fzFilter[k]->SetDerivativeOption( 2 );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( fzFilter[k]->Update() );
	fzVector.push_back( fzFilter[k]->GetOutput() );
	fzVector[k]->DisconnectPipeline();

    fxxFilter.push_back( PartialDerivativeType::New() );
	fxxFilter[k]->SetInput( fVector[k] );
	fxxFilter[k]->SetDerivativeOption( 3 );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( fxxFilter[k]->Update() );
	fxxVector.push_back( fxxFilter[k]->GetOutput() );
	fxxVector[k]->DisconnectPipeline();

    fyyFilter.push_back( PartialDerivativeType::New() );
	fyyFilter[k]->SetInput( fVector[k] );
	fyyFilter[k]->SetDerivativeOption( 4 );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( fyyFilter[k]->Update() );
	fyyVector.push_back( fyyFilter[k]->GetOutput() );
	fyyVector[k]->DisconnectPipeline();
	
    fzzFilter.push_back( PartialDerivativeType::New() );
	fzzFilter[k]->SetInput( fVector[k] );
	fzzFilter[k]->SetDerivativeOption( 5 );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( fzzFilter[k]->Update() );
	fzzVector.push_back( fzzFilter[k]->GetOutput() );
	fzzVector[k]->DisconnectPipeline();

    fxyFilter.push_back( PartialDerivativeType::New() );
	fxyFilter[k]->SetInput( fVector[k] );
	fxyFilter[k]->SetDerivativeOption( 6 );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( fxyFilter[k]->Update() );
	fxyVector.push_back( fxyFilter[k]->GetOutput() );
	fxyVector[k]->DisconnectPipeline();
	
	fxzFilter.push_back( PartialDerivativeType::New() );
	fxzFilter[k]->SetInput( fVector[k] );
	fxzFilter[k]->SetDerivativeOption( 7 );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( fxzFilter[k]->Update() );
	fxzVector.push_back( fxzFilter[k]->GetOutput() );
	fxzVector[k]->DisconnectPipeline();
	
    fyzFilter.push_back( PartialDerivativeType::New() );
	fyzFilter[k]->SetInput( fVector[k] );
	fyzFilter[k]->SetDerivativeOption( 8 );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( fyzFilter[k]->Update() );
	fyzVector.push_back( fyzFilter[k]->GetOutput() );
	fyzVector[k]->DisconnectPipeline();
   }
  derivativeProbe.Stop();
  if(args_info.verbose_flag)
	std::cout << "It took " << derivativeProbe.GetMean() << ' ' << derivativeProbe.GetUnit() << std::endl;
	
  /* Read prior respiratory correlated images */
  std::vector< ImageReaderType::Pointer > priorReaderVector;
  if(args_info.priorpath_given)
    {
    for(unsigned int k = 0; k < NBin; ++k)
      priorReaderVector.push_back( ImageReaderType::New() );
    itk::RegularExpressionSeriesFileNames::Pointer priorNames = itk::RegularExpressionSeriesFileNames::New();
    priorNames->SetDirectory(args_info.priorpath_arg);
    priorNames->SetNumericSort(args_info.priornsort_flag);
    priorNames->SetRegularExpression(args_info.priorexp_arg);
    if(args_info.verbose_flag)
      std::cout << "Regular expression matches "
                << priorNames->GetFileNames().size()
                << " prior image file(s)..."
                << std::endl;
    if( priorNames->GetFileNames().size() != NBin )
      {
      std::cerr << "The number of bins from the sort file and the prior images do not match" << std::endl;
      return EXIT_FAILURE;
	  }
    if(args_info.verbose_flag)
      std::cout << "Reading prior images from "
                << args_info.priorpath_arg
                << "..."
                << std::flush;	
    ioProbe.Reset();
    ioProbe.Start();	
    for(unsigned int k = 0; k < NBin; ++k)
      {
      priorReaderVector[k]->SetFileName( priorNames->GetFileNames()[k] );
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( priorReaderVector[k]->Update() );
      }
    ioProbe.Stop();
    if(args_info.verbose_flag)
	  std::cout << "It took " << ioProbe.GetMean() << ' ' << ioProbe.GetUnit() << std::endl;
    }
  
  /* Read in geometry */
  if(args_info.verbose_flag)
    std::cout << "Reading geometry information from "
              << args_info.geometry_arg
              << "..."
              << std::endl;
  rtk::ThreeDCircularProjectionGeometryXMLFileReader::Pointer geometryReader;
  geometryReader = rtk::ThreeDCircularProjectionGeometryXMLFileReader::New();
  geometryReader->SetFilename(args_info.geometry_arg);
  TRY_AND_EXIT_ON_ITK_EXCEPTION( geometryReader->GenerateOutputInformation() );
  // Need to enforce full-scan geometry even if it's not for the small angular range of DTS
  geometryReader->GetOutputObject()->SetShortScanOff();
  geometryReader->GetOutputObject()->SetDTSOn();
  
  // Initialize output data matrix
  // First column: DTS angle, second to fourth: target origin
  vnl_matrix<double> outputData(NProj,18, 0.0);
  
  // Calculate DTS angles
  if(args_info.verbose_flag)
    std::cout << "Calculating and writing DTS angles... " << std::endl;
  // Note that here we let k_frame loop through all projections instead of from first frame to last frame
  for(unsigned int k_frame = 0; k_frame < NProj; ++k_frame)
    {
    const unsigned int respiratoryBin = binReader->GetOutput()[ projIndices[k_frame] ];

	/* Generate the sort vector for this frame */
	std::vector< unsigned int > sortVector( binReader->GetOutput().size(), 0 );
	unsigned int firstProj = 
		( (int(k_frame) - args_info.arcsize_arg) >= 0 )?(k_frame - args_info.arcsize_arg):0;
	for(unsigned int k = firstProj; k <= k_frame; ++k)
	  {
	  if( binReader->GetOutput()[ projIndices[k] ] == respiratoryBin )
		sortVector[ projIndices[k] ] = 1;
	  }
	
	/* Sort geometry */
	rtk::ThreeDCircularProjectionGeometrySorter::Pointer geometrySorter = rtk::ThreeDCircularProjectionGeometrySorter::New();
	geometrySorter->SetInputGeometry( geometryReader->GetOutputObject() );
	geometrySorter->SetBinNumberVector( sortVector );
	geometrySorter->Sort();
	
	std::vector<double> angularGaps = 
	   geometrySorter->GetOutput()[0]->GetAngularGapsWithNext( geometrySorter->GetOutput()[0]->GetGantryAngles() );
    const int nProj = angularGaps.size();
    int maxAngularGapPos = 0;
    for(unsigned int iProj=1; iProj<nProj; iProj++)
	  {
      if(angularGaps[iProj] > angularGaps[maxAngularGapPos])
        maxAngularGapPos = iProj;
	  }
    const std::vector<double> rotationAngles = geometrySorter->GetOutput()[0]->GetGantryAngles();
    const std::map<double,unsigned int> sortedAngles = 
	   geometrySorter->GetOutput()[0]->GetUniqueSortedAngles(geometrySorter->GetOutput()[0]->GetGantryAngles() );
    // First angle
    std::map<double,unsigned int>::const_iterator itFirstAngle;
    itFirstAngle = sortedAngles.find(rotationAngles[maxAngularGapPos]);
    itFirstAngle = (++itFirstAngle==sortedAngles.end())?sortedAngles.begin():itFirstAngle;
    const double firstAngle = itFirstAngle->first;
    // Last angle
    std::map<double,unsigned int>::const_iterator itLastAngle;
    itLastAngle = sortedAngles.find(rotationAngles[maxAngularGapPos]);
    const double lastAngle = itLastAngle->first;
	// Get the middle accute angle
	double dtsAngle = (firstAngle + lastAngle) / 2.;
	dtsAngle = fmod( dtsAngle, 2*M_PI );
	if ( fmod(fabs(dtsAngle - firstAngle), 2*M_PI) > fmod(fabs(dtsAngle + M_PI - firstAngle), 2*M_PI) )
	  dtsAngle = fmod( dtsAngle + M_PI, 2*M_PI );
	outputData(k_frame,0) = dtsAngle;
	}
  
  /* Loop through reconstruction frames */
  for(unsigned int k_frame = 0; k_frame <= lastFrame - firstFrame; k_frame += args_info.step_arg)
    {
	
    if(args_info.verbose_flag)
      std::cout << "Tracking frame "
                << k_frame + firstFrame
                << "..."
                << std::endl;
	
	/* Current respiratory bin */
	const unsigned int respiratoryBin = binReader->GetOutput()[ projIndices[k_frame + firstFrame - 1] ];
	
	/* Generate the sort vector for this frame */
	// And also generate the sort vectors for the pre-tracking arc
	std::vector< unsigned int > sortVector( binReader->GetOutput().size(), 0 );
	std::vector< unsigned int > sortVector_pre( binReader->GetOutput().size(), 0 );
	unsigned int firstProj = 
		( (int(k_frame + firstFrame) - args_info.arcsize_arg) >= 0 )?(k_frame + firstFrame - args_info.arcsize_arg):0;
	unsigned int firstProj_pre = 
		( (int(k_frame + firstFrame) - args_info.prearc_arg) >= 0 )?(k_frame + firstFrame - args_info.prearc_arg):0;
	unsigned int NProj_arc = 0;
	unsigned int NProj_prearc = 0; 
	for(unsigned int k = firstProj; k <= k_frame + firstFrame - 1; ++k)
	  {
	  if( binReader->GetOutput()[ projIndices[k] ] == respiratoryBin )
	    {
		sortVector[ projIndices[k] ] = 1;
		sortVector_pre[ projIndices[k] ] = 1;
		++NProj_arc;
		++NProj_prearc;
		}
	  }
	for(unsigned int k = firstProj_pre; k < firstProj_pre + args_info.arcsize_arg; ++k)
	  {
	  if( binReader->GetOutput()[ projIndices[k] ] == respiratoryBin && k < firstProj )
	    {
		sortVector_pre[ projIndices[k] ] = 1;
		++NProj_prearc;
		}
	  }
	  
	/* Read projections for pre-tracking*/
	ProjectionReaderType::Pointer projectionReader = ProjectionReaderType::New();
    if(args_info.verbose_flag)
      std::cout << "   " << std::flush;
	rtk::SetProjectionsReaderFromGgo<ProjectionReaderType, args_info_rtktargettracking>(projectionReader, args_info, sortVector_pre, 1 );
    if(args_info.verbose_flag)
	  std::cout << "   Reading projections... " << std::flush;
	ioProbe.Reset();
	ioProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( projectionReader->Update() )
	ioProbe.Stop();
	if(args_info.verbose_flag)
	  std::cout << "It took " << ioProbe.GetMean() << ' ' << ioProbe.GetUnit() << std::endl;
	
	/* Sort geometry */
	rtk::ThreeDCircularProjectionGeometrySorter::Pointer geometrySorter_pre = rtk::ThreeDCircularProjectionGeometrySorter::New();
	geometrySorter_pre->SetInputGeometry( geometryReader->GetOutputObject() );
	geometrySorter_pre->SetBinNumberVector( sortVector_pre );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( geometrySorter_pre->Sort() );
	rtk::ThreeDCircularProjectionGeometrySorter::Pointer geometrySorter = rtk::ThreeDCircularProjectionGeometrySorter::New();
	geometrySorter->SetInputGeometry( geometryReader->GetOutputObject() );
	geometrySorter->SetBinNumberVector( sortVector );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( geometrySorter->Sort() );
	
	/* Prepare racking filter for pre-tracking*/
	TrackingType::Pointer pretracking;
	if(args_info.metric_arg == 0)
	  pretracking = TrackingType::New();
	else if(args_info.metric_arg == 1)
	  pretracking = NCCTrackingType::New();
	pretracking->SetGeometry( geometrySorter_pre->GetOutput()[0] );
	pretracking->SetOptimizerConfiguration( args_info.optimizer_arg );
	CastInputType::Pointer castPriorFilter = CastInputType::New();
    CastInputType::Pointer castProjectionFilter = CastInputType::New();
	castProjectionFilter->SetInput( projectionReader->GetOutput() );
	pretracking->SetInput( castProjectionFilter->GetOutput() );
	if(args_info.priorpath_given)
	  {
	  castPriorFilter->SetInput( priorReaderVector[respiratoryBin - 1]->GetOutput() );
	  pretracking->SetPriorImage( castPriorFilter->GetOutput() );
	  }
    if (args_info.fp_given)
      pretracking->SetForwardProjectionFilter(args_info.fp_arg);
    else
      pretracking->SetForwardProjectionFilter(0);
    pretracking->SetNumberOfIterations( args_info.niterations_arg );
	pretracking->SetStoppingCriterion( args_info.stop_arg );
	  
	// Histogram matching parameters for rigid registration
	pretracking->SetNumberOfHistogramLevels( 16 );
	pretracking->SetNumberOfMatchPoints( 2 );
	  
	// Calculate the derivatives of the model
	pretracking->Setf( fVector[respiratoryBin - 1] );
	pretracking->Setfx( fxVector[respiratoryBin - 1] );
	pretracking->Setfy( fyVector[respiratoryBin - 1] );
	pretracking->Setfz( fzVector[respiratoryBin - 1] );
	pretracking->Setfxx( fxxVector[respiratoryBin - 1] );
	pretracking->Setfyy( fyyVector[respiratoryBin - 1] );
	pretracking->Setfzz( fzzVector[respiratoryBin - 1] );
	pretracking->Setfxy( fxyVector[respiratoryBin - 1] );
	pretracking->Setfxz( fxzVector[respiratoryBin - 1] );
	pretracking->Setfyz( fyzVector[respiratoryBin - 1] );
	
	// Set initial displacement
	itk::Vector<double, 3> targetOrigin0;
	if (k_frame == 0)
	  {
	  for(unsigned int k = 0; k < 3; ++k)
	    targetOrigin0[k] = priorOrigins[respiratoryBin - 1][k];
	  }
	else
	  {
	  for(unsigned int k = 0; k < 3; ++k)
	    targetOrigin0[k] = outputData(k_frame + firstFrame - args_info.step_arg - 1,k + 11);
	  }
	pretracking->SetPriorOrigin( priorOrigins[respiratoryBin - 1] );
	pretracking->SetTargetOrigin( targetOrigin0 );
	if(NProj_prearc > NProj_arc)
	  pretracking->SetLambdaInDepth( 0 );
	else
	  pretracking->SetLambdaInDepth( args_info.lambdaID_arg );
	pretracking->SetLambdaInPlane( args_info.lambdaIP_arg );
	pretracking->SetLambdaSI( args_info.lambdaSI_arg );
	pretracking->SetGammaInDepth( args_info.gammaID_arg );
	pretracking->SetGammaInPlane( args_info.gammaIP_arg );
	pretracking->SetGammaSI( args_info.gammaSI_arg );
	pretracking->SetConstrainAngle( outputData(k_frame + firstFrame - 1,0) );
	
	// Tracking starts
	// Pre-tracking
	if(args_info.verbose_flag)
      std::cout << "   Pre-tracking... " << std::flush;
	itk::TimeProbe pretrackingProbe;
	pretrackingProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( pretracking->Update() );
	pretrackingProbe.Stop();
    if(args_info.verbose_flag)
	  std::cout << "It took " << pretrackingProbe.GetMean() << ' ' << pretrackingProbe.GetUnit() << std::endl;
	  
    // Print timing
    if(args_info.verbose_flag)
      pretracking->PrintTiming(std::cout);
	  
	// Record results from pre-tracking
	// Record rigid registration parameters
	if(args_info.priorpath_given)
	  {
	  outputData(k_frame + firstFrame - 1, 1) = pretracking->GetRigidTransformParameters()[0];
	  outputData(k_frame + firstFrame - 1, 2) = pretracking->GetRigidTransformParameters()[1];
	  outputData(k_frame + firstFrame - 1, 3) = pretracking->GetRigidTransformParameters()[2];
	  }
    for(unsigned int k = 0; k < 3; ++k)
	  outputData(k_frame + firstFrame - 1, k + 4) = pretracking->GetTargetOrigin()[k];
	// The initial residual and objective values
	outputData(k_frame + firstFrame - 1, 7) = pretracking->GetResidualVector()[0];
	outputData(k_frame + firstFrame - 1, 8) = pretracking->GetObjectiveVector()[0];
	// Record final residual and objective values too
	outputData(k_frame + firstFrame - 1, 9) = pretracking->GetFinalResidual();
	outputData(k_frame + firstFrame - 1, 10) = pretracking->GetFinalObjective();
	
	// Actual tracking filter
	TrackingType::Pointer tracking;
	if(args_info.metric_arg == 0)
	  tracking = TrackingType::New();
	else if(args_info.metric_arg == 1)
	  tracking = NCCTrackingType::New();
	
	// Extract the subset of projections for actual tracking arc
	OutputImageType::Pointer p = pretracking->Getp();
	p->DisconnectPipeline();
	ExtractProjType::Pointer extractFilter = ExtractProjType::New();
	extractFilter->SetInput( p );
	OutputImageType::SizeType extractSize = p->GetLargestPossibleRegion().GetSize();
	extractSize[2] = NProj_arc;
	OutputImageType::IndexType extractIndex = p->GetLargestPossibleRegion().GetIndex();
	extractIndex[2] = NProj_prearc - NProj_arc;
	OutputImageType::RegionType extractRegion(extractIndex, extractSize);
	extractFilter->SetRegionOfInterest( extractRegion );
	TRY_AND_EXIT_ON_ITK_EXCEPTION( extractFilter->Update() );
	extractFilter->GetOutput()->SetOrigin( p->GetOrigin() );
	tracking->SetInput( extractFilter->GetOutput() );
	tracking->SetGeometry( geometrySorter->GetOutput()[0] );
	tracking->SetOptimizerConfiguration( args_info.optimizer_arg );
	
    if (args_info.fp_given)
      tracking->SetForwardProjectionFilter(args_info.fp_arg);
    else
      tracking->SetForwardProjectionFilter(0);
	  
    tracking->SetNumberOfIterations( args_info.niterations_arg );
	tracking->SetStoppingCriterion( args_info.stop_arg );
	tracking->Setf( fVector[respiratoryBin - 1] );
	tracking->Setfx( fxVector[respiratoryBin - 1] );
	tracking->Setfy( fyVector[respiratoryBin - 1] );
	tracking->Setfz( fzVector[respiratoryBin - 1] );
	tracking->Setfxx( fxxVector[respiratoryBin - 1] );
	tracking->Setfyy( fyyVector[respiratoryBin - 1] );
	tracking->Setfzz( fzzVector[respiratoryBin - 1] );
	tracking->Setfxy( fxyVector[respiratoryBin - 1] );
	tracking->Setfxz( fxzVector[respiratoryBin - 1] );
	tracking->Setfyz( fyzVector[respiratoryBin - 1] );
	
	// Use pre tracking results to constrain actual tracking
	tracking->SetLambdaInDepth( args_info.lambdaID_arg );
	tracking->SetLambdaInPlane( args_info.lambdaIP_arg );
	tracking->SetLambdaSI( args_info.lambdaSI_arg );
	tracking->SetGammaInDepth( args_info.gammaID_arg );
	tracking->SetGammaInPlane( args_info.gammaIP_arg );
	tracking->SetGammaSI( args_info.gammaSI_arg );
	tracking->SetConstrainAngle( outputData(k_frame + firstFrame - 1,0) );
	// tracking->SetTargetOrigin( priorOrigins[respiratoryBin - 1] );
	// tracking->SetPriorOrigin( pretracking->GetTargetOrigin() );
	itk::Vector<double,3> hybridTargetOrigin0 = pretracking->GetTargetOrigin();
	hybridTargetOrigin0[1] = targetOrigin0[1];
	tracking->SetTargetOrigin( hybridTargetOrigin0 );
	tracking->SetPriorOrigin( priorOrigins[respiratoryBin - 1] );
	
	// Actual tracking
	if(args_info.verbose_flag)
      std::cout << "   Tracking the target... " << std::flush;
	itk::TimeProbe trackingProbe;
	trackingProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( tracking->Update() );
	trackingProbe.Stop();
    if(args_info.verbose_flag)
	  std::cout << "It took " << trackingProbe.GetMean() << ' ' << trackingProbe.GetUnit() << std::endl;
	  
    for(unsigned int k = 0; k < 3; ++k)
	  outputData(k_frame + firstFrame - 1, k + 11) = tracking->GetTargetOrigin()[k];
	// The initial residual and objective values
	outputData(k_frame + firstFrame - 1, 14) = tracking->GetResidualVector()[0];
	outputData(k_frame + firstFrame - 1, 15) = tracking->GetObjectiveVector()[0];
	// Record final residual and objective values too
	outputData(k_frame + firstFrame - 1, 16) = tracking->GetFinalResidual();
	outputData(k_frame + firstFrame - 1, 17) = tracking->GetFinalObjective();

    // Print timing
    if(args_info.verbose_flag)
      tracking->PrintTiming(std::cout);
	
	// Write output to file
	// FIXME: Ideally we only want to write a new line every time
	// 		  Not sure how to do "write to the end of file" with itk CSV writer
	itk::CSVNumericObjectFileWriter<double>::Pointer outputWriter = itk::CSVNumericObjectFileWriter<double>::New();
	if(args_info.verbose_flag)
      std::cout << "   Writing output to file... " << std::flush;
	outputWriter->SetFileName( args_info.output_arg );
	outputWriter->SetInput( &outputData );
	ioProbe.Reset();
	ioProbe.Start();
	TRY_AND_EXIT_ON_ITK_EXCEPTION( outputWriter->Write() );
	ioProbe.Stop();
    if(args_info.verbose_flag)
	  std::cout << "It took " << ioProbe.GetMean() << ' ' << ioProbe.GetUnit() << std::endl;
	 
	// Writing display to current folder
	if(args_info.display_flag)
	  {
	  if(args_info.verbose_flag)
        std::cout << "   Writing display images to file... " << std::flush;
	  ioProbe.Reset();
	  ioProbe.Start();
	  /*
	  CastOutputType::Pointer castP = CastOutputType::New();
	  CastOutputType::Pointer castRf = CastOutputType::New();
	  castP->SetInput( tracking->Getp() );
	  castRf->SetInput( tracking->GetRfImage() );
	  ExtractDisplayType::Pointer extractRf = ExtractDisplayType::New();
      ExtractDisplayType::Pointer extractP = ExtractDisplayType::New();
      extractRf->SetDirectionCollapseToSubmatrix();
      extractP->SetDirectionCollapseToSubmatrix();
	  extractP->SetInput( castP->GetOutput() );
	  extractRf->SetInput( castRf->GetOutput() );
	  OutputImageType::SizeType displaySize = tracking->Getp()->GetLargestPossibleRegion().GetSize();
	  displaySize[2] = 0;
	  OutputImageType::IndexType displayIndex = tracking->Getp()->GetLargestPossibleRegion().GetIndex();
	  displayIndex[2] = NProj_arc - 1;
	  OutputImageType::RegionType displayRegion(displayIndex, displaySize);
	  extractP->SetExtractionRegion( displayRegion );
	  extractRf->SetExtractionRegion( displayRegion );
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( extractP->Update() );
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( extractRf->Update() );
	  */
	  
	  itk::ImageFileWriter<OutputImageType>::Pointer pWriter = itk::ImageFileWriter<OutputImageType>::New();
	  pWriter->SetFileName("Display_p.mha");
	  pWriter->SetInput( tracking->Getp() );
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( pWriter->Update() );
	  
	  itk::ImageFileWriter<OutputImageType>::Pointer RfWriter = itk::ImageFileWriter<OutputImageType>::New();
	  RfWriter->SetFileName("Display_Rf.mha");
	  RfWriter->SetInput( tracking->GetRfImage() );
	  TRY_AND_EXIT_ON_ITK_EXCEPTION( RfWriter->Update() );
	  
	  ioProbe.Stop();
      if(args_info.verbose_flag)
	    std::cout << "It took " << ioProbe.GetMean() << ' ' << ioProbe.GetUnit() << std::endl;
	  }
	
	/*
	// Testing purpose
	itk::ImageFileWriter<OutputImageType>::Pointer imageWriter = itk::ImageFileWriter<OutputImageType>::New();
	imageWriter->SetFileName("test.mha");
	imageWriter->SetInput(tracking->GetOutput());
	TRY_AND_EXIT_ON_ITK_EXCEPTION(imageWriter->Update());
	*/
	
	}

  return EXIT_SUCCESS;
}