function RPMInfo = VxpRead(filename)
%% RPMInfo = VxpRead(filename)
% ------------------------------------------
% FILE   : VxpRead.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2012-11-02  Created. Borrowed from JK's code.
% ------------------------------------------
% PURPOSE
%   Read Varian .vxp RPM file into a matlab matrix.
% ------------------------------------------
% INPUT
%   filename : The full path to the file.
% ------------------------------------------
% OUTPUT
%   RPMInfo  : [row,amplitude,phase,ttlin,ttlchange;...]
% ------------------------------------------

%% Checking input arguments & Opening the file

RPMInfo = [];

% If no input filename => Open file-open-dialog
if nargin < 1
    
    % go into a default directory
    DefaultDir = pwd;
    
    % get the input file (hnc) & extract path & base name
    [FileName,PathName] = uigetfile( {
        '*.vxp',  'Varian RPM file (*.vxp)'}, ...
        'Select a RPM file', ...
        DefaultDir);
    
    % catch error if no file selected
    if isnumeric(FileName)
        error('ERROR: No file selected. \n');
        return
    end
    
    % make same format as input
    filename = fullfile(PathName, FileName);
    
end

% Open the file
VXPid = fopen(filename,'r');

% catch error if failure to open the file
if VXPid == -1
    error('ERROR: Failure in opening the file. \n');
    return
end
    
%% Reading header and body (borrowed from JK's code)

% Skip .VXP header (usually 10 lines):
fscanf(VXPid,'%s\r\n',10);

% Get first data line:
NEXTLINE = fgetl(VXPid);
line = 1;
VXPline = textscan(NEXTLINE,'%f,%f,%f,%u,%u,%s,%u');

% Convert to RPMInfo format:
RPMInfo(line,:) = [line VXPline{1,1} VXPline{1,2} ...
    double(VXPline{1,5}) 0];

% Get next line:
line = line + 1;
NEXTLINE = fgetl(VXPid);

% Begin reading loop for all lines (EOF gives NEXTLINE == -1):
while NEXTLINE ~= -1
    
    VXPline = textscan(NEXTLINE,'%f,%f,%f,%u,%u,%s,%u');
    
    RPMInfo(line,:) = [line VXPline{1,1} VXPline{1,2} ... 
        double(VXPline{1,5}) double(VXPline{1,5})-RPMInfo(line-1,4)]; 
    
    line = line + 1;
    NEXTLINE = fgetl(VXPid);

end

fclose(VXPid);

return