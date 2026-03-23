WRAP_GGO(rtkconjugategradient_GGO_C rtkconjugategradient.ggo)
ADD_EXECUTABLE(rtkconjugategradient rtkconjugategradient.cxx ${rtkconjugategradient_GGO_C})
TARGET_LINK_LIBRARIES(rtkconjugategradient IGT)

IF (IGT_USE_CUDA)
  TARGET_LINK_LIBRARIES(rtkconjugategradient igtcuda)
ENDIF (IGT_USE_CUDA)

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME rtkconjugategradient) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

