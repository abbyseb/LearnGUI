function ProjSort(projDir,binInfo,regExp)
%% ProjSort(projDir,binInfo,regExp)
% ------------------------------------------
% FILE   : ProjSort.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-11-22   Created
% ------------------------------------------
% PURPOSE
%   Renaming projections and sorting them into different bins. 4D recon can
%   then later be done by using different regular expression for projection
%   input.
%   For example, a set of projections: SPI00001.att, SPI00002.att, SPI00003.att...
%   will be sorted into something like:
%   SPI00001_bin01.att, SPI00002_bin01.att, SPI00003_bin02.att, ...
%   The sortInfo can be obtained using the other phase/displacement module 
%   (e.g. phasecal then phase2bin).
%
% ------------------------------------------
% INPUT
%
%  projDir
%   The full path to the directory containing the entire projection set.
%   The user may input empty string '' in order to use a directory
%   selection dialogue.
%
%  binInfo
%   The sorting information in a vector with the same length as the number
%   of projection images. Each entry needs to be a positive integer
%   specifying the particular respiratory bin a projection should be copied
%   to.
%   binInfo can be obtained using the function "phase2bin", which converts a
%   phase signal to binning information.
%
%  regExp (Optional)
%   The regular expression of the projection images (e.g. 'SPI').
%   The user can input empty string '', to simply use all the projection
%   files in the folder.

%% Input check

if nargin < 3
    regExp = '';
end;

% Check if projection directory exists, and if it does, check if projection
% images matching the regular expression are in there
projList = lscp(fullfile(projDir,['*',regExp,'*']));
nproj = size(projList,1);
if nproj == 0
    error('ERROR: There is no projection file matching the regular expression in the specified directory. Please try again.');
end
    
% Check if sortInfo is valid
if ~isvector(binInfo)
    error('ERROR: sortInfo must be a vector.');
elseif nproj ~= length(binInfo)
    error('ERROR: The number of projection images does not match the length of sortInfo.');
elseif ~all(mod(binInfo,1)==0) | ~all(binInfo > 0)
    error('ERROR: sortInfo must contain only positive integer entries.');
end

%% Sorting projections by renaming them

for k = 1:nproj
    [~,projName,projExt] = fileparts(strtrim(projList(k,:)));
    newName = [projName,num2str(binInfo(k),'_bin%02d'),projExt];
    if ispc
        rename_command = 'rename ';
    elseif isunix || ismac
        rename_command = 'mv ';
    else
        rename_command = 'rename ';
    end;
    [fail,~] = system([rename_command,'"',strtrim(projList(k,:)),'"',' ','"',newName,'"']);
    if fail
        error('ERROR: Unknown error with cmd command "rename".');
    end;
end;

return;
