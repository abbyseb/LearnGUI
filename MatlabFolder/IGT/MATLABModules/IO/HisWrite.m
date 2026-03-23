function HisWrite(info,M,outputfp)
%% HisWrite(info,M,outputfp)
% ------------------------------------------
% FILE   : HisWrite.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-09-18  Created.
% ------------------------------------------
% PURPOSE
%   Write the input header and image to an Elekta HIS file.
% ------------------------------------------
% INPUT
%   info:       The header MATLAB struct.
%   M:          The image body, 2D (uint16).
%   outputfp:   The path of the output file.
% ------------------------------------------

%% Writing header

HISHeadLength = 68;

fid = fopen(strtrim(outputfp),'w');
if fid == -1
    error('Error: Invalid file path.');
    return
end

% Writing header file
header = char(zeros(1,info.HeaderSize));
header(1) = 0;
header(2) = 112;
header(3) = 68;
header(4) = 0;
header(11) = mod(info.HeaderSize - HISHeadLength, 2^8);
header(12) = floor((info.HeaderSize - HISHeadLength) / 2^8);
header(13) = 1;
header(15) = 1;
header(17) = mod(info.SizeY, 2^8);
header(18) = floor(info.SizeY / 2^8);
header(19) = mod(info.SizeX, 2^8);
header(20) = floor(info.SizeX / 2^8);
header(21) = mod(info.NFrames, 2^8);
header(22) = floor(info.NFrames / 2^8);
header(33) = mod(info.Type, 2^8);
header(35) = floor(info.Type / 2^8);

fwrite(fid,header,'int8');

%% Write the image body
% Truncate to prevent overflow
switch info.Type
    case 4
        M(M>2^16-1) = 2^16 -1;
        fwrite(fid,M(:),'uint16');
    otherwise
        M(M>2^16-1) = 2^16 -1;
        fwrite(fid,M(:),'uint16');
end

fclose(fid);

return
