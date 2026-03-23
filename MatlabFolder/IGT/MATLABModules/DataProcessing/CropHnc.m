function CropHnc(fl,outputdir,corners)
%%  CropHnc(fl,outputdir,corners)
% ------------------------------------------
% FILE   : CropHnc.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2016-07-26 Created.
% ------------------------------------------
% PURPOSE
%   Crop hnc images.
% ------------------------------------------
% INPUT
%   fl:             The input file list in cell format. (Use "lscell.m")
%   outputdir:      The output directory.
%   corners:        The corners that define the region the images will be
%                   cropped to. [x1,y1; x2,y2]
% ------------------------------------------

%% 

if ~exist(outputdir,'dir')
    mkdir(outputdir);
end

for k = 1:length(fl)
    [~,name,ext] = fileparts(fl{k});
    [info,P] = HncRead(fl{k});
    if any(corners(:)<1)
        error('ERROR: corner indices cannot be smaller than 1');
    end
    if corners(2,1) > size(P,1) || corners(2,2) > size(P,2)
        error('ERROR: corner indices cannot be greater than image size');
    end
    info.uiSizeX = corners(2,1) - corners(1,1) + 1;
    info.uiSizeY = corners(2,2) - corners(1,2) + 1;
    info.uiFileLength = 512 + 2 * info.uiSizeX * info.uiSizeY;
    info.uiActualFileLength = info.uiFileLength;
    P = P(corners(1,1):corners(2,1),corners(1,2):corners(2,2));
    HncWrite(info,P,fullfile(outputdir,[name,ext]));
end

return;