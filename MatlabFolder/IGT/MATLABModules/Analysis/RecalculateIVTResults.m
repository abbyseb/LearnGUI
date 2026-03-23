function out = RecalculateIVTResults(in,geoFile)
%% out = RecalculateIVTResults(in,geoFile)
% ------------------------------------------
% FILE   : RecalculateIVTResults.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-08-22   Creation
% ------------------------------------------
% PURPOSE
%   Recalculate In-vivo tracking results using the new modified input
%   resullt struct.
%
% ------------------------------------------
% INPUT
%   in:      The input result struct that has been processed by
%            ProcessIVTResults.
%   geoFile: The RTK geometry file for the tracking fraction.
% ------------------------------------------
% OUTPUT
%   out:     The recalculated output result struct.
%
%% Input check

if nargin < 2
    G = ReadRTKGeometryMatrices;
else
    G = ReadRTKGeometryMatrices(geoFile);
end

%%
out = in;

[out.GTProjTrj,rayDist] = rtk3DTo2D(G(:,:,in.Frame),in.GTTrj);
out.GTProjTrj = [out.GTProjTrj,rayDist];
[out.IVTProjTrj,rayDist] = rtk3DTo2D(G(:,:,in.Frame),in.IVTTrj);
out.IVTProjTrj = [out.IVTProjTrj,rayDist];
[out.PriorProjTrj,rayDist] = rtk3DTo2D(G(:,:,in.Frame),in.PriorTrj);
out.PriorProjTrj = [out.PriorProjTrj,rayDist];
[out.Prior3DProjTrj,rayDist] = rtk3DTo2D(G(:,:,in.Frame),ones(length(in.Frame),1) * in.Prior3DPos);
out.Prior3DProjTrj = [out.Prior3DProjTrj,rayDist];

out.IVTErrors = in.IVTTrj - in.GTTrj;
out.PriorErrors = in.PriorTrj - in.GTTrj;
out.Prior3DErrors = ones(length(in.Frame),1) * in.Prior3DPos - in.GTTrj;
for k = 1:length(in.Frame);
    out.IVTErrorNorms(k,1) = norm(out.IVTErrors(k,:));
    out.PriorErrorNorms(k,1) = norm(out.PriorErrors(k,:));
    out.Prior3DErrorNorms(k,1) = norm(out.Prior3DErrors(k,:));
    out.IVTKVErrorNorms(k,1) = norm(out.IVTProjTrj(k,1:2) - out.GTProjTrj(k,1:2));
    out.IVTMVErrorNorms(k,1) = norm([out.IVTTrj(k,2),out.IVTProjTrj(k,3)] ...
        - [out.GTTrj(k,2),out.GTProjTrj(k,3)]);
    out.PriorKVErrorNorms(k,1) = norm(out.PriorProjTrj(k,1:2) - out.GTProjTrj(k,1:2));
    out.PriorMVErrorNorms(k,1) = norm([out.PriorTrj(k,2),out.PriorProjTrj(k,3)] ...
        - [out.GTTrj(k,2),out.GTProjTrj(k,3)]);
    out.Prior3DKVErrorNorms(k,1) = norm(out.Prior3DProjTrj(k,1:2) - out.GTProjTrj(k,1:2));
    out.Prior3DMVErrorNorms(k,1) = norm([out.Prior3DPos(2),out.Prior3DProjTrj(k,3)] ...
        - [out.GTTrj(k,2),out.GTProjTrj(k,3)]);
end