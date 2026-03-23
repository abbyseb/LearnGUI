function [outputImgFile,outputTransFile] = elastixRigid(fixImgFile,movingImgFile,outputImgFile,outputTransFile)
%% [outputImgFile,outputTransFile] = elastixRigid(fixImgFile,movingImgFile,outputImgFile,outputTransFile)
% ------------------------------------------
% FILE   : elastixRigid.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-11-19  Created.
% ------------------------------------------
% PURPOSE
%   Perform rigid registration using elastix.
% ------------------------------------------
% INPUT
%   fixImgFile:     File path to the fixed image file.
%   movingImgFile:  File path to the moving image file.
%   outputImgFile:  The path of the output image file. (default: [movingImgFile]_Rigid_[fixImgFile].mha at current dircetory)
%   outputTransFile:The path of the output transform parameter file. (default: [movingImgFile]_Rigid_[fixImgFile].txt at current dircetory)
% ------------------------------------------
% OUTPUT
%   outputImgFile:  Simply echo outputImgFile.
%   outputTransFile:Simply echo outputTransFile.

%% Input check
    
if nargin < 1 || isempty(fixImgFile)
    [fixImgFile,ind] = uigetfilepath({'*.mha;*.MHA;','MHA files (*.mha,*.MHA)';},...
        'Select the fixed image');
    if ind==0
        error('ERROR: no fixed image was selected. Try again.');
    end
end

if nargin < 2 || isempty(movingImgFile)
    [movingImgFile,ind] = uigetfilepath({'*.mha;*.MHA;','MHA files (*.mha,*.MHA)';},...
        'Select the moving image');
    if ind==0
        error('ERROR: no moving image was selected. Try again.');
    end
end

[~,fixName] = fileparts(fixImgFile);
[~,movingName] = fileparts(movingImgFile);

if nargin < 3 || isempty(outputImgFile)
    outputImgFile = fullfile(pwd,['[',movingName,']_Rigid_[',fixName,'].mha']);
end;

if nargin < 4 || isempty(outputTransFile)
    outputTransFile = fullfile(pwd,['[',movingName,']_Rigid_[',fixName,'].txt']);
end;

%% Rigid registration via Elastix

% Temporary directory for registration output
tmpDir = tempname;
mkdir(tmpDir);

% Elastix
fail = system(['elastix -f "',fixImgFile,'" -m "',movingImgFile,'" -p "',which('Elastix_Rigid.txt'),'" -out "',tmpDir,'" >nul 2>&1']);
if fail; error('ERROR: Elastix failed for unknown reason.'); end;

% Copy elastix output
copyfile(fullfile(tmpDir,'result.0.mha'),outputImgFile);
copyfile(fullfile(tmpDir,'TransformParameters.0.txt'),outputTransFile);

% Delete temporary files
if exist(tmpDir,'dir'); rmdir(tmpDir,'s');  end;

return