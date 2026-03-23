function outputFile = rtkGenerateVarianGeometryFile(projDir,rexp,configXML,outputFile)
%% outputFile = rtkGenerateVarianGeometryFile(projDir,rexp,configXML,outputFile)
% ------------------------------------------
% FILE   : rtkGenerateVarianGeometryFile.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-11-03  Created.
% ------------------------------------------
% PURPOSE
%   Generate RTK geometry XML file for Varian acquisition.
% ------------------------------------------
% INPUT
%   projDir:    Path to the projection directory. (default: ui selection)
%   rexp:       Regular expression. (default: 'hn')
%   configXML:  File path to the geometry configuation XML file from vendor.
%               (default: ui selection)
%               (There are two pre-saved configuration files in the
%               4DCBCTRecon library. You can access their filepath in
%               MATLAB by which('VarianHalfFan.xml') and
%               which('VarianFullFan.xml') )
%   outputFile: The file path of the output geometry xml file.
%               (default: RTKGeometry.xml in the current directory)
% ------------------------------------------
% OUTPUT
%   outputFile: Simply return the file path of the output geometry xml file.

%% 
    
if nargin < 1
    projDir = uigetdir(pwd,'Select the projection directory');
    if projDir==0
        error('ERROR: no projection directory was selected. Try again.');
    end;
end

if nargin < 2
    rexp = 'hn';
end

if nargin < 3
    [configXML,ind] = uigetfilepath({'*.xml;*.XML;','Geometry configuration XML (*.xml,*.XML)';},...
        'Select a geometry configuration xml file');
    if ind==0
        error('ERROR: no geometry configuration xml file was selected. Try again.');
    end
end

if nargin < 4
    outputFile = fullfile(pwd,'RTKGeometry.xml');
end

% Return full file path
if isempty(fileparts(outputFile))
    outputFile = fullfile(pwd,outputFile);
end

fail = system(['rtkvarianobigeometry --nsort on ',...
    '-p "',projDir,'" ',...
    '-r "',rexp,'" ',...
    '-x "',configXML,'" ',...
    '-o "',outputFile,'"']);
if fail~=0
    error('rtkvarianobigeometry failed for unknown reason.');
end;

return