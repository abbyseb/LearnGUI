function data = ReadBeltData(filename)
%% data = ReadBeltData(filename)
% ------------------------------------------
% FILE   : ReadBeltData.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-05-13  Created. Modified from Vincent's code.
% ------------------------------------------
% PURPOSE
%   Read .rec belt data.
% ------------------------------------------
% INPUT
%   filename:   The full path to the .rec file.
% ------------------------------------------
% OUTPUT
%   data:       The output data struct.
% ------------------------------------------

%%
startRow = 12;

data = struct;

% If no input filename => Open file-open-dialog
if nargin < 1
    
    % go into a default directory
    DefaultDir = pwd;
    
    % get the input file (hnc) & extract path & base name
    [FileName,PathName] = uigetfile( {'*.rec;','Belt trace file (*.rec)'}, ...
        'Select the belt trace file to be read', ...
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
fileID = fopen(filename,'r');
tline = fgetl(fileID);
firstLine = tline;
k = 0;
while ischar(tline)
    k = k + 1;
    if k >= startRow
        parts = regexp(tline,' ','split');
        data.Displacement(k+1-startRow) = str2double(parts{1});
        data.WaveGuide(k+1-startRow) = str2double(parts{2});
        data.RawTime(k+1-startRow) = str2double(parts{3});
        data.Phase(k+1-startRow) = str2double(parts{4});
    end
    tline = fgetl(fileID);
end
fclose(fileID);

% Read the first line too to get absolute time
fileID = fopen(filename,'r');
firstLine = fgetl(fileID);
fclose(fileID);

%% Process absolute start time
parts = regexp(firstLine,'/','split');

start_day = regexp(parts{1},' ','split');
start_day = str2double(start_day{end});
start_month = str2double(parts{2});
start_year = regexp(parts{3},' ','split');
start_year = str2double(start_year{1});

parts = regexp(parts{3},' ','split');
time_parts = parts{2};
time_parts = regexp(time_parts,':','split');
start_hour = str2double(time_parts{1}) + 12 * strcmpi(parts{end},'PM');
start_min = str2double(time_parts{2});
start_sec = str2double(time_parts{3});

startTime = datetime(start_year,start_month,start_day,start_hour,start_min,start_sec);

%% Put into struct
hourVec = floor((data.RawTime - data.RawTime(1)) / 1000 / 3600);
minVec = floor((data.RawTime - data.RawTime(1) - hourVec * 1000 * 3600) / 1000 / 60);
secVec = mod(data.RawTime - data.RawTime(1),1000 * 60) / 1000;

data.Time = startTime + duration(hourVec,minVec,secVec);

end
