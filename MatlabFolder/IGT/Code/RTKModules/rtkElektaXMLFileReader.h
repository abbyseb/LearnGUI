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

#ifndef __rtkElektaXMLFileReader_h
#define __rtkElektaXMLFileReader_h

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif

#include <itkXMLFile.h>
#include <itkMetaDataDictionary.h>
#include <itkMetaDataObject.h>

namespace rtk
{

/** \class ElektaXMLFileReader
 *
 * Reads the XML-format file written by a Varian OBI
 * machine for every acquisition
 */
class ElektaXMLFileReader : public itk::XMLReader<itk::MetaDataDictionary>
{
public:
  /** Standard typedefs */
  typedef ElektaXMLFileReader                  Self;
  typedef itk::XMLReader<itk::MetaDataDictionary> Superclass;
  typedef itk::SmartPointer<Self>                 Pointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(ElektaXMLFileReader, itk::XMLReader);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Determine if a file can be read */
  int CanReadFile(const char* name);
  
  itkGetMacro(GantryAngles, std::vector< double >);
  itkGetMacro(UCenters, std::vector< double >);
  itkGetMacro(VCenters, std::vector< double >);
  // Andy Shieh 2015
  itkGetMacro(SID, double);
  itkGetMacro(SDD, double);

protected:
  ElektaXMLFileReader(){m_OutputObject = &m_Dictionary; m_SID = 1000.; m_SDD = 1536.;};
  virtual ~ElektaXMLFileReader() {};

  virtual void StartElement(const char * name,const char **atts);

  virtual void EndElement(const char *name);

  void CharacterDataHandler(const char *inData, int inLength);

private:
  ElektaXMLFileReader(const Self&); //purposely not implemented
  void operator=(const Self&);         //purposely not implemented

  itk::MetaDataDictionary m_Dictionary;
  std::string             m_CurCharacterData;
  std::vector< double >	  m_GantryAngles;
  std::vector< double >   m_UCenters;
  std::vector< double >   m_VCenters;
  // Andy Shieh 2015
  double				  m_SID, m_SDD;
};

}
#endif
