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

#include "igtsimulatedgeometry_ggo.h"
#include "rtkGgoFunctions.h"

#include "rtkThreeDCircularProjectionGeometryXMLFile.h"

int main(int argc, char * argv[])
{
  GGO(igtsimulatedgeometry, args_info);
  
  // RTK geometry object
  typedef rtk::ThreeDCircularProjectionGeometry GeometryType;
  GeometryType::Pointer geometry = GeometryType::New();

  unsigned int NProj = args_info.nproj_arg;
  std::vector<double> angles;
  // Read angle file if provided
  if (args_info.angle_file_given)
  {
	  std::ifstream is(args_info.angle_file_arg);
	  if( !is.is_open() )
		  std::cerr << "Could not open angle file " << args_info.angle_file_arg;

      double value;
	  is >> value;
      while( !is.eof() )
      {
		angles.push_back(value);
		is >> value;
      }
	  NProj = angles.size();
  }
  else
  {
	  for (int noProj = 0; noProj < NProj; noProj++)
		  angles.push_back(args_info.first_angle_arg + noProj * args_info.arc_arg / args_info.nproj_arg);
  }

  // Read detector displacement file
  std::vector<double> detdisX, detdisY;
  if (args_info.detdis_file_given)
  {
	  std::ifstream infile(args_info.detdis_file_arg);
	  if( !infile.is_open() )
		  std::cerr << "Could not open detector displacement file " << args_info.detdis_file_arg;
	  
	  while (infile)
	  {
		  std::string s;
		  if (!getline( infile, s )) break;

		  std::istringstream ss( s );
		  std::vector <std::string> record;

		  while (ss)
		  {
			  std::string part;
			  if (!getline( ss, part, ',' )) break;
			  record.push_back( part );
		  }
		  detdisX.push_back(atof(record[0].c_str()));
		  detdisY.push_back(atof(record[1].c_str()));
	  }
	  if (NProj != detdisX.size() || NProj != detdisY.size())
		  std::cerr << "The length of the detector displacement file does not match with the number of angular values";
  }
  else
  {
	  for (int noProj = 0; noProj < NProj; noProj++)
	  {
	      detdisX.push_back(args_info.proj_iso_x_arg);
		  detdisY.push_back(args_info.proj_iso_y_arg);
	  }
  }
  
  // Projection matrices
  for(int noProj=0; noProj<NProj; noProj++)
    {
    geometry->AddProjection(args_info.sid_arg,
                            args_info.sdd_arg,
							angles[noProj],
                            detdisX[noProj],
                            detdisY[noProj],
                            args_info.out_angle_arg,
                            args_info.in_angle_arg,
                            args_info.source_x_arg,
                            args_info.source_y_arg);
    }

  // Write
  rtk::ThreeDCircularProjectionGeometryXMLFileWriter::Pointer xmlWriter =
    rtk::ThreeDCircularProjectionGeometryXMLFileWriter::New();
  xmlWriter->SetFilename(args_info.output_arg);
  xmlWriter->SetObject(&(*geometry) );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( xmlWriter->WriteFile() )

  return EXIT_SUCCESS;
}
