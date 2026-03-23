function [sol,objValues,convStatus] = conjGradOpt(objFunc,gradFunc,hesFunc,initGuess,stopCond,NMax)
%% [sol,objValues,convStatus] = conjGradOpt(objFunc,gradFunc,hesFunc,initGuess,stopCond,NMax)
% ------------------------------------------
% FILE   : conjGradOpt.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-09-04  Created.
% ------------------------------------------
% PURPOSE
%   Solve an optimziation problem by nonlinear conjugate gradient together
%   with Newton's method for line search and the Fletcher–Reeves formula
%   for beta.
% ------------------------------------------
% INPUT
%   objFunc:        The handle to the objective function.
%   gradFunc:       The handle to the gradient function.
%   hesFuncc:       The handle to the hessian function. Can be empty.
%   initGuess:      The initial guess of the solution.
%   stopCond:       The stopping criterion as the decrease in objective 
%                   value being smaller than a certain fraction of the 
%                   initial decrease in objective value.
%                   (Default: 0.01)
%   NMax:           The maximum number of iterations allowed.
%                   (Default: 100)
%
% OUTPUT
%   sol:            The final solution.
%   objValues:      The values of the objective function at each iteration.
%   convStatus:     Whether the algorithm has converged based on stopCond.

%% Input check
if nargin < 4
    error('ERROR: objFunc, gradFunc, hesFunc, initGuess must be provided.');
end

if nargin < 5
    stopCond = 0.01;
end

if nargin < 6
    NMax = 100;
end

NConvIter = 5;

%% Start conjugate gradient

objValues(1) = objFunc(initGuess(:));
sol = initGuess(:);
convStatus = false;

for n = 1:NMax
    neggrad = -gradFunc(sol(:));
    
    if norm(neggrad) < eps
        convStatus = true;
        break;
    end
    
    if n == 1
        beta = 0;
        conjdir = neggrad;
    else
        beta = norm(neggrad)^2 / norm(neggrad_prev)^2;
        conjdir = neggrad + beta * conjdir_prev;
    end
    rho = conjdir(:)' * neggrad(:);
    
    % Line search
    if ~isempty(hesFunc)
        alpha = rho / (conjdir(:)' * hesFunc(sol(:)) * conjdir);
    else
        alpha = rho / norm(conjdir)^2;
    end
    sol_candidate = sol + alpha * conjdir(:);
    objValues(end+1) = objFunc(sol_candidate);
    while objValues(end) > objValues(end-1)
        alpha = 0.5 * alpha;
        sol_candidate = sol + alpha * conjdir(:);
        objValues(end) = objFunc(sol_candidate);
    end
    
    sol = sol_candidate;
    
    if n == 1
        initImprovement = objValues(1) - objValues(2);
    end
    
    if length(objValues) > NConvIter + 1
        if all(-diff(objValues(end-NConvIter:end)) < stopCond * initImprovement)
            convStatus = true;
            break;
        end
    end
    
    neggrad_prev = neggrad;
    conjdir_prev = conjdir;
end


