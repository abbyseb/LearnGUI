function rts_out = rtStruct2Mask(rts,rois,substr)
%% rts = rtStruct2Mask(rts,rois,substr)
% ------------------------------------------
% FILE   : rtStruct2Mask.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-09-09  Created.
% ------------------------------------------
% PURPOSE
%   Generate binary mask for RT structures.
% ------------------------------------------
% INPUT
%   rts :  RT struct read in by "ReadRTStruct.m"
%   rois : Specifying which ROIs the masks are to be generated for.
%          It can be specified as:
%          1. an array of indices, e.g. [1 2 5 10]
%          2. cell of ROI names, e.g. {'GTV','ITV'}
%          If left blank, masks are generated for all ROIs
%   substr: True if the user wants to find ROI name that has part matching
%           with the input "rois". False if the user wants to find ROI name
%           exactly matching "rois". Default: false.
% ------------------------------------------
% OUTPUT
%   rts_out: The new RT struct with mask for the ROI specified.
% ------------------------------------------

%% Some convenient terms
dimmin = [0 0 0 1]';
dimmax = double([rts.ImageHeaders{1}.Columns-1 rts.ImageHeaders{1}.Rows-1 length(rts.ImageHeaders)-1 1])';
template = false([rts.ImageHeaders{1}.Columns rts.ImageHeaders{1}.Rows length(rts.ImageHeaders)]);

rts_out = rts;

if nargin < 3
    substr = false;
end

if substr
    strCmpHandle = @(x)~isempty(strfind(x,rois));
else
    strCmpHandle = @(x)strcmpi(x,rois);
end
%% Loop through ROIs
if nargin < 2 || isempty(rois)
    rois = 1:length(rts.ROIs);
elseif iscell(rois)
    roiIndices = zeros(length(rois),1);
    for k = 1:length(rois)
        for kr = 1:length(rts.ROIs)
            if strcmpi(rts.ROIs(kr).ROIName, rois{k})
                break;
            end
        end
        roiIndices(k) = kr;
    end
    rois = sort(unique(roiIndices));
elseif ischar(rois)
    for kr = 1:length(rts.ROIs)
        if strCmpHandle(rts.ROIs(kr).ROIName)
            break;
        elseif kr == length(rts.ROIs)
            error(['ERROR: Cannot find ROI: ',rois]);
        end
    end
    rois = kr;
end

for k = 1:length(rois)
  rts_out.ROIs(rois(k)).BinaryMask = template;
  rts_out.ROIs(rois(k)).VoxPoints = [];
  % Loop through contours
  for kc = 1:length(rts.ROIs(rois(k)).Contours)
      if ~strcmpi('CLOSED_PLANAR',rts.ROIs(rois(k)).Contours(kc).ContourGeometricType)
          continue;
      end
      
      % Make lattice
      points = rts.AffineTransform \...
          [rts.ROIs(rois(k)).Contours(kc).Points,...
          ones(size(rts.ROIs(rois(k)).Contours(kc).Points,1), 1)]';
      start = rts.AffineTransform \ [rts.ROIs(rois(k)).Contours(kc).Points(1,:) 1]';
      minvox = max(floor(min(points, [], 2)), dimmin);
      maxvox = min(ceil(max(points, [], 2)), dimmax);
      minvox(3) = round(start(3));
      maxvox(3) = round(start(3));
      [x,y,z] = meshgrid(minvox(1):maxvox(1), minvox(2):maxvox(2), minvox(3):maxvox(3));
      points = rts.AffineTransform * [x(:) y(:) z(:) ones(size(x(:)))]';
        
      % Make binary image
      inside = inpolygon(points(1,:), points(2,:),...
          rts.ROIs(rois(k)).Contours(kc).Points(:,1), ...
          rts.ROIs(rois(k)).Contours(kc).Points(:,2));
       rts_out.ROIs(rois(k)).BinaryMask(...
           (minvox(1):maxvox(1))+1,...
           (minvox(2):maxvox(2))+1,...
           (minvox(3):maxvox(3))+1) = ...
           permute(reshape(inside, size(x)), [2 1]);
       rts_out.ROIs(rois(k)).VoxPoints(end+1:end+size(points',1),:) = points';
  end
end

end