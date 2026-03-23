WRAP_GGO(rtki0estimation_GGO_C rtki0estimation.ggo ../rtkinputprojections_section.ggo ${IGT_BINARY_DIR}/igtVersion.ggo)
ADD_EXECUTABLE(rtki0estimation rtki0estimation.cxx ${rtki0estimation_GGO_C})
TARGET_LINK_LIBRARIES(rtki0estimation IGT)

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME rtki0estimation) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

