function [result,cudaCC] = iscudacompatible
%% [result,cudaCC] = iscudacompatible
% ------------------------------------------
% FILE   : iscudacompatible.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-05-26   Creation
% ------------------------------------------
% PURPOSE
%   To determine whether the system has a sufficient cuda compute
%   capability for CUDA MATLAB acceleration.
%
% ------------------------------------------
% OUTPUT
%   result:     "true" or "false" depending on whether the cuda compuate
%               capability is sufficient.
%   cudaCC:     The CUDA compute capability.
%%

try
    gpuInfo = gpuDevice;
catch
    result = false;
    return;
end;
cudaCC = str2double(gpuInfo.ComputeCapability);
result = cudaCC >= 1.1;

return;