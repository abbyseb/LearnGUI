function elastixRegistration(movImgFile,fixImgFile,outputDir,paraFile,movImgMask,fixImgMask,initialTransform,verbose,extCmd,maxThreads)
%% elastixRegistration(movImgFile,fixImgFile,outputDir,paraFile,movImgMask,fixImgMask,initialTransform,verbose,extCmd,maxThreads)
% ------------------------------------------
% FILE   : elastixRegistration.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-06-22  Created.
% ------------------------------------------
% PURPOSE
%   Perform elastix rigid or deformable registration and output the
%   registered image + transform parameter file.
% ------------------------------------------
% INPUT
%   movImgFile:     The moving image file.
%   fixImgFile:     The fix image file.
%   outputDir:      The output directory. The image and transform file will
%                   be saved as "movImg_Rigid(BSpline)_fixImg.mha" and 
%                   "movImg_Rigid(BSpline)_fixImg.txt".
%                   Default: current directory
%   paraFile:       Parameter file.
%                   Default: default Rigid registration.
%   movImgMask:     Mask for the moving image.
%                   Default: no mask.
%   fixImgMask:     Mask for the fixed image.
%                   Default: no mask.
%   initialTransform:   The txt file of an initial transformation.
%                       Default: no initial transformation.
%   verbose:        Whether to show elastix messages.
%                   Default: false.
%   extCmd:         Any extra commands that are to be added.
%   maxThreads:     (Optional) The maximum number of threads.

%% Input check

if nargin < 3
    outputDir = '';
end

if nargin < 4
    paraFile = which('Elastix_Rigid.txt');
end

if nargin < 5;
    movImgMask = '';
end;

if nargin < 6
    fixImgMask = '';
end

if nargin < 7
    initialTransform = '';
end

if nargin < 8
    verbose = false;
end

if nargin < 9
    extCmd = '';
end

if nargin < 10
    maxThreads = [];
end

%% Registration via Elastix

% Temporary directory for registration output
tmpDir = tempname;
mkdir(tmpDir);

% Elastix
elastix = which('elastix.exe');
env_PATH = getenv('PATH');
if isempty(elastix);
    elastix = 'elastix';
else
    setenv('PATH',[env_PATH,';',elastix]);
    elastix = ['"',elastix,'"'];
end;

cmd_movImgMask = '';
if ~isempty(movImgMask);
    cmd_movImgMask = ['-mMask "',movImgMask,'"'];
end

cmd_fixImgMask = '';
if ~isempty(fixImgMask);
    cmd_fixImgMask = ['-fMask "',fixImgMask,'"'];
end

cmd_initialTransform = '';
if ~isempty(initialTransform)
    cmd_initialTransform = ['-t0 "',initialTransform,'"'];
end

cmd_verbose = '>nul 2>&1';
if verbose; cmd_verbose = '';   end;

if isempty(maxThreads)
    cmd_thread = '';
else
    cmd_thread = [' -threads ',num2str(maxThreads,'%d')];
end

[fail,msg] = system([elastix,' -f "',fixImgFile,'" -m "',movImgFile,'" -p "',paraFile,'" -out "',tmpDir,'" ',...
    cmd_movImgMask,' ',cmd_fixImgMask,' ',cmd_initialTransform,' ',extCmd,cmd_thread]);%,' ',cmd_verbose]);
setenv('PATH',env_PATH);
if fail; error(['ERROR: Elastix failed while registering ',movImgFile,' to ',fixImgFile,' with the following message:',msg]); end;
[~,movimgname,movimgext] = fileparts(movImgFile);
[~,fiximgname] = fileparts(fixImgFile);
[~,transformName] = fileparts(paraFile);
copyfile(fullfile(tmpDir,'result.0.mha'),fullfile(outputDir,[movimgname,'_',transformName,'_',fiximgname,movimgext]));
copyfile(fullfile(tmpDir,'TransformParameters.0.txt'),fullfile(outputDir,[movimgname,'_',transformName,'_',fiximgname,'.txt']));

%% Delete temporary files
if exist(tmpDir,'dir'); rmdir(tmpDir,'s');  end;

return