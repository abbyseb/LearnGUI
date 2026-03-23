WRAP_GGO(igtmhatodicomseries_GGO_C igtmhatodicomseries.ggo)
ADD_EXECUTABLE(igtmhatodicomseries igtmhatodicomseries.cxx ${igtmhatodicomseries_GGO_C})
TARGET_LINK_LIBRARIES(igtmhatodicomseries IGT)

IF(IGT_USE_CUDA)
  TARGET_LINK_LIBRARIES(igtmhatodicomseries igtcuda)
ENDIF()

IF(IGT_USE_OPENCL)
  TARGET_LINK_LIBRARIES(igtmhatodicomseries igtopencl)
ENDIF()

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME igtmhatodicomseries) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

