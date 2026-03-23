function writeGTVDRRModel(gtvModel,outputFile)
%% writeGTVDRRModel(gtvModel,outputFile)
% ------------------------------------------
% FILE   : writeGTVDRRModel.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-03-06  Created.
% ------------------------------------------
% PURPOSE
%  Write GTV DRR gtvModel to a binary file.
% ------------------------------------------
%   gtvModel:      The GTV DRR gtvModel MATLAB struct.
%   modelFile:  The filename of the output gtvModel file (.bin).

%% Write to gtvModel file
fid = fopen(outputFile,'w');

% Put everything into an uint8 array and write at one go
bufArr = uint8([]);

bufArr = [bufArr,typecast(double(gtvModel.SID),'uint8')];
bufArr = [bufArr,typecast(double(gtvModel.SDD),'uint8')];
bufArr = [bufArr,typecast(double(gtvModel.VolOffset(1)),'uint8')];
bufArr = [bufArr,typecast(double(gtvModel.VolOffset(2)),'uint8')];
bufArr = [bufArr,typecast(double(gtvModel.VolOffset(3)),'uint8')];
bufArr = [bufArr,typecast(int32(gtvModel.ProjSize(1)),'uint8')];
bufArr = [bufArr,typecast(int32(gtvModel.ProjSize(2)),'uint8')];
bufArr = [bufArr,typecast(double(gtvModel.ProjSpacing(1)),'uint8')];
bufArr = [bufArr,typecast(double(gtvModel.ProjSpacing(2)),'uint8')];

bufArr = [bufArr,typecast(double(gtvModel.GTVExhPos(:)'),'uint8')];
bufArr = [bufArr,typecast(double(gtvModel.GTVInhPos(:)'),'uint8')];
bufArr = [bufArr,typecast(double(gtvModel.GTVPosCov(:)'),'uint8')];

bufArr = [bufArr,typecast(double(gtvModel.ViewScoreThreshold),'uint8')];

for nang = 1:length(gtvModel.Angles)
    bufArr = [bufArr,typecast(double(gtvModel.Angles(nang)),'uint8')];
    
    tmpArr = gtvModel.G(:,:,nang);
    bufArr = [bufArr,typecast(double(tmpArr(:)'),'uint8')];
    
    % Corners. Minus one because only MATLAB indices start at 1
    bufArr = [bufArr,typecast(uint16(gtvModel.Corners(1,1,nang) - 1),'uint8')];
    bufArr = [bufArr,typecast(uint16(gtvModel.Corners(2,1,nang) - 1),'uint8')];
    bufArr = [bufArr,typecast(uint16(gtvModel.Corners(1,2,nang) - 1),'uint8')];
    bufArr = [bufArr,typecast(uint16(gtvModel.Corners(2,2,nang) - 1),'uint8')];
    
    % DRR
    bufArr = [bufArr,typecast(single(gtvModel.DRR{nang}(:)'),'uint8')];
    % Original DRR Min, Max, Mean, Std
    bufArr = [bufArr,typecast(single(gtvModel.DRRMin(nang,1)),'uint8')];
    bufArr = [bufArr,typecast(single(gtvModel.DRRMax(nang,1)),'uint8')];
    bufArr = [bufArr,typecast(single(gtvModel.DRRMean(nang,1)),'uint8')];
    bufArr = [bufArr,typecast(single(gtvModel.DRRStd(nang,1)),'uint8')];
    % DRR CNR
    bufArr = [bufArr,typecast(single(gtvModel.ViewScore(nang,1)),'uint8')];
    
    % N contour points
    bufArr = [bufArr,typecast(uint32(size(gtvModel.ContourIdx1D{nang},1)),'uint8')];
    
    % Contour point indices
    bufArr = [bufArr,typecast(uint32(gtvModel.ContourIdx1D{nang}(:)'),'uint8')];
end

fwrite(fid,bufArr,'uint8');

fclose(fid);