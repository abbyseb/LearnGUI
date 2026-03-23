function AddRespSigToTiff(tiffList,respSig,respPhase,outDir)
%% AddRespSigToTiff(tiffList,respSig,respPhase,outDir)
% ------------------------------------------
% FILE   : AddRespSigToTiff.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-06-18 Created.
% ------------------------------------------
% PURPOSE
%   Write respiratory signal and phase to tiff files.
%
% ------------------------------------------
% INPUT
%   tiffList:       List of TIFF files obtained by the "lscell.m" function.
%   respSig:        The vector of respiratory signal to be added.
%   respPhase:      The vector of respiratory phase to be added (0-360).
%   outDir:         The directory where the new TIFF images are saved.

%% 
if ~exist(outDir,'dir')
    mkdir(outDir);
end

for k = 1:length(tiffList)
    [~,name,ext] = fileparts(tiffList{k});
    outname = fullfile(outDir,[name,ext]);
    
    M = imread(tiffList{k});
    
    % The bytes that record the signal
    M(770,781:784) = typecast(double(respSig(k)),'uint16');
    
    % The bytes that record the phase
    M(771,27:30) = typecast(double(respPhase(k)),'uint16');
    
    writer = Tiff(outname,'w');
    writer.setTag('ImageLength',size(M,1));
    writer.setTag('ImageWidth',size(M,2));
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
    writer.write(M);
    close(writer);
end
