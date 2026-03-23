function CT2CBCT(cbctFile,ctList,outputFile,maskFiles,isMaskBinary)
%% CT2CBCT(cbctFile,ctList,outputFile,maskFiles,isMaskBinary)
% ------------------------------------------
% FILE   : CT2CBCT.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-11-04  Created.
% ------------------------------------------
% PURPOSE
%   Convert a series of dicom slices to a MHA volume with IEC geometry
%   standard, and then rigidly align it with the CBCT image, together with
%   proper intensity re-scaling.
% ------------------------------------------
% INPUT
%   cbctFile:   The path of the CBCT image to be aligned with. (default: ui selection)
%   ctList:     The list of dicom CT files. (default: ui for multiselect)
%               Can also be a single mha file.
%   outputFile: The path of the output file. (default: CT.mha at current dircetory)
%   maskFiles:  A list of other files (.mha) the user wishes to apply the
%               same geometric transformation that has been done on the CT.
%               (default: none)
%   isMaskBinary: Whether the mask files are to be treated as binary values
%                 (true) or float values (false). Default: true
%% Input check
    
if nargin < 1 || isempty(cbctFile)
    [cbctFile,ind] = uigetfilepath({'*.mha;*.MHA;','MHA files (*.mha,*.MHA)';},...
        'Select a CBCT volume');
    if ind==0
        error('ERROR: no CBCT mha file was selected. Try again.');
    end
end

if nargin < 2 || isempty(ctList)
    [listCell,ind] = uigetfilepath(...
        {'*.dcm;*.DCM;','DICOM files (*.dcm,*.DCM)';...
         '*.mha;*.MHA;','Metaimage file (*.mha,*.MHA)';},...
        'Select a series of CT DICOM files or a single CT MHA file','Multiselect','on');
    if ind==0
        error('ERROR: no DICOM file or mha file was selected. Try again.');
    end
    if iscell(listCell)
        for k = 1:length(listCell)
            ctList(k,1:length(listCell{k})) = listCell{k};
        end
    else
        ctList = listCell;
    end
end

if nargin < 3 || isempty(outputFile)
    outputFile = fullfile(pwd,'CT.mha');
end;
outputDir = fileparts(outputFile);

if nargin < 4
    maskFiles = '';
end;

if nargin < 5
    isMaskBinary = true;
elseif ~islogical(isMaskBinary)
    error('ERROR: isMaskBinary must be a logical tag');
end;

%% Convert from DICOM to IEC geometry standard

if size(ctList,1) > 1
    % Read in the CT
    ctInfo = dicominfo(ctList(1,:));
    for k = 1:size(ctList,1)
        M_CT(:,:,k) = dicomread(ctList(k,:));
    end;
else
    [ctInfo,M_CT] = MhaRead(ctList);
end

if size(ctList,1) > 1
    % Change to coronal,axial,sagittal
    M_CT = permute(M_CT,[2 3 1]);
    % Sagittal dimension needs to be reversed
    M_CT = M_CT(:,end:-1:1,end:-1:1);
else
    % Change to coronal,axial,sagittal
    M_CT = permute(M_CT,[1 3 2]);
    % Sagittal dimension needs to be reversed
    M_CT = M_CT(:,end:-1:1,end:-1:1);
end
% Convert to single precision
M_CT = single(M_CT);

% Read in CBCT
[cbctInfo,M_CBCT] = MhaRead(cbctFile);

% Change header information
if size(ctList,1) > 1
    cbctInfo.Dimensions = double([ctInfo.Width,size(ctList,1),ctInfo.Height]);
    cbctInfo.PixelDimensions = [ctInfo.PixelSpacing(1),ctInfo.SliceThickness,ctInfo.PixelSpacing(2)];
else
    cbctInfo.Dimensions = [ctInfo.Dimensions(1),ctInfo.Dimensions(3),ctInfo.Dimensions(2)];
    cbctInfo.PixelDimensions = [ctInfo.PixelDimensions(1),ctInfo.PixelDimensions(3),ctInfo.PixelDimensions(2)];
end
cbctInfo.Offset = -(cbctInfo.Dimensions-1)/2 .* cbctInfo.PixelDimensions;
    

%% Intensity rescaling
scale_handle = valueEqualize(M_CT(round(end/4):round(3*end/4),round(end/4):round(3*end/4),round(end/4):round(3*end/4)),...
                          M_CBCT(round(end/4):round(3*end/4),round(end/4):round(3*end/4),round(end/4):round(3*end/4)),...
                          [10,90]);
M_CT = scale_handle(M_CT);
% M_CT(M_CT<0) = 0;

%% Write the image

MhaWrite(cbctInfo,M_CT,outputFile);

%% Rigid registration via Elastix

% Temporary directory for registration output
tmpDir = tempname;
mkdir(tmpDir);

% Elastix
fail = system(['elastix -f "',cbctFile,'" -m "',outputFile,'" -p "',which('Elastix_Rigid.txt'),'" -out "',tmpDir,'" >nul 2>&1']);
if fail; error('ERROR: Elastix failed for unknown reason.'); end;

% Copy elastix output
copyfile(fullfile(tmpDir,'result.0.mha'),outputFile);

% Enforce positivty
%[info,M_CT] = MhaRead(outputFile);
% M_CT(M_CT<0) = 0;
%MhaWrite(info,M_CT,outputFile);

%% Process other files
% Some tags to be modified in the transformation file
tag_interpolation = '(FinalBSplineInterpolationOrder 3)';
tag_pixeltype = '(ResultImagePixelType "float")';
if isMaskBinary
    new_interpolation = '(FinalBSplineInterpolationOrder 0)';
else
    new_interpolation = '(FinalBSplineInterpolationOrder 3)';
end
new_pixeltype = '(ResultImagePixelType "float")';

% Read parameter file
fid = fopen(fullfile(tmpDir,'TransformParameters.0.txt'));
k = 1;
tline = fgetl(fid);
if length(tline) >= length(tag_interpolation) ...
        && strcmpi(tline(1:length(tag_interpolation)),tag_interpolation)
    tline = new_interpolation;
end
if length(tline) >= length(tag_pixeltype) ...
        && strcmpi(tline(1:length(tag_pixeltype)),tag_pixeltype)
    tline = new_pixeltype;
end
paratext{k} = tline;
while ischar(tline)
    k = k + 1;
    tline = fgetl(fid);
    if length(tline) >= length(tag_interpolation) ...
            && strcmpi(tline(1:length(tag_interpolation)),tag_interpolation)
        tline = new_interpolation;
    end
    if length(tline) >= length(tag_pixeltype) ...
            && strcmpi(tline(1:length(tag_pixeltype)),tag_pixeltype)
        tline = new_pixeltype;
    end
    paratext{k} = tline;
end
fclose(fid);

% Write new file
fid = fopen(fullfile(tmpDir,'TransformParameters.mask.txt'),'w');
for k = 1:numel(paratext)
    if paratext{k+1} == -1
        fprintf(fid,'%s',paratext{k});
        break;
    else
        fprintf(fid,'%s\n',paratext{k});
    end
end
fclose(fid);

% If there are other files to be geometrically transformed, read them and
% process them here
for k = 1: size(maskFiles,1)
    [~,otherNames{k}] = fileparts(maskFiles(k,:));
    [otherInfo,otherM] = MhaRead(maskFiles(k,:));
    
    % Change to coronal,axial,sagittal
    otherM = permute(otherM,[1 3 2]);
    % Sagittal dimension needs to be reversed
    otherM = otherM(:,end:-1:1,end:-1:1);
    
    % Change header information
    otherInfo.Dimensions = [otherInfo.Dimensions(1),otherInfo.Dimensions(3),otherInfo.Dimensions(2)];
    otherInfo.PixelDimensions = [otherInfo.PixelDimensions(1),otherInfo.PixelDimensions(3),otherInfo.PixelDimensions(2)];
    otherInfo.Offset = -(otherInfo.Dimensions-1)/2 .* otherInfo.PixelDimensions;
    MhaWrite(otherInfo,otherM,fullfile(outputDir,['CT_',otherNames{k},'.mha']));
    fail = system(['transformix -in "',fullfile(outputDir,['CT_',otherNames{k},'.mha']),'" -tp "',fullfile(tmpDir,'TransformParameters.mask.txt'),'" -out "',tmpDir,'" >nul 2>&1']);
    if fail; error('ERROR: transformix failed for unknown reason.'); end;
   
   % Copy elastix output
   copyfile(fullfile(tmpDir,'result.mha'),fullfile(outputDir,['CT_',otherNames{k},'.mha']));
end

%% Delete temporary files
if exist(tmpDir,'dir'); rmdir(tmpDir,'s');  end;

return