# Use the include path and library for Qt that is used by VTK.
include_directories(
					${CMAKE_CURRENT_BINARY_DIR}
					${CMAKE_CURRENT_SOURCE_DIR}
					../../Utilities/QCustomPlot
					)

# Set your files and resources here
set( Srcs main.cxx InVivoTrackingOffline.cxx TargetTracker.cxx 
		../../Utilities/QCustomPlot/QCustomPlot.cpp)
set( Hdrs InVivoTrackingOffline.h TargetTracker.h 
		../../Utilities/QCustomPlot/QCustomPlot.h)
set( MOC_Hdrs InVivoTrackingOffline.h TargetTracker.h 
		../../Utilities/QCustomPlot/QCustomPlot.h)
set( UIs InVivoTrackingOffline.ui )
set( QRCs Icons/icons.qrc )
set( EXE_ICON Icons/AppIcon.png )

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# We have ui files, this will bring in the macro: qt5_wrap_ui
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

add_executable(InVivoTrackingOffline #WIN32
			 ${Srcs}
			 ${Hdrs}
			 ${UI_Srcs}
			 ${MOC_Hdrs}
			 ${QRC_Srcs}
			 )
qt5_use_modules(InVivoTrackingOffline Core Gui Widgets PrintSupport)
  
TARGET_LINK_LIBRARIES(InVivoTrackingOffline IGT)

IF(IGT_USE_CUDA)
  TARGET_LINK_LIBRARIES(InVivoTrackingOffline igtcuda)
ENDIF()

IF(IGT_USE_OPENCL)
  TARGET_LINK_LIBRARIES(InVivoTrackingOffline igtopencl)
ENDIF()

# Installation code
IF(NOT IGT_INSTALL_NO_EXECUTABLES)
  FOREACH(EXE_NAME InVivoTrackingOffline) 
    INSTALL(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${IGT_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${IGT_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${IGT_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  ENDFOREACH(EXE_NAME) 
ENDIF(NOT IGT_INSTALL_NO_EXECUTABLES)
