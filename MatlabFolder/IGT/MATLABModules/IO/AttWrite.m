function AttWrite(info,M,outputfp)
%% AttWrite(M,info,outputfp)
% ------------------------------------------
% FILE   : AttWrite.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-07-05  Created.
% ------------------------------------------
% PURPOSE
%   Write the input header and image to a customary image format which
%   contains a typical Varian Hnc header and attenuation value image in
%   single format
% ------------------------------------------
% INPUT
%   info:       The header MATLAB struct.
%   M:          The image body, 2D (single).
%   outputfp:   The path of the output file.
% ------------------------------------------

%% Writing header

ATTHEADLENGTH = 512;

fid = fopen(strtrim(outputfp),'w');
if fid == -1
    error('Error: Invalid file path.');
    return
end

% Initialize header
load attHeaderTemplate;
info_out = attHeaderTemplate;   clear attHeaderTemplate;

info_out.('bFileType') = 'ATT PROJECTION FILE             ';

pixeltype = class(M);

switch pixeltype
    case 'single'
        info_out.('uiFileLength') = ATTHEADLENGTH + 4*numel(M);
    otherwise
        info_out.('uiFileLength') = ATTHEADLENGTH + 4*numel(M);     
end

inputFields = fields(info);
for k = 1:length(inputFields);
    info_out.(inputFields{k}) = info.(inputFields{k});
end;

% Writing header file
fwrite(fid,info_out.bFileType,'uint8');
fwrite(fid,info_out.uiFileLength,'uint32');
fwrite(fid,info_out.bChecksumSpec,'uint8');
fwrite(fid,info_out.uiCheckSum,'uint32');
fwrite(fid,info_out.bCreationDate,'uint8');
fwrite(fid,info_out.bCreationTime,'uint8');
fwrite(fid,info_out.bPatientID,'uint8');
fwrite(fid,info_out.uiPatientSer,'uint32');
fwrite(fid,info_out.bSeriesID,'uint8');
fwrite(fid,info_out.uiSeriesSer,'uint32');
fwrite(fid,info_out.bSliceID,'uint8');
fwrite(fid,info_out.uiSliceSer,'uint32');
fwrite(fid,info_out.uiSizeX,'uint32');
fwrite(fid,info_out.uiSizeY,'uint32');
fwrite(fid,info_out.dSliceZPos,'double');
fwrite(fid,info_out.bModality,'uint8');
fwrite(fid,info_out.uiWindow,'uint32');
fwrite(fid,info_out.uiLevel,'uint32');
fwrite(fid,info_out.uiPixelOffset,'uint32');
fwrite(fid,info_out.bImageType,'uint8');
fwrite(fid,info_out.dGantryRtn,'double');
fwrite(fid,info_out.dSAD,'double');
fwrite(fid,info_out.dSFD,'double');
fwrite(fid,info_out.dCollX1,'double');
fwrite(fid,info_out.dCollX2,'double');
fwrite(fid,info_out.dCollY1,'double');
fwrite(fid,info_out.dCollY2,'double');
fwrite(fid,info_out.dCollRtn,'double');
fwrite(fid,info_out.dFieldX,'double');
fwrite(fid,info_out.dFieldY,'double');
fwrite(fid,info_out.dBladeX1,'double');
fwrite(fid,info_out.dBladeX2,'double');
fwrite(fid,info_out.dBladeY1,'double');
fwrite(fid,info_out.dBladeY2,'double');
fwrite(fid,info_out.dIDUPosLng,'double');
fwrite(fid,info_out.dIDUPosLat,'double');
fwrite(fid,info_out.dIDUPosVrt,'double');
fwrite(fid,info_out.dIDUPosRtn,'double');
fwrite(fid,info_out.dPatientSupportAngle,'double');
fwrite(fid,info_out.dTableTopEccentricAngle,'double');
fwrite(fid,info_out.dCouchVrt,'double');
fwrite(fid,info_out.dCouchLng,'double');
fwrite(fid,info_out.dCouchLat,'double');
fwrite(fid,info_out.dIDUResolutionX,'double');
fwrite(fid,info_out.dIDUResolutionY,'double');
fwrite(fid,info_out.dImageResolutionX,'double');
fwrite(fid,info_out.dImageResolutionY,'double');
fwrite(fid,info_out.dEnergy,'double');
fwrite(fid,info_out.dDoseRate,'double');
fwrite(fid,info_out.dXRayKV,'double');
fwrite(fid,info_out.dXRayMA,'double');
fwrite(fid,info_out.dMetersetExposure,'double');
fwrite(fid,info_out.dAcqAdjustment,'double');
fwrite(fid,info_out.dCTProjectionAngle,'double');
fwrite(fid,info_out.dCTNormChamber,'double');
fwrite(fid,info_out.dGatingTimeTag,'double');
fwrite(fid,info_out.dGating4DInfoX,'double');
fwrite(fid,info_out.dGating4DInfoY,'double');
fwrite(fid,info_out.dGating4DInfoZ,'double');
fwrite(fid,info_out.dGating4DInfoTime,'double');
fwrite(fid,info_out.dOffsetX,'double');
fwrite(fid,info_out.dOffsetY,'double');
fwrite(fid,info_out.dUnusedField,'double');

%% Write the image body
switch pixeltype
    case 'single'
        fwrite(fid,M(:),'single');
end

fclose(fid);

return
