#=========================================================
# Propagate cmake options in a header file
CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/igtConfiguration.h.in
  ${CMAKE_BINARY_DIR}/igtConfiguration.h)
SET (IGT_LIBRARIES IGT lpsolve55)
#=========================================================

#=========================================================
IF(IGT_USE_Qt)
	SET(QT_FILES igtITKToQImage.cxx)
ENDIF()

ADD_LIBRARY(IGT
igtCircularConeBeamKalmanFilter.cxx
igtFixedRayDistanceTracking.cxx
igtHITConeBeamReconstructionFilter.txx
igtKalmanFilter.cxx
igtLeastSquareSubtractionImageFilter.cxx
igtLinearCombinationFilter.txx
igtMultiple2DMeasurementTracking.cxx
igtProjectionAlignmentFilter.cxx
igtProjectionsReader.cxx
igtTargetTracking.cxx
igtTemplateMatchingFilter.cxx
igtVarianXimGeometryReader.cxx
igtVarianXimXMLFileReader.cxx
igtWarpBackProjectionImageFilter.txx
igtXimImageIO.cxx
igtXimImageIOFactory.cxx
RTKModules/rtkADMMTotalVariationConeBeamReconstructionFilter.txx
RTKModules/rtkADMMTotalVariationConjugateGradientOperator.txx
RTKModules/rtkADMMWaveletsConeBeamReconstructionFilter.txx
RTKModules/rtkADMMWaveletsConjugateGradientOperator.txx
RTKModules/rtkAdditiveGaussianNoiseImageFilter.txx
RTKModules/rtkAmsterdamShroudImageFilter.txx
RTKModules/rtkAttGeometryReader.cxx
RTKModules/rtkAttImageIO.cxx
RTKModules/rtkAttImageIOFactory.cxx
RTKModules/rtkAttXMLFileReader.cxx
RTKModules/rtkAverageOutOfROIImageFilter.txx
RTKModules/rtkBackProjectionImageFilter.txx
RTKModules/rtkBackwardDifferenceDivergenceImageFilter.txx
RTKModules/rtkBinNumberReader.txx
RTKModules/rtkBoellaardScatterCorrectionImageFilter.txx
RTKModules/rtkConjugateGradientConeBeamReconstructionFilter.txx
RTKModules/rtkConjugateGradientGetP_kPlusOneImageFilter.txx
RTKModules/rtkConjugateGradientGetR_kPlusOneImageFilter.txx
RTKModules/rtkConjugateGradientGetX_kPlusOneImageFilter.txx
RTKModules/rtkConjugateGradientImageFilter.txx
RTKModules/rtkConjugateGradientOperator.txx
RTKModules/rtkConstantImageSource.txx
RTKModules/rtkConvertEllipsoidToQuadricParametersFunction.cxx
RTKModules/rtkCyclicDeformationImageFilter.hxx
RTKModules/rtkDCMImagXImageIO.cxx
RTKModules/rtkDCMImagXImageIOFactory.cxx
RTKModules/rtkDPExtractShroudSignalImageFilter.txx
RTKModules/rtkDaubechiesWaveletsConvolutionImageFilter.txx
RTKModules/rtkDaubechiesWaveletsDenoiseSequenceImageFilter.txx
RTKModules/rtkDbf.cxx
RTKModules/rtkDeconstructImageFilter.txx
RTKModules/rtkDeconstructSoftThresholdReconstructImageFilter.txx
RTKModules/rtkDigisensGeometryReader.cxx
RTKModules/rtkDigisensGeometryXMLFileReader.cxx
RTKModules/rtkDirectConeBeamNCCTracking.txx
RTKModules/rtkDirectConeBeamTracking.txx
RTKModules/rtkDisplacedDetectorImageFilter.txx
RTKModules/rtkDivergenceOfGradientConjugateGradientOperator.txx
RTKModules/rtkDownsampleImageFilter.txx
RTKModules/rtkDrawConeImageFilter.txx
RTKModules/rtkDrawCubeImageFilter.txx
RTKModules/rtkDrawCylinderImageFilter.txx
RTKModules/rtkDrawEllipsoidImageFilter.txx
RTKModules/rtkDrawGeometricPhantomImageFilter.txx
RTKModules/rtkDrawSheppLoganFilter.txx
RTKModules/rtkEdfImageIO.cxx
RTKModules/rtkEdfImageIOFactory.cxx
RTKModules/rtkEdfRawToAttenuationImageFilter.txx
RTKModules/rtkElektaSynergyGeometryReader.cxx
RTKModules/rtkElektaSynergyLookupTableImageFilter.txx
RTKModules/rtkElektaSynergyRawToAttenuationImageFilter.txx
RTKModules/rtkElektaXMLFileReader.cxx
RTKModules/rtkElektaXMLGeometryReader.cxx
RTKModules/rtkFDKBackProjectionImageFilter.txx
RTKModules/rtkFDKConeBeamReconstructionFilter.txx
RTKModules/rtkFDKWarpBackProjectionImageFilter.txx
RTKModules/rtkFDKWeightProjectionFilter.txx
RTKModules/rtkFFTConvolutionImageFilter.txx
RTKModules/rtkFFTRampImageFilter.txx
RTKModules/rtkFieldOfViewImageFilter.txx
RTKModules/rtkForwardDifferenceGradientImageFilter.txx
RTKModules/rtkForwardProjectionImageFilter.txx
RTKModules/rtkForwardWarpImageFilter.txx
RTKModules/rtkFourDConjugateGradientConeBeamReconstructionFilter.txx
RTKModules/rtkFourDROOSTERConeBeamReconstructionFilter.txx
RTKModules/rtkFourDReconstructionConjugateGradientOperator.txx
RTKModules/rtkFourDToProjectionStackImageFilter.txx
RTKModules/rtkGeometricPhantomFileReader.cxx
RTKModules/rtkHisImageIO.cxx
RTKModules/rtkHisImageIOFactory.cxx
RTKModules/rtkHncImageIO.cxx
RTKModules/rtkHncImageIOFactory.cxx
RTKModules/rtkHndImageIO.cxx
RTKModules/rtkHndImageIOFactory.cxx
RTKModules/rtkI0EstimationProjectionFilter.txx
RTKModules/rtkIOFactories.cxx
RTKModules/rtkImagXImageIO.cxx
RTKModules/rtkImagXImageIOFactory.cxx
RTKModules/rtkImagXRawToAttenuationImageFilter.txx
RTKModules/rtkImagXXMLFileReader.cxx
RTKModules/rtkImportImageFilter.txx
RTKModules/rtkInterpolatorWithKnownWeightsImageFilter.txx
RTKModules/rtkIterativeConeBeamReconstructionFilter.txx
RTKModules/rtkJosephBackProjectionImageFilter.txx
RTKModules/rtkJosephForwardProjectionImageFilter.txx
RTKModules/rtkMKBConeBeamReconstructionFilter.txx
RTKModules/rtkMagnitudeThresholdImageFilter.txx
RTKModules/rtkMedianImageFilter.cxx
RTKModules/rtkMultiplyByVectorImageFilter.txx
RTKModules/rtkNormalizedJosephBackProjectionImageFilter.txx
RTKModules/rtkOpenCLFDKBackProjectionImageFilter.cxx
RTKModules/rtkOpenCLFDKConeBeamReconstructionFilter.txx
RTKModules/rtkOpenCLUtilities.cxx
RTKModules/rtkPICCSConeBeamReconstructionFilter.txx
RTKModules/rtkParkerShortScanImageFilter.txx
RTKModules/rtkPartialDerivativeImageFilter.txx
RTKModules/rtkPhaseGatingImageFilter.txx
RTKModules/rtkPhaseReader.txx
RTKModules/rtkPhasesToInterpolationWeights.txx
RTKModules/rtkProjectGeometricPhantomImageFilter.txx
RTKModules/rtkProjectionGeometry.txx
RTKModules/rtkProjectionStackToFourDImageFilter.txx
RTKModules/rtkProjectionsReader.txx
RTKModules/rtkRayBoxIntersectionFunction.txx
RTKModules/rtkRayBoxIntersectionImageFilter.txx
RTKModules/rtkRayCastInterpolateImageFunction.txx
RTKModules/rtkRayCastInterpolatorForwardProjectionImageFilter.txx
RTKModules/rtkRayEllipsoidIntersectionImageFilter.txx
RTKModules/rtkRayQuadricIntersectionFunction.txx
RTKModules/rtkRayQuadricIntersectionImageFilter.txx
RTKModules/rtkReconstructImageFilter.txx
RTKModules/rtkReconstructionConjugateGradientOperator.txx
RTKModules/rtkReg1DExtractShroudSignalImageFilter.txx
RTKModules/rtkReg23ProjectionGeometry.cxx
RTKModules/rtkSARTConeBeamReconstructionFilter.txx
RTKModules/rtkSheppLoganPhantomFilter.txx
RTKModules/rtkSoftThresholdImageFilter.txx
RTKModules/rtkSoftThresholdTVImageFilter.txx
RTKModules/rtkSplatWithKnownWeightsImageFilter.txx
RTKModules/rtkThreeDCircularProjectionGeometry.cxx
RTKModules/rtkThreeDCircularProjectionGeometrySorter.txx
RTKModules/rtkThreeDCircularProjectionGeometryXMLFile.cxx
RTKModules/rtkTotalVariationDenoiseSequenceImageFilter.txx
RTKModules/rtkTotalVariationDenoisingBPDQImageFilter.txx
RTKModules/rtkTotalVariationGradientImageFilter.txx
RTKModules/rtkTotalVariationHessianOperatorImageFilter.txx
RTKModules/rtkTotalVariationImageFilter.txx
RTKModules/rtkUnwarpSequenceConjugateGradientOperator.txx
RTKModules/rtkUnwarpSequenceImageFilter.txx
RTKModules/rtkUpsampleImageFilter.txx
RTKModules/rtkVarianObiGeometryReader.cxx
RTKModules/rtkVarianObiXMLFileReader.cxx
RTKModules/rtkVectorSorter.txx
RTKModules/rtkWarpSequenceImageFilter.txx
RTKModules/rtkXRadGeometryReader.cxx
RTKModules/rtkXRadImageIO.cxx
RTKModules/rtkXRadImageIOFactory.cxx
RTKModules/rtkXRadRawToAttenuationImageFilter.txx
${QT_FILES}
            )
			
TARGET_LINK_LIBRARIES(IGT ${ITK_LIBRARIES})
TARGET_LINK_LIBRARIES(IGT lpsolve55)
IF(IGT_USE_Qt)
	TARGET_LINK_LIBRARIES(IGT Qt5::Widgets)
ENDIF()
#=========================================================
# OpenCL library stuff
IF(IGT_USE_OPENCL)
  SET(IGT_LIBRARIES ${IGT_LIBRARIES} igtopencl)
  ADD_LIBRARY(igtopencl
              RTKModules/rtkOpenCLUtilities.cxx
              RTKModules/rtkOpenCLFDKBackProjectionImageFilter.cxx)
  TARGET_LINK_LIBRARIES(igtopencl ${OPENCL_LIBRARIES} ${ITK_LIBRARIES})

  FILE(GLOB openclSRCs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cl)
  ADD_CUSTOM_TARGET(openclSRCCopy ALL COMMENT "Copying OpenCL source files")
  FOREACH(openclSRC ${openclSRCs})
    ADD_CUSTOM_COMMAND(TARGET openclSRCCopy
                       COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${openclSRC}
                                                        ${IGT_BINARY_DIR}/${openclSRC})
  ENDFOREACH(openclSRC)
  ADD_DEPENDENCIES(igtopencl openclSRCCopy)
ENDIF(IGT_USE_OPENCL)
#=========================================================

#=========================================================
# Cuda library stuff
IF (IGT_USE_CUDA)
  SET (IGT_LIBRARIES ${IGT_LIBRARIES} igtcuda ITKCudaCommon ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY} ${CUDA_CUFFT_LIBRARIES})
  SET (igtcuda_CUDA_FILES
	   igtCudaLinearCombinationFilter.cu
       RTKModules/rtkCudaCropImageFilter.cu
       RTKModules/rtkCudaUtilities.cu
       RTKModules/rtkCudaFDKBackProjectionImageFilter.cu
       RTKModules/rtkCudaFFTConvolutionImageFilter.cu
       RTKModules/rtkCudaFDKWeightProjectionFilter.cu
       RTKModules/rtkCudaDisplacedDetectorImageFilter.cu
       RTKModules/rtkCudaParkerShortScanImageFilter.cu
       RTKModules/rtkCudaForwardProjectionImageFilter.cu
       RTKModules/rtkCudaBackProjectionImageFilter.cu
       RTKModules/rtkCudaSplatImageFilter.cu
       RTKModules/rtkCudaInterpolateImageFilter.cu
       RTKModules/rtkCudaWarpImageFilter.cu
       RTKModules/rtkCudaForwardWarpImageFilter.cu
       RTKModules/rtkCudaRayCastBackProjectionImageFilter.cu
       RTKModules/rtkCudaTotalVariationDenoisingBPDQImageFilter.cu
       RTKModules/rtkCudaLastDimensionTVDenoisingImageFilter.cu
       RTKModules/rtkCudaConstantVolumeSource.cu
       RTKModules/rtkCudaConstantVolumeSeriesSource.cu
       RTKModules/rtkCudaConjugateGradientImageFilter_3f.cu
       RTKModules/rtkCudaConjugateGradientImageFilter_4f.cu
       RTKModules/rtkCudaAverageOutOfROIImageFilter.cu
	   RTKModules/rtkCudaTotalVariationImageFilter.cu
	   RTKModules/rtkCudaTotalVariationGradientImageFilter.cu
	   RTKModules/rtkCudaTotalVariationHessianOperatorImageFilter.cu
	   RTKModules/rtkCudaWarpForwardProjectionImageFilter.cu
	   RTKModules/rtkCudaWarpBackProjectionImageFilter.cu
	   RTKModules/rtkCudaCyclicDeformationImageFilter.cu)
       
  # This is a fix for ITK4 to avoid too long cmd lines error
  # (http://www.itk.org/Bug/view.php?id=12198)
  IF(WIN32)
    GET_DIRECTORY_PROPERTY(dirInc INCLUDE_DIRECTORIES)
    FIND_PATH(itkMacroPath itkMacro.h ${dirInc})
    FIND_PATH(itkConfigurePath itkConfigure.h ${dirInc})
    FIND_PATH(igtConfigurationPath igtConfiguration.h ${dirInc})
	FIND_PATH(igtWin32HeaderPath igtWin32Header.h ${dirInc})
    SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${itkMacroPath};${itkConfigurePath};${igtConfigurationPath};${igtWin32HeaderPath}")
  ENDIF()

  CUDA_COMPILE (igtcuda_CUDA_WRAPPERS ${igtcuda_CUDA_FILES})

  # Rollback to all includes
  IF(WIN32)
    SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${dirInc}")
  ENDIF()

  ADD_LIBRARY(igtcuda
			  igtCudaLinearCombinationFilter.cxx
              RTKModules/rtkCudaFDKWeightProjectionFilter.cxx
              RTKModules/rtkCudaDisplacedDetectorImageFilter.cxx
              RTKModules/rtkCudaParkerShortScanImageFilter.cxx
              RTKModules/rtkCudaCropImageFilter.cxx
              RTKModules/rtkCudaFDKBackProjectionImageFilter.cxx
              RTKModules/rtkCudaFDKConeBeamReconstructionFilter.cxx
              RTKModules/rtkCudaBackProjectionImageFilter.cxx
			  RTKModules/rtkCudaSplatImageFilter.cxx
			  RTKModules/rtkCudaInterpolateImageFilter.cxx
			  RTKModules/rtkCudaWarpImageFilter.cxx
			  RTKModules/rtkCudaForwardWarpImageFilter.cxx
			  RTKModules/rtkCudaRayCastBackProjectionImageFilter.cxx
			  RTKModules/rtkCudaTotalVariationDenoisingBPDQImageFilter.cxx
			  RTKModules/rtkCudaLastDimensionTVDenoisingImageFilter.cxx
			  RTKModules/rtkCudaConstantVolumeSource.cxx
			  RTKModules/rtkCudaConstantVolumeSeriesSource.cxx
              RTKModules/rtkCudaConjugateGradientImageFilter_3f.cxx
              RTKModules/rtkCudaConjugateGradientImageFilter_4f.cxx
			  RTKModules/rtkCudaAverageOutOfROIImageFilter.cxx
			  RTKModules/rtkCudaTotalVariationImageFilter.cxx
			  RTKModules/rtkCudaTotalVariationGradientImageFilter.cxx
			  RTKModules/rtkCudaTotalVariationHessianOperatorImageFilter.cxx
			  RTKModules/rtkCudaWarpForwardProjectionImageFilter.cxx
			  RTKModules/rtkCudaWarpBackProjectionImageFilter.cxx
			  RTKModules/rtkCudaCyclicDeformationImageFilter.cxx
			  ${igtcuda_CUDA_WRAPPERS}
              ${igtcuda_CUDA_FILES})
  SET_TARGET_PROPERTIES (igtcuda PROPERTIES LINKER_LANGUAGE CXX)
  TARGET_LINK_LIBRARIES(igtcuda ${CUDA_LIBRARIES} ${CUDA_cufft_LIBRARY} ${CUDA_cublas_LIBRARY} IGT ITKCudaCommon)
ENDIF (IGT_USE_CUDA)
#=========================================================

#=========================================================
# Installation code
IF(NOT IGT_INSTALL_NO_DEVELOPMENT)
  # Generate IGTConfig.cmake for the install tree.
  SET (IGT_USE_FILE "${CMAKE_INSTALL_PREFIX}/${IGT_INSTALL_PACKAGE_DIR}/UseIGT.cmake")
  SET (IGT_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/${IGT_INSTALL_INCLUDE_DIR};${CMAKE_INSTALL_PREFIX}/${IGT_INSTALL_INCLUDE_DIR}/lpsolve")
  SET (IGT_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/${IGT_INSTALL_LIB_DIR}")
  CONFIGURE_FILE (${PROJECT_SOURCE_DIR}/Cmake/IGTConfig.cmake.in ${IGT_BINARY_DIR}/CMakeFiles/IGTConfig.cmake @ONLY)
  INSTALL(FILES ${IGT_BINARY_DIR}/CMakeFiles/IGTConfig.cmake
                ${IGT_SOURCE_DIR}/Cmake/UseIGT.cmake
          DESTINATION ${IGT_INSTALL_PACKAGE_DIR})

  # Include .h and .txx files
  FILE(GLOB __files1 "${PROJECT_SOURCE_DIR}/Code/*.h")
  FILE(GLOB __files2 "${PROJECT_SOURCE_DIR}/Code/*.txx")
  INSTALL(FILES ${__files1} ${__files2} ${IGT_BINARY_DIR}/igtConfiguration.h
    DESTINATION ${IGT_INSTALL_INCLUDE_DIR}
    COMPONENT Development)
ENDIF(NOT IGT_INSTALL_NO_DEVELOPMENT)

IF(NOT IGT_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS IGT
    RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)

  IF(IGT_USE_CUDA)
    INSTALL(TARGETS igtcuda
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDIF(IGT_USE_CUDA)

  IF(OPENCL_FOUND)
    INSTALL(TARGETS igtopencl
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDIF(OPENCL_FOUND)
ENDIF(NOT IGT_INSTALL_NO_LIBRARIES)
#=========================================================

SET(IGT_LIBRARIES ${IGT_LIBRARIES} PARENT_SCOPE)