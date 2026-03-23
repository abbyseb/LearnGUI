function DicomToMha_MAGIK(dicomDir,outputDir)
%% DicomToMha_MAGIK(dicomDir,outputDir)
% ------------------------------------------
% FILE   : DicomToMha_MAGIK.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-11-22  Created.
% ------------------------------------------
% PURPOSE
%   Designed for the MAGIK trial. Convert 4D-CT and RT structure DICOM
%   files into MHA format.
% ------------------------------------------
% INPUT
%   dicomDir:       The full path to the directory containing all the 4D-CT
%                   and DICOM structure files. All the DICOM files should
%                   be directly under this directory instead of in
%                   sub-folders.
%                   If left empty, the program will prompt the user to
%                   select the directory.
%   outputDir:      The output folder (will be created). The 4D-CT, CT
%                   average, and GTV00, GTV10, ..., GTV90 will be saved.
%                   Additional structures such as lungs, body, ITV, PTV,
%                   etc will also be output if found.
%                   Default: "CT" under the current working directory.

%% Input check
if nargin < 1
    disp('Please select the folder containing all the 4D-CT and RT structure DICOM files.');
    dicomDir = uigetdir;
end

if nargin < 2
    outputDir = fullfile(pwd,'CT');
end

waterAtt = 0.013;

%% List of files
ctList = lscell(fullfile(dicomDir,'CT*.dcm'));

%% Read CT DICOM info
for k = 1:length(ctList)
    dcmHeader{k} = dicominfo(ctList{k});
    seriesDscripts{k} = dcmHeader{k}.SeriesDescription;
    seriesUID{k} = dcmHeader{k}.SeriesInstanceUID;
end

%% Identify CT phases
phaseBins = [];
phaseOfEachFile = zeros(length(seriesDscripts),1);
for k = 1:length(seriesDscripts)
    if isfield(dcmHeader{k},'Private_01f1_1041')
        tmpStr = dcmHeader{k}.Private_01f1_1041;
        tmpStr(tmpStr == '%') = '';
        phaseBin = round(str2double(strtrim(tmpStr)) / 10) + 1;
    else
        phaseBin = dicomDscrptToPhaseBin(seriesDscripts{k});
    end
    if ~isnan(phaseBin)
        phaseBins = [phaseBins,phaseBin];
        phaseOfEachFile(k) = phaseBin;
    end
end
phaseBins = sort(unique(phaseBins));
NBin = max(phaseBins);

%% Sort out CT z locations
for nbin = 1:NBin
    zLocs{nbin} = [];
end
for k = 1:length(phaseOfEachFile)
    if phaseOfEachFile(k) == 0
        continue;
    end
    zLocs{phaseOfEachFile(k)} = [zLocs{phaseOfEachFile(k)},dcmHeader{k}.ImagePositionPatient(3)];
end

for nbin = 1:NBin
    firstRelevantFileIdx = find(phaseOfEachFile == nbin);
    firstRelevantFileIdx = firstRelevantFileIdx(1);
    zOrientation = dcmHeader{firstRelevantFileIdx}.ImageOrientationPatient(5);
    % Positive z is towards the head if image orientation(5) is positive
    if zOrientation > 0
        [zLocs_sorted{nbin},zSortIdx{nbin}] = sort(zLocs{nbin},'descend');
    else
        [zLocs_sorted{nbin},zSortIdx{nbin}] = sort(zLocs{nbin},'ascend');
    end
    zIdx{nbin} = 1:length(zSortIdx{nbin});
    zIdx{nbin}(zSortIdx{nbin}) = zIdx{nbin};
end

binCount = zeros(NBin,1);
for k = 1:length(phaseOfEachFile)
    if phaseOfEachFile(k) > 0
        binCount(phaseOfEachFile(k)) = binCount(phaseOfEachFile(k)) + 1;
        zIdxForEachFile(k) = zIdx{phaseOfEachFile(k)}(binCount(phaseOfEachFile(k)));
    end
end

%% Read CT volumes and stack them up according to z location
for k = 1:length(ctList)
    if phaseOfEachFile(k) == 0
        continue;
    end
    imgStacks{phaseOfEachFile(k)}(:,:,zIdxForEachFile(k)) = single(dicomread(ctList{k}));
end

%% Convert to IEC geometry and scale intensity value
for nbin = 1:NBin
    firstRelevantFileIdx = find(phaseOfEachFile == nbin);
    firstRelevantFileIdx = firstRelevantFileIdx(1);
    
    imgStacks{nbin} = permute(imgStacks{nbin},[2 3 1]);
    if dcmHeader{firstRelevantFileIdx}.ImageOrientationPatient(1) < 0
        imgStacks{n} = imgStacks{n}(end:-1:1,:,:);
    end
    imgStacks{nbin} = imgStacks{nbin}(:,:,end:-1:1);
    
    if ~isnan(waterAtt)
        if min(imgStacks{nbin}(:)) >= 0
            imgStacks{nbin} = imgStacks{nbin} * waterAtt / 1000;
        else
            imgStacks{nbin} = (imgStacks{nbin} + 1000) * waterAtt / 1000;
        end
    end
end

%% Write to file
if ~exist(outputDir,'dir')
    mkdir(outputDir)
end
load(which('mhaHeaderTemplate.mat'));
mhaHeader = mhaHeaderTemplate;
mhaHeader.BitDepth = 32;
mhaHeader.DataType = 'float';
for nbin = 1:NBin
    mhaHeaders{nbin} = mhaHeader;
    firstRelevantFileIdx = find(phaseOfEachFile == nbin);
    firstRelevantFileIdx = firstRelevantFileIdx(1);
    mhaHeaders{nbin}.Dimensions = size(imgStacks{nbin});
    mhaHeaders{nbin}.PixelDimensions(2) = dcmHeader{firstRelevantFileIdx}.SliceThickness;
    mhaHeaders{nbin}.PixelDimensions(1) = dcmHeader{firstRelevantFileIdx}.PixelSpacing(1);
    mhaHeaders{nbin}.PixelDimensions(3) = dcmHeader{firstRelevantFileIdx}.PixelSpacing(2);
    mhaHeaders{nbin}.Offset = -0.5 * (mhaHeaders{nbin}.Dimensions - 1) .* mhaHeaders{nbin}.PixelDimensions;
    MhaWrite(mhaHeaders{nbin},imgStacks{nbin},fullfile(outputDir,num2str(nbin,'CT_%02d.mha')));
end

%% Read RT structures
rtAll = ReadRTStruct(dicomDir);

% Look for GTVs
itv_derived = false(size(imgStacks{nbin}));
for n = 1:length(rtAll.ROIs)
    if isempty(strfind(rtAll.ROIs(n).ROIName,'GTV'))
       continue;
    end
    
    gtvNumStr = regexp(rtAll.ROIs(n).ROIName,'GTV','split');
    gtvNum = str2double(strtrim(gtvNumStr{2}));
    if isnan(gtvNum) || gtvNum < 0 || gtvNum > 90 || mod(gtvNum,10) > 0
        continue;
    end
    
    nbin = gtvNum / 10 + 1;
    
    firstRelevantFileIdx = find(phaseOfEachFile == nbin);
    firstRelevantFileIdx = firstRelevantFileIdx(1);

    [gtvs{nbin},gtvHeaders{nbin}] = dcmContourToMhaMask(...
        rtAll.ROIs(n).Contours,mhaHeaders{nbin},dcmHeader{firstRelevantFileIdx},zLocs_sorted{nbin});
    MhaWrite(gtvHeaders{nbin},gtvs{nbin},fullfile(outputDir,num2str(nbin,'GTV_%02d.mha')));
    
    itv_derived = itv_derived | gtvs{nbin};
end
itvCenter = GetMHAImageCentroid(gtvHeaders{nbin},itv_derived);

% Look for PTV, Body, Lungs, Spine
exhaleBin = floor(NBin / 2) + 1;
firstRelevantFileIdx = find(phaseOfEachFile == exhaleBin);
firstRelevantFileIdx = firstRelevantFileIdx(1);
for n = 1:length(rtAll.ROIs)
    if ~isempty(strfind(rtAll.ROIs(n).ROIName,'Lung R'))
        outputMaskName = 'LungR';
    elseif ~isempty(strfind(rtAll.ROIs(n).ROIName,'Lung L'))
        outputMaskName = 'LungL';
    elseif strcmpi(rtAll.ROIs(n).ROIName,'PTV')
        outputMaskName = 'PTV';
    elseif strcmpi(rtAll.ROIs(n).ROIName,'ITV')
        outputMaskName = 'ITV';
    elseif strcmpi(rtAll.ROIs(n).ROIName,'Body')
        outputMaskName = 'Body';
    elseif strcmpi(rtAll.ROIs(n).ROIName,'Heart')
        outputMaskName = 'Heart';
    elseif strcmpi(rtAll.ROIs(n).ROIName,'LungExh')
        outputMaskName = 'LungExh';
    elseif strcmpi(rtAll.ROIs(n).ROIName,'LungInh')
        outputMaskName = 'LungInh';
    elseif strcmpi(rtAll.ROIs(n).ROIName,'Body')
        outputMaskName = 'Body';
    else
        continue;
    end

    [mask,maskHeader] = dcmContourToMhaMask_MAGIK(...
        rtAll.ROIs(n).Contours,mhaHeaders{exhaleBin},dcmHeader{firstRelevantFileIdx},zLocs_sorted{exhaleBin});
    if strcmpi(rtAll.ROIs(n).ROIName,'ITV')
        itvCenter = GetMHAImageCentroid(maskHeader,mask);
    end
    MhaWrite(maskHeader,mask,fullfile(outputDir,[outputMaskName,'.mha']));
end

%% Shift the mid-point of ITV to isocenter
mhaList = lscell(fullfile(outputDir,'*.mha'));
for k = 1:length(mhaList)
    [mhaHeader,mhaBody] = MhaRead(mhaList{k});
    mhaHeader.Offset = mhaHeader.Offset - itvCenter;
    MhaWrite(mhaHeader,mhaBody,mhaList{k});
end

end

function phaseBin = dicomDscrptToPhaseBin(descript)
if isempty(strfind(descript,'Gated'))
    phaseBin = NaN;
else
    parts = regexp(descript,'Gated','split');
    parts = regexp(parts{2},'%','split');
    numStr = strtrim(parts{1});
    if ~isempty(strfind(numStr,','))
        numStr = regexp(numStr,',','split');
        numStr = strtrim(numStr{2});
    end
    phaseBin = round(str2double(numStr)/10) + 1;
end
end

function [mask,maskHeader] = dcmContourToMhaMask_MAGIK(rtContours,mhaHeader,dcmHeader,zLoc)
maskHeader = mhaHeader;
maskHeader.BitDepth = 8;
maskHeader.DataType = 'uchar';
mask = false(mhaHeader.Dimensions);

for nc = 1:length(rtContours)
    pts = rtContours(nc).Points;
    polyXi = [];
    polyYi = [];
    polyZi = [];
    for np = 1:size(pts,1)
        idxSI = find(abs(zLoc - pts(np,3)) == min(abs(zLoc - pts(np,3))));
        % Might have to double check if the following is correct when
        % ImageOrientationPatient(1) is negative
        idxLR = mhaHeader.Dimensions(1) - round(...
            (pts(np,2) - dcmHeader.ImagePositionPatient(2)) / mhaHeader.PixelDimensions(1));
        idxAP = round(...
            (pts(np,1) - dcmHeader.ImagePositionPatient(1)) / mhaHeader.PixelDimensions(3)) + 1;
        
        polyXi = [polyXi;idxLR];
        polyYi = [polyYi;idxSI];
        polyZi = [polyZi;idxAP];
    end
    
    % Check to see on which plane the contour was constructed
    if length(unique(polyYi)) == 1
        slice = poly2mask(polyXi,polyZi,size(mask,1),size(mask,3));
        mask(:,unique(polyYi),:) = permute(slice,[1,3,2]);
    elseif length(unique(polyZi)) == 1
        slice = poly2mask(polyXi,polyYi,size(mask,1),size(mask,2));
        mask(:,:,unique(polyZi)) = slice;
    elseif length(unique(polyXi)) == 1
        slice = poly2mask(polyYi,polyZi,size(mask,2),size(mask,3));
        mask(unique(polyXi),:,:) = permute(slice,[3,1,2]);
    end
    
end
end