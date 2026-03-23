function MergeMhaVolumes(intImgFile,extImgFile,outputFile,fov)
%% MergeMhaVolumes(intImgFile,extImgFile,outputFile,fov)
% ------------------------------------------
% FILE   : MergeMhaVolumes.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-11-04  Created.
% ------------------------------------------
% PURPOSE
%   Merge two mha volumes together with the specified field-of-view.
%   This is useful when one wants to use the planning CT to fill up regions
%   outside the FOV in a CBCT image.
% ------------------------------------------
% INPUT
%   intFile:    The path of the image for interior region. (default: ui selection)
%               Input "zero" to use zero image.
%   extFile:    The path of the image for exterior region. (default: ui for multiselect)
%               Input "zero" to use zero image.
%   outputFile: The path of the output file. (default: Merged.mha at current dircetory)
%   fov:        Map specifying the interior region. (default: center half of the entire volume)

%% Input check
    
if nargin < 1 || isempty(intImgFile)
    [intImgFile,ind] = uigetfilepath({'*.mha;*.MHA;','MHA files (*.mha,*.MHA)';},...
        'Select the interior volume mha file');
    if ind==0
        intImgFile = 'zero';
    end
end

if nargin < 2 || isempty(extImgFile)
    [extImgFile,ind] = uigetfilepath({'*.mha;*.MHA;','MHA files (*.mha,*.MHA)';},...
        'Select the exterior volume mha file');
    if ind==0
        extImgFile = 'zero';
    end
end

% Read the image
if strcmpi(intImgFile,'zero') && strcmpi(extImgFile,'zero')
    error('ERROR: At least one image must be input.')
elseif strcmpi(extImgFile,'zero')
    [info,M_int] = MhaRead(intImgFile);
    dim = size(M_int);
elseif strcmpi(intImgFile,'zero')
    [info,M_ext] = MhaRead(extImgFile);
    dim = size(M_ext);
else
    [info,M_int] = MhaRead(intImgFile);
    [~,M_ext] = MhaRead(extImgFile);
    dim = size(M_int);
    if ~isequal(dim,size(M_ext))
        error('ERROR: The dimensions of the interior and exterior images must match.');
    end
end

if nargin < 3 || isempty(outputFile)
    outputFile = fullfile(pwd,'Merged.mha');
end;

if nargin < 4
    fov = false(dim);
    fov(round(end/4):round(3*end/4),round(end/4):round(3*end/4),round(end/4):round(3*end/4)) = true;
else
    if ~isequal(dim,size(fov))
    error('ERROR: The dimensions of the images and FOV must match.');
    end
end

%% Merge

if strcmpi(extImgFile,'zero')
    M_int = M_int .* fov;
    MhaWrite(info,M_int,outputFile);
elseif strcmpi(intImgFile,'zero')
    M_ext = M_ext .* (1-fov);
    MhaWrite(info,M_ext,outputFile);
else
    M_int = fov.*M_int + (1-fov).*M_ext;
    MhaWrite(info,M_int,outputFile);
end

return