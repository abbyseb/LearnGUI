function gtvModel = readGTVDRRModel(gtvModelFile)
%% gtvModel = readGTVDRRModel(gtvModelFile)
% ------------------------------------------
% FILE   : readGTVDRRModel.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-03-06  Created.
% ------------------------------------------
% PURPOSE
%  Read GTV DRR model file (.bin file) into a MATLAB struct.
% ------------------------------------------
% INPUT
%   gtvModelFile:   The .bin model file.
%
% OUTPUT
%   gtvModel:       The GTV DRR model in a MATLAB struct.
%% Getting filenames if not input
if nargin < 1
    disp('Select GTV DRR model file.');
    gtvModelFile = uigetfilepath;
end

%% Read model file
fid = fopen(gtvModelFile,'r');

gtvModel.SID = fread(fid,1,'double');
gtvModel.SDD = fread(fid,1,'double');
gtvModel.VolOffset = [fread(fid,1,'double'), fread(fid,1,'double'), fread(fid,1,'double')];
gtvModel.ProjSize = [fread(fid,1,'int'), fread(fid,1,'int')];
gtvModel.ProjSpacing = [fread(fid,1,'double'), fread(fid,1,'double')];

gtvModel.GTVExhPos = [fread(fid,1,'double'), fread(fid,1,'double'), fread(fid,1,'double')];
gtvModel.GTVInhPos = [fread(fid,1,'double'), fread(fid,1,'double'), fread(fid,1,'double')];
gtvModel.GTVPosCov = reshape(fread(fid,9,'double'),[3,3]);
gtvModel.ViewScoreThreshold = fread(fid,1,'double');

nang = 0;
ang = fread(fid,1,'double');
while ~isempty(ang)
    nang = nang + 1;
    gtvModel.Angles(nang) = ang;
    
    gtvModel.G(:,:,nang) = reshape(fread(fid,12,'double'),[3,4]);
    
    gtvModel.Corners(1,1,nang) = uint16(fread(fid,1,'uint16') + 1);
    gtvModel.Corners(2,1,nang) = uint16(fread(fid,1,'uint16') + 1);
    gtvModel.Corners(1,2,nang) = uint16(fread(fid,1,'uint16') + 1);
    gtvModel.Corners(2,2,nang) = uint16(fread(fid,1,'uint16') + 1);
    
    if gtvModel.Corners(2,1,nang) < gtvModel.Corners(1,1,nang) || ...
            gtvModel.Corners(2,2,nang) < gtvModel.Corners(1,2,nang)
        gtvModel.DRR{nang} = [];
    else
        drrDim = gtvModel.Corners(2,:,nang) - gtvModel.Corners(1,:,nang) + 1;
        gtvModel.DRR{nang} = reshape(single(fread(fid,prod(drrDim),'single')),drrDim);
    end
    gtvModel.DRRMin(nang,1) = single(fread(fid,1,'single'));
    gtvModel.DRRMax(nang,1) = single(fread(fid,1,'single'));
    gtvModel.DRRMean(nang,1) = single(fread(fid,1,'single'));
    gtvModel.DRRStd(nang,1) = single(fread(fid,1,'single'));
    gtvModel.ViewScore(nang,1) = single(fread(fid,1,'single'));
    
    % N contour points
    nPts = fread(fid,1,'uint32');
    
    % Contour 1D indices
    gtvModel.ContourIdx1D{nang} = uint32(fread(fid,nPts,'uint32') + 1);
    
    ang = fread(fid,1,'double');
end

fclose(fid);