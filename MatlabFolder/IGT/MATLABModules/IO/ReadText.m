function result = ReadText(filename)
%% result = ReadText(filename)
% ------------------------------------------
% FILE   : ReadText.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-09-13  Created.
% ------------------------------------------
% PURPOSE
%   Read text files which each line saved in a cell.
% ------------------------------------------
% INPUT
%   filename:   The full path to the text file.
% ------------------------------------------
% OUTPUT
%   result:     A cell array with each text line in a cell.
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
fid = fopen(filename,'r');
k = 0;
tline = fgetl(fid);
while ischar(tline)
    k = k + 1;
    result{k} = tline;
    tline = fgetl(fid);
end
fclose(fid);

end