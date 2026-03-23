WRAP_GGO(rtkimagxgeometry_GGO_C rtkimagxgeometry.ggo ${IGT_BINARY_DIR}/igtVersion.ggo)
add_executable(rtkimagxgeometry rtkimagxgeometry.cxx ${rtkimagxgeometry_GGO_C})
target_link_libraries(rtkimagxgeometry IGT)

# Installation code
if(NOT IGT_INSTALL_NO_EXECUTABLES)
  foreach(EXE_NAME rtkimagxgeometry) 
    install(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  endforeach() 
endif()

