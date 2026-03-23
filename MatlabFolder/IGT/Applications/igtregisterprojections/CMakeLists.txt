WRAP_GGO(igtregisterprojections_GGO_C igtregisterprojections.ggo)
ADD_EXECUTABLE(igtregisterprojections igtregisterprojections.cxx ${igtregisterprojections_GGO_C})
TARGET_LINK_LIBRARIES(igtregisterprojections IGT)

IF(IGT_USE_CUDA)
  TARGET_LINK_LIBRARIES(igtregisterprojections igtcuda)
ENDIF()

IF(IGT_USE_OPENCL)
  TARGET_LINK_LIBRARIES(igtregisterprojections igtopencl)
ENDIF()

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME igtregisterprojections) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

