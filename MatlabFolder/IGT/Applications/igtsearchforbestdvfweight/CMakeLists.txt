WRAP_GGO(igtsearchforbestdvfweight_GGO_C igtsearchforbestdvfweight.ggo)
ADD_EXECUTABLE(igtsearchforbestdvfweight igtsearchforbestdvfweight.cxx ${igtsearchforbestdvfweight_GGO_C})
TARGET_LINK_LIBRARIES(igtsearchforbestdvfweight IGT)

IF(IGT_USE_CUDA)
  TARGET_LINK_LIBRARIES(igtsearchforbestdvfweight igtcuda)
ENDIF()

IF(IGT_USE_OPENCL)
  TARGET_LINK_LIBRARIES(igtsearchforbestdvfweight igtopencl)
ENDIF()

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME igtsearchforbestdvfweight) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

