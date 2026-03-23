function model = get2DDphModel(dphLFile,dphRFile,dSI,dAP,dRot,rotCtrL,rotCtrR,volOffset,sid,sdd,detOffset,projSize,projSpacing,outputDir,verbose,exhCTFile,inhCTFile)
%% model = get2DDphModel(dphLFile,dphRFile,dSI,dAP,dRot,rotCtrL,rotCtrR,volOffset,sid,sdd,detOffset,projSize,projSpacing,outputDir,verbose,inhCTFile,exhCTFile)
% ------------------------------------------
% FILE   : get2DDphModel_Rot.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-08-14  Created.
% ------------------------------------------
% PURPOSE
%  Generate 2D diaphragm model in the 2D image space.
%  The model is generated for 0-360 degree gantry angle with 0.5 degree
%  increment.
%  This was modified from get2DDiaphragmModel to incorporate diaphragm
%  rotation.
% ------------------------------------------
% INPUT
%   dphLFile:       MHA file of the 3D model of the left diaphragm.
%   dphRFile:       MHA file of the 3D model of the right diaphragm.
%   dSI,dAP,dRot:   The principle component vector of 3D diaphragm
%                   motion, with each component corresponding to SI
%                   translation (mm), AP translation, and rotation (deg).
%                   This can be calculated by using dph3DRigidRef to
%                   match the exhale diaphragm mask with the inhale CT.
%   rotCtrL:        The 3D coordinate of the rotation center of the left
%                   diaphragm.
%   rotCtrR:        The 3D coordinate of the rotation center of the right
%                   diaphragm.
%   volOffset:      1x3 vector of the offset needed to shift the diaphragm
%                   model to align with patient position.
%   sid:            Source-to-isocenter distance (mm).
%   sdd:            Source-to-detector distance (mm).
%   detOffset:      1x2 vector of the [lateral,vertical] detector offset
%                   (mm).
%   projSize:       1x2 vector of the projection image dimension (int).
%   projSpacing:    1x2 vector of the projection pixel sizes (double).
%   outputDir:      (Optional) If input, the program will save the model as
%                   Dph2DModel.mat in outputDir.
%                   Default: does not save
%   verbose:        Whether or not to display progress.
%                   Default: true
%   exhCTFile:      (Optional) The end exhale CT file used to estimate the
%                   view compensation factor. Compensation formula is:
%                   y = (x - ViewComp(k,1)) / (ViewComp(k,2) -
%                   ViewComp(k,1))
%   inhCTFile:      (Optional) The end inhale CT file used to estimate the 
%                   view compensation factor.

% OUTPUT
%   model:          The model in a struct. The struct records the geometric
%                   specificis, geometry matrices, and the 2D diaphragm
%                   points for each angular view.

%% Initialize parameters and output sturct
if nargin < 15
    verbose = true;
end

model.SID = sid;
model.SDD = sdd;
model.VolOffset = volOffset;
model.DetOffset = detOffset;
model.ProjSize = projSize;
model.ProjSpacing = projSpacing;
model.Angles = 0:0.5:(360-0.5);
model.RotCtr_L = rotCtrL;
model.RotCtr_R = rotCtrR;
model.PCVec = [dSI,dAP,dRot];
model.ViewFactor = zeros(length(model.Angles),2);
model.ViewFactor(:,2) = 1;

%% Generate the RTK geometry matrices
if verbose
    fprintf('Generating RTK geometry matrices ......');
    tic;
end

rtksimulatedgeometry = which('rtksimulatedgeometry.exe');
if isempty(rtksimulatedgeometry)
    rtksimulatedgeometry = 'rtksimulatedgeometry';
end
geoFile = [tempname,'.xml'];
system(['"',rtksimulatedgeometry,'" ','-f 0 -n 720 -a 360 ',...
    '--sid ',num2str(sid,'%f'),' ',...
    '--sdd ',num2str(sdd,'%f'),' ',...
    '--proj_iso_x ',num2str(detOffset(1),'%f'),' ',...
    '--proj_iso_y ',num2str(detOffset(2),'%f'),' ',...
    '-o "',geoFile,'"']);
model.G = ReadRTKGeometryMatrices(geoFile);

if nargin < 8
    system(['del "',geoFile,'"']);
end

if verbose
    fprintf('COMPLETED using %s\n',seconds2human(toc));
end

%% Reading the 3D diaphragm model
if verbose
    fprintf('Reading the 3D diaphragm model ......');
    tic;
end

% From here on we assign 1 to the right diaphragm and 2 to the left.
% This allows us to process the two diaphragms in a loop without repeating
% codes

[header{1},dph{1}] = MhaRead(dphRFile);
[header{2},dph{2}] = MhaRead(dphLFile);

% Convert binary mask into 3D points (in physical coordinate mm)
for nside = 1:2
    ptsIdx = find(dph{nside});
    [pts3D{nside}(:,1),pts3D{nside}(:,2),pts3D{nside}(:,3)] = ...
        ind2sub(size(dph{nside}),ptsIdx);
    pts3D{nside} = (pts3D{nside} - 1) .* (ones(length(ptsIdx),1) * header{nside}.PixelDimensions) + ...
        ones(length(ptsIdx),1) * (header{nside}.Offset + volOffset);
    % We must take into account the fact that what we are tracking is the
    % transition between lung and diaphragm. Thus the SI pixel should be
    % corrected for half of the SI pixel size
    pts3D{nside}(:,2) = pts3D{nside}(:,2) - 0.5 * header{nside}.PixelDimensions(2);
end

model.Pts3D_R = pts3D{1};
model.Pts3D_L = pts3D{2};

% Find the coordinate of the top of the right and left diaphragm
indTop = find(pts3D{1}(:,2) == max(pts3D{1}(:,2)));
model.Top3D_R = mean(pts3D{1}(indTop,:),1);
indTop = find(pts3D{2}(:,2) == max(pts3D{2}(:,2)));
model.Top3D_L = mean(pts3D{2}(indTop,:),1);

if verbose
    fprintf('COMPLETED using %s\n',seconds2human(toc));
end

%% Projecting 3D points to 2D projection space
if verbose
    fprintf('Projecting 3D points to 2D projection space ......');
    tic;
end

for nside = 1:2
    % Projection space pixel margin to account for finite 3D voxel size
    wx = max(header{nside}.PixelDimensions(1),header{nside}.PixelDimensions(3)) * sdd / sid / projSpacing(1);
    wy = header{nside}.PixelDimensions(2) * sdd / sid / projSpacing(2);
    
    for k = 1:size(model.G,3)
        slice = zeros(projSize);
        pts2D = rtk3DTo2D(model.G(:,:,k),pts3D{nside});
        idx2D = round(pts2D ./ projSpacing + (projSize + 1) / 2);
        
        % Sum points up in the projection space
        % We apply a kernel that's weighted by distance to the central
        % pixel. This is to account for the partial volume effect
        for np = 1:size(pts2D,1)
            startU = max(1,floor(idx2D(np,1) - wx));
            endU = min(projSize(1),ceil(idx2D(np,1) + wx));
            startV = max(1,floor(idx2D(np,2) - wy));
            endV = min(projSize(2),ceil(idx2D(np,2) + wy));
            wker = ones(endU-startU+1,endV-startV+1);
            for u = 1:size(wker,1)
                for v = 1:size(wker,2)
                    wker(u,v) = 1 - abs(idx2D(np,1) - (startU + u - 1)) / wx * abs(idx2D(np,2) - (startV + v - 1)) / wx;
                end
            end
            slice(startU:endU,startV:endV) = slice(startU:endU,startV:endV) + wker;
        end
        % We filter out points with low intensity values. They generally
        % have minimal contribution to tracking
        thdVal = prctile(slice(slice>0),75);
        slice(slice<thdVal) = 0;
        
        % Now we simplify our point list into non-zero points in the
        % processed slice.
        clear idx2D;
        idx1D = find(slice>0);
        [idx2D(:,1),idx2D(:,2)] = ind2sub(projSize,idx1D);
        pts2D = (idx2D - (projSize + 1)/2) .* projSpacing;
        
        % Density weighting
        dstWght = slice(idx1D);
        % Lateral weighting
        latWght = single(zeros(size(dstWght)));
        for x = min(idx2D(:,1)):max(idx2D(:,1))
            indX = find(idx2D(:,1)==x);
            latWght(indX) = dstWght(indX) / sum(dstWght(indX));
        end
        
        if nside == 1
            model.Pts2D_R{k} = pts2D;
            model.Idx2D_R{k} = idx2D;
            model.Idx1D_R{k} = idx1D;
            model.DstWght_R{k} = single(dstWght);
            model.LatWght_R{k} = single(latWght);
        else
            model.Pts2D_L{k} = pts2D;
            model.Idx2D_L{k} = idx2D;
            model.Idx1D_L{k} = idx1D;
            model.DstWght_L{k} = single(dstWght);
            model.LatWght_L{k} = single(latWght);
        end
    end
end

if verbose
    fprintf('COMPLETED using %s\n',seconds2human(toc));
end

%% If exhCTFile and inhCTFile are provided, we estimate the view compensation factor here
if exist('exhCTFile','var') && exist('inhCTFile','var')
    if verbose
        fprintf('Estimating view compensation factor ......');
        tic;
    end
    
    tempdir = tempname;
    mkdir(tempdir);
    
    % First write CT files with volume offset
    writeTempCTFile(exhCTFile,volOffset,fullfile(tempdir,'ExhCT.mha'));
    writeTempCTFile(inhCTFile,volOffset,fullfile(tempdir,'InhCT.mha'));
        
    % Simulate projections
    simulateProjFromCT(fullfile(tempdir,'ExhCT.mha'),geoFile,projSize,projSpacing,fullfile(tempdir,'Proj_ExhCT.mha'));
    simulateProjFromCT(fullfile(tempdir,'InhCT.mha'),geoFile,projSize,projSpacing,fullfile(tempdir,'Proj_InhCT.mha'));
    splitProjInto2DFiles(fullfile(tempdir,'Proj_ExhCT.mha'));
    splitProjInto2DFiles(fullfile(tempdir,'Proj_InhCT.mha'));
    
    % Track them
    trjExh = trackDiaphragm(lscell(fullfile(tempdir,'Proj_ExhCT','Proj_*.mha')),model,geoFile,false);
    trjInh = trackDiaphragm(lscell(fullfile(tempdir,'Proj_InhCT','Proj_*.mha')),model,geoFile,false);
    model.ViewFactor(:,1) = hpfilter(trjExh(:,4),1600);
    model.ViewFactor(:,2) = hpfilter(trjInh(:,4),1600);
    
    if exist('outputDir','var')
        if ~exist(outputDir,'dir')
            mkdir(outputDir);
        end
        system(['copy "',fullfile(tempdir,'Proj_ExhCT','DphTracking.avi'),'" "',fullfile(outputDir,'ViewFactor_Exhale.avi'),'"']);
        system(['copy "',fullfile(tempdir,'Proj_InhCT','DphTracking.avi'),'" "',fullfile(outputDir,'ViewFactor_Inhale.avi'),'"']);
    end
    
    rmdir(tempdir,'s');
    
    if verbose
        fprintf('COMPLETED using %s\n',seconds2human(toc));
    end
end

%% Saving
if exist('outputDir','var')
    if verbose
        fprintf('Saving the 2D model ......');
        tic;
    end
    if ~exist(outputDir,'dir')
        mkdir(outputDir);
    end
    save(fullfile(outputDir,'Dph2DModel.mat'),'model','-v7.3');
    
    copyfile(geoFile,fullfile(outputDir,'ModelGeometry.xml'));
    system(['del "',geoFile,'"']);
    
    if verbose
        fprintf('COMPLETED using %s\n',seconds2human(toc));
    end
end

end

function writeTempCTFile(inputFile,volOffset,outputFile)
[info,M] = MhaRead(inputFile);
info.Offset = info.Offset + volOffset;
M = single(M);
if min(M(:)) < -500 % in HU
    M = (M + 1000) / 1000 * 0.013;
end
if min(M(:)) >= 0 && max(M(:)) > 1000
    M = M / 1000 * 0.013;
end
info.DataType = 'float';
info.BitDepth = 32;
MhaWrite(info,M,outputFile);
end

function simulateProjFromCT(ctFile,geoFile,projSize,projSpacing,outputFile)
[~,msg] = system(['rtkforwardprojections --fp CudaRayCast -g "',geoFile,'" ',...
    '--dimension ',num2str(projSize,'%d,%d'),' --spacing ',num2str(projSpacing,'%f,%f'),' ',...
    '-i "',ctFile,'" -o "',outputFile,'"']);
if ~isempty(msg)
    [~,msg] = system(['rtkforwardprojections -g "',geoFile,'" ',...
        '--dimension ',num2str(projSize,'%d,%d'),' --spacing ',num2str(projSpacing,'%f,%f'),' ',...
        '-i "',ctFile,'" -o "',outputFile,'"']);
end
end

function splitProjInto2DFiles(inputFile)
[top,name,ext] = fileparts(inputFile);
mkdir(fullfile(top,name));
[info,P] = MhaRead(inputFile);
info.NumberOfDimensions = 2;
info.TransformMatrix = [1 0 0 1];
info.Offset(3) = [];
info.CenterOfRotation(3) = [];
info.PixelDimensions(3) = [];
info.Dimensions(3) = [];
for k = 1:size(P,3)
    MhaWrite(info,P(:,:,k),fullfile(top,name,num2str(k,'Proj_%05d.mha')));
end
end