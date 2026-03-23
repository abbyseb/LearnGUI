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

#ifndef __itkVarianObiRawXimImageFilter_h
#define __itkVarianObiRawXimImageFilter_h

#include "itkUnaryFunctorImageFilter.h"
#include "itkConceptChecking.h"
#include <itkNumericTraits.h>

namespace igt
{
 
namespace Function {  

/** \class XimObiAttenuation
 * \brief Converts a raw value from the Xim format measured by the Varian OBI system to attenuation
 *
 * \author Andy Shieh, The University of Sydney
 *
 * \ingroup Functions
 */
 
  
template< class TInput, class TConstant, class TOutput>
class XimObiAttenuation 
{
public:
  XimObiAttenuation() : m_NormChamberValue(65535) {};
  ~XimObiAttenuation() {};
  
  bool operator!=( const XimObiAttenuation & other ) const
    {
    return !(*this == other);
    }
  bool operator==( const XimObiAttenuation & other ) const
    {
    return other.m_NormChamberValue == m_NormChamberValue;
    }
  inline TOutput operator()( const TInput & A ) const
    {
    TOutput output = A;
	if (A >= m_NormChamberValue)
	  {
	  output = 0;
	  }
	else
	  {
      if (A != itk::NumericTraits<TInput>::ZeroValue())
        {
        output = m_LogNormChamberValue - vcl_log( (double)output ); // OutputImageType in rtkfdk must be floating point    
        }
      else
	    {
		output = m_LogNormChamberValue;
		}
	  }
    return output;
    }
	
  void SetNormChamberValue(TConstant i)
    {
	if(i == itk::NumericTraits<TConstant>::Zero )
	  {
	  itkGenericExceptionMacro(<<"The intensity max value should not be set to zero");
	  }
	this->m_NormChamberValue = i;
	m_LogNormChamberValue = vcl_log((double)m_NormChamberValue);
	}
	
  const TConstant & GetNormChamberValue() const { return m_NormChamberValue; }	
	
  TConstant m_NormChamberValue;
  double    m_LogNormChamberValue;
}; 

}

template <class TInputImage, class TOutputImage>
class ITK_EXPORT VarianObiRawXimImageFilter :
    public
itk::UnaryFunctorImageFilter<TInputImage,TOutputImage, 
                        Function::XimObiAttenuation<
  typename TInputImage::PixelType,
  typename TOutputImage::PixelType,
  typename TOutputImage::PixelType>   >
{
public:
  /** Standard class typedefs. */
  typedef VarianObiRawXimImageFilter  Self;
  typedef itk::UnaryFunctorImageFilter<TInputImage,TOutputImage, 
                                  Function::XimObiAttenuation< typename TInputImage::PixelType,
												 typename TOutputImage::PixelType,
												 typename TOutputImage::PixelType> >  Superclass;
  typedef itk::SmartPointer<Self>        Pointer;
  typedef itk::SmartPointer<const Self>  ConstPointer;
  
  /** Some handy typedefs */
  typedef typename TOutputImage::PixelType ConstantType;
  
  typedef TInputImage                              InputImageType;
  
  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(VarianObiRawXimImageFilter,
               UnaryFunctorImageFilter);
			   
  void SetNormChamberValue(ConstantType ct)
     {
     if( ct != this->GetFunctor().GetNormChamberValue() )
       {
       this->GetFunctor().SetNormChamberValue(ct);
       this->Modified();
       }
     }
   
   const ConstantType & GetNormChamberValue() const
     {
     return this->GetFunctor().GetNormChamberValue();
     }			   

// define BeforeThreadedGenerateData rather than use SuperClass since
// we need to set the IntensityMax after the input image's metadata
// is available.  Since we only need the metadata for pixel data,
// use this method rather than GenerateOutputInformation
protected:
  VarianObiRawXimImageFilter() {};
  virtual ~VarianObiRawXimImageFilter() {};
  
private:
  VarianObiRawXimImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

};

} // end namespace itk

#endif
