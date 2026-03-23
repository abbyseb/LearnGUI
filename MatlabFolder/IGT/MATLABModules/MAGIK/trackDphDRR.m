function [trj,metricVal,trackFrame,trackedMap] = ...
    trackDphDRR(projList,model,geometryFile,verbose,outputDir,flipTag,invertTag,rt,excMargin,frameInd)
%% [trj,metricVal,trackFrame,trackedMap] = trackDphDRR(projList,model,geometryFile,verbose,outputDir,flipTag,invertTag,rt,excMargin,frameInd)
% ------------------------------------------
% FILE   : trackDphDRR.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2019-02-28   Created.
% ------------------------------------------
% PURPOSE
%   Track diaphragms from projection images using a pre-built model.
%   This is the version that uses the DRR-based model.
% ------------------------------------------
% INPUT
%   projList:       List of projection files in cell format (use lscell)
%   model:          The 2D diaphragm model (use dphDRRModel.m)
%   geometryFile:   The RTK geometry .xml file.
%   verbose:        Whether or not to display progress.
%                   (Optional) Default: true
%   outputDir:      Path to where to save the results and video.
%   flipTag:        Whether or not to flip the projection in the vertical
%                   direction.
%                   Default = false
%   invertTag:      Whether or not to invert projection intensity.
%                   Default = false
%   rt:             (Optional) Search radius in terms of diaphragm motion
%                   parameter t (0 = end exhale and 1 = end inhale at 4DCT)
%                   Default: 0.3 but 3 at the first frame
%   excMargin:      (Optional) Number of pixels to exclude on the left,
%                   top, right, and bottom boundaries.
%                   Default: [0,0,0,0]
%   frameInd:       (Optional) The indices of the frames to track.
%                   e.g. 1:500
%                   Default: All the frames
%
% OUTPUT
%   trj:            The [LR,SI,AP,t] value of the diaphragm trajectory.
%   metricVal:      The match scores for each frame (higher = better match)
%   trackFrames:    The visualization frames.
%   trackedMap:     The binary masks of the tracked diaphragm points.

%% Input check
if nargin < 4
    verbose = true;
end

if nargin < 5
    outputDir = fileparts(projList{1});
end

if nargin < 6
    flipTag = false;
end

if nargin < 7
    invertTag = false;
end

if nargin < 8
    rt = 0.3;
end

if nargin < 9
    excMargin = [0,0,0,0];
end

if nargin < 10
    frameInd = 1:length(projList);
end

%% Hard coded parameters
initrt = 3; % Initial search radius
if rt > initrt
    initr = rt;
end

% Maxinum number of sample points to constrain computation time
N_Sample = 1e4;

%% Initialization
if verbose
    fprintf('Initializing......');
    tic;
end

ang = ReadAnglesFromRTKGeometry(geometryFile);
sid = ReadTagFromRTKGeometry(geometryFile,'SourceToIsocenterDistance');
sdd = ReadTagFromRTKGeometry(geometryFile,'SourceToDetectorDistance');

% Pre-allocation
[~,P] = ProjRead(projList{frameInd(1)});
trackedMap = single(zeros([size(P),length(frameInd)]));

t_prev = 0; % t is diaphragm motion parameter
fh = figure('units','normalized','outerposition',[0.1,0.1,0.8,0.8]);
nCount = 0;

if verbose
    fprintf('%f seconds\n',toc);
end
%% Start tracking
for n = frameInd
    if verbose
        fprintf('Frame#%05d......',n);
        tic;
    end
    nCount = nCount + 1;
    % Read projection
    [~,~,ext] = fileparts(projList{n});
    [projInfo,P] = ProjRead(projList{n});
    P = single(P);
    
    if flipTag
        P = P(:,end:-1:1);
    end
    if invertTag
        P = 65535 - P;
    end
    if strcmpi(ext,'.hnc')
        P = log(65536 ./ (P + 1));
        spacingX = projInfo.dIDUResolutionX;
        spacingY = projInfo.dIDUResolutionY;
    elseif strcmpi(ext,'.hnd')
        P = log(65536 ./ (P + 1));
        % Hnd files acquired from iTools often have the resolution fiels
        % written incorrectly. Enforcing 0.388 mm pixel size here
        spacingX = 0.388;
        spacingY = 0.388;
    elseif strcmpi(ext,'.his')
        P = log(65536 ./ (65536 - P));
        spacingX = projInfo.PixelSpacingX;
        spacingY = projInfo.PixelSpacingY;
    elseif strcmpi(ext,'.mha')
        spacingX = projInfo.PixelDimensions(1);
        spacingY = projInfo.PixelDimensions(2);
    end
    if verbose
        fprintf('%f seconds...... ',toc);
    end
    
    % Estimate the optimal discretization of t
    dt = max(model.PCVec(:,2)) * sdd / sid / spacingY;
    dt = 1/dt;
    
    if verbose
        fprintf('Tracking......');
        tic;
    end
    
    % Exclude margins
    P(1:excMargin(1),:) = nan;
    P(end-excMargin(3)+1:end,:) = nan;
    P(:,1:excMargin(2)) = nan;
    P(:,end-excMargin(4)+1:end) = nan;
    
    % Select which frame of the 2D model to use
    indModel = find(abs(mod(ang(n),360) - mod(model.Angles,360)) == min(abs(mod(ang(n),360) - mod(model.Angles,360))));
    indModel = indModel(1);
    
    % Construct vectors for going through the search window
    % We include points that are both around the previous t, and also
    % around the past mean position
    if nCount == 1
        tCand = round((t_prev-initrt:dt:t_prev+initrt)*100)/100;
    else
        tCand = round((t_prev-rt:dt:t_prev+rt)*100)/100;
    end
    if nCount > 1
        indTracked = ismember(1:max(frameInd),frameInd) & 1:max(frameInd) < n;
        pastMeanT = round(mean(trj(indTracked,4)));
        tCand = round([tCand, (pastMeanT-rt:dt:pastMeanT+rt)]*100)/100;
        tCand = unique(tCand);
    end
    % Compute effective t using the view compensation factor
    dvCand_R = round(tCand * model.PCVec(1,2) * sdd / sid / spacingY);
    duCand_R = round(tCand * (cosd(ang(n)) * model.PCVec(1,1) - sind(ang(n)) * model.PCVec(1,3)) / spacingX);
    dvCand_L = round(tCand * model.PCVec(2,2) * sdd / sid / spacingY);
    duCand_L = round(tCand * (cosd(ang(n)) * model.PCVec(2,1) - sind(ang(n)) * model.PCVec(2,3)) / spacingX);
    
    oldIdx2D_R = model.Idx2D_R{indModel};
    dstWght_R = model.DstWght_R{indModel};
    drr_R = model.DRR_R{indModel};
    oldIdx2D_L = model.Idx2D_L{indModel};
    dstWght_L = model.DstWght_L{indModel};
    drr_L = model.DRR_L{indModel};
    drr_All = [drr_R;drr_L];
    
    % Normalize projection
    P_Norm = P;
    idx1D_Norm = sub2ind(size(P),[oldIdx2D_R(:,1);oldIdx2D_L(:,1)],...
        [oldIdx2D_R(:,2);oldIdx2D_L(:,2)]);
    p_min = min(P_Norm(idx1D_Norm));
    p_max = max(P_Norm(idx1D_Norm));
    P_Norm = (P_Norm - p_min) / (p_max - p_min);
    P_Norm(P_Norm<0) = 0;
    P_Norm(P_Norm>1) = 1;
    
    metricVec = -ones(length(tCand),1);
    clear newIdx2D_R newIdx2D_L;
    metric_Optim = -1;
    for k = 1:length(tCand)
        % Calculate the new pixel index
        newIdx2D_R(:,1) = round(oldIdx2D_R(:,1) + duCand_R(k));
        newIdx2D_R(:,2) = round(oldIdx2D_R(:,2) + dvCand_R(k));
        newIdx2D_L(:,1) = round(oldIdx2D_L(:,1) + duCand_L(k));
        newIdx2D_L(:,2) = round(oldIdx2D_L(:,2) + dvCand_L(k));
        
        % Points within FOV
        indIncl_R = newIdx2D_R(:,1) >= 1 + excMargin(1) & newIdx2D_R(:,1) <= size(P,1) - excMargin(3) & newIdx2D_R(:,2) >= 1 + excMargin(2) & newIdx2D_R(:,2) <= size(P,2) - excMargin(4);
        indIncl_L = newIdx2D_L(:,1) >= 1 + excMargin(1) & newIdx2D_L(:,1) <= size(P,1) - excMargin(3) & newIdx2D_L(:,2) >= 1 + excMargin(2) & newIdx2D_L(:,2) <= size(P,2) - excMargin(4);
        
        % Combine L and R
        idxIncl_All = find([indIncl_R;indIncl_L]);
        newIdx2D_All = [newIdx2D_R;newIdx2D_L];

        % Sample N_Sample points
        if length(idxIncl_All) > N_Sample
            idxIncl_All = idxIncl_All(round(linspace(1,length(idxIncl_All),N_Sample)'));
        end
        
        % Convert to 1D index
        newIdx1D_Incl = sub2ind(size(P),newIdx2D_All(idxIncl_All,1),newIdx2D_All(idxIncl_All,2));
        drr_Incl = drr_All(idxIncl_All);
        P_Incl = P_Norm(newIdx1D_Incl);
        
        % We cut the included points into four segments in the horizontal
        % direction and calculate the mean correlation for each segment.
        % This was found to be more robust than calculating for the entire
        % segment
        uIncl = newIdx2D_All(idxIncl_All,1);
        uMax = max(uIncl);
        uMin = min(uIncl);
        indSeg{1} = uIncl < 0.75 * uMin + 0.25 * uMax;
        indSeg{2} = uIncl >= 0.75 * uMin + 0.25 * uMax & uIncl < 0.5 * uMin + 0.5 * uMax;
        indSeg{3} = uIncl >= 0.5 * uMin + 0.5 * uMax & uIncl < 0.25 * uMin + 0.75 * uMax;
        indSeg{4} = uIncl >= 0.25 * uMin + 0.75 * uMax;
        
        % Calculate tracking metric
        metricVec(k) = 0;
        for nseg = 1:4
            metricVec(k) = metricVec(k) + 0.25 * corr(P_Incl(indSeg{nseg}),drr_Incl(indSeg{nseg}));
        end
        
        if k == 1 || metricVec(k) > metric_Optim
            indBest = k;
            metric_Optim = metricVec(k);
            newIdx2D_R_Optim = newIdx2D_R;
            newIdx2D_L_Optim = newIdx2D_L;
            indIncl_R_Optim = indIncl_R;
            indIncl_L_Optim = indIncl_L;
        end
    end
    
    % Best match
    metricVal(n) = metric_Optim;
    t_prev = tCand(indBest);
    
    % Convert to patient coordinate
    trj(n,1) = t_prev * model.PCVec(1);
    trj(n,2) = t_prev * model.PCVec(2);
    trj(n,3) = t_prev * model.PCVec(3);
    trj(n,4) = t_prev;
    
    if verbose
        fprintf('%f seconds...... ',toc);
    end
    
    % Create results for visualisation
    if verbose
        fprintf('Visualizing......');
        tic;
    end
    
    % Put it into a map image
    dstMap_R = single(zeros(size(P)));
    dstMap_L = single(zeros(size(P)));
    newIdx1D_R_Optim = sub2ind(size(P),newIdx2D_R_Optim(indIncl_R_Optim,1),newIdx2D_R_Optim(indIncl_R_Optim,2));
    newIdx1D_L_Optim = sub2ind(size(P),newIdx2D_L_Optim(indIncl_L_Optim,1),newIdx2D_L_Optim(indIncl_L_Optim,2));
    dstMap_R(newIdx1D_R_Optim) = dstWght_R(indIncl_R_Optim);
    dstMap_L(newIdx1D_L_Optim) = dstWght_L(indIncl_L_Optim);
    dstMap = dstMap_R + dstMap_L;
    maxMapVal = max([model.MaxDst_R(indModel),model.MaxDst_L(indModel)]);
    
    % Also save the DRR map
    drrMap = single(zeros(size(P)));
    drrMap([newIdx1D_R_Optim;newIdx1D_L_Optim]) = [drr_R(indIncl_R_Optim);drr_L(indIncl_L_Optim)];
    trackedMap(:,:,nCount) = drrMap;
    
    pVis = P_Norm * 255;
    pVis(isnan(pVis)) = 0;
    pVis(pVis>255) = 255; pVis(pVis<0) = 0;
    pVis = uint8(round(pVis'));
    pB = pVis;
    pVis = pVis - uint8(150 * dstMap' / maxMapVal);
    pVis(:,:,2) = pVis; pVis(:,:,3) = pB;
    imagesc(pVis); axis image; axis off;
    [~,fileName] = fileparts(projList{n});
    text(20,30,[fileName,...
        '   t = ',num2str(trj(n,4),'%f'),...
        '   score = ',num2str(metricVal(n),'%f')],...
        'color','blue','FontSize',14);
    trackFrame(nCount) = getframe(fh);

    if verbose
        fprintf('%f seconds......\n',toc);
    end
end

close(fh);

%% Write tracking movie to .avi and results to .mat file
if verbose
    fprintf('Saving results in %s ......',outputDir);
    tic;
end

if ~exist(outputDir,'dir')
    mkdir(outputDir);
end

save(fullfile(outputDir,'DphTrackingResults.mat'),...
    'trj','metricVal');

vw = VideoWriter(fullfile(outputDir,'DphTracking.avi'));
vw.open;
vw.writeVideo(trackFrame);
vw.close;

save(fullfile(outputDir,'DphTrackingFrames.mat'),...
    'trackFrame','trackedMap','-v7.3');

if verbose
    fprintf('%f seconds......\n',toc);
end

end