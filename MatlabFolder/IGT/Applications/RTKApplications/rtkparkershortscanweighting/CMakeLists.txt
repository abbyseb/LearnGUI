WRAP_GGO(rtkparkershortscanweighting_GGO_C rtkparkershortscanweighting.ggo)
ADD_EXECUTABLE(rtkparkershortscanweighting rtkparkershortscanweighting.cxx ${rtkparkershortscanweighting_GGO_C})
TARGET_LINK_LIBRARIES(rtkparkershortscanweighting IGT)

IF(IGT_USE_CUDA)
  TARGET_LINK_LIBRARIES(rtkparkershortscanweighting igtcuda)
ENDIF()

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME rtkparkershortscanweighting) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

