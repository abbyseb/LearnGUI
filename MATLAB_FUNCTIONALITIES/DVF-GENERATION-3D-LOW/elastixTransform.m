function elastixTransform(transformFile,imgFile,verbose,outputFile,maxThreads)
%% elastixTransform(transformFile,imgFile,verbose,outputFile,maxThreads)
% ------------------------------------------
% FILE   : elastixTransform.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2016-06-22  Created.
% ------------------------------------------
% PURPOSE
%   Warp image using elastix transform file OR output the transformation
%   DVF.
% ------------------------------------------
% INPUT
%   transformFile:  The Elastix transform parameter file used to warp the
%                   image.
%   imgFile:        The image file to be warped. If this field is empty,
%                   the DVF will be written into outputFile.
%   verbose:        Set this to "true" to turn verbosity on. Default: false
%   outputFile:     The output file name.
%   maxThreads:     (Optional) The maximum number of threads.

%% Input check

if nargin < 4 || isempty(outputFile)
    if isempty(imgFile)
        [~,outputName] = fileparts(transformFile);
        outputFile = fullfile(pwd,['DVF_',outputName,'.mha']);
    else
        [~,imgName] = fileparts(imgFile);
        outputFile = fullfile(pwd,['Warped_',imgName,'.mha']);
    end
end;

if nargin < 3
    verbose = false;
end;

if nargin < 5
    maxThreads = [];
end

tmpDir = tempname;
mkdir(tmpDir);
%% Process parameter file
% See if we should use interpolation order 3 (for most images) or 0 (for
% binary images)
if ~isempty(imgFile)
    [~,M] = MhaRead(imgFile);
    interpOrder = 3;
    if sum(M(:)~=0 & M(:)~=1) == 0
        interpOrder = 0;
    end;
    clear M;
    
    tag_interpolation = '(FinalBSplineInterpolationOrder 3)';
    new_interpolation = ['(FinalBSplineInterpolationOrder ',num2str(interpOrder,'%d'),')'];
    
    % Read parameter file
    fid = fopen(transformFile,'r');
    k = 1;
    tline = fgetl(fid);
    if length(tline) >= length(tag_interpolation) ...
            && strcmpi(tline(1:length(tag_interpolation)),tag_interpolation)
        tline = new_interpolation;
    end
    paratext{k} = tline;
    while ischar(tline)
        k = k + 1;
        tline = fgetl(fid);
        if length(tline) >= length(tag_interpolation) ...
                && strcmpi(tline(1:length(tag_interpolation)),tag_interpolation)
            tline = new_interpolation;
        end
        paratext{k} = tline;
    end
    fclose(fid);
    
    % Write new file
    fid = fopen(fullfile(tmpDir,'TransformParameters.Mod.txt'),'w');
    for k = 1:numel(paratext)
        if paratext{k+1} == -1
            fprintf(fid,'%s',paratext{k});
            break;
        else
            fprintf(fid,'%s\n',paratext{k});
        end
    end
    fclose(fid);
end;

%% Warp img
transformix = which('transformix.exe');
env_PATH = getenv('PATH');
if isempty(transformix);
    transformix = 'transformix';
else
    setenv('PATH',[env_PATH,';',transformix]);
    transformix = ['"',transformix,'"'];
end;

cmd_input = '';
cmd_dvf = ' -def all';
if ~isempty(imgFile)
    cmd_input = [' -in "',imgFile,'"'];
    cmd_dvf = '';
    transformFile = fullfile(tmpDir,'TransformParameters.Mod.txt');
end

cmd_verbose = '';
if ~verbose
    cmd_verbose = ' >nul 2>&1';
end

if isempty(maxThreads)
    cmd_thread = '';
else
    cmd_thread = [' -threads ',num2str(maxThreads,'%d')];
end

fail = system([transformix,cmd_dvf,cmd_input,...
    ' -tp "',transformFile,'"',...
    ' -out "',tmpDir,'"',cmd_thread,cmd_verbose]);
setenv('PATH',env_PATH);
if fail; error(['ERROR: transformix failed while warping img file: ',imgFile]); end;

% Copy elastix output
if ~isempty(imgFile)
    if interpOrder == 0
        [info,maskImg] = MhaRead(fullfile(tmpDir,'result.mha'));
        info.DataType = 'uchar';
        maskImg(maskImg>0) = 1;
        MhaWrite(info,maskImg,outputFile);
        clear info; clear maskImg;
    else
        copyfile(fullfile(tmpDir,'result.mha'),outputFile);
    end
else
    copyfile(fullfile(tmpDir,'deformationField.mha'),outputFile);
end

%% Delete temporary files
if exist(tmpDir,'dir'); rmdir(tmpDir,'s');  end;

return