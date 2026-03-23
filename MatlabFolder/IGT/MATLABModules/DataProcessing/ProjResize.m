function ProjResize(inputfl,outputdir,newsize)
%% ProjResize(inputfl,outputdir,newsize)
% ------------------------------------------
% FILE   : ProjResize.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-02-28  Created
%          2013-11-15  Modified to support att format.
% ------------------------------------------
% PURPOSE
%   This code resizes the hnc images in the input file list and output it
%   to the output directory. This is usually used to down sample the hnc
%   file to achieve faster reconstruction and lower noise level.
%
%   ProjResize(inputfl,outputdir,newsize)
% ------------------------------------------
% INPUT
%   inputfl:        The input file list (can be obtained using ls('*.hnc'))
%   outputdir:      The output directory.
%   newsize:        The [numrows,numcols] of the new hnc file. Bear in mind
%                   that the row corresponds to X and column correspond to
%                   Y.
% ------------------------------------------

%% 

HEADER_BYTES = 512;

fsep = fsepcp;
nproj = size(inputfl);  nproj = nproj(1);

for k = 1:nproj
    [info,M] = ProjRead(inputfl(k,:));
    
    pixsize = (info.uiFileLength - HEADER_BYTES) / info.uiSizeX / info.uiSizeY;
    info.dIDUResolutionX = info.dIDUResolutionX * info.uiSizeX / newsize(1);
    info.dIDUResolutionY = info.dIDUResolutionY * info.uiSizeY / newsize(2);
    info.uiSizeX = newsize(1);  info.uiSizeY = newsize(2);
    info.uiFileLength = HEADER_BYTES + prod(newsize) * pixsize;
    M = imresize(M,newsize,'bicubic');
    
    [~,projName,projExt] = fileparts(inputfl(k,:));
    
    ProjWrite(info,M,fullfile(outputdir,[projName,projExt]));
    
end

return;