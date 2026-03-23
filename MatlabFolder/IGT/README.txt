IGT - Image Guidance Toolkit
Author: Andy Shieh
		The University of Sydney, Sydney, Australia
		andy.shieh@sydney.edu.au
		
This is a toolkit for various image guidance tasks such as 4D CBCT reconstruction and tumor tracking.

It includes several major components:	
====================

A. C++ toolkit: This includes the main structure of the IGT directory. It needs to be configured using Cmake.

This C++ toolkit is developed based on the following toolkits:

	1. Insight Toolkit (ITK)
	   http://www.itk.org/
	   The user needs to compile ITK first before compiling IGT.
	   
	2. Reconstruction Toolkit (RTK) by Dr Simon Rit
	   http://www.openrtk.org
	   The user does not need to separately compile RTK.
	   The RTK modules, some in their original forms and some modified, are included in this toolkit, denoted by the prefix "rtk" and located in their own dedicated directories "RTKModules" and "RTKApplications".
	
	3. Qt
	   http://www.qt.io/

====================

B. MATLAB modules: A collection of MATLAB codes that can be used directly by adding "MATLABModules" into the MATLAB path (including subdirectories).

====================

C. Third party executables: A collection of third party open source programs in windows executable forms in "SharedExecutables", including:

	1. http://elastix.isi.uu.nl/
	   Elastix: An ITK-based image registration program.
	   

====================	  
INSTALLATION GUIDES

1. C++ toolkit
Pre-compiled executables are in IGTExecutables.
If the user wish to compile IGT himself, the following installations are required:
	a. Install Qt 5 or newer version (optional)
	    i. Note that the user also has to add Qt bin directory to system PATH variable.
	       Example of Qt bin directory: "C:\Qt\5.5\msvc2013_64\bin"
	    ii.The user also needs to add a new system variable "QT_QPA_PLATFORM_PLUGIN_PATH" pointing to the directory of Qt windows plugin.
	       Example: "C:\Qt\5.5\msvc2013_64\plugins\platforms"
	b. Compile ITK 3.2 or newer version (required)

Step a is only needed for compiling Qt related modules (GUI applications).
	
Then, use Cmake to configure IGT.
When asked for ITK_DIR, select their build directories.
When asked for Qt5_DIR, select Qt/(version)/(compiler info)/lib/cmake/Qt5

2. MATLAB modules:
Include MATLABModules with subdirectories in MATLAB path

3. Third party executables
Include each individual folder that contains an executable in SharedExecutables in the system PATH variable.
	
