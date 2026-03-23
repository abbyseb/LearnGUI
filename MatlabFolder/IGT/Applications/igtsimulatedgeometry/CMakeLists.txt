WRAP_GGO(igtsimulatedgeometry_GGO_C igtsimulatedgeometry.ggo)
ADD_EXECUTABLE(igtsimulatedgeometry igtsimulatedgeometry.cxx ${igtsimulatedgeometry_GGO_C})
TARGET_LINK_LIBRARIES(igtsimulatedgeometry IGT)

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME igtsimulatedgeometry) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

