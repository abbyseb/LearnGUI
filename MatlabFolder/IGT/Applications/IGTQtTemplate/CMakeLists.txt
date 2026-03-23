# Use the include path and library for Qt that is used by VTK.
include_directories(
					${CMAKE_CURRENT_BINARY_DIR}
					${CMAKE_CURRENT_SOURCE_DIR}
					)

# Set your files and resources here
set( Srcs main.cxx IGTQtTemplate.cxx )
set( Hdrs IGTQtTemplate.h )
set( MOC_Hdrs IGTQtTemplate.h )
set( UIs IGTQtTemplate.ui )
set( QRCs Icons/icons.qrc )
set( EXE_ICON Icons/exe_icon.ico )

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# We have ui files, this will bring in the macro: qt5_wrap_ui
#find_package(Qt5 COMPONENTS Widgets REQUIRED)
qt5_wrap_ui(UI_Srcs ${UIs})
qt5_add_resources(QRC_Srcs ${QRCs} )

source_group("Resources" FILES
			${UIs}
			${QRCs}
			${EXE_ICON}
			)

source_group("Generated" FILES
			${UI_Srcs}
			${MOC_Srcs}
			${QRC_Srcs}
			${QRC_Srcs}
			)

add_executable(IGTQtTemplate
			 ${Srcs}
			 ${Hdrs}
			 ${UI_Srcs}
			 ${MOC_Hdrs}
			 ${QRC_Srcs}
			 )
qt5_use_modules(IGTQtTemplate Core Gui Widgets)
  
TARGET_LINK_LIBRARIES(IGTQtTemplate IGT)

IF(IGT_USE_CUDA)
  TARGET_LINK_LIBRARIES(IGTQtTemplate igtcuda)
ENDIF()

IF(IGT_USE_OPENCL)
  TARGET_LINK_LIBRARIES(IGTQtTemplate igtopencl)
ENDIF()

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME IGTQtTemplate) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)
