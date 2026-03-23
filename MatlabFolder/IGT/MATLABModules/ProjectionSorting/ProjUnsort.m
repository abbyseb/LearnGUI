function ProjUnsort(projDir,regExp)
%% ProjSort(projDir,regExp)
% ------------------------------------------
% FILE   : ProjUnsort.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-06   Created
% ------------------------------------------
% PURPOSE
%   Remove the binning prefix from the filename.
%
% ------------------------------------------
% INPUT
%
%  projDir
%   The full path to the directory containing the entire projection set.
%   The user may input empty string '' in order to use a directory
%   selection dialogue.
%
%  regExp (Optional)
%   The regular expression of the projection images (e.g. 'SPI').
%   The user can input empty string '', to simply use all the projection
%   files in the folder.

%% Input check

if nargin < 2
    regExp = '';
end;

% Check if projection directory exists, and if it does, check if projection
% images matching the regular expression are in there
projList = lscp(fullfile(projDir,['*',regExp,'*']));
nproj = size(projList,1);
if nproj == 0
    error('ERROR: There is no projection file matching the regular expression in the specified directory. Please try again.');
end

%% Sorting projections by renaming them

for k = 1:nproj
    [~,projName,projExt] = fileparts(strtrim(projList(k,:)));
    tmp = regexp(projName,'_','split');
    tmp_ind = find( cellfun(@(x) length(findstr(x,'bin')), tmp) == 0 );
    if isempty(tmp_ind)
        tmp_ind = find(cellfun(@length,tmp) == max(cellfun(@length,tmp)));
    end;
    newName = tmp{tmp_ind};
    if ispc
        rename_command = 'rename ';
    elseif isunix || ismac
        rename_command = 'mv ';
    else
        rename_command = 'rename ';
    end;
    [fail,~] = system([rename_command,'"',strtrim(projList(k,:)),'"',' ','"',[newName,projExt],'"']);
    if fail
        error('ERROR: Unknown error with cmd command "rename".');
    end;
end;

return;
