function RemoveHncMarkers(inputfl,markerPos,R,outDir)
%% RemoveHncMarkers(inputfl,markerPos,R,outDir)
% ------------------------------------------
% FILE   : RemoveHncMarkers.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2015-02-09   Created
% ------------------------------------------
% PURPOSE
%   Remove fiducial markers in HNC projections
% ------------------------------------------
% INPUT
%   inputfl:        The input file list (can be obtained using ls('*.hnc'))
%   markerPos:      A cell containing the marker 2D positions in the format
%                   of {marker1},{marker2},..., and that for each cell:
%                   [x1,y1;x2,y2;x3,y3;...]
%   R:              The radius of the 2D block within which marker is
%                   removed. Default is 20. (Optional)
%   outDir:         The output directory. If non-existent, the code will
%                   create the folder for the user. Default is
%                   "OriginalFolderName_MarkerRemoved" in current
%                   directory. (Optional)
% ------------------------------------------

%% 

if nargin < 2
    error('ERROR: inputfl and markerPos must be provided');
end

if ~iscell(markerPos)
    error('ERROR: markerPos must be a cell');
end

for k = 1:length(markerPos)
    markerPos{k} = round(markerPos{k});
end

if nargin < 3
    R = 20;
end

if nargin < 4
    [~,dirName] = fileparts(fileparts(inputfl(1,:)));
    dirName = [dirName,'_MarkerRemoved'];
    outDir = fullfile(pwd,dirName);
end

if ~exist(outDir,'dir'); mkdir(outDir); end;

for k = 1:size(inputfl,1)
    [info,P0] = HncRead(inputfl(k,:));
    P = P0;
    for n = 1:length(markerPos)
        if (markerPos{n}(k,1)==0 && markerPos{n}(k,2)==0) || ...
                sum(isnan(markerPos{n}(k,:))) > 0
            continue;
        end
        block = P0(max(markerPos{n}(k,1)-R,1):min(markerPos{n}(k,1)+R,size(P0,1)),max(1,markerPos{n}(k,2)-R):min(markerPos{n}(k,2)+R,size(P0,2)));
        if isempty(block); continue; end;
        p10block = prctile(block(:),10);
        ind = block < p10block;
        block_new = block;
        block_new(ind) = mean(block(:))*0.9 + 0.1*block(ind);
        block_new(ind) = imnoise(block_new(ind),'poisson');
        P(max(markerPos{n}(k,1)-R,1):min(markerPos{n}(k,1)+R,size(P0,1)),max(1,markerPos{n}(k,2)-R):min(markerPos{n}(k,2)+R,size(P0,2))) = block_new;
    end
    [~,filename,fileext] = fileparts(inputfl(k,:));
    HncWrite(info,P,fullfile(outDir,[filename,fileext]));
end

return;