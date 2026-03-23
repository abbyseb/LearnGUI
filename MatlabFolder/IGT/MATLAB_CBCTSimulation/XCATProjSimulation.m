function XCATProjSimulation(dirPath,projDim,projSpacing,paraComp,keepVolume,keepLesion,keepPhantom,sigMinMax,frames,volumeOffset)
%% XCATProjSimulation(dirPath,projDim,projSpacing,paraComp,keepVolume,keepLesion,keepPhantom,sigMinMax,frames,volumeOffset)
% ------------------------------------------
% FILE   : XCATProjSimulation.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2017-12-14  Created.
% ------------------------------------------
% PURPOSE
%   Do a frame-by-frame XCAT volume generation followed by projection
%   simulation. This program looks for input file
%   s that are in the
%   "dirPath". See below for more information.
% ------------------------------------------
% INPUT
%   dirPath:        Path (string) to the input folder. This is also where
%                   the output files will be saved.
%   projDim:        The dimension of the projection to be simulated.
%                   (1 x 2 vector) Default: [1024,768]
%                   If left empty, the forward projection process will be
%                   skipped.
%   projSpacing:    The spacing of the projection to be simulated.
%                   (1 x 2 vector) Default: [0.388,0.388]
%   paraComp:       To use parallel computing or not. (bolean)
%                   (bolean. Default = false)
%   keepVolume:     To keep the volume generated for each frame or not.
%                   (bolean. Default = true)
%   keepLesion:     To keep the lesion volume generated for each frame or not.
%                   (bolean. Default = true)
%   keepPhantom:    To keep the phantom volume generated for each frame or not.
%                   (bolean. Default = false)
%   sigMinMax:      The minimum and maximum values used to normalize the
%                   motion trace. This should be a 2 by 2 matrix specifying
%                   the min and max for diaphragm trace (first row) and
%                   chest trace (second row).
%                   Example: sigMinMax = [0,2;0,2]
%                   (2 x 2 matrix) Default = empty (get min max from signal
%                   itself).
%   frames:         The indices of the frames to be simulated in a vector.
%                   (Default: 1:NFrame)
%   volumeOffset:   Offset volume to move certain regions to isocenter.
%                   (1 x 3 vector in mm following IEC geometry).
%                   (Default: [0,0,0])
% ------------------------------------------
% INPUT FILES
%   The program requires the following files to be in "dirPath".
%       atten_table.dat
%       heart_curve.txt
%       lesion.nrb
%       ap_curve.dat
%       diaphragm_curve.dat
%       vfemale50_heart.nrb
%       vmale50.nrb
%       vmale50_heart.nrb
%       xcat2.cfg
%       Phantom.par:     This is the parameter file for the XCAT
%                        simulation. All of the settings in this file,
%                        except for the ones related to lesion generation,
%                        will be used. The lesion parameter files will
%                        have all the non-lesion related parameters
%                        replaced by the ones in the Phantom.par file.
%       Lesion.par:      The lesion parameter file. Note that only the
%                        "mode = X" setting and the relevant lesion
%                        parameter setting will be used. The rest will be
%                        overwritten by parameters in the Phantom.par
%                        file to ensure dimensional consistency.
%                        The user can include multiple lesion files to
%                        generate multiple lesions like this:
%                        Lesion_lung1.par
%                        Lesion_heart1.par
%                        The output lesion volume will inherit the name of
%                        the lesion .par file.
%       Geometry.xml:    This is the RTK geometry file for the scan
%                        trajectory. Can be generated using functions like:
%                        GenerateVarianGeometry.m, GenerateElektaGeometry.m
%                        GenerateCustomGeometry.m, etc
%       MotionTrace.csv: This is the motion file with respiratory
%                        displacement and cardiac phase. An example .csv
%                        file looks like this:
%                        0.000  0.000 0.000
%                        0.564  0.564 0.200
%                        1.234  1.234 0.400
%                         ...    ...
%                        The first column is diaphragm displacement, with
%                        positve corresponding to exhale motion.
%                        The second column is chest displacement, with
%                        positve corresponding to exhale motion.
%                        The third column is cardiac phase (0-1). If no
%                        cardiac motion is wanted, just put a constant
%                        number for the third column.
%                        A fourth-sixth column can also be input for 
%                        independent tumor motion. The coordinate
%                        convention for tumor motion is:
%                        1st column: LR. Positive = L
%                        2nd column: SI. Positive = S
%                        3rd column: AP. Positive = A
% ------------------------------------------
% OUTPUT FILES
%       Lesion_XXX:      These are the folders containing all the lesion
%                        volumes for each frame.
%       Volumes:         This is the folder containing the whole volume for
%                        each frame.       
%       Phantom:         This is the folder containing the phantom volume
%                        for each frame. 
%       Projections:     This is the folder containing the simulated
%                        projections in HNC format (photon count, uint16).
%                        No Poisson noise is added.
%       Visualization:   This folder stores .png files that visualise the
%                        volume and projections generated for each frame.
% ------------------------------------------

%% Input check
if nargin < 1
    dirPath = pwd;
end

if nargin < 2
    projDim = [1024,768];
end

if nargin < 3
    projSpacing = [0.388,0.388];
end

if nargin < 4
    paraComp = false;
end

if nargin < 5
    keepVolume = true;
end

if nargin < 6
    keepLesion = true;
end

if nargin < 7
    keepPhantom = false;
end

if nargin < 8
    sigMinMax = [];
end

if nargin < 9
    frames = 1;
end

if nargin < 10
    volumeOffset = [0,0,0];
end

% Number of parallel nodes
myCluster = parcluster('local');
NParallel = myCluster.NumWorkers;

dxcat2 = which('dxcat2.exe');
if isempty(dxcat2)
    error('ERROR: Could not find dxcat2.exe. Please include "MATLAB_CBCTSimulation" in your MATLAB path.');
end
%% Start ticking for the initialization stage
tic;
fprintf('Initialization ......');

%% First, make sure the directory is clean
fl = lscell(fullfile(dirPath,'*'));
for k = 1:length(fl)
    [~,name] = fileparts(fl{k});
    if length(name) < 5; continue; end;
    if ~isnan(str2double(name(end-4:end))) || strcmpi(name(end-3:end),'_log')
        if exist(fl{k}) == 2
            [failed,msg] = system(['del "',fl{k},'"']);
            if failed
                error(['ERROR: Failed to delete ',fl{k},' with the following error message: ',msg]);
            end
        elseif exist(fl{k}) == 7
            [failed,msg] = system(['rmdir /s /q "',fl{k},'"']);
            if failed
                error(['ERROR: Failed to delete ',fl{k},' with the following error message: ',msg]);
            end
        end
    end
end

%% Read pixel spacing and image dimension
paraText = ReadText(fullfile(dirPath,'Phantom.par'));
for k = 1:length(paraText)
    if length(paraText{k}) > 11 && strcmpi(paraText{k}(1:11),'pixel_width')
        parts = regexp(paraText{k},'=','split');
        parts = regexp(parts{2},'#','split');
        spacing([1,3]) = str2double(parts{1}) * 10;
    elseif length(paraText{k}) > 11 && strcmpi(paraText{k}(1:11),'slice_width')
        parts = regexp(paraText{k},'=','split');
        parts = regexp(parts{2},'#','split');
        spacing(2) = str2double(parts{1}) * 10;
    elseif length(paraText{k}) > 10 && strcmpi(paraText{k}(1:10),'array_size')  
        parts = regexp(paraText{k},'=','split');
        parts = regexp(parts{2},'#','split');
        dim([1,3]) = str2double(parts{1});
    elseif length(paraText{k}) > 10 && strcmpi(paraText{k}(1:10),'startslice')  
        parts = regexp(paraText{k},'=','split');
        parts = regexp(parts{2},'#','split');
        startslice = str2double(parts{1});
    elseif length(paraText{k}) > 8 && strcmpi(paraText{k}(1:8),'endslice')  
        parts = regexp(paraText{k},'=','split');
        parts = regexp(parts{2},'#','split');
        endslice = str2double(parts{1});
    end
end
dim(2) = endslice - startslice + 1;

%% Make sure out_frames = 1
for k = 1:length(paraText)
    if length(paraText{k}) > 10 && strcmpi(paraText{k}(1:10),'out_frames')
        paraText{k} = 'out_frames = 1		# output_frames (# of output time frames )';
    end
end
WriteText(paraText,fullfile(dirPath,'Phantom.par'));

%% Generate scaled anatomy model first to save time
anatomyText = paraText;
for k = 1:length(anatomyText)
    if length(anatomyText{k}) > 4 && strcmpi(anatomyText{k}(1:4),'mode')
        anatomyText{k} = 'mode = 5		# program mode (0 = phantom, 1 = heart lesion, 2 = spherical lesion, 3 = plaque, 4 = vectors, 5 = save anatomical variation) SEE NOTE 0';
    end
    if length(anatomyText{k}) > 10 && strcmpi(anatomyText{k}(1:10),'heart_base')
        heartBaseFile = regexp(anatomyText{k},'=','split');
        heartBaseFile = regexp(heartBaseFile{2},'#','split');
        heartBaseFile = strtrim(heartBaseFile{1});
    end
    if length(anatomyText{k}) > 16 && strcmpi(anatomyText{k}(1:16),'heart_curve_file')
        heartCurveFile = regexp(anatomyText{k},'=','split');
        heartCurveFile = regexp(heartCurveFile{2},'#','split');
        heartCurveFile = strtrim(heartCurveFile{1});
    end
    if length(anatomyText{k}) > 10 && strcmpi(anatomyText{k}(1:10),'organ_file')
        organFile = regexp(anatomyText{k},'=','split');
        organFile = regexp(organFile{2},'#','split');
        organFile = strtrim(organFile{1});
    end
end
WriteText(anatomyText,fullfile(dirPath,'AnatomyVariation.par'));

[failed,msg] = system([dxcat2,' "',fullfile(dirPath,'AnatomyVariation.par'),'" "',fullfile(dirPath,'ScaledAnatomy'),'"']);
if failed > 1 || ~isempty(strfind(msg,'Can not open')) || ...
        ~exist(fullfile(dirPath,'ScaledAnatomy_heart.nrb'),'file') || ...
        ~exist(fullfile(dirPath,'ScaledAnatomy_heart_curve.txt'),'file') || ...
        ~exist(fullfile(dirPath,'ScaledAnatomy.nrb'),'file')
    error(['ERROR: XCAT failed to generate scaled anatomy files with the following error messages: ',msg]);
end

% Now, generate Phantom_ScaledAnatomy.par, which will actually be used in
% the simulation
scaledParaText = paraText;
for k = 1:length(scaledParaText)
    if length(scaledParaText{k}) > 4 && strcmpi(scaledParaText{k}(1:4),'mode')
        scaledParaText{k} = 'mode = 0		# program mode (0 = phantom, 1 = heart lesion, 2 = spherical lesion, 3 = plaque, 4 = vectors, 5 = save anatomical variation) SEE NOTE 0';
    end
    if length(scaledParaText{k}) > 10 && strcmpi(scaledParaText{k}(1:10),'heart_base')
        scaledParaText{k} = 'heart_base = ScaledAnatomy_heart.nrb';
    end
    if length(scaledParaText{k}) > 16 && strcmpi(scaledParaText{k}(1:16),'heart_curve_file')
        scaledParaText{k} = 'heart_curve_file = ScaledAnatomy_heart_curve.txt';
    end
    if length(scaledParaText{k}) > 10 && strcmpi(scaledParaText{k}(1:10),'organ_file')
        scaledParaText{k} = 'organ_file = ScaledAnatomy.nrb';
    end
    if ~isempty(strfind(scaledParaText{k},'_scale'))
        if isempty(strfind(scaledParaText{k},'motion_scale'))
            tagName = regexp(scaledParaText{k},'=','split');
            tagName = tagName{1};
            scaledParaText{k} = [tagName,'= 1.0'];
        end
    end
end
WriteText(scaledParaText,fullfile(dirPath,'Phantom_ScaledAnatomy.par'));

%% Merging phantom and lesion parameter files
lesionPFiles = lscell(fullfile(dirPath,'Lesion*.par'));
for n = 1:length(lesionPFiles)
    [~,lesionNames{n}] = fileparts(lesionPFiles{n});
    
    lesionText = ReadText(lesionPFiles{n});
    lesionTextNew = scaledParaText;
    
    % Change the "mode" line
    for k = 1:length(lesionText)
        if length(lesionText{k}) > 4 && strcmpi(lesionText{k}(1:4),'mode')
            line = lesionText{k};
        end
    end
    lesionMode = regexp(line,'=','split');
    lesionMode = regexp(lesionMode{2},'#','split');
    lesionMode = round(str2double(lesionMode{1}));
    for k = 1:length(lesionTextNew)
        if length(lesionTextNew{k}) > 4 && strcmpi(lesionTextNew{k}(1:4),'mode')
            lesionTextNew{k} = line;
        end
    end
    
    % Change lesion related lines
    indStart = 0; indEnd = 0;
    if lesionMode == 1  % Heart lesion
        lesionStartLine = 'Heart lesion parameters';
    elseif lesionMode == 2  % Spherical lesion
        lesionStartLine = 'Spherical lesion parameters';
    elseif lesionMode == 3  % Plaque
        lesionStartLine = 'Heart plaque parameters';
    else
        error('ERROR: Lesion mode must be 1, 2, or 3.');
    end
    
    for k = 1:length(lesionText)
        if ~isempty(strfind(lesionText{k},lesionStartLine))
            indStart = k;
            break;
        end
    end
    for k = (indStart+1):length(lesionText)
        if ~isempty(strfind(lesionText{k},'#----'))
            indEnd = k;
            break;
        end
    end
    
    for k = 1:length(lesionTextNew)
        if ~isempty(strfind(lesionTextNew{k},lesionStartLine))
            indStartInNew = k;
            break;
        end
    end
    for k = (indStartInNew+1):length(lesionTextNew)
        if ~isempty(strfind(lesionTextNew{k},'#----'))
            indEndInNew = k;
            break;
        end
    end
    
    lesionTextNew = rmcell1D(lesionTextNew,indStartInNew:indEndInNew);
    nLines = length(lesionTextNew);
    lesionTextNew{nLines + 1} = '';
    for k = indStart:indEnd
        lesionTextNew{nLines + k - indStart + 1 + 1} = lesionText{k};
    end
    
    WriteText(lesionTextNew,lesionPFiles{n});
end

%% Reading angles, respiratory displacements, and cardiac phase
angles = ReadAnglesFromRTKGeometry(fullfile(dirPath,'Geometry.xml'));
motionData = csvread(fullfile(dirPath,'MotionTrace.csv'));
respDis = motionData(:,1);
chestDis = motionData(:,2);
cardPh = motionData(:,3);

NFrame = length(angles);
if isempty(frames)
    frames = 1:NFrame;
end

respDis_Scaled = -respDis;
if isempty(sigMinMax)
    respDis_Scaled = (respDis_Scaled - min(respDis_Scaled))/(max(respDis_Scaled) - min(respDis_Scaled)) * 10;
else
    respDis_Scaled = (respDis_Scaled + max(sigMinMax(1,:)))/(max(sigMinMax(1,:)) - min(sigMinMax(1,:))) * 10;
end
respDis_Scaled(isnan(respDis_Scaled) | abs(respDis_Scaled) == Inf) = 0;

chestDis_Scaled = -chestDis;
if isempty(sigMinMax)
    chestDis_Scaled = (chestDis_Scaled - min(chestDis_Scaled))/(max(chestDis_Scaled) - min(chestDis_Scaled)) * 10;
else
    chestDis_Scaled = (chestDis_Scaled + max(sigMinMax(2,:)))/(max(sigMinMax(2,:)) - min(sigMinMax(2,:))) * 10;
end
chestDis_Scaled(isnan(chestDis_Scaled) | abs(chestDis_Scaled) == Inf) = 0;

if size(motionData,2) >= 6
    tumorDis = motionData(:,4:6);
    tmRange = max(max(tumorDis) - min(tumorDis));
    tumorDis_Scaled = tumorDis;
    tumorDis_Scaled(:,2) = -tumorDis_Scaled(:,2);
    tumorDis_Scaled(:,1) = (tumorDis_Scaled(:,1) - min(tumorDis_Scaled(:,1))) / tmRange * 10;
    tumorDis_Scaled(:,2) = (tumorDis_Scaled(:,2) - min(tumorDis_Scaled(:,2))) / tmRange * 10;
    tumorDis_Scaled(:,3) = (tumorDis_Scaled(:,3) - min(tumorDis_Scaled(:,3))) / tmRange * 10;
    tumorDis_Scaled(isnan(tumorDis_Scaled) | abs(tumorDis_Scaled) == Inf) = 0;
else
    tumorDis = [];
    tumorDis_Scaled = [];
end
if length(angles) ~= length(respDis) || length(angles) ~= length(cardPh)
    error('The number of frames in Geometry.xml does not match with that in MotionTrace.csv');
end

%% Make folders to store output
visDir = fullfile(dirPath,'Visualization'); mkdir(visDir);
projDir = fullfile(dirPath,'Projections');  mkdir(projDir);
if keepVolume
    volDir = fullfile(dirPath,'Volumes');  mkdir(volDir);
else
    volDir = '';
end
if keepPhantom
    phantomDir = fullfile(dirPath,'Phantom');  mkdir(phantomDir);
else
    phantomDir = '';
end
if keepLesion
    for k = 1:length(lesionNames)
        lesionDirs{k} = fullfile(dirPath,lesionNames{k});  mkdir(lesionDirs{k});
    end
else
    for k = 1:length(lesionNames)
        lesionDirs{k} = '';
    end
end

%% Stop ticking for the initialization stage
fprintf(['Completed using ',seconds2human(toc),'\n']);

%% Loop through each time frame.
% The actual tasks are done in a sub-function to enable parallel computing

% The heavy simulation starts here
if paraComp
    % Split frames into NParallel parts
    parfor n = frames
        doSimulation(n,n,...
            dirPath,fullfile(dirPath,'Phantom_ScaledAnatomy.par'),...
            lesionPFiles,fullfile(dirPath,'Geometry.xml'),...
            dim,spacing,projDim,projSpacing,...
            angles,cardPh,respDis_Scaled,chestDis_Scaled,tumorDis_Scaled,...
            projDir,volDir,phantomDir,lesionDirs,visDir,volumeOffset);
    end
else
    for n = frames
        doSimulation(n,n,...
            dirPath,fullfile(dirPath,'Phantom_ScaledAnatomy.par'),...
            lesionPFiles,fullfile(dirPath,'Geometry.xml'),...
            dim,spacing,projDim,projSpacing,...
            angles,cardPh,respDis_Scaled,chestDis_Scaled,tumorDis_Scaled,...
            projDir,volDir,phantomDir,lesionDirs,visDir,volumeOffset);
    end
end

fprintf(['XCAT CBCT simulation completed.']);

end

%% This is the sub-function that actually does the simulation
function doSimulation(startFrame,endFrame,...
    dirPath,phantomParaFile,lesionParaFiles,geometryFile,...
    dim,spacing,projDim,projSpacing,angles,cardPh,respDis,chestDis,tumorDis,...
    projDir,volDir,phantomDir,lesionDirs,visDir,volumeOffset)

dxcat2 = ['"',which('dxcat2.exe'),'"'];
rtkf = ['"',which('rtkforwardprojections.exe'),'"'];

for nf = startFrame:endFrame
    tic;
    disp(['Simulating ',num2str(nf,'Frame%05d'),'......']);
    
    % Firstly, modify trace_ap and trace_diaphragm based on respDis
    fid = fopen(fullfile(dirPath,num2str(nf,'ap_curve_%05d.dat')),'w');
    fprintf(fid,'%d :N\n\nControl Points',4);
    fprintf(fid,'\n%d %f 0',0,chestDis(nf));
    fprintf(fid,'\n%d %f 0',1,chestDis(nf));
    fprintf(fid,'\n%d %f 0',2,0); % These two lines are necessary
    fprintf(fid,'\n%d %f 0',3,10); % for XCAT to read as a reference
    fclose(fid);
    fid = fopen(fullfile(dirPath,num2str(nf,'diaphragm_curve_%05d.dat')),'w');
    fprintf(fid,'%d :N\n\nControl Points',4);
    fprintf(fid,'\n%d %f 0',0,-respDis(nf));
    fprintf(fid,'\n%d %f 0',1,-respDis(nf));
    fprintf(fid,'\n%d %f 0',2,0);  % These two lines are necessary
    fprintf(fid,'\n%d %f 0',3,-10); % for XCAT to read as a reference
    fclose(fid);
    if ~isempty(tumorDis)
        fid = fopen(fullfile(dirPath,num2str(nf,'tumor_curve_%05d.dat')),'w');
        fprintf(fid,'%d :N\n\nControl Points(dx,dy,dz):',4);
        fprintf(fid,'\n %f %f %f',tumorDis(nf,1),-tumorDis(nf,3),-tumorDis(nf,2));
        fprintf(fid,'\n %f %f %f',tumorDis(nf,1),-tumorDis(nf,3),-tumorDis(nf,2));
        fprintf(fid,'\n %f %f %f',0,0,0);  % These two lines are necessary
        fprintf(fid,'\n %f %f %f',10,10,10); % for XCAT to read as a reference
        fclose(fid);
    end
    
    % Modify parameter file to change cardiac phase
    paraFilesAll = lesionParaFiles;
    paraFilesAll{length(paraFilesAll) + 1} = phantomParaFile;
    for n = 1:length(paraFilesAll)
        [~,paraNames{n},ext] = fileparts(paraFilesAll{n});
        paraText = ReadText(paraFilesAll{n});
        for k = 1:length(paraText)
            if length(paraText{k}) > 18 && strcmpi(paraText{k}(1:18),'hrt_start_ph_index')
                paraText{k} = ['hrt_start_ph_index = ',num2str(cardPh(nf),'%f'),'	# hrt_start_phase_index (range=0 to 1; ED=0, ES=0.4) see NOTE 3 '];
            end
            if length(paraText{k}) > 12 && strcmpi(paraText{k}(1:12),'dia_filename')
                paraText{k} = ['dia_filename = ',num2str(nf,'diaphragm_curve_%05d.dat')];
            end
            if length(paraText{k}) > 11 && strcmpi(paraText{k}(1:11),'ap_filename')
                paraText{k} = ['ap_filename = ',num2str(nf,'ap_curve_%05d.dat')];
            end
            if length(paraText{k}) > 17 && strcmpi(paraText{k}(1:17),'tumor_motion_flag')
                if isempty(tumorDis)
                    paraText{k} = 'tumor_motion_flag = 0		# Sets tumor motion (0 = default motion based on lungs, 1 = motion defined by user curve below)';
                else
                    paraText{k} = 'tumor_motion_flag = 1		# Sets tumor motion (0 = default motion based on lungs, 1 = motion defined by user curve below)';
                end
            end
            if length(paraText{k}) > 21 && strcmpi(paraText{k}(1:21),'tumor_motion_filename')
                paraText{k} = ['tumor_motion_filename = ',num2str(nf,'tumor_curve_%05d.dat')];
            end
        end
        WriteText(paraText,fullfile(dirPath,[paraNames{n},'_',num2str(nf,'%05d'),'.par']));
    end
    
    % Generate XCAT volume
    for n = 1:length(paraFilesAll)
        [failed,msg] = system([dxcat2,' "',fullfile(dirPath,[paraNames{n},'_',num2str(nf,'%05d'),'.par']),'" "',fullfile(dirPath,[paraNames{n},'_',num2str(nf,'%05d')]),'"']);
        if failed > 1 || ~isempty(strfind(msg,'Can not open')) || length(lscell(fullfile(dirPath,[paraNames{n},'_',num2str(nf,'%05d'),'*.bin']))) == 0
            error(['ERROR: XCAT failed to generate volume for ',[paraNames{n},'_',num2str(nf,'%05d'),'.par'],' with the following error message: ',msg]);
        end
        [failed,msg] = system(['del "',fullfile(dirPath,[paraNames{n},'_',num2str(nf,'%05d'),'.par']),'"']);
        if failed
            error(['ERROR: Failed to delete ',fullfile(dirPath,[paraNames{n},'_',num2str(nf,'%05d'),'.par']),' with the following error message: ',msg]);
        end
    end
    
    % Delete temporary files
    [failed,msg] = system(['del "',fullfile(dirPath,num2str(nf,'ap_curve_%05d.dat')),'"']);
    if failed
        error(['ERROR: Failed to delete ',fullfile(dirPath,num2str(nf,'ap_curve_%05d.dat')),' with the following error message: ',msg]);
    end
    [failed,msg] = system(['del "',fullfile(dirPath,num2str(nf,'diaphragm_curve_%05d.dat')),'"']);
    if failed
        error(['ERROR: Failed to delete ',fullfile(dirPath,num2str(nf,'diaphragm_curve_%05d.dat')),' with the following error message: ',msg]);
    end
    [failed,msg] = system(['del "',fullfile(dirPath,num2str(nf,'tumor_curve_%05d.dat')),'"']);
    if failed
        error(['ERROR: Failed to delete ',fullfile(dirPath,num2str(nf,'tumor_curve_%05d.dat')),' with the following error message: ',msg]);
    end
    fl = lscell(fullfile(dirPath,'*_log'));
    for k = 1:length(fl)
        [failed,msg] = system(['del "',fl{k},'"']);
        if failed
            error(['ERROR: Failed to delete ',fl{k},' with the following error message: ',msg]);
        end
    end
    
    % Save XCAT bin files as mha files
    for n = 1:length(paraNames)
        file = lscell(fullfile(dirPath,[paraNames{n},'_',num2str(nf,'%05d'),'*.bin'])); file = file{1};
        SaveXCATBinAsMha(file,dim,spacing,fullfile(dirPath,[paraNames{n},'_',num2str(nf,'%05d'),'.mha']));
        [failed,msg] = system(['del "',file,'"']);
        if failed
            error(['ERROR: Failed to delete ',file,' with the following error message: ',msg]);
        end
    end
    
    % Merge mha files into a volume
    [~,phantomName] = fileparts(phantomParaFile);
    [info,M] = readMhaUntilSuccess(fullfile(dirPath,[phantomName,'_',num2str(nf,'%05d'),'.mha']));
    for n = 1:length(lesionParaFiles)
        [~,lesionName{n}] = fileparts(lesionParaFiles{n});
        [iL,L] = readMhaUntilSuccess(fullfile(dirPath,[lesionName{n},'_',num2str(nf,'%05d'),'.mha']));
        iL.Offset = iL.Offset + volumeOffset;
        MhaWrite(iL,L,fullfile(dirPath,[lesionName{n},'_',num2str(nf,'%05d'),'.mha']));
        M(L~=0) = L(L~=0);
    end
    info.Offset = info.Offset + volumeOffset;
    MhaWrite(info,M,fullfile(dirPath,num2str(nf,'Vol_%05d.mha')));
    figHandle = figure('units','normalized','outerposition',[0,0,1,0.5]);
    subplot(1,3,1); imagesc(M(:,:,round(end/2))'); colormap gray; axis image; axis off; title(num2str(nf,'Frame%05d'));
    subplot(1,3,2); imagesc(permute(M(:,round(end/2),end:-1:1),[3 1 2])); colormap gray; axis image; axis off; 
    subplot(1,3,3); imagesc(permute(M(round(end/2),:,end:-1:1),[2 3 1])); colormap gray; axis image; axis off; title(num2str(angles(nf),'Angle = %f deg'));
    saveas(figHandle,fullfile(visDir,num2str(nf,'Vol_%05d.png')));  close(figHandle);
    
    if ~isempty(projDim)
        % Tempoary sort file for simulation
        s = zeros(size(angles));
        s(nf) = 1;
        csvwrite(fullfile(dirPath,num2str(nf,'Sort_%05d.csv')),s(:));
        
        % RTK forward projection
        [failed,msg] = system([rtkf,' -g "',geometryFile,'" ',...
            '-i "',fullfile(dirPath,num2str(nf,'Vol_%05d.mha')),'" ',...
            '-o "',fullfile(dirPath,num2str(nf,'Proj_%05d.mha')),'" ',...
            '--sortfile "',fullfile(dirPath,num2str(nf,'Sort_%05d.csv')),'" ',...
            '--dimension ',num2str(projDim,'%d,%d'),' --spacing ',num2str(projSpacing,'%f,%f')]);
        if failed
            error(['ERROR: rtkforwardprojections failed to simulate ',fullfile(dirPath,num2str(nf,'Vol_%05d.mha')),' with the following error message: ',msg]);
        end
        [failed,msg] = system(['del "',fullfile(dirPath,num2str(nf,'Sort_%05d.csv')),'"']);
        if failed
            error(['ERROR: Failed to delete ',fullfile(dirPath,num2str(nf,'Sort_%05d.csv')),' with the following error message: ',msg]);
        end
        
        % Convert projections to HNC
        load hncHeaderTemplate; pInfo = hncHeaderTemplate;
        [~,Proj] = readMhaUntilSuccess(fullfile(dirPath,num2str(nf,'Proj_%05d.mha')));
        figHandle = figure('units','normalized','outerposition',[0,0,0.5,0.5]);
        imagesc(Proj'); colormap gray; axis image; axis off; title(num2str([nf,angles(nf)],'Frame%05d, Angle = %f deg'));
        saveas(figHandle,fullfile(visDir,num2str(nf,'Proj_%05d.png')));  close(figHandle);
        Proj = 65535 * exp(-Proj);
        Proj(Proj>65535) = 65535; Proj(Proj<0) = 0;
        Proj = uint16(round(Proj));
        pInfo.dCTProjectionAngle = angles(nf);
        pInfo.dCBCTPositiveAngle = mod(angles(nf) + 270, 360);
        pInfo.uiSizeX = projDim(1);
        pInfo.uiSizeY = projDim(2);
        pInfo.dIDUResolutionX = projSpacing(1);
        pInfo.dIDUResolutionY = projSpacing(2);
        pInfo.uiFileLength = 512 + 2 * prod(projDim);
        pInfo.uiActualFileLength = pInfo.uiFileLength;
        HncWrite(pInfo,Proj,fullfile(projDir,num2str(nf,'Proj_%05d.hnc')));
        [failed,msg] = system(['del "',fullfile(dirPath,num2str(nf,'Proj_%05d.mha')),'"']);
        if failed
            error(['ERROR: Failed to delete ',fullfile(dirPath,num2str(nf,'Proj_%05d.mha')),' with the following error message: ',msg]);
        end
    end
    
    if ~isempty(volDir)
        [failed,msg] = system(['move "',fullfile(dirPath,num2str(nf,'Vol_%05d.mha')),'" "',fullfile(volDir,num2str(nf,'Vol_%05d.mha')),'"']);
        if failed
            error(['ERROR: Failed to move ',fullfile(dirPath,num2str(nf,'Vol_%05d.mha')),' to ',fullfile(volDir,num2str(nf,'Vol_%05d.mha')),' with the following error message: ',msg]);
        end
    else
        [failed,msg] = system(['del "',fullfile(dirPath,num2str(nf,'Vol_%05d.mha')),'"']);
        if failed
            error(['ERROR: Failed to delete ',fullfile(dirPath,num2str(nf,'Vol_%05d.mha')),' with the following error message: ',msg]);
        end
    end
    
    if ~isempty(phantomDir)
        [failed,msg] = system(['move "',fullfile(dirPath,[phantomName,'_',num2str(nf,'%05d'),'.mha']),'" "',fullfile(phantomDir,[phantomName,'_',num2str(nf,'%05d'),'.mha']),'"']);
        if failed
            error(['ERROR: Failed to move ',fullfile(dirPath,[phantomName,'_',num2str(nf,'%05d'),'.mha']),' to ',fullfile(phantomDir,[phantomName,'_',num2str(nf,'%05d'),'.mha']),' with the following error message: ',msg]);
        end
    else
        [failed,msg] = system(['del "',fullfile(dirPath,[phantomName,'_',num2str(nf,'%05d'),'.mha']),'"']);
        if failed
            error(['ERROR: Failed to delete ',fullfile(dirPath,[phantomName,'_',num2str(nf,'%05d'),'.mha']),' with the following error message: ',msg]);
        end
    end
    
    for n = 1:length(lesionDirs)
        if ~isempty(lesionDirs{n})
            [failed,msg] = system(['move "',fullfile(dirPath,[lesionName{n},'_',num2str(nf,'%05d'),'.mha']),'" "',fullfile(lesionDirs{n},[lesionName{n},'_',num2str(nf,'%05d'),'.mha']),'"']);
            if failed
                error(['ERROR: Failed to move ',fullfile(dirPath,[lesionName{n},'_',num2str(nf,'%05d'),'.mha']),' to ',fullfile(lesionDirs{n},[lesionName{n},'_',num2str(nf,'%05d'),'.mha']),' with the following error message: ',msg]);
            end
        else
            [failed,msg] = system(['del "',fullfile(dirPath,[lesionName{n},'_',num2str(nf,'%05d'),'.mha']),'"']);
            if failed
                error(['ERROR: Failed to delete ',fullfile(dirPath,[lesionName{n},'_',num2str(nf,'%05d'),'.mha']),' with the following error message: ',msg]);
            end
        end
    end
    
     disp(['=====',num2str(nf,'Frame%05d'),' completed using ',seconds2human(toc)]);
end

end

function [info,M] = readMhaUntilSuccess(d)
failTag = true;
while failTag
    failTag = false;
    try
        [info,M] = MhaRead(d);
    catch e
        failTag = true;
        fclose('all');
        disp(['Failed to read ',d,' with the following error message:']);
        disp(e.message);
        disp('Trying again');
    end
end
end