WRAP_GGO(igtleastsquaresubtract_GGO_C igtleastsquaresubtract.ggo)
ADD_EXECUTABLE(igtleastsquaresubtract igtleastsquaresubtract.cxx ${igtleastsquaresubtract_GGO_C})
TARGET_LINK_LIBRARIES(igtleastsquaresubtract IGT)

IF(IGT_USE_CUDA)
  TARGET_LINK_LIBRARIES(igtleastsquaresubtract igtcuda)
ENDIF()

IF(IGT_USE_OPENCL)
  TARGET_LINK_LIBRARIES(igtleastsquaresubtract igtopencl)
ENDIF()

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME igtleastsquaresubtract) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

