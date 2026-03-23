function ProjWrite(info,M,outputfp)
%% ProjWrite(info,M,outputfp)
% ------------------------------------------
% FILE   : ProjWrite.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-10  Created.
% ------------------------------------------
% PURPOSE
%   Write the input header and image to hnc, att, or his projection format.
% ------------------------------------------
% INPUT
%   info:       The header MATLAB struct.
%   M:          The image body, 2D.
%   outputfp:   The path of the output file.
% ------------------------------------------

%% 

[~,~,ext] = fileparts(outputfp);

if strcmpi(ext(2:end),'hnc')
    HncWrite(info,uint16(M),outputfp);
elseif strcmpi(ext(2:end),'att')
    AttWrite(info,single(M),outputfp);
elseif strcmpi(ext(2:end),'his')
    HisWrite(info,uint16(M),outputfp);
else
    error('ERROR: Unsupported projection format.');
end



return
