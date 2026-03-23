function MergeRTKGeometryFiles(fileList,outputFile)
%% MergeRTKGeometryFiles(fileList,outputFile)
% ------------------------------------------
% FILE   : MergeRTKGeometryFiles.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2016-05-30  Created.
% ------------------------------------------
% PURPOSE
%   Merge multiple RTK geometry files into one.
%   SID, SDD, and offsets will be set according to the first file.
% ------------------------------------------
% INPUT
%   fileList:       The list of RTK geometry files to be merged in cell
%                   format (list can be obtained using lscell).
%   outputFile:     The output file path.

%% Input check

if nargin < 2
    outputFile = fullfile(pwd,'RTKGeometry_Merged.xml');
end

%% Read & Write

fout = fopen(outputFile,'w');
if fout == -1
    error(['ERROR: Cannot write to output file: ',outputFile]);
end

for k = 1:length(fileList)
    fin = fopen(fileList{k},'r');
    if fin == -1
        error(['ERROR: Cannot open file: ',fileList{k}]);
    end
    
    tline = fgets(fin);
    if k == 1
        while ischar(tline) && ...
                isempty(strfind(tline,'</RTKThreeDCircularGeometry>'))
            fprintf(fout,'%s',tline);
            tline = fgets(fin);
        end
    else
        while isempty(strfind(tline,'<Projection>'))
            tline = fgets(fin);
        end
        if k < length(fileList)
            while ischar(tline) && ...
                    isempty(strfind(tline,'</RTKThreeDCircularGeometry>'))
                fprintf(fout,'%s',tline);
                tline = fgets(fin);
            end
        else
            while ischar(tline)
                fprintf(fout,'%s',tline);
                tline = fgets(fin);
            end
        end
    end
    fclose(fin);
end;
fclose(fout);

return