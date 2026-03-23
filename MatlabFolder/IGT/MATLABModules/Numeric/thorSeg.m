function [segImg,segTime,segStruct,refSegStruct] = thorSeg(M,imgspacing,varargin)
%% [segImg,segTime,segStruct,refSegStruct] = thorSeg(M,imgspacing,varargin)
% ------------------------------------------
% FILE   : thorSeg.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-12-09   Created.
%          2014-03-10   Re-structured to improve performance
% ------------------------------------------
% PURPOSE
%   Segment the input thoracic 3D image into soft tissue, bony anatomy,
%   lungs, pulmonary details, and exterior region. 
%   For delineation of the whole lung boundary, the entire lung region
%   corresponds to the united region of Lung and Pulmonary, i.e.
%   segStruct.Lung | segStruct.Pulmonary.
%   For delineation of the body region, the entire body region beside the
%   lung corresponds to segStruct.Soft | segStruct.Bone
%   NOTE: THE DIMENSION OF THE IMAGE NEEDS TO BE [CORONAL,SAGITTAL,AXIAL]!!
%
% ------------------------------------------
% INPUT
%   (can be input in a structure array or as name/parameter pair)
%   (e.g. thorSeg('InputImg',M) )
%   (default parameter is used if a field is not input)
%
% ================MANDATORY================
%  M
%   The input image stored in a MATLAB 3D matrix. The dimension needs to be
%   [coronal,sagittal,axial]
%
%  imgspacing
%   The pixel spacing of the input image in [coronal,sagittal,axial].
%   e.g. [0.88,0.88,2]
%
% ================OPTIONAL================
%  Verbosity
%   Whether intermediate messages are displayed (true) or not (false).
%   (default: false)
%
%  FOVMask
%   The FOV mask stored in a MATLAB 3D matrix with the same dimension of
%   the input image. A FOV mask can be obtained using "cbctfov" and "permute".
%   (default: true(size(M)))
%
%  I_Soft
%   Soft tissue are segmented as pixels with intensities higher than this
%   threshold.
%   (default: 0.009)
%
%  I_Lung
%   A slightly lower threshold value than I_Soft used to separate the major
%   lung lobes and other airways.
%   (default: 0.006)
%
%  I_Airway
%   Airways may sometimes have higher attenuation values than the lung
%   lobes due to over-smoothing, and are therefore segmented with a
%   slightly different attenuation value.
%   (default: 0.01)
%
%  I_Pulmonary
%   Any details inside the lung are segmented with intensitieshigher than
%   this thresold.
%   (default: 0.0075)
%
%  I_Bone
%   Bony anatomy is segmented as pixels with intensities higher than this
%   threshold.
%   (default: 0.016)
%
%  V_Bone
%   In the segmentation of bony anatomy, structures with connected volume
%   (in mm^3) smaller than V_Bone will be excluded. A lower V_Bone will
%   capture finer bony structures, but more prone to noise/streaking.
%   (default: 50 mm^3)
%
%  V_Lung
%   The threshold volume for segmenting the lung.
%   (default: 1e5 mm^3)
%
%  V_Airway
%   In the segmentation of airways, structures with connected volume
%   (in mm^3) smaller than V_Airway will be excluded. A lower V_Airway will
%   capture finer airways, but more prone to noise/streaking.
%   (default: 5 mm^3)
%
%  AvgFiltSize
%   The size of the average filter for segmentation of very noisy images.
%   The average filter is applied to the segmented mask after the first
%   preliminary segmentation of the soft tissue/lung regions.
%   THe size should be input in mm, e.g. [10,10,15] means 10mm in coronal, 
%   sagittal, and 15mm in axial dimensions.
%   (default: no average filter used)
%
%  MedFiltSize
%   The size of the median filter applied directly to the image itself 
%   before any segmentation. It should be input in the same format as the
%   AvgFiltSize ([coronal,sagittal,axial] in mm).
%   (default: no median filter used)
%
%  RefSegStruct
%   A reference segmentation struct, which could come from a segmentation
%   from an earlier iteration, or from the motion blurred image. The
%   RefSegStruct should have the same format as the output segStruct from
%   this function. The segmentation in RefSegStruct will always be smeared
%   along all the directions first so to include motion range.
%   Note that although RefSegStruct is also used to replace the reference
%   attenuation values in the final image, reference attenuation values
%   obtained from the current image are still stored in segStruct.
%   (default: no reference segmentation struct used)
%
%  STDImg
%   The user can input the standard motion blurred image to guide the
%   segmentation. If STDImg is input, than any specified RefSegStruct will
%   be ignored, and be replaced by the segStruct obtained from segmenting
%   STDImg. The RefMotionRange will be used to smear the STD segmentation
%   in order to account for motion difference.
%
%  RefMotionRange
%   The estimated motion range between the standard image and the current 
%   input image. RefMotionRange is only used when STDImg is input.
%   The input should be in mm, and as a vector representing 
%   [coronal,sagittal,axial].
%   (default: if STDImg is input, RefMotionRange = [2,2,5] mm)
%
% OUTPUT
%  
%  segImg
%   The segmented map, with each segmented region assigned its reference
%   attenuation values.
%
%  segTime
%   The computation time.
%
%  segStruct
%   The segmentation information struct, containing a logical map for each
%   segmented region, and the reference attenuation value for each of the
%   region. The list of regions include "Soft", "Bone", "Lung",
%   "Pulmonary", "Exterior". Note that a tumor might be included in the
%   "Soft" region, or the "Pulmonary" region.
%   The "Value" field of a particular region contains two entries, the
%   first being the mode of the attenuation values, the second being the
%   standard deviation.
%   The "Bin" field of a particular region contains all the different
%   intensity values corresponding to the pixel values in the original
%   image covered by the logical map.
%
%  refSegStruct
%   If STDImg is input, the function will store the reference segmentation
%   struct obtained from STDImg in refSegStruct for future use.


%% Initilization and input check

% Default settings
default = struct;
default.Verbosity = false;
default.FOVMask = true(size(M));
default.I_Soft = 0.01;
default.I_Lung = 0.006;
default.I_Airway = 0.01;
default.I_Pulmonary = 0.008;
default.I_Bone = 0.016;
default.V_Bone = 50;
default.V_Lung = 1e5;
default.V_Airway = 5;
default.AvgFiltSize = [];
default.MedFiltSize = [];
default.RefSegStruct = [];
default.STDImg = [];
default.RefMotionRange = [2,2,5];

% Segmentation list
segList = aatvSegList;

% Checking and parsing input
inStruct = inputcheck(default,M,imgspacing,varargin,segList);

% Initialize output
dim = size(M);
segImg = single(zeros(dim));
segTime = 0;
segStruct = struct;
for k = 1:length(segList)
    segStruct.(segList{k}) = false(dim);
    segStruct.([segList{k},'Value']) = [0,0];
end;

% Volume of a pixel in mm^3
dV = prod(imgspacing);

% Message to the user
if inStruct.Verbosity
    fprintf('==========Thoracic segmentation starts ...\n');
end;

%% If STDImg is input, use it to get the reference segmentation struct
if ~isempty(inStruct.STDImg)
    
    if inStruct.Verbosity
        fprintf('Acquiring reference segmentation from the input STD motion blurred image......');
    end;
    
    % Segmentation
    stdInStruct = rmfield(inStruct,{'STDImg','Verbosity'});
    [~,time,refSegStruct] = thorSeg(inStruct.STDImg,imgspacing,stdInStruct);
    
    tic;
    % Smear the mask to include motion range
%     refSegStruct.Soft = smearMask(refSegStruct.Soft,round(inStruct.RefMotionRange./imgspacing));
    refSegStruct.Bone = smearMask(refSegStruct.Bone,round(inStruct.RefMotionRange./imgspacing));
%     refSegStruct.Exterior = smearMask(refSegStruct.Exterior,round(inStruct.RefMotionRange./imgspacing));
    % Lung move more, so double the motion range
%     refSegStruct.Lung = smearMask(refSegStruct.Lung,round(2 * inStruct.RefMotionRange./imgspacing));
    % We generally can't get pulmonary details from STD image, so simply
    % make it identical to the whole lung region
%     refSegStruct.Pulmonary = refSegStruct.Pulmonary | refSegStruct.Lung;
    % Replace the attenuation values of pulmonary details by the one for
    % soft tissue, since we generally get very low attenuation values for
    % pulmonary attenuation values from STDImg due to motion blur
    refSegStruct.PulmonaryValue(1) = refSegStruct.SoftValue(1);
    
    inStruct.RefSegStruct = refSegStruct;
    
    time = time + toc;
    
    segTime = segTime + time;
    if inStruct.Verbosity
        fprintf('COMPLETED using %f seconds!!\n',time);
    end
else
    refSegStruct = inStruct.RefSegStruct;
end;

%% Pre-median filter
if ~isempty(inStruct.MedFiltSize)
    if inStruct.Verbosity
        fprintf('Applying pre-median-filter......');
    end; tic;
    
    medFiltSize = round(inStruct.MedFiltSize ./ imgspacing);
    medFiltSize(medFiltSize<1) = 1;
    M = medfilt3(M,medFiltSize);
    
    time = toc; segTime = segTime + time;
    if inStruct.Verbosity
        fprintf('COMPLETED using %f seconds!!\n',time);
    end
end

%% Basic body region intensity thresholding

soft_THRMAP = (M > inStruct.I_Soft);

% Average filter. This takes a short annoying while
if ~isempty(inStruct.AvgFiltSize)
    if inStruct.Verbosity
        fprintf('Applying average filter to soft tissue intensity-thresholded mask......');
    end; tic;
    
    avgFiltSize = round(inStruct.AvgFiltSize ./ imgspacing);
    avgFiltSize(avgFiltSize<1) = 1;
    soft_THRMAP = soft_THRMAP & imfilter(soft_THRMAP,fspecial3('average',avgFiltSize));
    
    time = toc; segTime = segTime + time;
    if inStruct.Verbosity
        fprintf('COMPLETED using %f seconds!!\n',time(end));
    end
end

%% Soft tissue
% Use connectivity to identify soft-region (including bony anatomy at this
% step)
if inStruct.Verbosity
    fprintf('Segmenting the soft tissue region......');
end
tic;

% Getting the largest connected region
segStruct.Soft = findConnected(soft_THRMAP,0);

% % Use RefSegStruct if input
% if ~isempty(inStruct.RefSegStruct)
%     segStruct.Soft = segStruct.Soft & ...
%         (inStruct.RefSegStruct.Soft | inStruct.RefSegStruct.Bone);
% end

segStruct.Soft = segStruct.Soft & inStruct.FOVMask;

time = toc; segTime = segTime + time;
if inStruct.Verbosity
    fprintf('COMPLETED using %f seconds!!\n',time);
end

%% Lung & exterior
% Regions outside the soft tissue region belong to Exterior. We do this slice by
% slice. And the rest would be the lung.
% 2013-11-15 Update: do this both horizontally and vertically, and find the
% mutual exterior region. This tends to be more reliable.
if inStruct.Verbosity
    fprintf('Segmenting the lung and exterior regions......');
end; tic;

% Finding exterior region by linearly scanning through each slice. See
% function "findExterior.m" for details
% 2013-12-14: Decided to do this in axial, then coronal view, and use the
% united region
segStruct.Exterior = findExterior(~segStruct.Soft);
% segStruct.Exterior = segStruct.Exterior & ...
%     permute(findExterior(permute(~segStruct.Soft,[1 3 2])),[1 3 2]);
% % Use RefSegStruct if input
% if ~isempty(inStruct.RefSegStruct)
%     segStruct.Exterior = segStruct.Exterior & inStruct.RefSegStruct.Exterior;
% end
segStruct.Exterior = segStruct.Exterior & inStruct.FOVMask;
%segStruct.Exterior = findConnected(segStruct.Exterior,0);

% The rest of the unassigned region is lung, but only the lobes and clear
% airways
segStruct.Lung = (~segStruct.Soft) & (~segStruct.Exterior) & inStruct.FOVMask;
% Connectivity
segStruct.Lung = findConnected(segStruct.Lung,round(inStruct.V_Lung/dV));

% % % Use RefSegStruct if input
% if ~isempty(inStruct.RefSegStruct)
%     segStruct.Lung = segStruct.Lung & ...
%         (inStruct.RefSegStruct.Lung | inStruct.RefSegStruct.Pulmonary);
% end
% 
% It is helpful to use the lung and exterior region to re-correct
% the soft tissue region.
segStruct.Soft = ~segStruct.Exterior & ~segStruct.Lung;
% 
% if ~isempty(inStruct.RefSegStruct)
%     segStruct.Soft = segStruct.Soft & ...
%         (inStruct.RefSegStruct.Soft | inStruct.RefSegStruct.Bone);
% end

segStruct.Soft = segStruct.Soft & inStruct.FOVMask;

time = toc; segTime = segTime + time;
if inStruct.Verbosity
    fprintf('COMPLETED using %f seconds!!\n',time);
end

%% Airways
% Identify airways that are missed in the lung segmentation.
% Airways are then included in the lung region.

airway_THRMAP = (M < inStruct.I_Airway) & findBetween(segStruct.Lung);
segStruct.Lung = segStruct.Lung | airway_THRMAP;

segStruct.Soft = segStruct.Soft & ~segStruct.Lung;

%% Pulmonary details (vessels, bronchus wall, tumor...etc)
% Intensity thresholding and connectivity
if inStruct.Verbosity
    fprintf('Segmenting pulmonary details......');
end; tic;

% Intensity thresholding in the lung region, but excluding the airway
% region, as some airway regions may have high attenuation values due to
% low contrast. For this, we want to segment the major lung lobe region
% using I_Lung, and then identify the mediastinum.

lobe_THRMAP = M < inStruct.I_Lung;
lobe_THRMAP = ~findExterior(lobe_THRMAP) & lobe_THRMAP;
lobe_THRMAP = findConnected(lobe_THRMAP,round(inStruct.V_Lung/dV));
lobe_THRMAP = lobe_THRMAP & segStruct.Lung;

segStruct.Pulmonary = (M >= inStruct.I_Pulmonary) & (segStruct.Lung & ~findBetween(lobe_THRMAP));

% % Use RefSegStruct if input
% if ~isempty(inStruct.RefSegStruct)
%     segStruct.Pulmonary = segStruct.Pulmonary & inStruct.RefSegStruct.Pulmonary;
% end

segStruct.Pulmonary = segStruct.Pulmonary & inStruct.FOVMask;
%Exclude pulmonary details from lung region
segStruct.Lung = segStruct.Lung & ~segStruct.Pulmonary;

time = toc; segTime = segTime + time;
if inStruct.Verbosity
    fprintf('COMPLETED using %f seconds!!\n',time);
end

%% Bony anatomy
% Use intensity threshold + connectivity to get bony anatomy
if inStruct.Verbosity
    fprintf('Segmenting the bony anatomy......');
end; tic;

% Intensity thresholding
segStruct.Bone = (M >= inStruct.I_Bone) & segStruct.Soft;

% Connectivity
segStruct.Bone = findConnected(segStruct.Bone,round(inStruct.V_Bone/dV));

% Use RefSegStruct if input
if ~isempty(inStruct.RefSegStruct)
    segStruct.Bone = segStruct.Bone & inStruct.RefSegStruct.Bone;
end
segStruct.Bone = segStruct.Bone & inStruct.FOVMask;
% Exclude bony anatomy in the soft tissue region
segStruct.Soft = segStruct.Soft & ~segStruct.Bone;

time = toc; segTime = segTime + time;
if inStruct.Verbosity
    fprintf('COMPLETED using %f seconds!!\n',time);
end

%% Calculate reference attenuation values for each region, and construct the segmented image
if inStruct.Verbosity
    fprintf('Calculating reference attenuation values, and constructing segmented image......');
end; tic;

% Calculate reference attenuation values. We do this separately since each
% region could be treated slightly different

% Soft
tmp_I = M(segStruct.Soft);
segStruct.SoftValue(1) = mean(tmp_I(tmp_I>0));
segStruct.SoftValue(2) = std(tmp_I(tmp_I>0));
segStruct.SoftBin = tmp_I;

% Lung
tmp_I = M(segStruct.Lung);
segStruct.LungValue(1) = mean(tmp_I);
segStruct.LungValue(2) = std(tmp_I);
segStruct.LungBin = tmp_I;

% Bony anatomy
tmp_I = M(segStruct.Bone);
segStruct.BoneValue(1) = mean(tmp_I(tmp_I>0));
segStruct.BoneValue(2) = std(tmp_I(tmp_I>0));
segStruct.BoneBin = tmp_I;

% Pulmonary details
tmp_I = M(segStruct.Pulmonary);
segStruct.PulmonaryValue(1) = mean(tmp_I(tmp_I>0));
segStruct.PulmonaryValue(2) = std(tmp_I(tmp_I>0));
segStruct.PulmonaryBin = tmp_I;

% Exterior
tmp_I = M(segStruct.Exterior);
segStruct.ExteriorValue(1) = mode(tmp_I);
segStruct.ExteriorValue(2) = std(tmp_I);
segStruct.ExteriorBin = tmp_I;

% It is possile for no pulmonary details to be segmented in a motion
% blurred image. In that case, use soft tissue attenuation value to
% approximate it, and take the standard deviation as half the difference 
% between lung value and soft tissue value.
if any(isnan(segStruct.PulmonaryValue))
    segStruct.PulmonaryValue(1) = segStruct.SoftValue(1);
    segStruct.PulmonaryValue(2) = 0.5 * ...
        (segStruct.SoftValue(1) - segStruct.LungValue(1));
end;

% Construct the segmented image using attenuation values from current
% image, or from RefSegStruct
if ~isempty(inStruct.RefSegStruct)
    for k = 1:length(segList)
        segImg(segStruct.(segList{k})) = ...
            inStruct.RefSegStruct.([segList{k},'Value'])(1);
    end;
else
    for k = 1:length(segList)
        segImg(segStruct.(segList{k})) = ...
            segStruct.([segList{k},'Value'])(1);
    end;
end

time = toc; segTime = segTime + time;
if inStruct.Verbosity
    fprintf('COMPLETED using %f seconds!!\n',time);
end

if inStruct.Verbosity
    fprintf('==========THORACIC SEGMENTATION FINISHED USING %f seconds!!\n',segTime);
end

return;

function inStruct = inputcheck(default,M,imgspacing,userIn,segList)

% Checking mandatory fields
if ~isnumeric(M) || isscalar(M)
    error('ERROR: M must be the 3D input image matrix. Please try again.');
end;

if ~isnumeric(imgspacing) || ~isvector(imgspacing) || length(imgspacing)~=3 || ~all(imgspacing>0) 
    error('ERROR: The imgspacing field is not input in the correct format. Please try again.');
end;

% Input parser for optional fields
p = inputParser;    p.FunctionName = 'thorSeg';

p.addParamValue('Verbosity',default.Verbosity,@(x) isscalar(x) && islogical(x));
p.addParamValue('FOVMask',default.FOVMask,@(x) islogical(x) && isequal(size(x),size(M)));
p.addParamValue('I_Soft',default.I_Soft,@(x) isnumeric(x) && isscalar(x) && x>=0);
p.addParamValue('I_Lung',default.I_Lung,@(x) isnumeric(x) && isscalar(x) && x>=0);
p.addParamValue('I_Airway',default.I_Airway,@(x) isnumeric(x) && isscalar(x) && x>=0);
p.addParamValue('I_Pulmonary',default.I_Pulmonary,@(x) isnumeric(x) && isscalar(x) && x>=0);
p.addParamValue('I_Bone',default.I_Bone,@(x) isnumeric(x) && isscalar(x) && x>=0);
p.addParamValue('V_Bone',default.V_Bone,@(x) isnumeric(x) && isscalar(x) && x>=0);
p.addParamValue('V_Lung',default.V_Lung,@(x) isnumeric(x) && isscalar(x) && x>=0);
p.addParamValue('V_Airway',default.V_Airway,@(x) isnumeric(x) && isscalar(x) && x>=0);
p.addParamValue('AvgFiltSize',default.AvgFiltSize,@(x) (isnumeric(x) && isvector(x) && length(x)==3 && all(x>0)) || isempty(x));
p.addParamValue('MedFiltSize',default.MedFiltSize,@(x) (isnumeric(x) && isvector(x) && length(x)==3 && all(x>0)) || isempty(x));
p.addParamValue('RefSegStruct',default.RefSegStruct,@(x) isstruct(x) || isempty(x));
p.addParamValue('STDImg',default.STDImg,@(x) (isnumeric(x) && isequal(size(x),size(M))) || isempty(x));
p.addParamValue('RefMotionRange',default.RefMotionRange,@(x) isnumeric(x) && isvector(x) && length(x)==3 && all(x>=0));

p.parse(userIn{:});   inStruct = p.Results;

% Check RefSegStruct validity
if ~isempty(inStruct.RefSegStruct)
    for k = 1:length(segList)
        if ~isfield(inStruct.RefSegStruct,segList{k}) || ...
                ~isequal(size(inStruct.RefSegStruct.(segList{k})),size(M)) || ...
                ~islogical(inStruct.RefSegStruct.(segList{k}))
            error('ERROR: Incorrect RefSegStruct format. Please refer to the output format of "thorSeg" and try again.');
        end
        if ~isfield(inStruct.RefSegStruct,[segList{k},'Value']) || ...
                length(inStruct.RefSegStruct.([segList{k},'Value'])) ~= 2 || ...
                ~isnumeric(inStruct.RefSegStruct.([segList{k},'Value']))
            error('ERROR: Incorrect RefSegStruct format. Please refer to the output format of "thorSeg" and try again.');
        end
        if ~isfield(inStruct.RefSegStruct,[segList{k},'Bin']) || ...
                ~isnumeric(inStruct.RefSegStruct.([segList{k},'Bin'])) || ...
                ~isvector(inStruct.RefSegStruct.([segList{k},'Bin']))
            error('ERROR: Incorrect RefSegStruct format. Please refer to the output format of "thorSeg" and try again.');
        end
    end
end

return;    