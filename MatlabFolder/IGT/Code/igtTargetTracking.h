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

#ifndef __igtTargetTracking_h
#define __igtTargetTracking_h

#include <igtCircularConeBeamKalmanFilter.h>

#include <igtTemplateMatchingFilter.h>

#include <rtkThreeDCircularProjectionGeometry.h>
#include <rtkJosephForwardProjectionImageFilter.h>
#include <rtkConstantImageSource.h>

#include <itkImageRegistrationMethod.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkResampleImageFilter.h>
#include <itkTranslationTransform.h>

#include <itkMeanSquaresImageToImageMetric.h>
#include <itkMattesMutualInformationImageToImageMetric.h>
#include <itkNormalizedCorrelationImageToImageMetric.h>

#include <itkRegularStepGradientDescentOptimizer.h>
#include <itkGradientDescentOptimizer.h>
#include <itkExhaustiveOptimizer.h>
#include <itkLBFGSBOptimizer.h>

#include <itkStatisticsImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkImageMomentsCalculator.h>
#include <itkImageMaskSpatialObject.h>
#include <itkProcessObject.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkLaplacianSharpeningImageFilter.h>

#ifdef IGT_USE_CUDA
#include <rtkCudaForwardProjectionImageFilter.h>
#endif

namespace igt
{

/** \class TargetTracking
 * \brief This is the base class for target tracking methods.
 *		  The target image, whose position is to be found, is
 *		  set via SetTargetImage. The reference DRR image, which
 *		  is used for finding the target location, is set via
 *		  SetReferenceProjectionImage. This class expects a 3D target 
 *		  image and 2D reference DRR image.
 *		  The geometry object also needs to be set via SetGeometry.
 *		  The base class uses translation only registration to find 
 *		  the target location and a basic Kalman filter as the
 *		  estimator.
 * \author Andy Shieh 2016
 */
template<class ImageType>
class ITK_EXPORT TargetTracking :
	public itk::ProcessObject
{
public:
	/** Standard class typedefs. */
	typedef TargetTracking					Self;
	typedef itk::ProcessObject				Superclass;
	typedef itk::SmartPointer<Self>			Pointer;
	typedef itk::SmartPointer<const Self>	ConstPointer;

	itkStaticConstMacro(ImageDimension, unsigned int, ImageType::ImageDimension);

	/** Some convenient typedefs for images. */
	typedef typename ImageType::PixelType		ImagePixelType;
	typedef typename ImageType::Pointer			ImagePointerType;
	typedef typename ImageType::RegionType 		ImageRegionType;
	typedef typename ImageType::SizeType 		ImageSizeType;
	typedef typename ImageType::IndexType 		ImageIndexType;
	typedef typename ImageType::SpacingType		ImageSpacingType;
	typedef typename ImageType::PointType		ImagePointType;
	typedef typename ImageType::DirectionType	ImageDirectionType;

	typedef typename itk::Image<ImagePixelType, ImageDimension - 1>
													DRRImageType;
	typedef typename DRRImageType::PixelType		DRRImagePixelType;
	typedef typename DRRImageType::Pointer			DRRImagePointerType;
	typedef typename DRRImageType::RegionType 		DRRImageRegionType;
	typedef typename DRRImageType::SizeType 		DRRImageSizeType;
	typedef typename DRRImageType::IndexType 		DRRImageIndexType;
	typedef typename DRRImageType::SpacingType		DRRImageSpacingType;
	typedef typename DRRImageType::PointType		DRRImagePointType;
	typedef typename DRRImageType::DirectionType	DRRImageDirectionType;

	/* Some filter typedefs*/
	typedef igt::CircularConeBeamKalmanFilter<double>						EstimatorType;
	typedef rtk::ForwardProjectionImageFilter<ImageType, ImageType>			ForwardProjectionType;
	typedef rtk::JosephForwardProjectionImageFilter<ImageType, ImageType>	JosephForwardProjectionType;
#ifdef IGT_USE_CUDA
	typedef rtk::CudaForwardProjectionImageFilter<ImageType, ImageType>		CudaForwardProjectionType;
#endif
	typedef itk::ImageMomentsCalculator<ImageType>							ImageCentroidType;
	typedef rtk::ConstantImageSource<ImageType>								ConstantImageType;
	typedef itk::ExtractImageFilter<ImageType, DRRImageType>				ExtractImageType;
	typedef itk::BinaryThresholdImageFilter< ImageType, itk::Image < unsigned char, ImageDimension >>
																			ThresholdToMaskType;
	typedef itk::ImageMaskSpatialObject< ImageDimension >					MaskSpatialObjectType;
	typedef itk::RegionOfInterestImageFilter<ImageType, ImageType>			ImageROIType;
	typedef itk::StatisticsImageFilter<DRRImageType>						ImageStatsType;

	// Typedefs for image registration
	typedef itk::ImageRegistrationMethod< DRRImageType, DRRImageType >		 ImageRegistrationType;
	typedef itk::LinearInterpolateImageFunction< DRRImageType, double >		 LinearInterpolatorType;
	typedef itk::MattesMutualInformationImageToImageMetric< DRRImageType, DRRImageType>	MMIMetricType;
	typedef itk::MeanSquaresImageToImageMetric< DRRImageType, DRRImageType>				MSMetricType;
	typedef itk::NormalizedCorrelationImageToImageMetric<DRRImageType, DRRImageType>	NCMetricType;
	typedef itk::TranslationTransform< double, ImageDimension - 1>			TranslationTransformType;

	typedef igt::TemplateMatchingFilter<DRRImageType>						TemplateMatchingType; // Not used currently

	// Typedefs for image sharpening
	typedef itk::LaplacianSharpeningImageFilter<DRRImageType, DRRImageType>
		SharpenImageType;

	/* Typedefs for matrix operation */
	typedef typename EstimatorType::MatrixType	MatrixType;
	typedef typename EstimatorType::VectorType	VectorType;

	/* Standard New method. */
	itkNewMacro(Self);

	/** Runtime information support. */
	itkTypeMacro(TargetTracking, itk::ProcessObject);

	// The main tracking workflow to be called by user
	virtual void Compute();

	// Estimating step incorporting prior knowledge
	virtual void Estimate();

	// The measurement step, i.e. template matching
	virtual void MeasurePosition();

	/** Estimator */
	itkGetMacro(Estimator, typename EstimatorType::Pointer);

	/** Get / Set the target image */
	itkGetMacro(TargetImage, ImagePointerType);
	virtual void SetTargetImage(ImagePointerType img)
	{
		m_TargetImage = img;
		m_TargetOrigin = vnl_vector<double>(ImageDimension, 0);
		m_TargetCentroid = vnl_vector<double>(ImageDimension, 0);
		ImageCentroidType::Pointer centroidCalculator = ImageCentroidType::New();
		centroidCalculator->SetImage(m_TargetImage);
		centroidCalculator->Compute();
		for (unsigned int k = 0; k != ImageDimension; ++k)
		{
			m_TargetOrigin(k) = m_TargetImage->GetOrigin()[k];
			m_TargetCentroid(k) = centroidCalculator->GetCenterOfGravity().GetVnlVector()(k);
		}
	}

	// Get / Set the search radii for DRR template matching in mm
	itkGetMacro(DRRSearchRadius, DRRImageSpacingType);
	itkSetMacro(DRRSearchRadius, DRRImageSpacingType);

	// Get / Set the boolean of whether or not to sharpen DRR
	itkGetMacro(SharpenDRR, bool);
	itkSetMacro(SharpenDRR, bool);

	/** Get the target DRR image */
	itkGetMacro(TargetDRRImage, ImagePointerType);

	/** Get / Set the reference DRR image */
	itkGetMacro(ReferenceProjectionImage, ImagePointerType);
	itkSetMacro(ReferenceProjectionImage, ImagePointerType);

	/** Get / Set the object pointer to projection geometry */
	itkGetMacro(Geometry, rtk::ThreeDCircularProjectionGeometry::Pointer);
	virtual void SetGeometry(rtk::ThreeDCircularProjectionGeometry::Pointer geometry)
	{ m_Geometry = geometry; }

	/** Get & Set the translation vector in projection domain */
	itkGetMacro(DRRTranslationVector, VectorType);
	itkSetMacro(DRRTranslationVector, VectorType);

	/** Get & Set the target DRR centroid in projection domain */
	itkGetMacro(TargetDRRCentroid, VectorType);
	itkSetMacro(TargetDRRCentroid, VectorType);

	/** Get target centroid and origin */
	itkGetMacro(TargetCentroid, VectorType);
	itkGetMacro(TargetOrigin, VectorType);

	/** Get template matching metric metric value */
	itkGetMacro(MatchingMetric, double);

	/** Get / set template matching metric option */
	/** 0: MSE; 1: Mattes mutual information */
	itkGetMacro(TemplateMatchingMetricOption, unsigned int);
	itkSetMacro(TemplateMatchingMetricOption, unsigned int);

	// Number of iterations for the template matching optimizer
	itkGetMacro(NumberOfIterations, unsigned int);
	itkSetMacro(NumberOfIterations, unsigned int);

	/** Get target DRR ROI region */
	itkGetMacro(TargetDRRROI, ImageRegionType);

	/** Set target centroid using vnl vector type. Target origin will be changed too */
	virtual void SetTargetCentroid(VectorType centroidVector)
	{
		if (!m_TargetImage)
			std::cerr << "The target image is not set" << std::endl;
		m_TargetOrigin = centroidVector + m_TargetOrigin - m_TargetCentroid;
		m_TargetCentroid = centroidVector;

		ImagePointType originPoint;
		for (unsigned int k = 0; k != ImageDimension; ++k)
			originPoint[k] = m_TargetOrigin(k);
		m_TargetImage->SetOrigin(originPoint);
	}

	/** Set target origin. Target centroid will be changed too */
	virtual void SetTargetOrigin(ImagePointType originPoint)
	{
		if (!m_TargetImage)
			std::cerr << "The target image is not set" << std::endl;
		m_TargetImage->SetOrigin(originPoint);

		for (unsigned int k = 0; k != ImageDimension; ++k)
		{
			m_TargetCentroid(k) = originPoint[k] + m_TargetCentroid(k) - m_TargetOrigin(k);
			m_TargetOrigin(k) = originPoint[k];
		}
	}

	/** Get / Set UseCUDA tag */
	itkGetMacro(UseCUDA, bool);
	itkSetMacro(UseCUDA, bool);

protected:
	TargetTracking();
	~TargetTracking(){}

	virtual void VerifyInputInformation();

	// Forward project target model to get target DRR
	virtual void GenerateTargetDRR();

	// The actual template matching step
	virtual void ProjectionTemplateMatching();

	/** Set the target DRR image */
	itkSetMacro(TargetDRRImage, ImagePointerType);

	/** Geometry object */
	rtk::ThreeDCircularProjectionGeometry::Pointer m_Geometry;

	/** Estimator */
	typename EstimatorType::Pointer m_Estimator;

	/** Target and refernece image */
	ImagePointerType m_TargetImage, m_TargetDRRImage, m_ReferenceProjectionImage;

	/** The current target centroid and origin*/
	VectorType m_TargetCentroid, m_TargetOrigin;

	/** Centroid and translation vector in projection domain */
	VectorType m_TargetDRRCentroid, m_DRRTranslationVector;

	/** The ROI of the target DRR image */
	ImageRegionType m_TargetDRRROI;

	/** Template matching metric configuration */
	/** 0: MSE; 1: Mattes Mutual Information */
	unsigned int m_TemplateMatchingMetricOption;

	/** Tempalte matching metric */
	double m_MatchingMetric;

	// Number of iterations for the template matching optimizer
	unsigned int m_NumberOfIterations;

	// Whether or not to use CUDA
	bool m_UseCUDA;

	// Whether or not to sharpen DRR
	bool m_SharpenDRR;

	// Search radii
	DRRImageSpacingType m_DRRSearchRadius;

private:
	//purposely not implemented
	TargetTracking(const Self&);
	void operator=(const Self&);
}; // end of class

} // end namespace igt

#ifndef ITK_MANUAL_INSTANTIATION
#include "igtTargetTracking.cxx"
#endif

#endif
