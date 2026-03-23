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

#ifndef __igtLinearCombinationFilter_h
#define __igtLinearCombinationFilter_h

#include <itkImageToImageFilter.h>
#include <itkImageScanlineConstIterator.h>
#include <itkImageScanlineIterator.h>

namespace igt
{

/** \class LinearCombinationFilter
 * \brief A Hybrid ITerative CBCT reconstruction method
 *
 * \author Andy Shieh 2017
 *
 * \ingroup ReconstructionAlgorithm
 */
template<class TInputImage, class TOutputImage=TInputImage>
class ITK_EXPORT LinearCombinationFilter :
  public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef LinearCombinationFilter							 Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>                            Pointer;
  typedef itk::SmartPointer<const Self>                      ConstPointer;

  /** Convenient typedefs */
  typedef typename itk::SmartPointer<const TInputImage>		 ConstInputImagePointer;
  typedef typename itk::ImageScanlineConstIterator<TInputImage>	ConstIteratorType;
  typedef typename itk::ImageScanlineIterator<TInputImage>	    IteratorType;
  
  /** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(LinearCombinationFilter, itk::ImageToImageFilter);

  /** Add inputs.
      Input image pointers can be null, in which case the multiplier will be added
      as a constant on top. The first input image pointer must not be null */
  void AddInput(ConstInputImagePointer image, double multiplier)
  {
	  this->m_InputImages.push_back(image);
	  this->m_Multipliers.push_back(multiplier);
	  this->Modified();
  }

  /** Clear all inputs */
  void ClearInputs()
  {
	  this->m_InputImages.clear();
	  this->m_Multipliers.clear();
	  this->Modified();
  }

  void InputsModified(){ this->Modified(); }

  /** Get input image */
  ConstInputImagePointer GetInputImage(unsigned int index)
  {
	  if (this->m_InputImages.size() < index)
		  std::cerr << "InputImage #" << index << " is non-existent." << std::endl;
	  return this->m_InputImages[index];
  }

  /** Set multiplier. */
  void SetMultiplier(unsigned int index, double val)
  {
	  if (this->m_Multipliers.size() < index)
		  std::cerr << "Multiplier #" << index << " is non-existent." << std::endl;
	  this->m_Multipliers[index] = val;
  }

  /** Get multiplier */
  double GetMultiplier(unsigned int index)
  {
	  if (this->m_Multipliers.size() < index)
		  std::cerr << "Multiplier #" << index << " is non-existent." << std::endl;
	  return this->m_Multipliers[index];
  }

  /** Get number of input images */
  unsigned int NInputs()
  { return this->m_InputImages.size(); }

protected:
  LinearCombinationFilter(){this->SetNumberOfRequiredInputs(0);};
  ~LinearCombinationFilter(){}

  virtual void GenerateInputRequestedRegion();
  virtual void GenerateOutputInformation();
  virtual void BeforeThreadedGenerateData();
  virtual void ThreadedGenerateData(const typename TInputImage::RegionType& outputRegionForThread,
									typename itk::ThreadIdType threadID);
  virtual void VerifyInputInformation() {}

  std::vector<ConstInputImagePointer> m_InputImages;
  std::vector<double> m_Multipliers;

private:
  //purposely not implemented
  LinearCombinationFilter(const Self&);
  void operator=(const Self&);
}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtLinearCombinationFilter.txx"
#endif

#endif
