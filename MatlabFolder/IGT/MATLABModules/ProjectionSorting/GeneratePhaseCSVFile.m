function outputFile = GeneratePhaseCSVFile(projDir,rexp,N,outputFile)
%% outputFile = GeneratePhaseCSVFile(projDir,rexp,N,outputFile)
% ------------------------------------------
% FILE   : GeneratePhaseCSVFile.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-11-03  Created.
% ------------------------------------------
% PURPOSE
%   Generate phase bin csv files for RTK 4D reconstructions.
% ------------------------------------------
% INPUT
%   projDir:    Path to the projection directory. (default: ui selection)
%   rexp:       Regular expression of the projection files. (default: '')
%   N:          Number of phase bins desired. (default: 10)
%   outputFile: The filename of the csv file. 
%               (default: PhaseBinInfo.csv at current directroy)
% ------------------------------------------
% OUTPUT
%   outputFile: Simply return the file path of the output csv file.

%% 

if nargin < 1
    projDir = uigetdir(pwd,'Select the projection directory');
    if projDir==0
        error('ERROR: no projection directory was selected. Try again.');
    end;
end

if nargin < 2
    rexp = '';
end
    
if nargin < 3
    N = 10;
end

if nargin < 4
    outputFile = fullfile(pwd,'PhaseBinInfo.csv');
end

% Return full file path
if isempty(fileparts(outputFile))
    outputFile = fullfile(pwd,outputFile);
end

% Get amsterdam shroud signal
sig = rtkAmstShroud(projDir,rexp);

% Calculate phase
phase = phasecal(sig);

% Phase to binInfo
binInfo = phase2bin(phase,N);

% Write to csv file
csvwrite(outputFile,binInfo(:));

return