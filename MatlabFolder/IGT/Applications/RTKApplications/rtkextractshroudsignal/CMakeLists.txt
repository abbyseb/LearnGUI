WRAP_GGO(rtkextractshroudsignal_GGO_C rtkextractshroudsignal.ggo)
ADD_EXECUTABLE(rtkextractshroudsignal rtkextractshroudsignal.cxx ${rtkextractshroudsignal_GGO_C})
TARGET_LINK_LIBRARIES(rtkextractshroudsignal IGT)

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME rtkextractshroudsignal) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)

