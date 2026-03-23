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
#ifndef __rtkBinNumberReader_h
#define __rtkBinNumberReader_h

#include <itkCSVFileReaderBase.h>

namespace rtk
{

/** \class BinNumberReader
 * \brief Parses csv file containing the bin number for projection sorting. Andy Shieh 2014
 *        The format of the bin number csv is just a single column without any header.
 *		  Bin numbers are in unsigned short, with zero indicating unused projections.
 */

class ITK_EXPORT BinNumberReader:public itk::CSVFileReaderBase
{
public:
    /** Standard class typedefs */
    typedef BinNumberReader                 Self;
    typedef itk::CSVFileReaderBase          Superclass;
    typedef itk::SmartPointer<Self>         Pointer;
    typedef itk::SmartPointer<const Self>   ConstPointer;

    /** Standard New method. */
    itkNewMacro(Self)

    /** Run-time type information (and related methods) */
	itkTypeMacro(BinNumberReader, itk::CSVFileReaderBase)

    /** Parses the data from the file. Gets the bin numbers of the projections
  * into a vector, then generate an Array2D object containing the interpolation weights  */
    void Parse();

    /** Aliased to the Parse() method to be consistent with the rest of the
   * pipeline. */
    virtual void Update();

    /** Aliased to the GetDataFrameObject() method to be consistent with the
   *  rest of the pipeline */
    virtual std::vector<unsigned int> GetOutput();

	// Get number of respiratory bins
	unsigned int GetMaxBinNumber();

protected:

    BinNumberReader();
	~BinNumberReader() {};

    /** Print the reader. */
    void PrintSelf(std::ostream & os, itk::Indent indent) const;

private:

    std::vector<unsigned int> m_BinNumbers;

    BinNumberReader(const Self &);  //purposely not implemented
    void operator=(const Self &);          //purposely not implemented
};

} //end namespace rtk

#ifndef ITK_MANUAL_INSTANTIATION
#include "rtkBinNumberReader.txx"
#endif

#endif
