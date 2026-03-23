function result = ReadElastixRigidResult(filename)
%% result = ReadElastixRigidResult(filename)
% ------------------------------------------
% FILE   : ReadElektaRigidResult.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-07-05  Created.
% ------------------------------------------
% PURPOSE
%   Read Elastix rigid registration result.
% ------------------------------------------
% INPUT
%   filename:   The full path to the result text file.
% ------------------------------------------
% OUTPUT
%   result:     Result structure recording the shift (in mm) and rotation
%               (in degrees).
% ------------------------------------------

%%
% If no input filename => Open file-open-dialog
if nargin < 1
    
    % go into a default directory
    DefaultDir = pwd;
    
    % get the input file (hnc) & extract path & base name
    [FileName,PathName] = uigetfile( {'*.txt;','Text files (*.txt)'}, ...
        'Select the text file to be read', ...
        DefaultDir);
    
    % catch error if no file selected
    if isnumeric(FileName)
        error('ERROR: No file selected. \n');
    end
    
    % make same format as input
    filename = fullfile(PathName, FileName);
    filename = strtrim(filename);
end

%% Read
result = struct;
fid = fopen(filename,'r');
tline = fgetl(fid);
tag = '(TransformParameters';
while ischar(tline)
    if strcmpi(tline(1:min(length(tag),length(tline))),tag)
        parts = regexp(tline,')','split');
        parts = regexp(parts{1},' ','split');
        result.Translation = [str2double(parts{5}); str2double(parts{6}); str2double(parts{7})];
        result.Rotation = [str2double(parts{2}); str2double(parts{3}); str2double(parts{4})] * 180 / pi;
        break;
    else
        tline = fgetl(fid);
    end
end
fclose(fid);