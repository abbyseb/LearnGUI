function data = ReadTextFileWithHeader(filename)
%% data = ReadTextFileWithHeader(filename)
% ------------------------------------------
% FILE   : ReadTextFileWithHeader.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-12-07 Created.
% ------------------------------------------
% PURPOSE
%   Read a text file with header and numeric data into a struct.
%
% ------------------------------------------
% INPUT
%   filename:       Path to the text file.
% ------------------------------------------
% OUTPUT
%   data:           The data read into a struct.

%% Input check
if nargin < 1
    filename = uigetfilepath;
    
    if isnumeric(filename)
        error('ERROR: No file was selected.');
    end
end

%% Read header
fid = fopen(filename,'r');
line = fgetl(fid);
headers = regexp(line,',','split');

%% Read data
line = fgetl(fid);
count = 0;
while ischar(line)
    count = count + 1;
    dataCells{count} = line;
    line = fgetl(fid);
end
fclose(fid);

%% Put into a struct
data = struct;
for k = 1:length(headers);
    headers{k}(isspace(headers{k})) = '';
    headers{k}(headers{k}=='-') = '_';
    headers{k}(headers{k}=='(') = '_';
    headers{k}(headers{k}==')') = '_';
end

for k = 1:length(dataCells)
    dataParts = regexp(dataCells{k},',','split');
    for kh = 1:length(headers)
        dataEntry = str2double(strtrim(dataParts{kh}));
        if k == 1
            if isnan(dataEntry)
                dataEntry = dataParts{kh};
                data.(headers{kh}){k} = dataEntry;
            else
                data.(headers{kh})(k) = dataEntry;
            end
        else
            if ~iscell(data.(headers{kh})) && ...
                    strcmpi(strtrim(dataParts{kh}),'Infinity')
                data.(headers{kh})(k) = Inf;
            elseif ~iscell(data.(headers{kh}))
                data.(headers{kh})(k) = dataEntry;
            else
                dataEntry = dataParts{kh};
                data.(headers{kh}){k} = dataEntry;
            end
        end
    end
end

return;