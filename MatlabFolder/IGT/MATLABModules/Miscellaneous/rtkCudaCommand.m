function cudacommand = rtkCudaCommand(option,tag)
%% cudacommand = rtkCudaCommand(option)
% ------------------------------------------
% FILE   : rtkCudaCommand.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-06-24   Creation
% ------------------------------------------
% PURPOSE
%   Just an easy way to generate the cuda command line for rtk
%   applications.
%
% ------------------------------------------
% INPUT
%   option:             Prefix, FDK, Backprojection, SART,
%                       Forwardprojection, or the corresponding rtk
%                       application name.
%   tag:                Default is true. If "false" is input, empty command
%                       will be return;
% ------------------------------------------
% OUTPUT
%   cudacommand:        The command line for cuda rtk applications.
%%

if nargin < 2
    tag = true;
elseif ~islogical(tag)
    error('ERROR: The field "tag" must be a logical input.');
end

if tag
    if strcmpi(option,'Prefix')
        cudacommand = 'cuda_';
    elseif strcmpi(option,'FDK') || strcmpi(option,'rtkfdk') || strcmpi(option,'rtkfdknoramp')
        cudacommand = ' --hardware "cuda"';
    elseif strcmpi(option,'Backprojection') || strcmpi(option,'rtkbackprojections')
        cudacommand = ' --bp "CudaBackProjection"';
    elseif strcmpi(option,'SART') || strcmpi(option,'rtksart')
        cudacommand = ' --fp "CudaRayCast" --bp "CudaVoxelBased"';
    elseif strcmpi(option,'Forwardprojection') || strcmpi(option,'rtkforwardprojections')
        cudacommand = ' --fp "CudaRayCast"';
    else
        error('ERROR: Unidentified option.');
    end;
else
    cudacommand = '';
end

return;