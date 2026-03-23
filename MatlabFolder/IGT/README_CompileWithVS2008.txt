There are a few issues when compiling IGT with VS 2008.
The issues and the corresponding solutions are outlined below.

1. itk_kwiml.h cannot be found in the igtcuda project

To solve this, copy itk_kwiml.h from ITKBuildDir\Modules\ThirdParty\KWIML\src to IGTSourceDir\Code
Also copy the itkkwiml folder from ITKSourceDir\Modules\ThirdParty\KWIML\src to IGTSourceDir\Code

2. .data() is not a member of std::vector<T_y>

To solve this, change "variable.data()" to "&variable[0]" in the lines that caused this error

3. igtcuda cannot locate ******.cu.obj files

To solve this, open IGTBuildDir\Code\igtcuda.vcproj in a text editor, 
and replace all the $(ConfigurationName) with Release or Debug depending on your compile configuration.