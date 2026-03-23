function ConvertDCMVolumesToMHA(dicomDir, outputDir, ctList)
%% ConvertDCMVolumesToMHA(dicomDir, outputDir, ctList)
% Fallback when phase-based method in DicomToMha_VALKIM fails.
% Groups DICOM files by series and converts each series to CT_01.mha, CT_02.mha, ...
% using DICOM2IEC. Writes SeriesInfo.txt for compatibility with DicomToMha_VALKIM.
%
% INPUT
%   dicomDir:   Directory containing DICOM files (unused if ctList is full paths)
%   outputDir:  Output directory for .mha files and SeriesInfo.txt
%   ctList:     Cell array of full paths to CT DICOM files

if nargin < 3 || isempty(ctList)
    error('ConvertDCMVolumesToMHA:NeedFileList', 'ctList (cell of DICOM paths) is required.');
end

waterAtt = 0.013;

if ~exist(outputDir, 'dir')
    mkdir(outputDir);
end

%% Group files by SeriesInstanceUID
seriesUIDs = cell(size(ctList));
seriesDesc = cell(size(ctList));
for k = 1:length(ctList)
    try
        info = dicominfo(ctList{k});
        seriesUIDs{k} = info.SeriesInstanceUID;
        if isfield(info, 'SeriesDescription')
            seriesDesc{k} = info.SeriesDescription;
        elseif isfield(info, 'SeriesNumber')
            seriesDesc{k} = num2str(info.SeriesNumber);
        else
            seriesDesc{k} = info.SeriesInstanceUID;
        end
    catch
        seriesUIDs{k} = '';
        seriesDesc{k} = 'Unknown';
    end
end

[uids, ~, groupIdx] = unique(seriesUIDs, 'stable');
numSeries = length(uids);

%% Convert each series to one MHA file and build SeriesInfo lines
fid = fopen(fullfile(outputDir, 'SeriesInfo.txt'), 'w+');
for g = 1:numSeries
    idx = find(groupIdx == g);
    listK = ctList(idx);
    outFile = fullfile(outputDir, sprintf('CT_%02d.mha', g));
    DICOM2IEC(listK, outFile, waterAtt);
    desc = seriesDesc{idx(1)};
    if ischar(desc)
        fprintf(fid, 'CT_%02d: %s\r\n', g, desc);
    else
        fprintf(fid, 'CT_%02d: %s\r\n', g, num2str(desc));
    end
end
fclose(fid);

end
