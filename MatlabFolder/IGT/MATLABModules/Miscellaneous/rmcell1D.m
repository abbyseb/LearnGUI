function outputCell = rmcell1D(inputCell,entries)
%% outputCell = rmcell1D(inputCell,entries)
% ------------------------------------------
% FILE   : rmcell1D.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-05-13   Creation
% ------------------------------------------
% PURPOSE
%   Remove one or multiple elements in the input cell and return a cell
%   with modified length. Only support 1D cell.
%
% ------------------------------------------
% INPUT
%   inputCell:    The input cell.
%   entries:      The position(s) of the elements to be removed. e.g. [1,2]
% ------------------------------------------
% OUTPUT
%   outputCell:   The output cell with the elements removed and length
%                 modified.

%%
cellSize = size(inputCell);
if sum(cellSize > 1) > 1;
    error('ERROR: rmcell1D only support 1D cells.');
end

newSize = cellSize;
newSize(cellSize ~= 1) = length(inputCell) - length(entries);
outputCell = cell(newSize);

count = 0;
for k = 1:length(inputCell)
    if any(k == entries);   continue;   end;
    
    count = count + 1;
    outputCell{count} = inputCell{k};
end

return;