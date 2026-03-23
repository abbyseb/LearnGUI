function [syncResult, imageCheck] = syncCalypsoToProj(calData,tiffList,projTS,G,projSpacing,scanStartInd,scanEndInd)
%% [syncResult, imageCheck] = syncCalypsoToProj(calData,tiffList,projTS,G,projSpacing,scanStartInd,scanEndInd)
% ------------------------------------------
% FILE   : syncCalypsoToProj.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2018-06-08 Created.
% ------------------------------------------
% PURPOSE
%   Sync Calypso data to a list of projection file.
%
% ------------------------------------------
% INPUT
%   calData:        Calypso report read as a MATLAB struct. This can be
%                   obatained by using the "ReadCalypsoReport.m" function 
%                   to read a Calypso .xls file
%   tiffList:       List of TIFF projection files in cell format. Can be
%                   obtained using the "lscell.m" function.
%   projTS:         Vector of projection time stamp acquired using the
%                   GetFileTimeStamp.m function (applied to the original
%                   .hnd projections that have the original file created
%                   time stamps).
%   G:              The geometry matrices of the projections. This can be
%                   obtained by using the "ReadRTKGeometryMatrices.m"
%                   function to read the RTK geometry .xml file.
%   projSpacing:    The pixel size (2x1, mm) of the projection image. e.g.
%                   [0.388,0.388].
%   scanStartInd:   An estimated index of where the scan starts in
%                   calData.Target.
%   scanEndInd:     An estimated index of where the scan ends in
%                   calData.Target.
% ------------------------------------------
% OUTPUT
%   syncResult:     The sync result in an output struct.
%   imageCheck:     3D matrix which can be visualized by imshow3D to check
%                   if the projected beacon positions are correct.

%% Input check
if length(tiffList) ~= size(G,3)
    error('ERROR: The number of projection files must match with the number of geometry matrices.')
end

%% Convert time stamps to time number
tn = interpolateDatetime(projTS);
syncResult.ProjTimestamp = projTS;
syncResult.ProjTimeNumber = tn;

%% Read projections
for k = 1:length(tiffList)
    P(:,:,k) = imread(tiffList{k})';
end

%% Find best sync
scanDuration = datetime(tn(end),'ConvertFrom','datenum') - datetime(tn(1),'ConvertFrom','datenum');
stVec = calData.Target.Time(scanStartInd):seconds(0.1):(calData.Target.Time(scanEndInd) - scanDuration);
for n = 1:length(stVec)
    s = 0;
    c = 0;
    for b = 1:length(calData.Beacons)
        bTime = datenum(calData.Beacons{b}.Time) - datenum(stVec(n)) + tn(1);
        bTrj3D(:,1) = interp1(bTime,calData.Beacons{b}.LR,tn)';
        bTrj3D(:,2) = -1 * interp1(bTime,calData.Beacons{b}.SI,tn)';
        bTrj3D(:,3) = interp1(bTime,calData.Beacons{b}.AP,tn)';
        bTrj2D = rtk3DTo2D(G,bTrj3D);
        bInd2D = round(bTrj2D ./ (ones(size(bTrj2D,1),1) * projSpacing) + ...
            (ones(size(bTrj2D,1),1) * (size(P(:,:,1))+1) / 2));
        for k = 1:size(bInd2D,1)
            if bInd2D(k,1) >= 1 && bInd2D(k,1) <= size(P,1) && bInd2D(k,2) >= 1 && bInd2D(k,2) <= size(P,2)
                c = c + 1;
                %w = P(max(1,bInd2D(k,1)-0):min(size(P,1),bInd2D(k,1)+0),max(1,bInd2D(k,2)-0):min(size(P,2),bInd2D(k,2)+0),k);
                w = single(P(max(1,bInd2D(k,1)-3):min(size(P,1),bInd2D(k,1)+3),max(1,bInd2D(k,2)-3):min(size(P,2),bInd2D(k,2)+3),k));
                %s = s + ((max(w(:)) - min(w(:)))/mean(w(:)))^2;
                %s = s + P(bInd2D(k,1),bInd2D(k,2),k)^2;
                %s = s + (P(bInd2D(k,1),bInd2D(k,2),k) - min(w(:)))^2 / mean(w(:))^2;
                s = s + max((w(:) - single(P(bInd2D(k,1),bInd2D(k,2),k))).^2);
            end
        end
    end
    matchScore(n) = s / c;
end

bestMatch = find(matchScore == max(matchScore));

%% Output image check
imageCheck = P;
for b = 1:length(calData.Beacons)
    bTime = datenum(calData.Beacons{b}.Time) - datenum(stVec(bestMatch)) + tn(1);
    bTrj3D(:,1) = interp1(bTime,calData.Beacons{b}.LR,tn)';
    bTrj3D(:,2) = -1 * interp1(bTime,calData.Beacons{b}.SI,tn)';
    bTrj3D(:,3) = interp1(bTime,calData.Beacons{b}.AP,tn)';
    beaconTrj3D(:,(b-1)*3+1:b*3) = bTrj3D;
    bTrj2D = rtk3DTo2D(G,bTrj3D);
    beaconTrj2D(:,(b-1)*2+1:b*2) = bTrj2D;
    bInd2D = round(bTrj2D ./ (ones(size(bTrj2D,1),1) * projSpacing) + ...
        (ones(size(bTrj2D,1),1) * (size(P(:,:,1))+1) / 2));
    beaconInd2D(:,(b-1)*2+1:b*2) = bInd2D;
    for k = 1:size(bInd2D,1)
        if bInd2D(k,1) >= 1 && bInd2D(k,1) <= size(P,1) && bInd2D(k,2) >= 1 && bInd2D(k,2) <= size(P,2)
            imageCheck(max(1,bInd2D(k,1)-2):min(size(P,1),bInd2D(k,1)+2),max(1,bInd2D(k,2)-2):min(size(P,2),bInd2D(k,2)+2),k) = 2^32 - 1;
        end
    end
end

%% Output sync result
bTime = datenum(calData.Target.Time) - datenum(stVec(bestMatch)) + tn(1);
[bTime,ia] = unique(bTime);
calData.Target.InferredPositions = calData.Target.InferredPositions(ia,:);
syncResult.targetTrj3D(:,1) = interp1(bTime,calData.Target.InferredPositions(:,1),tn)';
syncResult.targetTrj3D(:,2) = -1 * interp1(bTime,calData.Target.InferredPositions(:,2),tn)';
syncResult.targetTrj3D(:,3) = interp1(bTime,calData.Target.InferredPositions(:,3),tn)';
syncResult.targetTrj2D = rtk3DTo2D(G,syncResult.targetTrj3D);
syncResult.targetInd2D = round(syncResult.targetTrj2D ./ (ones(size(syncResult.targetTrj2D,1),1) * projSpacing) + ...
    (ones(size(syncResult.targetTrj2D,1),1) * (size(P(:,:,1))+1) / 2));
syncResult.beaconTrj3D = beaconTrj3D;
syncResult.beaconTrj2D = beaconTrj2D;
syncResult.beaconInd2D = beaconInd2D;
syncResult.G = G;
