function data = ReadCSVFileWithHeader(filename)
%% data = ReadCSVFileWithHeader(filename)
% ------------------------------------------
% FILE   : ReadCSVFileWithHeader.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-04-07 Created.
% ------------------------------------------
% PURPOSE
%   Read the csv file with header into a struct.
%
% ------------------------------------------
% INPUT
%   filename:       Path to the csv file.
% ------------------------------------------
% OUTPUT
%   data:           The data read into a struct.

%% Input check
if nargin < 1
    filename = uigetfilepath(...
        {'*.csv;*.CSV;','CSV file (*.csv,*.CSV)'},...
        'Select the CSV file with header to be read', ...
        pwd);
    
    if isnumeric(filename)
        error('ERROR: No file was selected.');
    end
end

%% Read header
fid = fopen(filename,'r');
line = fgetl(fid);
fclose(fid);
headers = regexp(line,',','split');

%% Read data
tmp = csvread(filename,1,0);

%% Put into a struct
data = struct;
for k = 1:length(headers);
    headers{k}(isspace(headers{k})) = '';
    headers{k}(headers{k}=='-') = '_';
    headers{k}(headers{k}=='(') = '_';
    headers{k}(headers{k}==')') = '_';
    data.(headers{k}) = tmp(:,k);
end

return;