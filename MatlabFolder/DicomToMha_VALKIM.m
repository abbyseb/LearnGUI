function DicomToMha_VALKIM
%% Modified version of DicomToMha_MAGIK by Andy Shieh

%% Initialise variables
dicomDir = pwd;
outputDir = pwd;
waterAtt = 0.013;

%% List of files
ctList = dir(fullfile(dicomDir,'CT*.dcm'));
ctList([ctList.isdir]) = [];

if isempty(ctList)
    ctList = dir(dicomDir);
    ctList([ctList.isdir]) = [];
end
ctList = fullfile({ctList.folder},{ctList.name});

if ~isempty(ctList)
    try
        %% Read CT DICOM info
        dcmHeader{length(ctList)} = [];
        seriesDetail{length(ctList)} = [];
        seriesUID{length(ctList)} = [];
        remove_file = [];
        for k = 1:length(ctList)
            try
                dcmHeader{k} = dicominfo(ctList{k});
                if ~strcmpi(dcmHeader{k}.Modality,'CT')
                    remove_file(end+1) = k;
                else
                    if isfield(dcmHeader{k}, 'SeriesDescription')
                        seriesDetail{k} = dcmHeader{k}.SeriesDescription;
                    elseif ~isempty(dcmHeader{k}.SeriesNumber)
                        seriesDetail{k} = dcmHeader{k}.SeriesNumber;
                    else
                        seriesDetail{k} = dcmHeader{k}.SeriesInstanceUID;
                    end
                    seriesUID{k} = dcmHeader{k}.SeriesInstanceUID;
                end
            catch
                remove_file(end+1) = k;
            end
        end
        dcmHeader(remove_file) = [];
        seriesUID(remove_file) = [];
        seriesDetail(remove_file) = [];
        ctList(remove_file) = [];
        
        %% Identify CT phases
        phaseBins = [];
        phaseOfEachFile = zeros(length(seriesDetail),1);
        for k = 1:length(seriesDetail)
            if ischar(seriesDetail{k}) || isstring (seriesDetail{k})
                phaseBin = dicomDscrptToPhaseBin(seriesDetail{k});
                if ~isnan(phaseBin)
                    phaseBins = [phaseBins,phaseBin];
                    phaseOfEachFile(k) = phaseBin;
                end
            end
        end
        phaseBins = sort(unique(phaseBins));
        NBin = max(phaseBins);
        
        %% Sort out CT z locations
        if ~NBin==0
            zLocs{NBin} = [];
            for k = 1:length(phaseOfEachFile)
                if phaseOfEachFile(k) == 0
                    continue;
                end
                zLocs{phaseOfEachFile(k)} = [zLocs{phaseOfEachFile(k)},dcmHeader{k}.ImagePositionPatient(3)];
            end
        
            zLocs_sorted{NBin} = [];
            zSortIdx{NBin} = [];
            zIdx{NBin} = [];
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
            zIdxForEachFile = nan(length(phaseOfEachFile));
            for k = 1:length(phaseOfEachFile)
                if phaseOfEachFile(k) > 0
                    binCount(phaseOfEachFile(k)) = binCount(phaseOfEachFile(k)) + 1;
                    zIdxForEachFile(k) = zIdx{phaseOfEachFile(k)}(binCount(phaseOfEachFile(k)));
                end
            end
        
            %% Read CT volumes and stack them up according to z location
            imgStacks{NBin} = [];
            for k = 1:length(ctList)
                if phaseOfEachFile(k) == 0
                    continue;
                end
                imgStacks{phaseOfEachFile(k)}(:,:,zIdxForEachFile(k)) = single(dicomread(ctList{k}));
            end
        
            %% Convert to IEC geometry and scale intensity value
            f = fopen(fullfile(outputDir,'SeriesInfo.txt'),'w+');
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
                
                if isfield(dcmHeader{firstRelevantFileIdx},'StudyDescription') && isfield(dcmHeader{firstRelevantFileIdx},'SeriesDescription')
                    fprintf(f,'CT_%02d: %s_%s\r\n', nbin, dcmHeader{firstRelevantFileIdx}.StudyDescription, dcmHeader{firstRelevantFileIdx}.SeriesDescription);
                elseif ~isfield(dcmHeader{firstRelevantFileIdx},'StudyDescription') && isfield(dcmHeader{firstRelevantFileIdx},'SeriesDescription')
                    fprintf(f,'CT_%02d: %s\r\n', nbin, dcmHeader{firstRelevantFileIdx}.SeriesDescription);
                elseif isfield(dcmHeader{firstRelevantFileIdx},'StudyDescription') && ~isfield(dcmHeader{firstRelevantFileIdx},'SeriesDescription')
                    fprintf(f,'CT_%02d: %s\r\n', nbin, dcmHeader{firstRelevantFileIdx}.StudyDescription);
                else
                    fprintf(f,'CT_%02d: No description fields in DICOM file\r\n', nbin);
                end
            end
            fclose(f);
        
            ChkSize = cellfun(@(s) size(s,2), imgStacks);
            if ~all(ChkSize == ChkSize(1))
                clear imgStacks
                % If the number of slices in the CTs is different then something has
                %   gone wrong and we need to try a different method
            end
        end
    catch
        warning('Issue encountered converting images to MHA, trying a different method')
    end
end

%% Write to file
if ~exist('imgStacks','var') || isempty(imgStacks)
    ConvertDCMVolumesToMHA(dicomDir, outputDir, ctList);
    
    opts = delimitedTextImportOptions('Delimiter','\n');
    SeriesOptions = readcell(fullfile(outputDir, 'SeriesInfo.txt'),opts);
    SeriesOptions = sortrows(SeriesOptions);
else
    if ~exist(outputDir,'dir')
        mkdir(outputDir)
    end
    load(which('mhaHeaderTemplate.mat'),'mhaHeaderTemplate');
    mhaHeader = mhaHeaderTemplate;
    mhaHeader.BitDepth = 32;
    mhaHeader.DataType = 'float';
    mhaHeaders{NBin} = [];
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
end