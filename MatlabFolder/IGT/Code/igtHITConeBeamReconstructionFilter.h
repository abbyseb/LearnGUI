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

#ifndef __igtHITConeBeamReconstructionFilter_h
#define __igtHITConeBeamReconstructionFilter_h

#include "rtkBackProjectionImageFilter.h"
#include "rtkForwardProjectionImageFilter.h"

#include <itkMultiplyImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkAddImageFilter.h>
#include <itkTimeProbe.h>
#include <itkStatisticsImageFilter.h>
#include <itkLaplacianImageFilter.h>

#include "rtkConstantImageSource.h"
#include "rtkIterativeConeBeamReconstructionFilter.h"
#include "rtkDisplacedDetectorImageFilter.h"
#include "rtkTotalVariationImageFilter.h"
#include "rtkTotalVariationGradientImageFilter.h"
#include "rtkTotalVariationHessianOperatorImageFilter.h"

namespace igt
{

/** \class HITConeBeamReconstructionFilter
 * \brief A Hybrid ITerative CBCT reconstruction method
 *
 * \author Andy Shieh 2017
 *
 * \ingroup ReconstructionAlgorithm
 */
template<class TInputImage, class TOutputImage=TInputImage>
class ITK_EXPORT HITConeBeamReconstructionFilter :
  public rtk::IterativeConeBeamReconstructionFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef HITConeBeamReconstructionFilter                                  Self;
  typedef rtk::IterativeConeBeamReconstructionFilter<TInputImage, TOutputImage> Superclass;
  typedef itk::SmartPointer<Self>                                          Pointer;
  typedef itk::SmartPointer<const Self>                                    ConstPointer;

  /** Some convenient typedefs. */
  typedef TInputImage  InputImageType;
  typedef TOutputImage OutputImageType;
  typedef typename InputImageType::Pointer	InputImagePointerType;
  
  /** Vector image type for W */
  typedef itk::VectorImage< typename OutputImageType::PixelType, TOutputImage::ImageDimension > VectorImageType;
  typedef typename VectorImageType::Pointer VectorImagePointer;

  /** Typedefs of each subfilter of this composite filter */
  typedef itk::StatisticsImageFilter< OutputImageType >										 ImageStatisticsFilterType;
  typedef rtk::ForwardProjectionImageFilter< OutputImageType, OutputImageType >              ForwardProjectionFilterType;
  typedef rtk::BackProjectionImageFilter< OutputImageType, OutputImageType >                 BackProjectionFilterType;
  typedef itk::AddImageFilter< OutputImageType, OutputImageType >                       	 AddFilterType;
  typedef itk::SubtractImageFilter< OutputImageType, OutputImageType >                       SubtractFilterType;
  typedef itk::MultiplyImageFilter< OutputImageType, OutputImageType, OutputImageType >      MultiplyFilterType;
  typedef rtk::DisplacedDetectorImageFilter<InputImageType>                                  DisplacedDetectorFilterType;
  typedef typename DisplacedDetectorFilterType::Pointer                                		 DisplacedDetectorPointerType;
  typedef rtk::TotalVariationImageFilter<OutputImageType>									 TVFilterType;
  typedef typename TVFilterType::Pointer													 TVFilterPointerType;
  typedef rtk::TotalVariationGradientImageFilter<OutputImageType, OutputImageType>			 TVGradientFilterType;
  typedef typename TVGradientFilterType::Pointer											 TVGradientPointerType;
  typedef rtk::TotalVariationHessianOperatorImageFilter<OutputImageType, OutputImageType>	 TVHessianFilterType;
  typedef typename TVHessianFilterType::Pointer												 TVHessianPointerType;

  typedef itk::LaplacianImageFilter<OutputImageType, OutputImageType>						 LaplacianFilterType;
  typedef typename LaplacianFilterType::Pointer												 LaplacianPointerType;

/** Standard New method. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(HITConeBeamReconstructionFilter, IterativeConeBeamReconstructionFilter);
  
  /** Get / Set the object pointer to projection geometry */
  itkGetMacro(Geometry, rtk::ThreeDCircularProjectionGeometry::Pointer);
  itkSetMacro(Geometry, rtk::ThreeDCircularProjectionGeometry::Pointer);

  void PrintTiming(std::ostream& os) const;

  /** Get / Set the number of iterations. Default is 20. */
  itkGetMacro(NumberOfIterations, unsigned int);
  itkSetMacro(NumberOfIterations, unsigned int);
  
  /** Get / Set verbosity. Default is false. */
  itkGetMacro(Verbose, bool);
  itkSetMacro(Verbose, bool);

  void SetVerboseOn()
  { this->SetVerbose(true); }

  void SetVerboseOff()
  { this->SetVerbose(false); }

  /** Get / Set regularization parameter Lambda. Default is 1. */
  itkGetMacro(Lambda, double);
  itkSetMacro(Lambda, double);
  
  /** Get / Set prior image constraint parameter Alpha. Default is 0.5. */
  itkGetMacro(Alpha, double);
  itkSetMacro(Alpha, double);
  
  /** Get / Set the normal L2 norm vs gradient L2 norm factor. Default is 0.05 */
  itkGetMacro(C, double);
  itkSetMacro(C, double);
  
  /** Get / Set C1, C2. Default is C1 = 1e-4, C2 = 0.5 */
  itkGetMacro(C1, double);
  itkSetMacro(C1, double);
  itkGetMacro(C2, double);
  itkSetMacro(C2, double);
  
  /** Get / Set residual and objective function vectors */
  itkGetMacro(ResidualVectors, std::vector<double>);
  itkGetMacro(ObjectiveVectors, std::vector<double>);
  
  /** Get / Set TV scaling factor */
  itkGetMacro(TVScalingFactor, double);
  itkSetMacro(TVScalingFactor, double);
  
  /** Get / Set stopping criterion */
  itkGetMacro(StoppingCriterion, double);
  itkSetMacro(StoppingCriterion, double);
  
  /** Set/Get the weighting image - Andy */
  itkSetMacro(TVWeightingImage, VectorImagePointer);
  itkGetConstMacro(TVWeightingImage, VectorImagePointer);
  itkSetMacro(PriorWeightingImage, VectorImagePointer);
  itkGetConstMacro(PriorWeightingImage, VectorImagePointer);
  
  /** Accessors to TV filter types (CPU or Cuda). */
  virtual void SetTVFilter (int tvtype);
  int GetTVFilter () { return m_CurrentTVConfiguration; }  

  /** Accessors to TV gradient filter types (CPU or Cuda). */
  virtual void SetTVGradientFilter (int tvgradtype);
  int GetTVGradientFilter () { return m_CurrentTVGradientConfiguration; }  
  
  /** Accessors to TV gradient filter types (CPU or Cuda). */
  virtual void SetTVHessianFilter (int tvhessiantype);
  int GetTVHessianFilter () { return m_CurrentTVHessianConfiguration; } 
  
  /** Accessors to displaced detector filter types (CPU or Cuda). */
  virtual void SetDisplacedDetectorFilter (int ddtype);
  int GetDisplacedDetectorFilter () { return m_CurrentDisplacedDetectorConfiguration; } 
  
  /** Select the ForwardProjection filter */
  void SetForwardProjectionFilter (int _arg);

  /** Select the backprojection filter */
  void SetBackProjectionFilter (int _arg);
  
  /** Set global maximum number of threads for multi thread methods, to prevent crashes sometime */
  void SetMaxNumberOfThreads( int _arg) { itk::MultiThreader::SetGlobalMaximumNumberOfThreads( _arg ); }

protected:
  HITConeBeamReconstructionFilter();
  ~HITConeBeamReconstructionFilter(){}

  virtual void GenerateInputRequestedRegion();

  virtual void GenerateOutputInformation();

  virtual void GenerateData();

  virtual TVFilterPointerType InstantiateTVFilter(int tvtype);
  
  virtual TVGradientPointerType InstantiateTVGradientFilter(int tvgradtype);

  virtual TVHessianPointerType InstantiateTVHessianFilter(int tvhessiantype);

  virtual DisplacedDetectorPointerType InstantiateDisplacedDetectorFilter(int ddtype);

  /** The two inputs should not be in the same space so there is nothing
   * to verify. */
  virtual void VerifyInputInformation() {}

  /** Pointers to each subfilter */
  
  /** Zero multiply filter */
  typename MultiplyFilterType::Pointer           	m_ZeroMultiplyFilterP, m_ZeroMultiplyFilterX;
  
  /** s = Rx - p = s_old + eta*Rd */
  // For the first iteration, eta = 1, s_old = -p, Rd = Rx0
  typename SubtractFilterType::Pointer				m_MinusP_SubtractFilter;
  typename ForwardProjectionFilterType::Pointer		m_Rd_FPFilter;
  typename MultiplyFilterType::Pointer				m_Rd_MultiplyFilter;
  typename AddFilterType::Pointer					m_s_AddFilter;
  
  /** Calculate norm(s)^2 */
  typename ImageStatisticsFilterType::Pointer		m_sNormSquared_StatsFilter;
  
  /** X - X_p term */
  typename SubtractFilterType::Pointer				m_Xp_SubtractFilter;
  
  /** Calculate TV(x) and TV(x-xp) */
  typename TVFilterType::Pointer					m_TVX_TVFilter;
  typename TVFilterType::Pointer					m_TVXp_TVFilter;
  
  /** g, Data fidelity term R^Ts */
  typename DisplacedDetectorFilterType::Pointer		m_RTs_DDFilter;
  typename BackProjectionFilterType::Pointer		m_RTs_BPFilter;
  
  /** Laplacian filters */
  LaplacianPointerType								m_

  /** g, image TV gradient term */
  typename TVGradientFilterType::Pointer			m_TVGradX_TVGradFilter;
  typename MultiplyFilterType::Pointer				m_TVGradX_MultiplyFilter;
  
  /** g, prior image TV gradient term */
  typename TVGradientFilterType::Pointer			m_TVGradXp_TVGradFilter;
  typename MultiplyFilterType::Pointer				m_TVGradXp_MultiplyFilter;
  
  /** g, Adding terms up */
  typename AddFilterType::Pointer					m_TVGrad_AddFilter;
  typename AddFilterType::Pointer					m_g_AddFilter;
  
  /** d = beta*d_prev - g */
  typename MultiplyFilterType::Pointer				m_betad_MultiplyFilter;
  typename SubtractFilterType::Pointer				m_d_SubtractFilter;
  
  /** xnew = x + eta*d */
  typename MultiplyFilterType::Pointer				m_d_etaMultiplyFilter;
  typename AddFilterType::Pointer					m_xnew_AddFilter;
  
  /** Calculate the norm of change in image */
  typename ImageStatisticsFilterType::Pointer		m_etad_StatisticsFilter;
  
  /** g2, R^TRd term */
  typename DisplacedDetectorFilterType::Pointer		m_RTRd_DDFilter;
  typename BackProjectionFilterType::Pointer		m_RTRd_BPFilter;  
  
  /** g2, image TV hessian term */
  typename TVHessianFilterType::Pointer				m_TVHessianX_TVHessianFilter;
  typename MultiplyFilterType::Pointer				m_TVHessianX_MultiplyFilter;
  
  /** g2, prior image TV hessian term */
  typename TVHessianFilterType::Pointer				m_TVHessianXp_TVHessianFilter;
  typename MultiplyFilterType::Pointer				m_TVHessianXp_MultiplyFilter;
  
  /** g2, adding terms up */
  typename AddFilterType::Pointer					m_TVHessian_AddFilter;
  typename AddFilterType::Pointer					m_g2_AddFilter;
  
  /** Calculate norm(g)^2 */
  typename ImageStatisticsFilterType::Pointer		m_gNormSquared_StatsFilter;
  
  /** Calculate rho1 = d^T * g */
  typename MultiplyFilterType::Pointer				m_dScaled_MultiplyFilter;
  typename MultiplyFilterType::Pointer				m_gScaled_MultiplyFilter;
  typename MultiplyFilterType::Pointer				m_dg_MultiplyFilter;
  typename ImageStatisticsFilterType::Pointer		m_rho1_StatsFilter;
  
  /** Calculate rho2 = d^T * g2 */
  typename MultiplyFilterType::Pointer				m_g2Scaled_MultiplyFilter;
  typename MultiplyFilterType::Pointer				m_dg2_MultiplyFilter;
  typename ImageStatisticsFilterType::Pointer		m_rho2_StatsFilter;
  
  /** Internal variables storing the current tv, tv gradient and hessian method*/
  int m_CurrentTVConfiguration;
  int m_CurrentTVGradientConfiguration;
  int m_CurrentTVHessianConfiguration;
  int m_CurrentDisplacedDetectorConfiguration;
  
  /** Geometry object */
  rtk::ThreeDCircularProjectionGeometry::Pointer m_Geometry;

  /** Number of iterations */
  unsigned int m_NumberOfIterations;
  
  /** Stop iterations if change in x is smaller than a fraction of 
	  the change in the first iteration. Default = 0 */
  double m_StoppingCriterion;

  /** Regularization parameter Lambda and prior image factor Alpha */
  double m_Lambda;
  double m_Alpha;
  double m_C;
  
  /** c1 and c2 for line searching */
  double m_C1;
  double m_C2;
  
  /** Verbose */
  bool m_Verbose;
  
  /** TV operation scaling factor, default 1e4 */
  double m_TVScalingFactor;
  
  // The adaptive weighting vector image W - Andy
  VectorImagePointer 	m_TVWeightingImage;
  VectorImagePointer 	m_PriorWeightingImage; 
  
  /** Vectors recording the residual and objective function in each iteration **/
  std::vector< double > m_ResidualVectors;
  std::vector< double > m_ObjectiveVectors;

  /** Time probes */
  itk::TimeProbe m_ForwardProjectionProbe;
  itk::TimeProbe m_BackProjectionProbe;
  itk::TimeProbe m_ImageArithmeticProbe;
  itk::TimeProbe m_ImageStatisticsProbe;
  itk::TimeProbe m_DisplacedDetectorProbe;
  itk::TimeProbe m_TVProbe;
  itk::TimeProbe m_TVGradientProbe;
  itk::TimeProbe m_TVHessianProbe;

private:
  //purposely not implemented
  HITConeBeamReconstructionFilter(const Self&);
  void operator=(const Self&);
}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtHITConeBeamReconstructionFilter.txx"
#endif

#endif
