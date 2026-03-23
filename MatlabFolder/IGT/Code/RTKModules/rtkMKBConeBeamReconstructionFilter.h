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

#ifndef __rtkMKBConeBeamReconstructionFilter_h
#define __rtkMKBConeBeamReconstructionFilter_h

#include "rtkFDKConeBeamReconstructionFilter.h"
#include "rtkForwardProjectionImageFilter.h"
#include "rtkParkerShortScanImageFilter.h"
#include "rtkDisplacedDetectorImageFilter.h"

#ifdef IGT_USE_CUDA
	#include "rtkCudaFDKConeBeamReconstructionFilter.h"
	#include "rtkCudaParkerShortScanImageFilter.h"
	#include "rtkCudaDisplacedDetectorImageFilter.h"
#endif

#include <itkExtractImageFilter.h>
#include <itkMultiplyImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkTimeProbe.h>
#include <itkThresholdImageFilter.h>

#include "rtkIterativeConeBeamReconstructionFilter.h"

namespace rtk
{

/** \class MKBConeBeamReconstructionFilter
 * \brief Implements the Mckinnon-Bates method. [Mckinnon & Bates, 1981]
 *
 * \author Andy Shieh
 *
 * \ingroup ReconstructionAlgorithm
 */
template<class TInputImage, class TOutputImage=TInputImage>
class ITK_EXPORT MKBConeBeamReconstructionFilter :
  public rtk::IterativeConeBeamReconstructionFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef MKBConeBeamReconstructionFilter                                  Self;
  typedef IterativeConeBeamReconstructionFilter<TInputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>                                          Pointer;
  typedef itk::SmartPointer<const Self>                                    ConstPointer;

  /** Some convenient typedefs. */
  typedef TInputImage  InputImageType;
  typedef TOutputImage OutputImageType;

  /** Typedefs of each subfilter of this composite filter */
  typedef rtk::ForwardProjectionImageFilter< OutputImageType, OutputImageType >              ForwardProjectionFilterType;
  typedef rtk::FDKConeBeamReconstructionFilter< OutputImageType >							 FDKReconstructionFilterType;
  typedef itk::ExtractImageFilter<OutputImageType, OutputImageType>                 		 ExtractFilterType;
  typedef itk::MultiplyImageFilter< OutputImageType, OutputImageType, OutputImageType >      MultiplyFilterType;
  typedef itk::SubtractImageFilter< OutputImageType, OutputImageType >                       SubtractFilterType;
  typedef itk::ThresholdImageFilter<OutputImageType>                                         ThresholdFilterType;
  typedef rtk::ParkerShortScanImageFilter< OutputImageType >								 ParkerWeightingFilterType;
  typedef rtk::DisplacedDetectorImageFilter< OutputImageType >								 DisplacedDetectorFilterType;

#ifdef IGT_USE_CUDA
  typedef rtk::CudaFDKConeBeamReconstructionFilter										     FDKReconstructionCUDAFilterType;
  typedef rtk::CudaParkerShortScanImageFilter												 ParkerWeightingCUDAFilterType;
  typedef rtk::CudaDisplacedDetectorImageFilter												 DisplacedDetectorCUDAFilterType;
#endif

/** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(MKBConeBeamReconstructionFilter, IterativeConeBeamReconstructionFilter);

  /** Get / Set the object pointer to projection geometry */
  itkGetMacro(Geometry, ThreeDCircularProjectionGeometry::Pointer);
  itkSetMacro(Geometry, ThreeDCircularProjectionGeometry::Pointer);
  
  /** Set/Get the positivity threshold tag */
  itkSetMacro(PositivityThreshold, bool);
  itkGetConstMacro(PositivityThreshold, bool);
  void SetPositivityThresholdOn() { this->SetPositivityThreshold( true ); }
  void SetPositivityThresholdOff() { this->SetPositivityThreshold( false ); }
  
  /** Set/Get the scaling factor */
  itkSetMacro(ScalingFactor, double);
  itkGetConstMacro(ScalingFactor, double);

  void PrintTiming(std::ostream& os) const;

  /** Select the ForwardProjection filter */
  void SetForwardProjectionFilter (int _arg);

  /** Set FDK reconstruction filter */
  void SetFDKReconstructionFilter( const char * _arg );

  /** Instantiate parker short scan weighting filter */
  void InstantiateParkerWeightingFilter( );

  /** Instantiate displaced detector filter */
  void InstantiateDisplacedDetectorFilter( );

  /** Get pointer to the fdk filter */
  typename FDKReconstructionFilterType::Pointer GetFDKReconstructionCPUFilter() { return m_FDKReconstructionCPUFilter; }

#ifdef IGT_USE_CUDA
  void InstantiateParkerWeightingCUDAFilter ();
  void InstantiateDisplacedDetectorCUDAFilter ();
  typename FDKReconstructionCUDAFilterType::Pointer GetFDKReconstructionCUDAFilter() { return m_FDKReconstructionCUDAFilter; }
#endif

protected:
  MKBConeBeamReconstructionFilter();
  ~MKBConeBeamReconstructionFilter(){}

  virtual void GenerateInputRequestedRegion();

  virtual void GenerateOutputInformation();

  virtual void GenerateData();

  /** The two inputs should not be in the same space so there is nothing
   * to verify. */
  virtual void VerifyInputInformation() {}

  /** Pointers to each subfilter of this composite filter */
  typename ForwardProjectionFilterType::Pointer  m_ForwardProjectionFilter;
  typename FDKReconstructionFilterType::Pointer	 m_FDKReconstructionCPUFilter;
  typename ExtractFilterType::Pointer			 m_ExtractImageFilter;
  typename MultiplyFilterType::Pointer			 m_ZeroMultiplyFilter;
  typename MultiplyFilterType::Pointer			 m_ScalingMultiplyFilter;
  typename SubtractFilterType::Pointer           m_SubtractFilter;
  typename ThresholdFilterType::Pointer          m_ThresholdFilter;
  typename DisplacedDetectorFilterType::Pointer  m_DisplacedDetectorFilter;
  typename ParkerWeightingFilterType::Pointer	 m_ParkerWeightingFilter;
  
#ifdef IGT_USE_CUDA
  typename FDKReconstructionCUDAFilterType::Pointer		 m_FDKReconstructionCUDAFilter;
#endif

private:
  //purposely not implemented
  MKBConeBeamReconstructionFilter(const Self&);
  void operator=(const Self&);

  /** Geometry object */
  ThreeDCircularProjectionGeometry::Pointer m_Geometry;

  /** FDK method type*/
  const char * m_FDKType;
  
  bool m_PositivityThreshold;
  
  double m_ScalingFactor;

  /** Time probes */
  itk::TimeProbe m_ExtractImageProbe;
  itk::TimeProbe m_MKBReconstructionProbe;

}; // end of class

} // end namespace rtk

#ifndef ITK_MANUAL_INSTANTIATION
#include "rtkMKBConeBeamReconstructionFilter.txx"
#endif

#endif
