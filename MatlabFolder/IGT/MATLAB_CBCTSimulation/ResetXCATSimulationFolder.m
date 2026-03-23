function ResetXCATSimulationFolder(dirPath)
%% ResetXCATSimulationFolder(dirPath)
% ------------------------------------------
% FILE   : ResetXCATSimulationFolder.m
% AUTHOR : Andy Shieh, ACRF Image X Institute, The University of Sydney
% DATE   : 2017-12-15  Created.
% ------------------------------------------
% PURPOSE
%   Clean all the temporary files in the XCAT CBCT simulation folder to
%   make sure unwanted files do not interefere with a new simulation.
% ------------------------------------------
% INPUT
%   dirPath:    Path to the folder to be reset.

%%
if nargin < 1
    dirPath = uigetdir(pwd,'Choose the folder to be reset.');
end

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

fl = lscell(fullfile(dirPath,'*.bin'));
for k = 1:length(fl)
    [failed,msg] = system(['del "',fl{k},'"']);
    if failed
        error(['ERROR: Failed to delete ',fl{k},' with the following error message: ',msg]);
    end
end