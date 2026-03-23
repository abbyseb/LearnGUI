FIND_PACKAGE(Gengetopt)

# Common revision info between applications
FILE(WRITE ${IGT_BINARY_DIR}/igtVersion.ggo "version \"${IGT_VERSION_MAJOR}.${IGT_VERSION_MINOR}.${IGT_VERSION_PATCH}\"\n")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# add path to IGT libraries to the install path
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${IGT_INSTALL_LIB_DIR}")

#=========================================================

# IGT Executables
ADD_SUBDIRECTORY(igtleastsquaresubtract)
ADD_SUBDIRECTORY(igtsearchforbestdvfweight)
ADD_SUBDIRECTORY(igttesttemplate)
ADD_SUBDIRECTORY(igtvarianximgeometry)
ADD_SUBDIRECTORY(igtsimulatedgeometry)
ADD_SUBDIRECTORY(igtregisterprojections)
ADD_SUBDIRECTORY(igtmhatodicomseries)
ADD_SUBDIRECTORY(igttransformwithdvf)

# IGT applications that use Qt
IF(IGT_USE_Qt)
	ADD_SUBDIRECTORY(IGTQtTemplate)
	ADD_SUBDIRECTORY(InVivoTrackingOffline)
ENDIF()

# Cuda only applications
IF(IGT_USE_CUDA)
ADD_SUBDIRECTORY(igtcudamcfdk)
ENDIF()

# RTK Executables
ADD_SUBDIRECTORY(RTKApplications/rtkattgeometry)
ADD_SUBDIRECTORY(RTKApplications/rtki0estimation)
ADD_SUBDIRECTORY(RTKApplications/rtkprojections)
ADD_SUBDIRECTORY(RTKApplications/rtkparkershortscanweighting)
ADD_SUBDIRECTORY(RTKApplications/rtkwangdisplaceddetectorweighting)
ADD_SUBDIRECTORY(RTKApplications/rtkfdktwodweights)
ADD_SUBDIRECTORY(RTKApplications/rtkramp)
ADD_SUBDIRECTORY(RTKApplications/rtkfdk)
ADD_SUBDIRECTORY(RTKApplications/rtkfourdfdk)
ADD_SUBDIRECTORY(RTKApplications/rtkmkb)
ADD_SUBDIRECTORY(RTKApplications/rtkinlinefdk)
ADD_SUBDIRECTORY(RTKApplications/rtkamsterdamshroud)
ADD_SUBDIRECTORY(RTKApplications/rtkforwardprojections)
ADD_SUBDIRECTORY(RTKApplications/rtkbackprojections)
ADD_SUBDIRECTORY(RTKApplications/rtkrayboxintersection)
ADD_SUBDIRECTORY(RTKApplications/rtksart)
ADD_SUBDIRECTORY(RTKApplications/rtkpiccs)
ADD_SUBDIRECTORY(RTKApplications/rtkrayquadricintersection)
ADD_SUBDIRECTORY(RTKApplications/rtkprojectgeometricphantom)
ADD_SUBDIRECTORY(RTKApplications/rtkdrawgeometricphantom)
ADD_SUBDIRECTORY(RTKApplications/rtkprojectshepploganphantom)
ADD_SUBDIRECTORY(RTKApplications/rtkfieldofview)
ADD_SUBDIRECTORY(RTKApplications/rtksubselect)
ADD_SUBDIRECTORY(RTKApplications/rtkdrawshepploganphantom)
ADD_SUBDIRECTORY(RTKApplications/rtkextractshroudsignal)
ADD_SUBDIRECTORY(RTKApplications/rtkmedian)
ADD_SUBDIRECTORY(RTKApplications/rtklut)
ADD_SUBDIRECTORY(RTKApplications/rtktotalvariationdenoising)
ADD_SUBDIRECTORY(RTKApplications/rtkadmmwavelets)
ADD_SUBDIRECTORY(RTKApplications/rtkadmmtotalvariation)
ADD_SUBDIRECTORY(RTKApplications/rtkfourdrooster)
ADD_SUBDIRECTORY(RTKApplications/rtkfourdconjugategradient)
ADD_SUBDIRECTORY(RTKApplications/rtkconjugategradient)
ADD_SUBDIRECTORY(RTKApplications/rtkwaveletsdenoising)
#ADD_SUBDIRECTORY(RTKApplications/rtktargettracking)
ADD_SUBDIRECTORY(RTKApplications/rtkvarianobigeometry)
ADD_SUBDIRECTORY(RTKApplications/rtksimulatedgeometry)
ADD_SUBDIRECTORY(RTKApplications/rtkelektasynergygeometry)
ADD_SUBDIRECTORY(RTKApplications/rtkelektaxmlgeometry)
ADD_SUBDIRECTORY(RTKApplications/rtkgetelektaangles)
ADD_SUBDIRECTORY(RTKApplications/rtkdigisensgeometry)
ADD_SUBDIRECTORY(RTKApplications/rtkxradgeometry)
ADD_SUBDIRECTORY(RTKApplications/rtkimagxgeometry)

#=========================================================