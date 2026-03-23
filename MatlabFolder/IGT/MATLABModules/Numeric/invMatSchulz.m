function [V,d,n] = invMatSchulz(mat)
%% [V,d,n] = invMatSchulz(mat)
% ------------------------------------------
% FILE   : invMatSchulz.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-02-14  Created.
% ------------------------------------------
% PURPOSE
%   Iteration solution of the inverse of a square matrix based on the
%   Schulz-style method.
%   Resource: https://www.hindawi.com/journals/jam/2014/731562/
% ------------------------------------------
% INPUT
%   mat:        The input square matrix matrix
%
% OUTPUT
%   V:          The inverse matrix.
%   d:          The step change in the last iteration.
%   n:          Number of iterations run.

%% Input check
if length(size(mat)) ~= 2 || size(mat,1) ~= size(mat,2)
    error('ERROR: The input must be a square matrix.');
end;

%% 
NIter = 400;

I = eye(size(mat,1));
if issymmetric(mat) && all(eig(mat)>0)
    V0 = I / norm(mat,'fro');
else
    V0 = mat' / size(mat,1) / sum(abs(mat(:))) / max(abs(mat(:)));
end

V = V0;
d = realmax;
for n = 1:NIter
    V_prev = V;
    V = (I + (I - V*mat)*(3*I - V*mat)^2 / 4 ) * V;
    d = norm(V - V_prev);
    if d <= eps * norm(mat)
        break;
    end
end
