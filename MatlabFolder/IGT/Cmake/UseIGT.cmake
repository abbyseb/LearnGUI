# This file sets up include directories, link directories, and
# compiler settings for a project to use IGT.  It should not be
# included directly, but rather through the IGT_USE_FILE setting
# obtained from IGTConfig.cmake.

# Find ITK (required)
FIND_PACKAGE(ITK REQUIRED)
INCLUDE(${ITK_USE_FILE})

# Add include directories needed to use IGT.
INCLUDE_DIRECTORIES(BEFORE ${IGT_INCLUDE_DIRS})

# Add link directories needed to use RTK.
LINK_DIRECTORIES(${IGT_LIBRARY_DIRS})
