WRAP_GGO(rtkwangdisplaceddetectorweighting_GGO_C rtkwangdisplaceddetectorweighting.ggo)
ADD_EXECUTABLE(rtkwangdisplaceddetectorweighting rtkwangdisplaceddetectorweighting.cxx ${rtkwangdisplaceddetectorweighting_GGO_C})
TARGET_LINK_LIBRARIES(rtkwangdisplaceddetectorweighting IGT)

IF(IGT_USE_CUDA)
  TARGET_LINK_LIBRARIES(rtkwangdisplaceddetectorweighting igtcuda)
ENDIF()

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME rtkwangdisplaceddetectorweighting) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

