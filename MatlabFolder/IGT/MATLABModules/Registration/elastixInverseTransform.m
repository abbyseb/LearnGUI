function elastixInverseTransform(transformFile,imgFile,verbose,outputFile)
%% elastixInverseTransform(transformFile,imgFile,verbose,outputFile)
% ------------------------------------------
% FILE   : elastixInverseTransform.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-06-29  Created.
% ------------------------------------------
% PURPOSE
%   Generate the inverse transform file of an existing transform file.
% ------------------------------------------
% INPUT
%   transformFile:  The transform file to be inverted.
%   imgFile:        The fixed image file based on which the transform file
%                   was generated.
%   verbose:        Whether to show elastix messages.
%                   Default: false.
%   outputFile:     The file path of the output inverse transform file.
%                   Default: OriginalFileName_Inverse.txt

%% Input check

if nargin < 3
    verbose = false;
end

if nargin < 4
    [fileDir,fileName] = fileparts(transformFile);
    outputFile = fullfile(fileDir,[fileName,'_Inverse.txt']);
end

%% Use elastix to find inverse transform

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

cmd_verbose = '>nul 2>&1';
if verbose; cmd_verbose = '';   end;

fail = system([elastix,' -f "',imgFile,'" -m "',imgFile,...
    '" -p "',which('Elastix_BSpline_Inverse.txt'),...
    '" -t0 "',transformFile,...
    '" -out "',tmpDir,'" ',...
    cmd_verbose]);
setenv('PATH',env_PATH);
if fail; error(['ERROR: Elastix failed while calculating inverse transform for: ',transformFile]); end;

%% Modify output transform file and copy to outputFile
tmpTransformFile = fullfile(tmpDir,'TransformParameters.0.txt');

tag_line = '(InitialTransformParametersFileName';
new_line = '(InitialTransformParametersFileName "NoInitialTransform")';

% Read parameter file
fid = fopen(tmpTransformFile,'r');
k = 1;
tline = fgetl(fid);
if length(tline) >= length(tag_line) ...
        && strcmpi(tline(1:length(tag_line)),tag_line)
    tline = new_line;
end
paratext{k} = tline;
while ischar(tline)
    k = k + 1;
    tline = fgetl(fid);
    if length(tline) >= length(tag_line) ...
            && strcmpi(tline(1:length(tag_line)),tag_line)
        tline = new_line;
    end
    paratext{k} = tline;
end
fclose(fid);

% Write new file
fid = fopen(outputFile,'w');
for k = 1:numel(paratext)
    if paratext{k+1} == -1
        fprintf(fid,'%s',paratext{k});
        break;
    else
        fprintf(fid,'%s\n',paratext{k});
    end
end
fclose(fid);

%% Delete temporary files
if exist(tmpDir,'dir'); rmdir(tmpDir,'s');  end;

return