
===SET UP===
To run XCAT using the MATLAB codes in this folder, simply include "MATLAB_CBCTSimulation" in your MATLAB path.
XCAT does NOT need to be installed or included in the system path.
============

===License===
If you encounter an error message about the license being expired, replace the "xcat2.cfg" file in your simulation folder with the one in "Development\CBCTTools\XCAT".
=============

===Test case===
To run on an example case, copy all the files in "Example" to your own directory, and run this in MATLAB in the directory:
XCATProjSimulation;
===============

===Full simulation workflow===
1. Create MotionTrace.csv

This is a N x 4 or N x 3 csv file.
First column is diaphragm trace.
Second column is chest trace, which usually matches exactly with the diaphragm trace, but can be modified to simulate lag between chest and diaphragm motion.
Third column is cardiac phase. 
Fourth column is tumor trace. If the third column is not written, the tumor will be scaled to the overal diaphragm and chest motion.

Note that the tumor trace is defined under the same coordinate and scaling system with the diaphragm and chest trace. If the tumor trace is identical to the diaphragm trace, the tumor will move as much as the diaphragm does!

2. Create Geometry.xml file
This file determines the simulation geometry.
It can be generated using the function "GenerateSimulatedGeometry.m".

3. Edit the "Phantom.par" parameter to your need, as well as all the Lesion_XXX.par files.
Note that you can have multiple leasion parameter files for spherical lesions like this:
"Lesion_Lung01.par" "Lesion_Lung02.par" ......

4. Set lesion location
For assistance in putting the spherical lesion at the right location, use the MATLAB function "SetXCATLesionLocation.m".
This only works for spherical lesion parameter file and not for heart lesion and plaque.

5. Run the simulation using "XCATProjSimulation.m"
==============================

Contact Dr Andy Shieh <andy.shieh@sydney.edu.au> if you have any questions.