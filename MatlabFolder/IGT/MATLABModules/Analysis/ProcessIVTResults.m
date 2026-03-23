function outStruct = ProcessIVTResults(inStruct,G)
%% outStruct = ProcessIVTResults(inStruct,G)
% ------------------------------------------
% FILE   : ProcessIVTResults.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-07-12   Creation
% ------------------------------------------
% PURPOSE
%   Process the tracking results from In-vivo Tracking and summarize it
%   into an easier-to-analyze format.
%
% ------------------------------------------
% INPUT
%   inStruct:        The input struct read from the In-vivo Tracking result
%                    .csv file.
%   G:               The RTK geometry matrices for that tracking session.
%                    If left unspecified, the program will prompt the user
%                    for the RTK geometry file.
% ------------------------------------------
% OUTPUT
%   outStruct:       The processed output struct.

%% Input Check
if nargin < 2
    G = ReadRTKGeometryMatrices;
end

%%
outStruct = struct;

outStruct.Frame = inStruct.FrameNumber(:);
outStruct.Bin = inStruct.BinNumber(:);

outStruct.IVTTrj = [inStruct.TrackingLR(:),inStruct.TrackingSI(:),inStruct.TrackingAP(:)];
outStruct.PriorTrj = [inStruct.PriorLR(:),inStruct.PriorSI(:),inStruct.PriorAP(:)];
outStruct.GTTrj = [inStruct.GroundTruthLR(:),inStruct.GroundTruthSI(:),inStruct.GroundTruthAP(:)];

% Find mean 3D prior position
outStruct.Prior3DPos = [0, 0, 0];
for b = min(outStruct.Bin):max(outStruct.Bin)
    ind = find(outStruct.Bin == b); ind = ind(1);
    outStruct.Prior3DPos = outStruct.Prior3DPos + outStruct.PriorTrj(ind,:);
end
outStruct.Prior3DPos = outStruct.Prior3DPos / (max(outStruct.Bin) - min(outStruct.Bin) + 1);

if isempty(G)
    outStruct.IVTProjTrj = [inStruct.TrackingLateral(:),inStruct.TrackingSI(:),inStruct.TrackingIn_depth(:)];
    outStruct.PriorProjTrj = [inStruct.PriorLateral(:),inStruct.PriorSI(:),inStruct.PriorIn_depth(:)];
    outStruct.GTProjTrj = [inStruct.GroundTruthLateral(:),inStruct.GroundTruthSI(:),inStruct.GroundTruthIn_depth(:)];
else
    [outStruct.IVTProjTrj,rayDist] = rtk3DTo2D(G(:,:,outStruct.Frame),outStruct.IVTTrj);
    outStruct.IVTProjTrj = [outStruct.IVTProjTrj,rayDist];
    [outStruct.PriorProjTrj,rayDist] = rtk3DTo2D(G(:,:,outStruct.Frame),outStruct.PriorTrj);
    outStruct.PriorProjTrj = [outStruct.PriorProjTrj,rayDist];
    [outStruct.Prior3DProjTrj,rayDist] = rtk3DTo2D(G(:,:,outStruct.Frame),...
        ones(length(outStruct.Frame),1) * outStruct.Prior3DPos);
    outStruct.Prior3DProjTrj = [outStruct.Prior3DProjTrj,rayDist];
    [outStruct.GTProjTrj,rayDist] = rtk3DTo2D(G(:,:,outStruct.Frame),outStruct.GTTrj);
    outStruct.GTProjTrj = [outStruct.GTProjTrj,rayDist];
end

outStruct.IVTErrors = outStruct.IVTTrj - outStruct.GTTrj;
outStruct.PriorErrors = outStruct.PriorTrj - outStruct.GTTrj;
outStruct.Prior3DErrors = ones(size(outStruct.GTTrj,1),1) * outStruct.Prior3DPos - outStruct.GTTrj;

for k = 1:size(outStruct.GTTrj,1)
    outStruct.IVTErrorNorms(k,1) = norm(outStruct.IVTErrors(k,:));
    outStruct.PriorErrorNorms(k,1) = norm(outStruct.PriorErrors(k,:));
    outStruct.Prior3DErrorNorms(k,1) = norm(outStruct.Prior3DErrors(k,:));
end

for k = 1:length(inStruct.StateCovariance11)
    outStruct.StateCovariances(:,:,k) = ...
        [inStruct.StateCovariance11(k),inStruct.StateCovariance12(k),inStruct.StateCovariance13(k);...
        inStruct.StateCovariance12(k),inStruct.StateCovariance22(k),inStruct.StateCovariance23(k);...
        inStruct.StateCovariance13(k),inStruct.StateCovariance23(k),inStruct.StateCovariance33(k)];
end

if isfield(inStruct,'TemplateMatchingMetric')
    outStruct.TMMetric = inStruct.TemplateMatchingMetric(:);
end

if isfield(inStruct,'BackgroundMatchingScore')
    outStruct.BGMatchScore = inStruct.BackgroundMatchingScore(:);
end

if isfield(inStruct,'BGShiftU') && isfield(inStruct,'BGShiftV')
    outStruct.BGShift = [inStruct.BGShiftU(:),inStruct.BGShiftV(:)];
end

% kV plane error and MV plane error
for k = 1:size(outStruct.GTTrj,1)
    outStruct.IVTKVErrorNorms(k,1) = norm(...
        outStruct.IVTProjTrj(k,1:2) - outStruct.GTProjTrj(k,1:2));
    outStruct.PriorKVErrorNorms(k,1) = norm(...
        outStruct.PriorProjTrj(k,1:2) - outStruct.GTProjTrj(k,1:2));
    outStruct.Prior3DKVErrorNorms(k,1) = norm(...
        outStruct.Prior3DProjTrj(k,1:2) - outStruct.GTProjTrj(k,1:2));
    outStruct.IVTMVErrorNorms(k,1) = norm([outStruct.IVTTrj(k,2),outStruct.IVTProjTrj(k,3)] ...
        - [outStruct.GTTrj(k,2),outStruct.GTProjTrj(k,3)]);
    outStruct.PriorMVErrorNorms(k,1) = norm([outStruct.PriorTrj(k,2),outStruct.PriorProjTrj(k,3)] ...
        - [outStruct.GTTrj(k,2),outStruct.GTProjTrj(k,3)]);
    outStruct.Prior3DMVErrorNorms(k,1) = norm([outStruct.Prior3DPos(2),outStruct.Prior3DProjTrj(k,3)] ...
        - [outStruct.GTTrj(k,2),outStruct.GTProjTrj(k,3)]);
end

return;