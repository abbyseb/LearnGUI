function SaveXCATBinAsMha(binFile,dim,spacing,output)
%% SaveXCATBinAsMha(binFile,dim,spacing,output)
% ------------------------------------------
% FILE   : SaveXCATBinAsMha.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2017-12-11  Created.
% ------------------------------------------
% PURPOSE
%   Convert XCAT bin volume to a mha file.
% ------------------------------------------
% INPUT
%   binFile:        Path to the bin file to be converted.
%   dim:            The dimension of the images in [coronal,axial,sagittal]
%   spacing:        The pixel spacing in [coronal,axial,sagittal] in mm
%   output:         Output file path.
% ------------------------------------------

%%
% Header template
info = struct('Filename',output,'Format','MHA','CompressedData','false',...
    'ObjectType','image','NumberOfDimensions',3,'BinaryData','true',...
    'ByteOrder','false','TransformMatrix',[1 0 0 0 1 0 0 0 1],...
    'Offset',-(dim/2-0.5).*spacing,...
    'CenterOfRotation',[0 0 0],'AnatomicalOrientation','RAI',...
    'PixelDimensions',spacing,...
    'Dimensions',dim,'DataType','float',...
    'DataFile','LOCAL','BitDepth',32,'HeaderSize',318);

%% Convert
failTag = true;
while failTag
    failTag = false;
    try
        fid = fopen(binFile,'r');
        P = fread(fid,'float32');
        fclose(fid);
        P = single(P);
        P = reshape(P,[dim(1),dim(3),dim(2)]);
    catch e
        failTag = true;
        fclose('all');
        disp(['Failed to read ',binFile,' with the following error message:']);
        disp(e.message);
        disp('Trying again');
    end
end
    
% Convert to RTK geometry format
P = permute(P,[1 3 2]); P = P(:,end:-1:1,end:-1:1);

info.Filename = output;
failTag = true;
while failTag
    failTag = false;
    try
        MhaWrite(info,P,output);
    catch e
        failTag = true;
        fclose('all');
        disp(['Failed to write ',output,' with the following error message:']);
        disp(e.message);
        disp('Trying again');
    end
end