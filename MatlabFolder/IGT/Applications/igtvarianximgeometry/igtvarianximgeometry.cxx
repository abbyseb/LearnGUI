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

#include "igtvarianximgeometry_ggo.h"
#include "rtkMacro.h"
#include "igtVarianXimGeometryReader.h"
#include "rtkThreeDCircularProjectionGeometryXMLFile.h"

#include <itkRegularExpressionSeriesFileNames.h>

int main(int argc, char * argv[])
{
  GGO(igtvarianximgeometry, args_info);

  // Generate file names of projections
  itk::RegularExpressionSeriesFileNames::Pointer names = itk::RegularExpressionSeriesFileNames::New();
  names->SetDirectory(args_info.path_arg);
  names->SetNumericSort(args_info.nsort_flag);
  names->SetRegularExpression(args_info.regexp_arg);
  names->SetSubMatch(args_info.submatch_arg);

  // Create geometry reader
  igt::VarianXimGeometryReader::Pointer reader;
  reader = igt::VarianXimGeometryReader::New();
  reader->SetXMLFileName(args_info.xml_file_arg);
  reader->SetProjectionsFileNames(names->GetFileNames());

  // If a separate angle file is provided, use it to override the kV angles
  std::vector<double> angles;
  // Read angle file if provided
  if (args_info.angle_file_given)
  {
	  std::ifstream is(args_info.angle_file_arg);
	  if (!is.is_open())
		  std::cerr << "Could not open angle file " << args_info.angle_file_arg;

	  double value;
	  is >> value;
	  while (!is.eof())
	  {
		  angles.push_back(value);
		  is >> value;
	  }
	  if (angles.size() != names->GetFileNames().size())
		  std::cerr << "The length of the angle file does not match with the number of projections.";

	  reader->SetAnglesToOverride(angles);
  }

  TRY_AND_EXIT_ON_ITK_EXCEPTION( reader->UpdateOutputData() );

  // Write
  rtk::ThreeDCircularProjectionGeometryXMLFileWriter::Pointer xmlWriter =
    rtk::ThreeDCircularProjectionGeometryXMLFileWriter::New();
  xmlWriter->SetFilename(args_info.output_arg);
  xmlWriter->SetObject( reader->GetGeometry() );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( xmlWriter->WriteFile() )

  return EXIT_SUCCESS;
}
