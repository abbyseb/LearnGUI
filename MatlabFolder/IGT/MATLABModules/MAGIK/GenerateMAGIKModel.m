function GenerateMAGIKModel(dicomDir,outputDir,sdd,sid,volOffset_GTV,volOffset_Dph,detOffset,projSize,projSpacing,skipDCMConvt)
%% GenerateMAGIKModel(dicomDir,outputDir,sdd,sid,volOffset_GTV,volOffset_Dph,detOffset,projSize,projSpacing,skipDCMConvt)
% ------------------------------------------
% FILE   : GenerateMAGIKModel.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-03-04  Created.
% ------------------------------------------
% PURPOSE
%   Generate everything needed to perform markerless tracking in the MAGIK
%   trial.
%
%   For the MAGIK trial, only the first input, dicomDir, is required. e.g.:
%   GenerateMAGIKPatientModel(pwd);
%
%   If SDD is not 1500 mm, the second input can be used to specify the
%   correct SDD. e.g. for 1800 mm SDD:
%   GenerateMAGIKPatientModel(pwd,1800);
%
%   For research purpose, the other paramaters can be used to cope with
%   specific scenarios.
% ------------------------------------------
% INPUT
%   dicomDir:       The full path to the directory containing the 4D-CT or
%                   breath-hold exhale + inhale CT and DICOM structure 
%                   files. All the DICOM files should be directly under
%                   this directory instead of in sub-folders.
%                   If left empty, the program will prompt the user to
%                   select the directory.
%   outputDir:      (Optional) The full path to the output directory.
%                   Default: "MAGIKModel" under dicomDir.
%   sdd:            (Optional) Source-to-detector distance.
%                   Default: 1500 mm
%   sid:            (Optional) Source-to-isocenter distance.
%                   Default: 1000 mm
%                   Default: false (if left empty or ignored).
%   volOffset_GTV:  (Optional) A 1x3 vector specifying how the isocenter
%                   should be translated to match with patient GTV position 
%                   during tracking. 
%                   Default: [0,0,0] mm
%   volOffset_Dph:  (Optional) A 1x3 vector specifying how the isocenter
%                   should be translated to match with patient diaphragm 
%                   position during tracking. 
%                   Default: [0,0,0] mm
%   detOffset:      (Optional) A 1x2 vector specifying the kV detector
%                   offset.
%                   Default: [0,0] mm
%   projSize:       (Optional) A 1x2 vector specifying the projection
%                   dimension.
%                   Default: [1024,768]
%   projSpacing:    (Optional) A 1x2 vector specifying the projection pixel
%                   spacings.
%                   Default: [0.388,0.388] mm
%   skipDCMConvt:   (Optional) True if the user wants to skip DICOM->MHA
%                   conversion.
%                   Default: false

%% Input check and hard-coded parameters
% Pre-tx arc fraction (out of 360 degrees)
preTxArcFraction = 1/2;

% Default correlation scaling matrix
corrScale = [1,0.3,0.3;0.3,1,0.8;0.3,0.8,1];

if nargin < 1
    disp('Please select the folder containing the CT and RT structure DICOM files.');
    dicomDir = uigetdir;
end

if nargin < 2
    outputDir = fullfile(dicomDir,'MAGIKModel');
end

if nargin < 3
    sdd = 1500;
end

if nargin < 4
    sid = 1000;
end

if nargin < 5
    volOffset_GTV = [0,0,0];
end

if nargin < 6
    volOffset_Dph = [0,0,0];
end

if nargin < 7
    detOffset = [0,0];
end

if nargin < 8
    projSize = [1024,768];
end

if nargin < 9
    projSpacing = [0.388,0.388];
end

if nargin < 10
    skipDCMConvt = false;
end

% In case this is used as a standalone, we need to convert the input (as
% string) to our desired format
if ischar(sdd)
    sdd = str2double(sdd);
end
if ischar(sid)
    sid = str2double(sid);
end
if ischar(volOffset_GTV)
    volOffset_GTV = inputCharToArray(volOffset_GTV);
end
if ischar(volOffset_Dph)
    volOffset_Dph = inputCharToArray(volOffset_Dph);
end
if ischar(detOffset)
    detOffset = inputCharToArray(detOffset);
end
if ischar(projSize)
    projSize = inputCharToArray(projSize);
end
if ischar(projSpacing)
    projSpacing = inputCharToArray(projSpacing);
end
if ischar(skipDCMConvt)
    skipDCMConvt = strcmpi(skipDCMConvt,'true');
end

if ~exist(fullfile(outputDir,'MAGIK_ModelFiles'),'dir')
    mkdir(fullfile(outputDir,'MAGIK_ModelFiles'));
end

%% DICOM to MHA conversion
if ~skipDCMConvt
    fprintf('Converting DICOM files to MHA files ...... ');
    tic;
    DicomToMha_MAGIK(dicomDir,outputDir);
    fprintf('COMPLETED using %s \n',seconds2human(toc));
else
    fprintf('Copying existing MHA files ...... ');
    tic;
    for k = 1:10
        % 4D-CT if available
        filesToCopy{2*k-1} = fullfile(dicomDir,num2str(k,'CT_%02d.mha'));
        % 4D-GTV if available
        filesToCopy{2*k} = fullfile(dicomDir,num2str(k,'GTV_%02d.mha'));
    end
    filesToCopy{end+1} = fullfile(dicomDir,'CT_Exh.mha');
    filesToCopy{end+1} = fullfile(dicomDir,'CT_Inh.mha');
    filesToCopy{end+1} = fullfile(dicomDir,'GTV_Exh.mha');
    filesToCopy{end+1} = fullfile(dicomDir,'GTV_Inh.mha');
    filesToCopy{end+1} = fullfile(dicomDir,'Body.mha');

    % Copy whatever we have
    for f = filesToCopy
        if exist(f{1},'file')
            [~,name,ext] = fileparts(f{1});
            [~,~] = system(['copy "',f{1},'" "',fullfile(outputDir,[name,ext]),'"']);
        end
    end
    
    fprintf('COMPLETED using %s \n',seconds2human(toc));
end

%% Checking if we have the necessary CT and GTV files
fprintf('Making sure we have the necessary files ...... ');
tic;
% If CT_Exh and CT_Inh and GTV_Exh do not exist but we have 4D-CT,
% use the corresponding phases
if ~exist(fullfile(outputDir,'CT_Exh.mha'),'file')
    [~,~] = system(['copy "',fullfile(outputDir,'CT_06.mha'),'" "',fullfile(outputDir,'CT_Exh.mha'),'"']);
end
if ~exist(fullfile(outputDir,'CT_Inh.mha'),'file')
    [~,~] = system(['copy "',fullfile(outputDir,'CT_01.mha'),'" "',fullfile(outputDir,'CT_Inh.mha'),'"']);
end
if ~exist(fullfile(outputDir,'GTV_Exh.mha'),'file')
    [~,~] = system(['copy "',fullfile(outputDir,'GTV_06.mha'),'" "',fullfile(outputDir,'GTV_Exh.mha'),'"']);
end
if ~exist(fullfile(outputDir,'GTV_Inh.mha'),'file')
    [~,~] = system(['copy "',fullfile(outputDir,'GTV_01.mha'),'" "',fullfile(outputDir,'GTV_Inh.mha'),'"']);
end

% Check if we have necessary files
if ~exist(fullfile(outputDir,'CT_Exh.mha'),'file') || ...
        ~exist(fullfile(outputDir,'CT_Inh.mha'),'file') || ...
        ~exist(fullfile(outputDir,'GTV_Exh.mha'),'file') || ...
        ~exist(fullfile(outputDir,'GTV_Inh.mha'),'file') || ...
        ~exist(fullfile(outputDir,'Body.mha'),'file')
    error('ERROR: Body mask and end-inhale and end-exhale CTs/GTVs must be provided.');
end

% Generate CT_Avg from exhale and inhale
[~,ctInh] = MhaRead(fullfile(outputDir,'CT_Inh.mha'));
[headerCT,ctExh] = MhaRead(fullfile(outputDir,'CT_Exh.mha'));
ctAvg = 0.5 * single(ctInh) + 0.5 * single(ctExh);
headerCT.DataType = 'float';
headerCT.BitDepth = 32;
MhaWrite(headerCT,ctAvg,fullfile(outputDir,'CT_Avg.mha'));
clear ctInh headerCT ctExh;

fprintf('COMPLETED using %s \n',seconds2human(toc));

%% Applying volume Offset
fprintf('Applying translational offset ...... ');
tic;
fileList = lscell(fullfile(outputDir,'*.mha'));
% Might as well apply the body mask here since we are reading all images
[~,bodyMask] = MhaRead(fullfile(outputDir,'Body.mha'));
for k = 1:length(fileList)
    [header,img] = MhaRead(fileList{k});
    header.Offset = header.Offset + volOffset_GTV;
    if header.BitDepth > 8 % not mask
        % Also make sure all the CTs are in attenuation
        if min(img(:)) < -500
            img = (single(img) + 1000) * 0.013 / 1000;
        elseif max(img(:)) - min(img(:)) > 1000
            img = single(img) * 0.013 / 1000;
        end
        img(img<0) = 0;
        header.BitDepth = 32;
        header.DataType = 'float';
    end
    img(~bodyMask) = 0;
    MhaWrite(header,img,fileList{k});
end
fprintf('COMPLETED using %s \n',seconds2human(toc));

%% Generate GTV model
fprintf('Building the GTV model ...... ');
tic;
% Generate model geometry
modelGeoFile = fullfile(outputDir,'ModelGeometry.xml');
[~,~] = system(['rtksimulatedgeometry -o "',modelGeoFile,'"',...
    ' -f 0 -a 360 -n 720',...
    ' --sid ',num2str(sid,'%f'),' --sdd ',num2str(sdd,'%f'),...
    ' --proj_iso_x ',num2str(detOffset(1),'%f'),' --proj_iso_y ',num2str(detOffset(2),'%f')]);
% Get GTV position covariance either from 4D-GTV or estimate from exhale
% and inhale positions
[headerGTVExh,gtvExh] = MhaRead(fullfile(outputDir,'GTV_Exh.mha'));
[headerGTVInh,gtvInh] = MhaRead(fullfile(outputDir,'GTV_Inh.mha'));
gtvExhPos = GetMHAImageCentroid(headerGTVExh,gtvExh>0);
gtvInhPos = GetMHAImageCentroid(headerGTVInh,gtvInh>0);
gtv4DExist = true;
for k = 1:10
    gtv4DExist = gtv4DExist && exist(fullfile(outputDir,num2str(k,'GTV_%02d.mha')),'file');
end
if gtv4DExist
    gtv4DPos = [];
    for k = 1:10
        [gtvHeader,gtvVol] = MhaRead(fullfile(outputDir,num2str(k,'GTV_%02d.mha')));
        gtv4DPos = [gtv4DPos;GetMHAImageCentroid(gtvHeader,gtvVol>0)];
    end
    gtvPosCov = cov(gtv4DPos);
else
    gtvPosCov = cov([gtvExhPos;gtvInhPos]) .* corrScale;
end
% GTV model
gtvModel = gtvDRRModel(fullfile(outputDir,'GTV_Exh.mha'),...
    fullfile(outputDir,'GTV_Inh.mha'),gtvPosCov,...
    fullfile(outputDir,'CT_Exh.mha'),[0,0,0],...
    modelGeoFile,projSize,projSpacing,outputDir,false);
writeGTVDRRModel(gtvModel,fullfile(outputDir,'MAGIK_ModelFiles','GTVDRRModel.bin'));
fprintf('COMPLETED using %s \n',seconds2human(toc));

%% Build diaphragm model
fprintf('Building the diaphragm model ...... ');
tic;
% Diaphragm segmentation
[headerCT,ctExh] = MhaRead(fullfile(outputDir,'CT_Exh.mha'));
if exist(fullfile(outputDir,'Lung_Exh.mha'),'file')
    [~,lungExh] = MhaRead(fullfile(outputDir,'Lung_Exh.mha'));
    [dphR,dphL] = segmentDiaphragm(ctExh,header,fullfile(outputDir),false,lungExh);
else
    [dphR,dphL] = segmentDiaphragm(ctExh,header,fullfile(outputDir),false);
end
% Calculate motion axis
[~,ctInh] = MhaRead(fullfile(outputDir,'CT_Inh.mha'));
if exist(fullfile(outputDir,'Lung_Inh.mha'),'file')
    [~,lungInh] = MhaRead(fullfile(outputDir,'Lung_Inh.mha'));
    [pcVec,dphInh] = dph3DDualReg(dphR,dphL,single(~lungInh),headerCT);
else
    [pcVec,dphInh] = dph3DDualReg(dphR,dphL,ctInh,headerCT);
end
% Generate diaphragm DRR model
dphModel = dphDRRModel(fullfile(outputDir,'dphR.mha'),fullfile(outputDir,'dphL.mha'),...
    fullfile(outputDir,'CT_Exh.mha'),volOffset_Dph - volOffset_GTV,pcVec,modelGeoFile,...
    projSize,projSpacing,outputDir,false,fullfile(outputDir,'DRR_CT_Exh.mha'));
writeDphDRRModel(dphModel,fullfile(outputDir,'MAGIK_ModelFiles','DphDRRModel.bin'));
fprintf('COMPLETED using %s \n',seconds2human(toc));

%% Build tumor-diaphragm-tumor model
fprintf('Building the diaphragm-tumor model ...... ');
tic;
dphTargetModel(gtvExhPos,gtvInhPos,outputDir);
[~,~] = system(['move "',fullfile(outputDir,'DphTargetModel.txt'),'" "',fullfile(outputDir,'MAGIK_ModelFiles','DphTargetModel.txt'),'"']);
fprintf('COMPLETED using %s \n',seconds2human(toc));

%% Check model
fprintf('Generating and saving model visualization ...... ');
tic;

% Save offset
save(fullfile(outputDir,'volOffsets.mat'),'volOffset_GTV','volOffset_Dph');

% Index of the center of the GTV
gtvCtdIdx = round((0.5 * gtvExhPos + 0.5 * gtvInhPos - headerCT.Offset) ...
    ./ headerCT.PixelDimensions) + 1;

% The mask for angles where we skip preTx learning
skipLearnMask = false([headerCT.Dimensions(1),headerCT.Dimensions(3)]);
isoCtrIdx = -headerCT.Offset ./ headerCT.PixelDimensions + 1;
for x = 1:size(skipLearnMask,1)
    for y = 1:size(skipLearnMask,2)
        opp = (x - isoCtrIdx(1)) * headerCT.PixelDimensions(1);
        adj = (y - isoCtrIdx(3)) * headerCT.PixelDimensions(3);
        ang = atand(opp/adj) + 180 * (opp.*adj < 0) + 180 * (opp < 0);
        modelIdx = find(abs(mod(ang,360) - mod(gtvModel.Angles,360)) == ...
            min(abs(mod(ang,360) - mod(gtvModel.Angles,360))));
        modelIdx = modelIdx(1);
        skipLearnMask(x,y) = gtvModel.ViewScore(modelIdx) < gtvModel.ViewScoreThreshold;
    end
end

% Visualization - Coronal
for k = 1:2
    if k == 1
        ctImg = ctExh(:,:,gtvCtdIdx(3));
        gtvMask = gtvExh(:,:,gtvCtdIdx(3)) > 0;
        dphMask = dphR(:,:,gtvCtdIdx(3)) > 0 | dphL(:,:,gtvCtdIdx(3)) > 0;
    else
        ctImg = ctInh(:,:,gtvCtdIdx(3));
        gtvMask = gtvInh(:,:,gtvCtdIdx(3)) > 0;
        dphMask = dphInh(:,:,gtvCtdIdx(3)) > 0;
    end
    bm = uint8(round(ctImg / 0.03 * 255));
    bm(gtvMask | dphMask) = bm(gtvMask | dphMask) - 70;
    bmG = bm; bmB = bm;
    bmB(gtvMask) = bmB(gtvMask) + 150;
    bmG(dphMask) = bmG(dphMask) + 200;
    bm(:,:,1) = bm; bm(:,:,2) = bmG; bm(:,:,3) = bmB;
    image(permute(bm,[2 1 3])); 
    daspect([headerCT.PixelDimensions(2),headerCT.PixelDimensions(1),1]); axis off;
    fCoronal(k) = getframe;
end

% Visualization - Axial
for k = 1:2
    if k == 1
        ctImg = permute(ctExh(:,gtvCtdIdx(2),end:-1:1),[1 3 2]);
        gtvMask = permute(gtvExh(:,gtvCtdIdx(2),end:-1:1),[1 3 2]) > 0;
        dphMask = permute(dphR(:,gtvCtdIdx(2),end:-1:1) | dphL(:,gtvCtdIdx(2),end:-1:1),[1 3 2]) > 0;
    else
        ctImg = permute(ctInh(:,gtvCtdIdx(2),end:-1:1),[1 3 2]);
        gtvMask = permute(gtvInh(:,gtvCtdIdx(2),end:-1:1),[1 3 2]) > 0;
        dphMask = permute(dphInh(:,gtvCtdIdx(2),end:-1:1),[1 3 2]) > 0;
    end
    skiplearnMask_Vis = skipLearnMask(:,end:-1:1);
    bm = uint8(round(ctImg / 0.03 * 255));
    bm(gtvMask | dphMask) = bm(gtvMask | dphMask) - 70;
    bmR = bm; bmB = bm; bmG = bm;
    bmR(skiplearnMask_Vis) = bmR(skiplearnMask_Vis) + 50;
    bmB(gtvMask) = bmB(gtvMask) + 150;
    bmG(dphMask) = bmG(dphMask) + 200;
    bm(:,:,1) = bmR; bm(:,:,2) = bmG; bm(:,:,3) = bmB;
    image(permute(bm,[2 1 3])); 
    daspect([headerCT.PixelDimensions(3),headerCT.PixelDimensions(1),1]); axis off;
    fAxial(k) = getframe;
end

% Visualization - Sagittal
for k = 1:2
    if k == 1
        ctImg = permute(ctExh(gtvCtdIdx(1),:,end:-1:1),[3 2 1]);
        gtvMask = permute(gtvExh(gtvCtdIdx(1),:,end:-1:1),[3 2 1]) > 0;
        dphMask = permute(dphR(gtvCtdIdx(1),:,end:-1:1) | dphL(gtvCtdIdx(1),:,end:-1:1),[3 2 1]) > 0;
    else
        ctImg = permute(ctInh(gtvCtdIdx(1),:,end:-1:1),[3 2 1]);
        gtvMask = permute(gtvInh(gtvCtdIdx(1),:,end:-1:1),[3 2 1]) > 0;
        dphMask = permute(dphInh(gtvCtdIdx(1),:,end:-1:1),[3 2 1]) > 0;
    end
    bm = uint8(round(ctImg / 0.03 * 255));
    bm(gtvMask | dphMask) = bm(gtvMask | dphMask) - 70;
    bmG = bm; bmB = bm;
    bmB(gtvMask) = bmB(gtvMask) + 150;
    bmG(dphMask) = bmG(dphMask) + 200;
    bm(:,:,1) = bm; bm(:,:,2) = bmG; bm(:,:,3) = bmB;
    image(permute(bm,[2 1 3])); 
    daspect([headerCT.PixelDimensions(2),headerCT.PixelDimensions(3),1]); axis off;
    fSagittal(k) = getframe;
end
close('all');

movie2gif(fCoronal,fullfile(outputDir,'ModelVisualization_Coronal.gif'),'DelayTime',0.2,'LoopCount',Inf);
movie2gif(fAxial,fullfile(outputDir,'ModelVisualization_Axial.gif'),'DelayTime',0.2,'LoopCount',Inf);
movie2gif(fSagittal,fullfile(outputDir,'ModelVisualization_Sagittal.gif'),'DelayTime',0.2,'LoopCount',Inf);

% Visualize GTV and diaphragm DRR model
vGTV = visualizeGTVDRRModel(gtvModel);
vDph = visualizeDphDRRModel(dphModel);
vDph = 0.5 * vDph{1} + 0.5 * vDph{2};
figure('unit','normalized','outerposition',[0,0,1,1]);
for k = 1:size(vGTV,3)
    imagesc(vGTV(:,:,k)',[-1,1]); colormap gray; axis image; axis off;
    text(6,12,num2str(gtvModel.Angles(k),'GTV Model, %f degrees'),'color','blue','FontSize',14);
    fGTV(k) = getframe;
end
for k = 1:size(vDph,3)
    imagesc(vDph(:,:,k)',[0,1]); colormap gray; axis image; axis off;
    text(6,12,num2str(dphModel.Angles(k),'Diaphragm Model, %f degrees'),'color','blue','FontSize',14);
    fDph(k) = getframe;
end
close('all');

vw = VideoWriter(fullfile(outputDir,'ModelVisualization_GTV.avi'));
vw.open;
vw.writeVideo(fGTV);
vw.close;

vw = VideoWriter(fullfile(outputDir,'ModelVisualization_Diaphragm.avi'));
vw.open;
vw.writeVideo(fDph);
vw.close;

fprintf('COMPLETED using %s \n',seconds2human(toc));
fprintf('Model visualization saved under MAGIKModel as ModelVisualization_XXX files. \n');
end

function outputArr = inputCharToArray(arg)
arg(arg == '[' | arg == ']') = '';

if isempty(strfind(arg,','))
    parts = regexp(arg,' ','split');
else
    parts = regexp(arg,',','split');
end

for k = 1:length(parts)
    outputArr(1,k) = str2double(strtrim(parts{k}));
end
end