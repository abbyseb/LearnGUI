function outputFile = rtkGenerateElektaGeometryFile(imageDBF,frameDBF,uid,outputFile)
%% rtkGenerateElektaGeometryFile(imageDBF,frameDBF,uid,outputFile)
% ------------------------------------------
% FILE   : rtkGenerateElektaGeometryFile.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-11-03  Created.
% ------------------------------------------
% PURPOSE
%   Generate RTK geometry XML file for Elekta acquisition.
% ------------------------------------------
% INPUT
%   imageDBF:   File path to the image dbf file. (default: ui selection)
%   frameDBF:   File path to the frame dbf file. (default: ui selection)
%   uid:        The uid of the projection folder. (default: prompt the user
%                                                           for input)
%   outputFile: The file path of the output geometry xml file. 
%               (default: RTKGeometry.xml at current directory)
% ------------------------------------------
% OUTPUT
%   outputFile: Simply return the file path of the output geometry xml file.

%% 
    
if nargin < 1
    [imageDBF,ind] = uigetfilepath({'*.DBF;*.dbf;','Image DBF files (*.DBF,*.dbf)';},...
        'Select an image DBF file');
    if ind==0
        error('ERROR: no image DBF file was selected. Try again.');
    end
end

if nargin < 2
    [frameDBF,ind] = uigetfilepath({'*.DBF;*.dbf;','Frame DBF files (*.DBF,*.dbf)';},...
        'Select a frame DBF file');
    if ind==0
        error('ERROR: no frame DBF file was selected. Try again.');
    end
end

if nargin < 3
    uid = input('Please enter the UID of the projection set:','s');
end

if nargin < 4
    outputFile = fullfile(pwd,'RTKGeometry.xml');
end

% Return full file path
if isempty(fileparts(outputFile))
    outputFile = fullfile(pwd,outputFile);
end

fail = system(['rtkelektasynergygeometry ',...
    '-i "',imageDBF,'" ',...
    '-f "',frameDBF,'" ',...
    '-u "',uid,'" ',...
    '-o "',outputFile,'"']);
if fail~=0
    error('rtkelektasynergygeometry failed for unknown reason.');
end;

return