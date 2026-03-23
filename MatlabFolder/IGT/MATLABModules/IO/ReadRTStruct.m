function rts = ReadRTStruct(dicomDir)
%% rts = ReadRTStruct(dicomDir)
% ------------------------------------------
% FILE   : ReadRTStruct.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-09-09  Created.
% ------------------------------------------
% PURPOSE
%   Read RTK struct from DICOM RT struct file.
% ------------------------------------------
% INPUT
%   dicomDir : The directory containing the CT and the RT struct files.
% ------------------------------------------
% OUTPUT
%   rts:     : The output RT struct collection.
% ------------------------------------------

%% Checking input arguments & Opening the file

rts = struct;

% If no input filename => Open file-open-dialog
if nargin < 1
    dicomDir = uigetdir(pwd,'Select the DICOM directory');
    
    % catch error if no file selected
    if isnumeric(dicomDir)
        error('ERROR: No directory selected. \n');
    end
end

%% Read RT struct, which is stored in header
rtsFile = dir(fullfile(dicomDir,'RS*.dcm'));
rtsFile = fullfile(dicomDir,rtsFile.name);

rtHeader = dicominfo(rtsFile);
rts.StudyInstanceUID = rtHeader.StudyInstanceUID;
rts.ROIs = struct('ROIName','','Contours',[]);

ROIContourSequence = fieldnames(rtHeader.ROIContourSequence);
refSOPInst_all = {};
for k = 1:length(ROIContourSequence)
    rts.ROIs(k).ROIName = rtHeader.StructureSetROISequence.(ROIContourSequence{k}).ROIName;
    
    if ~isfield(rtHeader.ROIContourSequence.(ROIContourSequence{k}),'ContourSequence');
        continue;
    end
    
    ContourSequence = fieldnames(rtHeader.ROIContourSequence.(ROIContourSequence{k}).ContourSequence);
    rts.ROIs(k).Contours = struct(...
        'Points',[],...
        'ReferencedSOPClassUID','',...
        'ReferencedSOPInstanceUID','',...
        'ContourGeometricType','');
    for kc = 1:length(ContourSequence)
        rts.ROIs(k).Contours(kc).Points = reshape(...
            rtHeader.ROIContourSequence.(ROIContourSequence{k}).ContourSequence.(ContourSequence{kc}).ContourData,...
            length(rtHeader.ROIContourSequence.(ROIContourSequence{k}).ContourSequence.(ContourSequence{kc}).ContourData) / ...
            rtHeader.ROIContourSequence.(ROIContourSequence{k}).ContourSequence.(ContourSequence{kc}).NumberOfContourPoints,...
            rtHeader.ROIContourSequence.(ROIContourSequence{k}).ContourSequence.(ContourSequence{kc}).NumberOfContourPoints)';
        rts.ROIs(k).Contours(kc).ContourGeometricType = ...
            rtHeader.ROIContourSequence.(ROIContourSequence{k}).ContourSequence.(ContourSequence{kc}).ContourGeometricType;
        rts.ROIs(k).Contours(kc).ReferencedSOPClassUID = ...
            rtHeader.ROIContourSequence.(ROIContourSequence{k}).ContourSequence.(ContourSequence{kc}).ContourImageSequence.Item_1.ReferencedSOPClassUID;
        rts.ROIs(k).Contours(kc).ReferencedSOPInstanceUID = ...
            rtHeader.ROIContourSequence.(ROIContourSequence{k}).ContourSequence.(ContourSequence{kc}).ContourImageSequence.Item_1.ReferencedSOPInstanceUID;
        refSOPInst_all{end+1} = rts.ROIs(k).Contours(kc).ReferencedSOPInstanceUID;
    end
end
refSOPInst_all = sort(unique(refSOPInst_all));

%% Put image information in
% Read image headers
rts.ImageHeaders = cell(length(refSOPInst_all),1);
for k = 1:length(refSOPInst_all)
    imgFile = dir(fullfile(dicomDir,['*',refSOPInst_all{k},'*.dcm']));
    imgFile = fullfile(dicomDir,imgFile.name);
    rts.ImageHeaders{k} = dicominfo(imgFile);
end

% Get affine transformation
dr = rts.ImageHeaders{1}.PixelSpacing(1);
dc = rts.ImageHeaders{1}.PixelSpacing(2);
F(:,1) = rts.ImageHeaders{1}.ImageOrientationPatient(1:3);
F(:,2) = rts.ImageHeaders{1}.ImageOrientationPatient(4:6);
T1 = rts.ImageHeaders{1}.ImagePositionPatient;
TN = rts.ImageHeaders{end}.ImagePositionPatient;
offset = (T1 - TN) ./ (1 - length(rts.ImageHeaders));
rts.AffineTransform = [[F(1,1)*dr F(1,2)*dc offset(1) T1(1)]; ...
                      [F(2,1)*dr F(2,2)*dc offset(2) T1(2)]; ...
                      [F(3,1)*dr F(3,2)*dc offset(3) T1(3)]; ...
                      [0         0         0    1    ]];

end