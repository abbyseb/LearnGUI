function HndToTiff(hndList,outDir,invert,flipX,flipY)
%% HndToTiff(hndList,outDir,invert,flipX,flipY)
% ------------------------------------------
% FILE   : HndToTiff.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-06-15 Created.
% ------------------------------------------
% PURPOSE
%   Convert HND files to TIFF files.
%
% ------------------------------------------
% INPUT
%   hndList:        List of HND projection files in cell format. Can be
%                   obtained using the "lscell.m" function.
%                   This function assumes the hnd filenames are in the
%                   format of "FrameXXXXX.hnd where XXXXX are frame
%                   numbers.
%   outDir:         The directory where the TIFF images are saved.
%                   The format of the filenames will be made to match what 
%                   KIM would expect (CDog output format).
%                   "Ch1_1_[FrameNo]_[Angles].tiff"
%   invert:         Whether or not to invert the intensity.
%   flipX:          Whether or not to flip the projection in the x
%                   direction.
%   flipY:          Whether or not to flip the projection in the y
%                   direction.

%% 
MAXVAL = 2^16 - 1;
heightHeader = 6;

if ~exist(outDir,'dir')
    mkdir(outDir);
end

for k = 1:length(hndList)
    [info,M] = HndRead(hndList{k});
    % The angle written in TIFF filename is expected to be the actual kV angle + 90
    ang = mod(info.dCTProjectionAngle + 90,360);
    [~,filename] = fileparts(hndList{k});
    frameno = str2double(filename(6:end));
    M(M>MAXVAL) = MAXVAL;
    M = uint32(M);
    if invert
        M = MAXVAL - M;
    end
    if flipX
        M = M(end:-1:1,:);
    end
    if flipY
        M = M(:,end:-1:1);
    end
    M(:,end+1:end+heightHeader) = 0;
    
    outname = fullfile(outDir,num2str([frameno,ang],'Ch1_1_%05d_%06.2f.tiff'));
    
    writer = Tiff(outname,'w');
    writer.setTag('ImageLength',size(M,2));
    writer.setTag('ImageWidth',size(M,1));
    writer.setTag('Photometric',Tiff.Photometric.MinIsBlack);
    writer.setTag('BitsPerSample',32);
    writer.setTag('SamplesPerPixel',1);
    writer.setTag('PlanarConfiguration',Tiff.PlanarConfiguration.Chunky);
    writer.setTag('SubFileType',Tiff.SubFileType.Default);
    writer.setTag('RowsPerStrip',1);
    writer.setTag('SubFileType',Tiff.SubFileType.Default);
    writer.setTag('Compression',Tiff.Compression.None);
    writer.setTag('SampleFormat',Tiff.SampleFormat.UInt);
    writer.setTag('Orientation',Tiff.Orientation.TopLeft);
    writer.write(M');
    close(writer);
end
