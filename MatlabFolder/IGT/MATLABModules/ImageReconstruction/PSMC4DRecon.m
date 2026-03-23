function PSMC4DRecon(projDir,geoFile,phaseFile,dvf4DFile,fdk4DList,outputDir)
%% PSMC4DRecon(projDir,geoFile,phaseFile,dvf4DFile,fdk4DList,outputDir)
% ------------------------------------------
% FILE   : PSMC4DRecon.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-08-08 Created.
% ------------------------------------------
% PURPOSE
%   Perform 4D projection space motion-compensated reconstructions for the
%   CBCT scans from the Light SABR trial.
%   The reconstruction method is described in the following paper:
%   Rit S. 2009. Med Phys. Jun;36(6):2283-96.
% ------------------------------------------
% INPUT
%   projDir:     	The directory containing (only) the projection files.
%   geoFile:        The RTK geometry file.
%   phaseFile:      The phase signal file.
%   dvf4DFile:  	The 4D DVF file. If the DVF file does not exist, the
%                   user must provide fdk4DList based on which the DVF file 
%                   will be constructed.
%   fdk4DList:      If dvf4DFile is not provided, the list of 4D FDK recon
%                   will be used to construct the 4D DVF. This list must be
%                   in cell format (can be obtained using lscell).
%   outputDir:      The output directory (default: current directory).

%% Input check
if nargin < 3
    beaconRemoved = false;
end

projDir = fullfile(fracDir,'ProcessedCBCTProjections');
rexp = 'hnc';
if ~exist(projDir,'dir')
    projDir = fullfile(cbctDir,'Scan0');
    rexp = 'hnd';
end

outputDir = fullfile(fracDir,'CBCTRecon');

if beaconRemoved
    projDir = fullfile(fracDir,'ProcessedCBCTProjections_MarkerRemoved');
    rexp = 'hnc';
    outputDir = fullfile(fracDir,'CBCTRecon_MarkerRemoved');
end

if ~exist(outputDir,'dir')
    mkdir(outputDir);
end

inputDir = fullfile(fracDir,'CBCTRecon');
if ~exist(inputDir,'dir');
    inputDir = fullfile(fracDir,'CBCTRecon_MarkerRemoved');
end

dvfDir = fullfile(inputDir,'4DDVF');
if ~exist(dvfDir,'dir')
    mkdir(dvfDir);
end;

%% Projection space motion compensated recon
fdkList = lscell(fullfile(inputDir,'FDK4D_*.mha'));
NBin = length(fdkList);

rtkfdk = which('rtkfdk.exe');
if isempty(rtkfdk)
    rtkfdk = 'rtkfdk';
else
    rtkfdk = ['"',rtkfdk,'"'];
end

% Read respiratory phase
phase = csvread(fullfile(fracDir,'RespiratoryPhase_CBCT.csv'));

% Check if 4DDVF and DVF signal exist already
dvf4DList = lscell(fullfile(dvfDir,'4DDVF_*.mha'));
dvfSignalList = lscell(fullfile(dvfDir,'DVFSignal_*.mha'));
dvf4DReady = false;
dvfSignalReady = false;
if ~isempty(dvf4DList);   dvf4DReady = true;  end;
if ~isempty(dvfSignalList);   dvfSignalReady = true;  end;

% Do MC recon for bin 01
for k = 1:1%length(fdkList);
    [~,fdkName] = fileparts(fdkList{k});
    
    transformList = lscell(fullfile(dvfDir,['*_Elastix_*_',fdkName,'.txt']));
    if ~dvf4DReady
        % Registration
        for b = k:(k+length(fdkList)-1);
            bin = mod(b,length(fdkList));
            bin = bin + length(fdkList) * (bin==0);
            elastixRegistration(fdkList{bin},fdkList{k},dvfDir,...
                which('Elastix_BSpline.txt'),'','','',false);
        end;
        
        % Output DVF
        transformList = lscell(fullfile(dvfDir,['*_Elastix_*_',fdkName,'.txt']));
        for kt = 1:length(transformList);
            [~,transformName] = fileparts(transformList{kt});
            elastixTransform(transformList{kt},'',false,fullfile(dvfDir,['DVF_',transformName,'.mha']));
        end;
        
        % Merge DVFs
        dvfList = lscell(fullfile(dvfDir,['DVF_*_',fdkName,'.mha']));
        dvf4DFile = fullfile(dvfDir,['4DDVF_',fdkName,'.mha']);
        Merge4DDVF(dvfList,dvf4DFile);
    else
        dvf4DFile = dvf4DList{k};
    end
    
    % DVF signal
    if ~dvfSignalReady
        dvfSignal = mod(phase - (k-1)/NBin,1);
        dvfSignalFile = fullfile(dvfDir,['DVFSignal_',fdkName,'.csv']);
        csvwrite(dvfSignalFile,dvfSignal(:));
    else
        dvfSignalFile = dvfSignalList{k};
    end
    
    % FDK
    fail = system([rtkfdk,' ',...
        '-p "',projDir,'" ',...
        '-r "',rexp,'" ',...
        '--signal "',dvfSignalFile,'" ',...
        '--dvf "',dvf4DFile,'" ',...
        '-g "',fullfile(fracDir,'CBCTGeometry.xml'),'" ',...
        '-o "',fullfile(outputDir,num2str(k,'PSMC4D_%02d.mha')),'" ',...
        '--pad 1 --hann 0.6 --hannY 0.4 ',...
        '--dimension 450,220,450 --spacing 1,1,1']);
    if fail~=0
        error(['ERROR: rtkfdk failed while trying to reconstruct 4D PSMC image bin ',num2str(k,'%02d'),' using projections in: ',projDir]);
    end;
end;

% Use DIR to warp bin01 to the rest
for k = 2:length(fdkList)
    % First, get inverse transform
    
    [inverseDir,inverseName,inverseExt] = fileparts(transformList{k});
    inverseTransformFile = fullfile(inverseDir,[inverseName,'_Inverse',inverseExt]);
    if ~exist(inverseTransformFile,'file')
        elastixInverseTransform(transformList{k},fdkList{1},false,inverseTransformFile);
    end
    
    % Then, warp PSMC4D_01 with the inverse transform
    elastixTransform(inverseTransformFile,...
        fullfile(outputDir,num2str(1,'PSMC4D_%02d.mha')),...
        false,...
        fullfile(outputDir,num2str(k,'PSMC4D_%02d.mha')));
end