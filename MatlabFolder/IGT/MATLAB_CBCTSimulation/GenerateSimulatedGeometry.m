function GenerateSimulatedGeometry(angles,outputFile,sid,sdd,detOffsetXs,detOffsetYs)
%% GenerateSimulatedGeometry(angles,outputFile,sid,sdd,detOffsetXs,detOffsetYs,outputFile)
% ------------------------------------------
% FILE   : GenerateSimulatedGeometry.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2017-12-15  Created.
% ------------------------------------------
% PURPOSE
%   Generate simulated RTK geometry file.
% ------------------------------------------
% INPUT
%   angles:         A vector of angle values.
%   outputFile:     The output filename.
%   sid:            Source-to-isocenter distance in mm. Default = 1000.
%   sdd:            Source-to-detector distance in mm. Default = 1500.
%   detOffsetXs:    A vector of detector horizontal offset values.
%                   Default = zeros(length(angles),1);
%   detOffsetYs:    A vector of detector vertical offset values.
%                   Default = zeros(length(angles),1);

%% Input check
if nargin < 3
    sid = 1000;
end

if nargin < 4
    sdd = 1500;
end

if nargin < 5
    detOffsetXs = zeros(length(angles),1);
end

if nargin < 6
    detOffsetYs = zeros(length(angles),1);
end

igtGeo = which('igtsimulatedgeometry.exe');

%% Generate geometry
angleFile = tempname;
detFile = tempname;
csvwrite(angleFile,angles(:));
csvwrite(detFile,[detOffsetXs(:), detOffsetYs(:)]);

system([igtGeo,' -o "',outputFile,'" -i "',angleFile,'" -d "',detFile,'" ',...
    '--sid ',num2str(sid,'%f'),' --sdd ',num2str(sdd,'%f')]);