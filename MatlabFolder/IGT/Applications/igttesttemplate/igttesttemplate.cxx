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
#include <igtLinearCombinationFilter.h>
#include <igtCudaLinearCombinationFilter.h>
#include "igttesttemplate_ggo.h"
#include "rtkGgoFunctions.h"
#include "igtConfiguration.h"
 
#include "igtTargetTracking.h"

#include <igtKalmanFilter.h>

#include <igtProjectionAlignmentFilter.h>
 
#include <igtProjectionsReader.h>

 #include "rtkThreeDCircularProjectionGeometryXMLFile.h"

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkTimeProbe.h>
#include <itkRegularExpressionSeriesFileNames.h>
#include <rtkProjectionsReader.h>

#include <itkImageFileWriter.h>

/* Author: Andy Shieh 2016 */

int main(int argc, char * argv[])
{
	GGO(igttesttemplate, args_info);

	typedef float OutputPixelType;
	const unsigned int Dimension = 3;

	typedef itk::Image< OutputPixelType, Dimension >     CPUOutputImageType;
#ifdef IGT_USE_CUDA
	typedef itk::CudaImage< OutputPixelType, Dimension > OutputImageType;
#else
	typedef CPUOutputImageType                           OutputImageType;
#endif
	
	typedef itk::ImageFileReader<OutputImageType> ReaderType;
	ReaderType::Pointer reader = ReaderType::New();
	reader->SetFileName("C:\\Data\\LightSABR_MarkerlessTracking\\Pat06\\Fx01\\2016.05.27 15_21_39\\CBCTRecon\\FDK3D.mha");
	reader->Update();

	typedef itk::ImageFileWriter<OutputImageType> WriterType;
	WriterType::Pointer writer = WriterType::New();

	typedef igt::CudaLinearCombinationFilter TestType;
	TestType::Pointer test = TestType::New();
	test->AddInput(reader->GetOutput(),1);
	test->AddInput(reader->GetOutput(),2);
	test->AddInput(reader->GetOutput(),2);
	test->AddInput(reader->GetOutput(),2);
	test->AddInput(reader->GetOutput(),2);
	test->AddInput(reader->GetOutput(),2);
	test->AddInput(reader->GetOutput(),2);
	test->AddInput(reader->GetOutput(),2);
	itk::TimeProbe t;
	t.Start();
	test->Update();
	t.Stop();
	std::cout << t.GetMeanTime() << std::endl;

	writer->SetInput(test->GetOutput());
	writer->SetFileName("C:\\Data\\test1.mha");
	writer->Update();
  
    return EXIT_SUCCESS;
}