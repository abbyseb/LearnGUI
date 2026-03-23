WRAP_GGO(rtkdigisensgeometry_GGO_C rtkdigisensgeometry.ggo)
ADD_EXECUTABLE(rtkdigisensgeometry rtkdigisensgeometry.cxx ${rtkdigisensgeometry_GGO_C})
TARGET_LINK_LIBRARIES(rtkdigisensgeometry IGT)

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME rtkdigisensgeometry) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

