function dphModel = readDphDRRModel(dphModelFile)
%% model = readDphDRRModel(dphModelFile)
% ------------------------------------------
% FILE   : readDphDRRModel.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-03-03  Created.
% ------------------------------------------
% PURPOSE
%  Read diaphragm DRR model file (.bin file) into a MATLAB struct of
% 2D diaphragm model that can directly be used for trackDiaphragm_DRR.m.
% ------------------------------------------
% INPUT
%   dphModelFile:   The .bin model file that records all the 2D points,
%                   indices, and weights.
%
% OUTPUT
%   model:          The 2D diaphragm model in a MATLAB struct.
%% Getting filenames if not input
if nargin < 1
    disp('Select 2D diaphragm model file.');
    dphModelFile = uigetfilepath;
end

%% Read model file
fid = fopen(dphModelFile,'r');

dphModel.SID = fread(fid,1,'double');
dphModel.SDD = fread(fid,1,'double');
dphModel.VolOffset = [fread(fid,1,'double'), fread(fid,1,'double'), fread(fid,1,'double')];
dphModel.ProjSize = [fread(fid,1,'int'), fread(fid,1,'int')];
dphModel.ProjSpacing = [fread(fid,1,'double'), fread(fid,1,'double')];
dphModel.PCVec = [fread(fid,1,'double'), fread(fid,1,'double'), fread(fid,1,'double');...
    fread(fid,1,'double'), fread(fid,1,'double'), fread(fid,1,'double')];

nang = 0;
ang = fread(fid,1,'double');
while ~isempty(ang)
    nang = nang + 1;
    dphModel.Angles(nang) = ang;
    
    dphModel.G(:,:,nang) = reshape(fread(fid,12,'double'),[3,4]);
    
    % Right
    N = fread(fid,1,'uint32');
    % Index starts at 1 for MATLAB but 0 for other languages
    dphModel.Idx1D_R{nang} = fread(fid,N,'uint32') + 1;
    if isempty(dphModel.Idx1D_R{nang})
        dphModel.Idx2D_R{nang} = [];
    else
        [dphModel.Idx2D_R{nang}(:,1),dphModel.Idx2D_R{nang}(:,1)] = ...
            ind2sub(dphModel.ProjSize,dphModel.Idx1D_R{nang});
    end
    dphModel.DRR_R{nang} = fread(fid,N,'float');
    dphModel.DstWght_R{nang} = fread(fid,N,'float');
    dphModel.MaxDst_R(nang,1) = fread(fid,1,'float');
    
    % Left
    N = fread(fid,1,'uint32');
    % Index starts at 1 for MATLAB but 0 for other languages
    dphModel.Idx1D_L{nang} = fread(fid,N,'uint32') + 1;
    if isempty(dphModel.Idx1D_L{nang})
        dphModel.Idx2D_L{nang} = [];
    else
        [dphModel.Idx2D_L{nang}(:,1),dphModel.Idx2D_L{nang}(:,1)] = ...
            ind2sub(dphModel.ProjSize,dphModel.Idx1D_L{nang});
    end
    dphModel.DRR_L{nang} = fread(fid,N,'float');
    dphModel.DstWght_L{nang} = fread(fid,N,'float');
    dphModel.MaxDst_L(nang,1) = fread(fid,1,'float');
    
    ang = fread(fid,1,'double');
end

fclose(fid);