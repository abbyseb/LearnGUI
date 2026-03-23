# - Find an IGT installation or build tree.

# When IGT is found, the IGTConfig.cmake file is sourced to setup the
# location and configuration of IGT.  Please read this file, or
# IGTConfig.cmake.in from the IGT source tree for the full list of
# definitions.  Of particular interest is IGT_USE_FILE, a CMake source file
# that can be included to set the include directories, library directories,
# and preprocessor macros.  In addition to the variables read from
# IGTConfig.cmake, this find module also defines
#  IGT_DIR  - The directory containing IGTConfig.cmake.  
#             This is either the root of the build tree, 
#             or the lib/InsightToolkit directory.  
#             This is the only cache entry.
#   
#  IGT_FOUND - Whether IGT was found.  If this is true, 
#              IGT_DIR is okay.
#

SET(IGT_DIR_STRING "directory containing IGTConfig.cmake.  This is either the root of the build tree, or PREFIX/lib for an installation.")

# Search only if the location is not already known.
IF(NOT IGT_DIR)
  # Get the system search path as a list.
  IF(UNIX)
    STRING(REGEX MATCHALL "[^:]+" IGT_DIR_SEARCH1 "$ENV{PATH}")
  ELSE(UNIX)
    STRING(REGEX REPLACE "\\\\" "/" IGT_DIR_SEARCH1 "$ENV{PATH}")
  ENDIF(UNIX)
  STRING(REGEX REPLACE "/;" ";" IGT_DIR_SEARCH2 ${IGT_DIR_SEARCH1})

  # Construct a set of paths relative to the system search path.
  SET(IGT_DIR_SEARCH "")
  FOREACH(dir ${IGT_DIR_SEARCH2})
    SET(IGT_DIR_SEARCH ${IGT_DIR_SEARCH} "${dir}/../lib")
  ENDFOREACH(dir)

  #
  # Look for an installation or build tree.
  #
  FIND_PATH(IGT_DIR IGTConfig.cmake
    # Look for an environment variable IGT_DIR.
    $ENV{IGT_DIR}

    # Look in places relative to the system executable search path.
    ${IGT_DIR_SEARCH}

    # Look in standard UNIX install locations.
    /usr/local/lib
    /usr/lib

    # Read from the CMakeSetup registry entries.  It is likely that
    # RTK will have been recently built.
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild1]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild2]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild3]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild4]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild5]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild6]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild7]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild8]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild9]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild10]

    # Help the user find it if we cannot.
    DOC "The ${IGT_DIR_STRING}"
  )
ENDIF(NOT IGT_DIR)

# If IGT was found, load the configuration file to get the rest of the
# settings.
IF(IGT_DIR)
  SET(IGT_FOUND 1)
  INCLUDE(${IGT_DIR}/IGTConfig.cmake)
ELSE(IGT_DIR)
  SET(IGT_FOUND 0)
  IF(IGT_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Please set IGT_DIR to the ${IGT_DIR_STRING}")
  ENDIF(IGT_FIND_REQUIRED)
ENDIF(IGT_DIR)
