WRAP_GGO(rtktotalvariationdenoising_GGO_C rtktotalvariationdenoising.ggo)
ADD_EXECUTABLE(rtktotalvariationdenoising rtktotalvariationdenoising.cxx ${rtktotalvariationdenoising_GGO_C})
TARGET_LINK_LIBRARIES(rtktotalvariationdenoising IGT)

IF(IGT_USE_CUDA)
  TARGET_LINK_LIBRARIES(rtktotalvariationdenoising igtcuda)
ENDIF()

IF(IGT_USE_OPENCL)
  TARGET_LINK_LIBRARIES(rtktotalvariationdenoising igtopencl)
ENDIF()

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME rtktotalvariationdenoising) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

