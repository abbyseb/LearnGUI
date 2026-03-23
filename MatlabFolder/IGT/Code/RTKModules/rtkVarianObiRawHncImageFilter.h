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

#ifndef __itkVarianObiRawHncImageFilter_h
#define __itkVarianObiRawHncImageFilter_h

#include "itkUnaryFunctorImageFilter.h"
#include "itkConceptChecking.h"
#include <itkNumericTraits.h>

#define HNC_INTENSITY_MAX (65535)
#define HNC_NORMCHAMBER_FACTOR (218.45)

namespace rtk
{
 
namespace Function {  

/** \class HncObiAttenuation
 * \brief Converts a raw value from the HNC format measured by the Varian OBI system to attenuation
 *
 * The current implementation assues a norm chamber factor of 300.
 *
 * \author Geoff Hugo, VCU 
 * \ (later modified by Andy Shieh for use in RPL. HNC_NORMCHAMBER_FACTOR * m_NormChamberValue = IntensityMax)
 *
 * \ingroup Functions
 */
 
  
template< class TInput, class TConstant, class TOutput>
class HncObiAttenuation 
{
public:
  HncObiAttenuation() : m_NormChamberValue(HNC_INTENSITY_MAX / HNC_NORMCHAMBER_FACTOR) {};
  ~HncObiAttenuation() {};
  
  bool operator!=( const HncObiAttenuation & other ) const
    {
    return !(*this == other);
    }
  bool operator==( const HncObiAttenuation & other ) const
    {
    return other.m_NormChamberValue == m_NormChamberValue;
    }
  inline TOutput operator()( const TInput & A ) const
    {
    TOutput output = A;
	if (A >= HNC_NORMCHAMBER_FACTOR * m_NormChamberValue)
	  {
	  output = 0;
	  }
	else
	  {
      if (A != itk::NumericTraits<TInput>::ZeroValue())
        {
        output = vcl_log( HNC_NORMCHAMBER_FACTOR * m_NormChamberValue / output ); // OutputImageType in rtkfdk must be floating point    
        }
      else
	    {
		output = vcl_log( HNC_NORMCHAMBER_FACTOR * m_NormChamberValue );
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
	}
	
  const TConstant & GetNormChamberValue() const { return m_NormChamberValue; }	
	
  TConstant m_NormChamberValue;
}; 

}

template <class TInputImage, class TOutputImage>
class ITK_EXPORT VarianObiRawHncImageFilter :
    public
itk::UnaryFunctorImageFilter<TInputImage,TOutputImage, 
                        Function::HncObiAttenuation<
  typename TInputImage::PixelType,
  typename TOutputImage::PixelType,
  typename TOutputImage::PixelType>   >
{
public:
  /** Standard class typedefs. */
  typedef VarianObiRawHncImageFilter  Self;
  typedef itk::UnaryFunctorImageFilter<TInputImage,TOutputImage, 
                                  Function::HncObiAttenuation< typename TInputImage::PixelType,
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
  itkTypeMacro(VarianObiRawHncImageFilter,
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
  VarianObiRawHncImageFilter() {};
  virtual ~VarianObiRawHncImageFilter() {};
  
private:
  VarianObiRawHncImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

};

} // end namespace itk

#endif
