function result = ReadTagFromRTKGeometry(rtkFile,tag)
%% result = ReadTagFromRTKGeometry(rtkFile,tag)
% ------------------------------------------
% FILE   : ReadAnglesFromRTKGeometry.m
% AUTHOR : Andy Shieh, ACRF Image-X Institute, The University of Sydney
% DATE   : 2018-01-17 Created.
% ------------------------------------------
% PURPOSE
%   Read the data series corresponding to the user specified xml tag from
%   RTK geometry file.
%
% ------------------------------------------
% INPUT
%   rtkFile:       The RTK geometry file path.
%   tag:           The XML tag. e.g. 'ProjectionOffsetX'
% ------------------------------------------
% OUTPUT
%   result:        The extracted data series.

%% 
% If no input filename => Open file-open-dialog
if nargin < 1
    
    % go into a default directory
    DefaultDir = pwd;
    
    % get the input file (hnc) & extract path & base name
    [FileName,PathName] = uigetfile( {'*.xml;*.XML;','RTK geometry file (*.xml,*XML)'}, ...
        'Select an RTK geometry file', ...
        DefaultDir);
    
    % catch error if no file selected
    if isnumeric(FileName)
        result = [];
        error('ERROR: No file selected. \n');
    end
    
    % make same format as input
    rtkFile = fullfile(PathName, FileName);
    rtkFile = strtrim(rtkFile);
end

if nargin < 2
    tag = 'GantryAngle';
end

fid = fopen(rtkFile,'r');
line = fgetl(fid); k = 0;
while(line~=-1);
    if(strfind(line,['<',tag,'>']));
        k = k + 1;
        line = regexp(line,['<',tag,'>'],'split');
        line = regexp(line{2},['</',tag,'>'],'split');
        result(k) = str2double(line{1});
    end;
    line = fgetl(fid);
end;
fclose(fid);

return;