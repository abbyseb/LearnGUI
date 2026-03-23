WRAP_GGO(rtkprojectgeometricphantom_GGO_C rtkprojectgeometricphantom.ggo)
ADD_EXECUTABLE(rtkprojectgeometricphantom rtkprojectgeometricphantom.cxx ${rtkprojectgeometricphantom_GGO_C})
TARGET_LINK_LIBRARIES(rtkprojectgeometricphantom IGT)

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME rtkprojectgeometricphantom) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

