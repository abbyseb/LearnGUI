function WriteText(inputCells,filename)
%% WriteText(inputCells)
% ------------------------------------------
% FILE   : WriteText.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-09-13  Created.
% ------------------------------------------
% PURPOSE
%   Write text cell arrays to text files
% ------------------------------------------
% INPUT
%   inputCells:   The input cell array to be wrriten.
%   filename:     Full path to the output file.
% ------------------------------------------

%%
fid = fopen(filename,'w');
for k = 1:length(inputCells)
    fprintf(fid,'%s\n',inputCells{k});
end
fclose(fid);

end