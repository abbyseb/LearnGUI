function writeDphDRRModel(dphModel,modelFile)
%% writeDphDRRModel(dphModel,modelFile)
% ------------------------------------------
% FILE   : writeDphDRRModel.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-03-03  Created.
% ------------------------------------------
% PURPOSE
%  Write diaphragm DRR model (input as a MATLAB struct) to diaphragm model
%  file (.bin file).
% ------------------------------------------
% INPUT
%   dphModel:      The 2D diaphragm dphModel MATLAB struct.
%   modelFile:      The filename of the output dphModel file (.bin).

%% Write to dphModel file
fid = fopen(modelFile,'w');

% Put everything into an uint8 array and write at one go
bufArr = uint8([]);

bufArr = [bufArr,typecast(double(dphModel.SID),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.SDD),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.VolOffset(1)),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.VolOffset(2)),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.VolOffset(3)),'uint8')];
bufArr = [bufArr,typecast(int32(dphModel.ProjSize(1)),'uint8')];
bufArr = [bufArr,typecast(int32(dphModel.ProjSize(2)),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.ProjSpacing(1)),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.ProjSpacing(2)),'uint8')];

bufArr = [bufArr,typecast(double(dphModel.PCVec(1,1)),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.PCVec(1,2)),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.PCVec(1,3)),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.PCVec(2,1)),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.PCVec(2,2)),'uint8')];
bufArr = [bufArr,typecast(double(dphModel.PCVec(2,3)),'uint8')];

for nang = 1:length(dphModel.Angles)
    bufArr = [bufArr,typecast(double(dphModel.Angles(nang)),'uint8')];
    
    tmpArr = dphModel.G(:,:,nang);
    bufArr = [bufArr,typecast(double(tmpArr(:)'),'uint8')];
    
    % Right
    N = length(dphModel.Idx1D_R{nang});
    bufArr = [bufArr,typecast(uint32(N),'uint8')];
    tmpArr = dphModel.Idx1D_R{nang} - 1; % Index starts at 1 for MATLAB but 0 for other languages
    bufArr = [bufArr,typecast(uint32(tmpArr(:)'),'uint8')];
    tmpArr = dphModel.DRR_R{nang};
    bufArr = [bufArr,typecast(single(tmpArr(:)'),'uint8')];
    tmpArr = dphModel.DstWght_R{nang};
    bufArr = [bufArr,typecast(single(tmpArr(:)'),'uint8')];
    tmpArr = dphModel.MaxDst_R(nang);
    bufArr = [bufArr,typecast(single(tmpArr(:)'),'uint8')];
    
    % Left
    N = length(dphModel.Idx1D_L{nang});
    bufArr = [bufArr,typecast(uint32(N),'uint8')];
    tmpArr = dphModel.Idx1D_L{nang} - 1; % Index starts at 1 for MATLAB but 0 for other languages
    bufArr = [bufArr,typecast(uint32(tmpArr(:)'),'uint8')];
    tmpArr = dphModel.DRR_L{nang};
    bufArr = [bufArr,typecast(single(tmpArr(:)'),'uint8')];
    tmpArr = dphModel.DstWght_L{nang};
    bufArr = [bufArr,typecast(single(tmpArr(:)'),'uint8')];
    tmpArr = dphModel.MaxDst_L(nang);
    bufArr = [bufArr,typecast(single(tmpArr(:)'),'uint8')];
end

fwrite(fid,bufArr,'uint8');

fclose(fid);