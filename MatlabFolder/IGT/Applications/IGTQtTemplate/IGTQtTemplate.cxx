/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */


#include "ui_IGTQtTemplate.h"
#include "IGTQtTemplate.h"

#include <QMessageBox>
#include <itkImage.h>


#include "rtkThreeDCircularProjectionGeometryXMLFile.h"
#include "rtkPICCSConeBeamReconstructionFilter.h"
#include "rtkThreeDCircularProjectionGeometrySorter.h"
#include "rtkBinNumberReader.h"
#include "itkVectorImage.h"
#include <itkImageFileWriter.h>
#include <rtkProjectionsReader.h>
#include <rtkVectorSorter.h>


#ifdef IGT_USE_CUDA
#include "itkCudaImage.h"
#endif

// Constructor
IGTQtTemplate::IGTQtTemplate()
{
  this->ui = new Ui_IGTQtTemplate;
  this->ui->setupUi(this);
};

IGTQtTemplate::~IGTQtTemplate()
{
  // The smart pointers should clean up for up

}

void IGTQtTemplate::on_pushButton_released()
{
	typedef float OutputPixelType;
	const unsigned int Dimension = 3;

	// Read in bin number vector if csv file provided
	typedef rtk::BinNumberReader BinNumberReaderType;
	BinNumberReaderType::Pointer binReader = BinNumberReaderType::New();
	binReader->SetFileName("F:\\AndyShieh\\Nepean Data\\LungSBRT\\RawDicoms_CBCTProjections\\SBRT0011\\XVI LA1\\IMAGES\\img_1.3.46.423632.135616.1375671620.74\\RTKRecon\\SortInfo_4D.csv");
	binReader->Parse();

#ifdef IGT_USE_CUDA
	typedef itk::CudaImage< OutputPixelType, Dimension > OutputImageType;
#else
	typedef itk::Image< OutputPixelType, Dimension > OutputImageType;
#endif

	// Projections reader
	typedef rtk::ProjectionsReader< OutputImageType > ReaderType;
	ReaderType::Pointer reader = ReaderType::New();
	// Generate file names
	itk::RegularExpressionSeriesFileNames::Pointer names = itk::RegularExpressionSeriesFileNames::New();
	names->SetDirectory("F:\\AndyShieh\\Nepean Data\\LungSBRT\\RawDicoms_CBCTProjections\\SBRT0011\\XVI LA1\\IMAGES\\img_1.3.46.423632.135616.1375671620.74\\ATTProj");
	names->SetNumericSort(false);
	names->SetRegularExpression("");
	names->SetSubMatch(0);

	// Sort filenames
	rtk::VectorSorter<std::string>::Pointer sorter = rtk::VectorSorter<std::string>::New();
	sorter->SetInputVector(names->GetFileNames());
	sorter->SetBinNumberVector(binReader->GetOutput());
	sorter->Sort();

	std::cout << "Regular expression matches "
		<< names->GetFileNames().size()
		<< " file(s)..."
		<< "but only using a subset of "
		<< sorter->GetOutput()[1 - 1].size()
		<< " file(s)..."
		<< std::endl;

	// Pass list to projections reader
	reader->SetFileNames(sorter->GetOutput()[1 - 1]);
	TRY_AND_EXIT_ON_ITK_EXCEPTION(reader->UpdateOutputInformation());

	// Geometry
	std::cout << "Reading geometry information from "
		<< "F:\\AndyShieh\\Nepean Data\\LungSBRT\\RawDicoms_CBCTProjections\\SBRT0011\\XVI LA1\\IMAGES\\img_1.3.46.423632.135616.1375671620.74\\RTKRecon\\Geometry.xml"
		<< "..."
		<< std::endl;
	rtk::ThreeDCircularProjectionGeometryXMLFileReader::Pointer geometryReader;
	geometryReader = rtk::ThreeDCircularProjectionGeometryXMLFileReader::New();
	geometryReader->SetFilename("F:\\AndyShieh\\Nepean Data\\LungSBRT\\RawDicoms_CBCTProjections\\SBRT0011\\XVI LA1\\IMAGES\\img_1.3.46.423632.135616.1375671620.74\\RTKRecon\\Geometry.xml");
	TRY_AND_EXIT_ON_ITK_EXCEPTION(geometryReader->GenerateOutputInformation())

		// Geometry pointer type
		rtk::ThreeDCircularProjectionGeometrySorter::GeometryPointer geometryPointer;

	rtk::ThreeDCircularProjectionGeometrySorter::Pointer geometrySorter = rtk::ThreeDCircularProjectionGeometrySorter::New();
	geometrySorter->SetInputGeometry(geometryReader->GetOutputObject());
	geometrySorter->SetBinNumberVector(binReader->GetOutput());
	geometrySorter->Sort();
	geometryPointer = geometrySorter->GetOutput()[1 - 1];

	// Create input: either an existing volume read from a file or a blank image
	itk::ImageSource< OutputImageType >::Pointer inputFilter;
	// Create new empty volume
	typedef rtk::ConstantImageSource< OutputImageType > ConstantImageSourceType;
	ConstantImageSourceType::Pointer constantImageSource = ConstantImageSourceType::New();

	OutputImageType::SizeType imageDimension;
	imageDimension.Fill(256);

	OutputImageType::SpacingType imageSpacing;
	imageSpacing.Fill(1);

	OutputImageType::PointType imageOrigin;
	for (unsigned int i = 0; i<Dimension; i++)
		imageOrigin[i] = imageSpacing[i] * (imageDimension[i] - 1) * -0.5;

	OutputImageType::DirectionType imageDirection;
	imageDirection.SetIdentity();


	constantImageSource->SetOrigin(imageOrigin);
	constantImageSource->SetSpacing(imageSpacing);
	constantImageSource->SetDirection(imageDirection);
	constantImageSource->SetSize(imageDimension);
	constantImageSource->SetConstant(0.);
	TRY_AND_EXIT_ON_ITK_EXCEPTION(constantImageSource->UpdateOutputInformation());
	inputFilter = constantImageSource;

	// Read in prior image
	typedef itk::ImageFileReader<  OutputImageType > InputReaderType;
	InputReaderType::Pointer priorReader = InputReaderType::New();
	priorReader->SetFileName("F:\\AndyShieh\\test.mha");

	// PICCS reconstruction filter
	rtk::PICCSConeBeamReconstructionFilter< OutputImageType >::Pointer piccs =
		rtk::PICCSConeBeamReconstructionFilter< OutputImageType >::New();

	// Set CPU/Cuda methods
	piccs->SetForwardProjectionFilter(0);
	piccs->SetBackProjectionFilter(0);
	piccs->SetTVFilter(0);
	piccs->SetTVGradientFilter(0);
	piccs->SetTVHessianFilter(0);
	piccs->SetDisplacedDetectorFilter(0);

	piccs->SetNumberOfThreads(128);

	// Set maximum allowed number of threads if given

	typedef itk::VectorImage< OutputPixelType, Dimension > VectorImageType;
	typedef itk::ImageFileReader< VectorImageType > VectorImageReaderType;

	// Set adaptive TV weighting image if input

	// Set adaptive prior weighting image if input

	// Set other inputs
	piccs->SetVerbose(true);
	piccs->SetInput(inputFilter->GetOutput());
	piccs->SetInput(1, reader->GetOutput());

	piccs->SetGeometry(geometryPointer);
	piccs->SetNumberOfIterations(20);
	piccs->SetStoppingCriterion(0.001);
	piccs->SetLambda(1);
	piccs->SetInput(2, priorReader->GetOutput());
	piccs->SetAlpha(0.2);

	itk::TimeProbe totalTimeProbe;

	std::cout << "Recording elapsed time... " << std::endl << std::flush;
	totalTimeProbe.Start();

	TRY_AND_EXIT_ON_ITK_EXCEPTION(piccs->Update())

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

		piccs->PrintTiming(std::cout);
	totalTimeProbe.Stop();
	std::cout << "It took...  " << totalTimeProbe.GetMean() << ' ' << totalTimeProbe.GetUnit() << std::endl;

	// Write
	typedef itk::ImageFileWriter< OutputImageType > WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName("F:\\AndyShieh\\O.mha");
	writer->SetInput(piccs->GetOutput());
	TRY_AND_EXIT_ON_ITK_EXCEPTION(writer->Update());
}

// Action to be taken upon file open
void IGTQtTemplate::slotOpenFile()
{

}

void IGTQtTemplate::slotExit() {
  qApp->exit();
}
