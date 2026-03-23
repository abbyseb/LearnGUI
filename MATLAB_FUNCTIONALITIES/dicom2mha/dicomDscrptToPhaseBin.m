function phaseBin = dicomDscrptToPhaseBin(descript)
% Extracted from DicomToMha_MAGIK.m for VALKIM dicom2mha standalone use.
% Maps DICOM series description (e.g. "Gated 50%") to phase bin number.
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
