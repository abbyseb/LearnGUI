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

//#ifndef RTKGGOFUNCTIONS_H
//#define RTKGGOFUNCTIONS_H

#ifndef __rtkGgoFunctions_h
#define __rtkGgoFunctions_h

#include "rtkMacro.h"
#include "rtkConstantImageSource.h"
#include "rtkProjectionsReader.h"
#include "rtkVectorSorter.h"
#include <itkRegularExpressionSeriesFileNames.h>
#include <itksys/RegularExpression.hxx>

#include <numeric>

namespace rtk
{

/** \brief Create 3D image from gengetopt specifications.
 *
 * This function sets a ConstantImageSource object from command line options stored in ggo struct.
 *  The image is not buffered to allow streaming. The image is filled with zeros.
 *  The required options in the ggo struct are:
 *     - dimension: image size in pixels
 *     - spacing: image spacing in coordinate units
 *     - origin: image origin in coordinate units
 *
 * \author Simon Rit
 *
 * \ingroup Functions
 */
template< class TConstantImageSourceType, class TArgsInfo >
void
SetConstantImageSourceFromGgo(typename TConstantImageSourceType::Pointer source, const TArgsInfo &args_info)
{
  typedef typename TConstantImageSourceType::OutputImageType ImageType;

  const unsigned int Dimension = ImageType::GetImageDimension();

  typename ImageType::SizeType imageDimension;
  imageDimension.Fill(args_info.dimension_arg[0]);
  for(unsigned int i=0; i<vnl_math_min(args_info.dimension_given, Dimension); i++)
    imageDimension[i] = args_info.dimension_arg[i];

  typename ImageType::SpacingType imageSpacing;
  imageSpacing.Fill(args_info.spacing_arg[0]);
  for(unsigned int i=0; i<vnl_math_min(args_info.spacing_given, Dimension); i++)
    imageSpacing[i] = args_info.spacing_arg[i];

  typename ImageType::PointType imageOrigin;
  for(unsigned int i=0; i<Dimension; i++)
    imageOrigin[i] = imageSpacing[i] * (imageDimension[i]-1) * -0.5;
  for(unsigned int i=0; i<vnl_math_min(args_info.origin_given, Dimension); i++)
    imageOrigin[i] = args_info.origin_arg[i];

  typename ImageType::DirectionType imageDirection;
  if(args_info.direction_given)
    for(unsigned int i=0; i<Dimension; i++)
      for(unsigned int j=0; j<Dimension; j++)
        imageDirection[i][j] = args_info.direction_arg[i*Dimension+j];
  else
    imageDirection.SetIdentity();


  source->SetOrigin( imageOrigin );
  source->SetSpacing( imageSpacing );
  source->SetDirection( imageDirection );
  source->SetSize( imageDimension );
  source->SetConstant( 0. );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( source->UpdateOutputInformation() );
}

/** \brief Read a stack of 2D projections from gengetopt specifications.
 *
 * This function sets a ProjectionsReader object from command line options stored in ggo struct.
 * The projections are not buffered to allow streaming.
 * The required options in the ggo struct are:
 *     - verbose
 *     - path: path containing projections
 *     - regexp: regular expression to select projection files in path
 *     - nsort: boolean to (des-)activate the numeric sort for expression matches
 *     - submatch: index of the submatch that will be used to sort matches
 *
 * \author Simon Rit
 *
 * \ingroup Functions
 */
template< class TProjectionsReaderType, class TArgsInfo >
void
SetProjectionsReaderFromGgo(typename TProjectionsReaderType::Pointer reader, const TArgsInfo &args_info)
{
  // Generate file names
  itk::RegularExpressionSeriesFileNames::Pointer names = itk::RegularExpressionSeriesFileNames::New();
  names->SetDirectory(args_info.path_arg);
  names->SetNumericSort(args_info.nsort_flag);
  names->SetRegularExpression(args_info.regexp_arg);
  names->SetSubMatch(args_info.submatch_arg);

  if(args_info.verbose_flag)
    std::cout << "Regular expression matches "
              << names->GetFileNames().size()
              << " file(s)..."
              << std::endl;

  if (args_info.nsort_flag)
  {
	  std::vector<std::string> unsortedNames = names->GetFileNames();
	  std::vector<std::string> sortedFilenames;
	  std::vector<int> unsortedFN;
	  std::vector<int> idx(unsortedNames.size());
	  std::iota(idx.begin(), idx.end(), 0);
	  for (int k = 0; k != unsortedNames.size(); ++k)
	  {
		  int beginIdx_slash = unsortedNames[k].rfind('/');
		  int beginIdx_backslash = unsortedNames[k].rfind('\\');
		  int beginIdx;
		  if (beginIdx_slash >= 0 && beginIdx_backslash < 0)
			  beginIdx = beginIdx_slash;
		  else if (beginIdx_backslash >= 0 && beginIdx_slash < 0)
			  beginIdx = beginIdx_backslash;
		  else
			  beginIdx = std::max(beginIdx_slash, beginIdx_backslash);
		  int endIdx = unsortedNames[k].rfind('.');
		  std::string filename = unsortedNames[k].substr(beginIdx + 1, endIdx - beginIdx - 1);
		  std::stringstream filenoss(filename.substr(args_info.submatch_arg, filename.size() - args_info.submatch_arg));
		  int fileno;
		  filenoss >> fileno;
		  unsortedFN.push_back(fileno);
	  }
	  // sort indexes based on comparing values in v
	  sort(idx.begin(), idx.end(),
		  [&unsortedFN](int i1, int i2) {return unsortedFN[i1] < unsortedFN[i2]; });

	  for (int k = 0; k != unsortedNames.size(); ++k)
		  sortedFilenames.push_back(unsortedNames[idx[k]]);
	  reader->SetFileNames(sortedFilenames);
  }
  else
	reader->SetFileNames( names->GetFileNames() );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( reader->UpdateOutputInformation() );
}

/** \brief Return in all projection file names as a string vector.
 *     - verbose
 *     - path: path containing projections
 *     - regexp: regular expression to select projection files in path
 *     - nsort: boolean to (des-)activate the numeric sort for expression matches
 *     - submatch: index of the submatch that will be used to sort matches
 *
 * \author Andy Shieh
 *
 * \ingroup Functions
 */
template< class TProjectionsReaderType, class TArgsInfo>
void
SetProjectionsReaderFromGgo(typename TProjectionsReaderType::Pointer reader, const TArgsInfo &args_info, const std::vector<unsigned int> binVector)
{
  // Generate file names
  itk::RegularExpressionSeriesFileNames::Pointer names = itk::RegularExpressionSeriesFileNames::New();
  names->SetDirectory(args_info.path_arg);
  names->SetNumericSort(args_info.nsort_flag);
  names->SetRegularExpression(args_info.regexp_arg);
  names->SetSubMatch(args_info.submatch_arg);
  
  std::vector<std::string> sortedFilenames;
  if (args_info.nsort_flag)
  {
	  std::vector<std::string> unsortedNames = names->GetFileNames();
	  std::vector<int> unsortedFN;
	  std::vector<int> idx(unsortedNames.size());
	  std::iota(idx.begin(), idx.end(), 0);
	  for (int k = 0; k != unsortedNames.size(); ++k)
	  {
		  int beginIdx_slash = unsortedNames[k].rfind('/');
		  int beginIdx_backslash = unsortedNames[k].rfind('\\');
		  int beginIdx;
		  if (beginIdx_slash >= 0 && beginIdx_backslash < 0)
			  beginIdx = beginIdx_slash;
		  else if (beginIdx_backslash >= 0 && beginIdx_slash < 0)
			  beginIdx = beginIdx_backslash;
		  else
			  beginIdx = std::max(beginIdx_slash, beginIdx_backslash);
		  int endIdx = unsortedNames[k].rfind('.');
		  std::string filename = unsortedNames[k].substr(beginIdx + 1, endIdx - beginIdx - 1);
		  std::stringstream filenoss(filename.substr(args_info.submatch_arg, filename.size() - args_info.submatch_arg));
		  int fileno;
		  filenoss >> fileno;
		  unsortedFN.push_back(fileno);
	  }
	  // sort indexes based on comparing values in v
	  sort(idx.begin(), idx.end(),
		  [&unsortedFN](int i1, int i2) {return unsortedFN[i1] < unsortedFN[i2]; });

	  for (int k = 0; k != unsortedNames.size(); ++k)
		  sortedFilenames.push_back(unsortedNames[idx[k]]);
  }
  else
	  sortedFilenames = names->GetFileNames();

  // Sort filenames
  if(names->GetFileNames().size()==1)
  {
	  if(args_info.verbose_flag)
		std::cout << "Regular expression matches "
			      << names->GetFileNames().size()
                  << " file(s)..."
                  << std::endl;

	  // Pass list to projections reader
	  reader->SetFileNames(sortedFilenames);
  }
  else
  {
	  rtk::VectorSorter<std::string>::Pointer sorter = rtk::VectorSorter<std::string>::New();
	  sorter->SetInputVector(sortedFilenames);
	  sorter->SetBinNumberVector( binVector );
	  sorter->Sort();

	  if(args_info.verbose_flag)
		  std::cout << "Regular expression matches "
				    << names->GetFileNames().size()
					<< " file(s)..."
					<< "but only using a subset of "
					<< sorter->GetOutput()[args_info.binnumber_arg - 1].size()
					<< " file(s)..."
					<< std::endl;

	  // Pass list to projections reader
      reader->SetFileNames( sorter->GetOutput()[args_info.binnumber_arg - 1] );
  }
  TRY_AND_EXIT_ON_ITK_EXCEPTION( reader->UpdateOutputInformation() );
}

template< class TProjectionsReaderType, class TArgsInfo>
void
SetProjectionsReaderFromGgo(typename TProjectionsReaderType::Pointer reader, const TArgsInfo &args_info, const std::vector<unsigned int> binVector, unsigned int binNumber)
{
  // Generate file names
  itk::RegularExpressionSeriesFileNames::Pointer names = itk::RegularExpressionSeriesFileNames::New();
  names->SetDirectory(args_info.path_arg);
  names->SetNumericSort(args_info.nsort_flag);
  names->SetRegularExpression(args_info.regexp_arg);
  names->SetSubMatch(args_info.submatch_arg);

  std::vector<std::string> sortedFilenames;
  if (args_info.nsort_flag)
  {
	  std::vector<std::string> unsortedNames = names->GetFileNames();
	  std::vector<int> unsortedFN;
	  std::vector<int> idx(unsortedNames.size());
	  std::iota(idx.begin(), idx.end(), 0);
	  for (int k = 0; k != unsortedNames.size(); ++k)
	  {
		  int beginIdx_slash = unsortedNames[k].rfind('/');
		  int beginIdx_backslash = unsortedNames[k].rfind('\\');
		  int beginIdx;
		  if (beginIdx_slash >= 0 && beginIdx_backslash < 0)
			  beginIdx = beginIdx_slash;
		  else if (beginIdx_backslash >= 0 && beginIdx_slash < 0)
			  beginIdx = beginIdx_backslash;
		  else
			  beginIdx = std::max(beginIdx_slash, beginIdx_backslash);
		  int endIdx = unsortedNames[k].rfind('.');
		  std::string filename = unsortedNames[k].substr(beginIdx + 1, endIdx - beginIdx - 1);
		  std::stringstream filenoss(filename.substr(args_info.submatch_arg, filename.size() - args_info.submatch_arg));
		  int fileno;
		  filenoss >> fileno;
		  unsortedFN.push_back(fileno);
	  }
	  // sort indexes based on comparing values in v
	  sort(idx.begin(), idx.end(),
		  [&unsortedFN](int i1, int i2) {return unsortedFN[i1] < unsortedFN[i2]; });

	  for (int k = 0; k != unsortedNames.size(); ++k)
		  sortedFilenames.push_back(unsortedNames[idx[k]]);
  }
  else
	  sortedFilenames = names->GetFileNames();

  // Sort filenames
  if (sortedFilenames.size() == 1)
  {
	  if(args_info.verbose_flag)
		std::cout << "Regular expression matches "
		<< sortedFilenames.size()
                  << " file(s)..."
                  << std::endl;

	  // Pass list to projections reader
	  reader->SetFileNames(sortedFilenames);
  }
  else
  {
	  rtk::VectorSorter<std::string>::Pointer sorter = rtk::VectorSorter<std::string>::New();
	  sorter->SetInputVector(sortedFilenames);
	  sorter->SetBinNumberVector( binVector );
	  sorter->Sort();

	  if(args_info.verbose_flag)
		  std::cout << "Regular expression matches "
					<< sortedFilenames.size()
					<< " file(s)..."
					<< "but only using a subset of "
					<< sorter->GetOutput()[binNumber - 1].size()
					<< " file(s)..."
					<< std::endl;

	  // Pass list to projections reader
      reader->SetFileNames( sorter->GetOutput()[binNumber - 1] );
  }
  // TRY_AND_EXIT_ON_ITK_EXCEPTION( reader->UpdateOutputInformation() );
}

/** \brief Read a stack of 2D projections from gengetopt specifications.
 *
 * This function sets a ProjectionsReader object from command line options stored in ggo struct.
 * The projections are not buffered to allow streaming.
 * The required options in the ggo struct are:
 *     - verbose
 *     - path: path containing projections
 *     - regexp: regular expression to select projection files in path
 *     - nsort: boolean to (des-)activate the numeric sort for expression matches
 *     - submatch: index of the submatch that will be used to sort matches
 *
 * \author Simon Rit
 *
 * \ingroup Functions
 */
template< class TArgsInfo >
const std::vector< std::string >
GetProjectionsFileNamesFromGgo(const TArgsInfo &args_info)
{
  // Generate file names
  itk::RegularExpressionSeriesFileNames::Pointer names = itk::RegularExpressionSeriesFileNames::New();
  names->SetDirectory(args_info.path_arg);
  names->SetNumericSort(args_info.nsort_flag);
  names->SetRegularExpression(args_info.regexp_arg);
  names->SetSubMatch(args_info.submatch_arg);

  if(args_info.verbose_flag)
    std::cout << "Regular expression matches "
              << names->GetFileNames().size()
              << " file(s)..."
              << std::endl;

  std::vector<std::string> sortedFilenames;
  if (args_info.nsort_flag)
  {
	  std::vector<std::string> unsortedNames = names->GetFileNames();
	  std::vector<int> unsortedFN;
	  std::vector<int> idx(unsortedNames.size());
	  std::iota(idx.begin(), idx.end(), 0);
	  for (int k = 0; k != unsortedNames.size(); ++k)
	  {
		  int beginIdx_slash = unsortedNames[k].rfind('/');
		  int beginIdx_backslash = unsortedNames[k].rfind('\\');
		  int beginIdx;
		  if (beginIdx_slash >= 0 && beginIdx_backslash < 0)
			  beginIdx = beginIdx_slash;
		  else if (beginIdx_backslash >= 0 && beginIdx_slash < 0)
			  beginIdx = beginIdx_backslash;
		  else
			  beginIdx = std::max(beginIdx_slash, beginIdx_backslash);
		  int endIdx = unsortedNames[k].rfind('.');
		  std::string filename = unsortedNames[k].substr(beginIdx + 1, endIdx - beginIdx - 1);
		  std::stringstream filenoss(filename.substr(args_info.submatch_arg, filename.size() - args_info.submatch_arg));
		  int fileno;
		  filenoss >> fileno;
		  unsortedFN.push_back(fileno);
	  }
	  // sort indexes based on comparing values in v
	  sort(idx.begin(), idx.end(),
		  [&unsortedFN](int i1, int i2) {return unsortedFN[i1] < unsortedFN[i2]; });

	  for (int k = 0; k != unsortedNames.size(); ++k)
		  sortedFilenames.push_back(unsortedNames[idx[k]]);
  }
  else
	  sortedFilenames = names->GetFileNames();

  return sortedFilenames;
}

}

#endif // __rtkGgoFunctions_h
