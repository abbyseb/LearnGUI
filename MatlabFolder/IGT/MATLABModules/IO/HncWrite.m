function HncWrite(info,M,outputfp)
%% HncWrite(info,M,outputfp)
% ------------------------------------------
% FILE   : HncWrite.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2012-11-26  Created.
% ------------------------------------------
% PURPOSE
%   Write the input header and image to a Varian HNC file.
% ------------------------------------------
% INPUT
%   info:       The header MATLAB struct.
%   M:          The image body, 2D (uint16).
%   outputfp:   The path of the output file.
% ------------------------------------------

%% Writing header

HNCHEADLENGTH = 512;

pixeltype = class(M);

switch pixeltype
    case 'uint8'
        info.('uiFileLength') = HNCHEADLENGTH + length(M(:));
        info.('uiActualFileLength') = HNCHEADLENGTH + length(M(:));
    case 'uint16'
        info.('uiFileLength') = HNCHEADLENGTH + 2*length(M(:));
        info.('uiActualFileLength') = HNCHEADLENGTH + 2*length(M(:));
    case 'uint32'
        info.('uiFileLength') = HNCHEADLENGTH + 4*length(M(:));
        info.('uiActualFileLength') = HNCHEADLENGTH + 2*length(M(:));
end

fid = fopen(strtrim(outputfp),'w');
if fid == -1
    error('Error: Invalid file path.');
    return
end

% Writing header file
fwrite(fid,info.bFileType,'uint8');
fwrite(fid,info.uiFileLength,'uint32');
fwrite(fid,info.bChecksumSpec,'uint8');
fwrite(fid,info.uiCheckSum,'uint32');
fwrite(fid,info.bCreationDate,'uint8');
fwrite(fid,info.bCreationTime,'uint8');
fwrite(fid,info.bPatientID,'uint8');
fwrite(fid,info.uiPatientSer,'uint32');
fwrite(fid,info.bSeriesID,'uint8');
fwrite(fid,info.uiSeriesSer,'uint32');
fwrite(fid,info.bSliceID,'uint8');
fwrite(fid,info.uiSliceSer,'uint32');
fwrite(fid,info.uiSizeX,'uint32');
fwrite(fid,info.uiSizeY,'uint32');
fwrite(fid,info.dSliceZPos,'double');
fwrite(fid,info.bModality,'uint8');
fwrite(fid,info.uiWindow,'uint32');
fwrite(fid,info.uiLevel,'uint32');
fwrite(fid,info.uiPixelOffset,'uint32');
fwrite(fid,info.bImageType,'uint8');
fwrite(fid,info.dGantryRtn,'double');
fwrite(fid,info.dSAD,'double');
fwrite(fid,info.dSFD,'double');
fwrite(fid,info.dCollX1,'double');
fwrite(fid,info.dCollX2,'double');
fwrite(fid,info.dCollY1,'double');
fwrite(fid,info.dCollY2,'double');
fwrite(fid,info.dCollRtn,'double');
fwrite(fid,info.dFieldX,'double');
fwrite(fid,info.dFieldY,'double');
fwrite(fid,info.dBladeX1,'double');
fwrite(fid,info.dBladeX2,'double');
fwrite(fid,info.dBladeY1,'double');
fwrite(fid,info.dBladeY2,'double');
fwrite(fid,info.dIDUPosLng,'double');
fwrite(fid,info.dIDUPosLat,'double');
fwrite(fid,info.dIDUPosVrt,'double');
fwrite(fid,info.dIDUPosRtn,'double');
fwrite(fid,info.dPatientSupportAngle,'double');
fwrite(fid,info.dTableTopEccentricAngle,'double');
fwrite(fid,info.dCouchVrt,'double');
fwrite(fid,info.dCouchLng,'double');
fwrite(fid,info.dCouchLat,'double');
fwrite(fid,info.dIDUResolutionX,'double');
fwrite(fid,info.dIDUResolutionY,'double');
fwrite(fid,info.dImageResolutionX,'double');
fwrite(fid,info.dImageResolutionY,'double');
fwrite(fid,info.dEnergy,'double');
fwrite(fid,info.dDoseRate,'double');
fwrite(fid,info.dXRayKV,'double');
fwrite(fid,info.dXRayMA,'double');
fwrite(fid,info.dMetersetExposure,'double');
fwrite(fid,info.dAcqAdjustment,'double');
fwrite(fid,info.dCTProjectionAngle,'double');
fwrite(fid,info.dCTNormChamber,'double');
fwrite(fid,info.dGatingTimeTag,'double');
fwrite(fid,info.dGating4DInfoX,'double');
fwrite(fid,info.dGating4DInfoY,'double');
fwrite(fid,info.dGating4DInfoZ,'double');
fwrite(fid,info.dGating4DInfoTime,'double');
fwrite(fid,info.dOffsetX,'double');
fwrite(fid,info.dOffsetY,'double');
fwrite(fid,info.dUnusedField,'double');

if strcmp(pixeltype,'uint32')
    my_nulls = uint8(zeros(512,1));
    fwrite(fid,my_nulls(:), 'uint8');
end
%% Write the image body
% Truncate to prevent overflow
switch pixeltype
    case 'uint8'
        M(M>2^8-1) = 2^8 -1;
        fwrite(fid,M(:),'uint8');
    case 'uint16'
        M(M>2^16-1) = 2^16 -1;
        fwrite(fid,M(:),'uint16');
    case 'uint32'
        M(M>2^32-1) = 2^32 -1;
        fwrite(fid,M(:),'uint32');
end

fclose(fid);

return
