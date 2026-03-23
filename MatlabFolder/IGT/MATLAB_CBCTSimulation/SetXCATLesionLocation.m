function SetXCATLesionLocation(phantomParaFile,lesionParaFile)
%% SetXCATLesionLocation(phantomParaFile,lesionParaFile)
% ------------------------------------------
% FILE   : SetXCATLesionLocation.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2017-12-15  Created.
% ------------------------------------------
% PURPOSE
%   Visualize the XCAT phantom and let the user click on the image to set
%   the lesion location. The lesion location will then be directly written
%   in the lesionParaFile specified. Note that it will take quite a while
%   (several minutes) for the program to generate a XCAT phantom for
%   visualisation.
% ------------------------------------------
% INPUT
%   phantomParaFile:    The phantom parameter file.
%   lesionParaFile:     The lesion parameter file. Note that the lesion
%                       location in this file will be overwritten after the
%                       user selects the desired lesion location.

%% Input check
if nargin < 1
    phantomParaFile = uigetfilepath('*.par','Select phantom parameter file.');
end
if ~ischar(phantomParaFile) || isempty(phantomParaFile)
    error('ERROR: no phantomParaFile was selected.');
end

if nargin < 2
    lesionParaFile = uigetfilepath('*.par','Select lesion parameter file.');
end
if ~ischar(lesionParaFile) || isempty(lesionParaFile)
    error('ERROR: no lesionParaFile was selected.');
end

dxcat2 = which('dxcat2.exe');
if isempty(dxcat2)
    error('ERROR: Could not find dxcat2.exe. Please include "MATLAB_CBCTSimulation" in your MATLAB path.');
end

%% Read pixel spacing and image dimension
paraText = ReadText(phantomParaFile);
for k = 1:length(paraText)
    if length(paraText{k}) > 11 && strcmpi(paraText{k}(1:11),'pixel_width')
        parts = regexp(paraText{k},'=','split');
        parts = regexp(parts{2},'#','split');
        spacing([1,3]) = str2double(parts{1}) * 10;
    elseif length(paraText{k}) > 11 && strcmpi(paraText{k}(1:11),'slice_width')
        parts = regexp(paraText{k},'=','split');
        parts = regexp(parts{2},'#','split');
        spacing(2) = str2double(parts{1}) * 10;
    elseif length(paraText{k}) > 10 && strcmpi(paraText{k}(1:10),'array_size')  
        parts = regexp(paraText{k},'=','split');
        parts = regexp(parts{2},'#','split');
        dim([1,3]) = str2double(parts{1});
    elseif length(paraText{k}) > 10 && strcmpi(paraText{k}(1:10),'startslice')  
        parts = regexp(paraText{k},'=','split');
        parts = regexp(parts{2},'#','split');
        startslice = str2double(parts{1});
    elseif length(paraText{k}) > 8 && strcmpi(paraText{k}(1:8),'endslice')  
        parts = regexp(paraText{k},'=','split');
        parts = regexp(parts{2},'#','split');
        endslice = str2double(parts{1});
    end
end
dim(2) = endslice - startslice + 1;

%% Make sure out_frames = 1
for k = 1:length(paraText)
    if length(paraText{k}) > 10 && strcmpi(paraText{k}(1:10),'out_frames')
        paraText{k} = 'out_frames = 1		# output_frames (# of output time frames )';
    end
end
tempPhantomName = tempname;
WriteText(paraText,[tempPhantomName,'.par']);

%% Generate the phantom for visualisation
[failed,msg] = system([dxcat2,' "',[tempPhantomName,'.par'],'" "',tempPhantomName,'"']);
if failed > 1 || ~isempty(strfind(msg,'Can not open'))
    error(['ERROR: XCAT failed to generate volume for ',[tempPhantomName,'.par'],' with the following error message: ',msg]);
end

fl = lscell([tempPhantomName,'*.bin']);
fid = fopen(fl{1},'r');
M = fread(fid,'float32');
fclose(fid);

fl = lscell([tempPhantomName,'*']);
for k = 1:length(fl)
    system(['del "',fl{k},'"']);
end

%% Visualisation and selection of location
M = single(M);
M = reshape(M,[dim(1),dim(3),dim(2)]);
M = permute(M,[1 3 2]); M = M(:,end:-1:1,end:-1:1);

figH = figure;  imshow3D(permute(M,[2 1 3]),[min(M(:)),max(M(:))]);
z = input('Please scroll to the coronal slice where you want to place the lesion, and type here the slice number:');
while isnan(z)
    z = input('Invalid slice number. Please enter the slice number:');
end
z = round(z);

disp('Please click on the location where the lesion will be placed.');
[x,y] = ginput(1);
x = round(x);
y = round(y);

close(figH);

%% Merging phantom and lesion parameter files, and write lesion location
lesionText = ReadText(lesionParaFile);

for k = 1:length(lesionText)
    if length(lesionText{k}) > 10 && strcmpi(lesionText{k}(1:10),'x_location')
        lesionText{k} = ['x_location = ',num2str(x - 1,'%d'),...
            '		# x coordinate (pixels) to place lesion'];
    elseif length(lesionText{k}) > 10 && strcmpi(lesionText{k}(1:10),'y_location')
        lesionText{k} = ['y_location = ',num2str(dim(3) - z,'%d'),...
            '		# y coordinate (pixels) to place lesion'];
    elseif length(lesionText{k}) > 10 && strcmpi(lesionText{k}(1:10),'z_location')
        lesionText{k} = ['z_location = ',num2str(dim(2) - y,'%d'),...
            '		# z coordinate (pixels) to place lesion'];
    end
end

WriteText(lesionText,lesionParaFile);
end
