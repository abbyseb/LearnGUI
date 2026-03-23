function result = tvhessian3(M,v,epsilon)
%% result = tvhessian3(M,v,epsilon)
% ------------------------------------------
% FILE   : tvhessian3.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2014-10-19 Created.
% ------------------------------------------
% PURPOSE
%   Calculate TV Hessian Matrix-vector operation, i.e. H*v.
% ------------------------------------------
% INPUT
%   M:         The input 3D image based on which Hessian is calculated.
%   v:         The vector to be multiplied by the Hessian matrix, same
%              dimension as M.
%   epsilon:   (Optional) A tiny number to prevent singular values. If not
%              input, the default is taken as "eps" defined in MATLAB.
% ------------------------------------------
% OUTPUT
%   result:    The gradient 3D matrix of the 3D total variation with respect to the image.
% ------------------------------------------

%% Calculate

dim = size(M);

if ~isequal(dim,size(v))
    error('ERROR: M and v must have the same dimension.');
end;

if nargin < 3
    epsilon = single(eps);
end;

result = tvhessian3_c(M,v,epsilon);

% % Initialize output
% result = single(zeros(dim));
%
% % Loop through pixels
% for x = 2:dim(1)-1
%     for y = 2:dim(2)-1
%         for z = 2:dim(3)-1
%             
%             tmp_result = 0;
%             
%             f0 = M(x,y,z);
%             fx1 = M(x-1,y,z);   fx2 = M(x+1,y,z);
%             fy1 = M(x,y-1,z);   fy2 = M(x,y+1,z);
%             fz1 = M(x,y,z-1);   fz2 = M(x,y,z+1);
%             fx1y2 = M(x-1,y+1,z); fx1z2 = M(x-1,y,z+1);
% 			fx2y1 = M(x+1,y-1,z); fx2z1 = M(x+1,y,z-1);
% 			fy1z2 = M(x,y-1,z+1); fy2z1 = M(x,y+1,z-1);
% 
%             % partial f_(x,y,z) partial f_(x,y,z)
%             tmp_result = tmp_result + v(x,y,z)*...
%                 ( ( (fx1 - fy1)^2 + (fx1 - fz1)^2 + (fy1 - fz1)^2 ) * ( (f0 - fx1)^2 + (f0 - fy1)^2 + (f0 - fz1)^2 + epsilon )^-1.5 + ...
%                   ( (fx2 - fx2y1)^2 + (fx2 - fx2z1)^2 ) * ( (fx2 - f0)^2 + (fx2 - fx2y1)^2 + (fx2 - fx2z1)^2 + epsilon )^-1.5 + ...
%                   ( (fy2 - fx1y2)^2 + (fy2 - fy2z1)^2 ) * ( (fy2 - fx1y2)^2 + (fy2 - f0)^2 + (fy2 - fy2z1)^2 + epsilon )^-1.5 + ...
%                   ( (fz2 - fx1z2)^2 + (fz2 - fy1z2)^2 ) * ( (fz2 - fx1z2)^2 + (fz2 - fy1z2)^2 + (fz2 - f0)^2 + epsilon )^-1.5 );
%               
%             % partial f_(x,y,z) partial f_(x-1,y,z)
%             tmp_result = tmp_result + v(x-1,y,z)*...
%                 ( (fx1 + f0)*(fy1 + fz1) - 2*f0*fx1 - fy1^2 - fz1^2 ) * ( (f0 - fx1)^2 + (f0 - fy1)^2 + (f0 - fz1)^2 + epsilon )^-1.5;
%             
%             % partial f_(x,y,z) partial f_(x,y-1,z)
%             tmp_result = tmp_result + v(x,y-1,z)*...
%                 ( (fy1 + f0)*(fx1 + fz1) - 2*f0*fy1 - fx1^2 - fz1^2 ) * ( (f0 - fx1)^2 + (f0 - fy1)^2 + (f0 - fz1)^2 + epsilon )^-1.5;
%                 
%             % partial f_(x,y,z) partial f_(x,y,z-1)
%             tmp_result = tmp_result + v(x,y,z-1)*...
%                 ( (fz1 + f0)*(fx1 + fy1) - 2*f0*fz1 - fx1^2 - fy1^2 ) * ( (f0 - fx1)^2 + (f0 - fy1)^2 + (f0 - fz1)^2 + epsilon )^-1.5;
%             
%             % partial f_(x,y,z) partial f_(x+1,y,z)
%             tmp_result = tmp_result + v(x+1,y,z)*...
%                 ( (f0 + fx2)*(fx2y1 + fx2z1) - 2*fx2*f0 - fx2y1^2 - fx2z1^2 ) * ( (fx2 - f0)^2 + (fx2 - fx2y1)^2 + (fx2 - fx2z1)^2 + epsilon )^-1.5;
%             
%             % partial f_(x,y,z) partial f_(x,y+1,z)
%             tmp_result = tmp_result + v(x,y+1,z)*...
%                 ( (f0 + fy2)*(fx1y2 + fy2z1) - 2*fy2*f0 - fx1y2^2 - fy2z1^2 ) * ( (fy2 - fx1y2)^2 + (fy2 - f0)^2 + (fy2 - fy2z1)^2 + epsilon )^-1.5;
%             
%             % partial f_(x,y,z) partial f_(x,y,z+1)
%             tmp_result = tmp_result + v(x,y,z+1)*...
%                 ( (f0 + fz2)*(fx1z2 + fy1z2) - 2*fz2*f0 - fx1z2^2 - fy1z2^2 ) * ( (fz2 - fx1z2)^2 + (fz2 - fy1z2)^2 + (fz2 - f0)^2 + epsilon )^-1.5;
%             
%             % partial f_(x,y,z) partial f_(x+1,y-1,z)
%             tmp_result = tmp_result + v(x+1,y-1,z)*...
%                 (-1)*(fx2 - f0) * (fx2 - fx2y1) * ( (fx2 - f0)^2 + (fx2 - fx2y1)^2 + (fx2 - fx2z1)^2 + epsilon )^-1.5;
%             
%             % partial f_(x,y,z) partial f_(x+1,y,z-1)
%             tmp_result = tmp_result + v(x+1,y,z-1)*...
%                 (-1)*(fx2 - f0) * (fx2 - fx2z1) * ( (fx2 - f0)^2 + (fx2 - fx2y1)^2 + (fx2 - fx2z1)^2 + epsilon )^-1.5;
%             
%             % partial f_(x,y,z) partial f_(x-1,y+1,z)
%             tmp_result = tmp_result + v(x-1,y+1,z)*...
%                 (-1)*(fy2 - f0) * (fy2 - fx1y2) * ( (fy2 - fx1y2)^2 + (fy2 - f0)^2 + (fy2 - fy2z1)^2 + epsilon )^-1.5;
%             
%             % partial f_(x,y,z) partial f_(x,y+1,z-1)
%             tmp_result = tmp_result + v(x,y+1,z-1)*...
%                 (-1)*(fy2 - f0) * (fy2 - fy2z1) * ( (fy2 - fx1y2)^2 + (fy2 - f0)^2 + (fy2 - fy2z1)^2 + epsilon )^-1.5;
%             
%             % partial f_(x,y,z) partial f_(x-1,y,z+1)
%             tmp_result = tmp_result + v(x-1,y,z+1)*...
%                 (-1)*(fz2 - f0) * (fz2 - fx1z2) * ( (fz2 - fx1z2)^2 + (fz2 - fy1z2)^2 + (fz2 - f0)^2 + epsilon )^-1.5;
%             
%             % partial f_(x,y,z) partial f_(x,y-1,z+1)
%             tmp_result = tmp_result + v(x,y-1,z+1)*...
%                 (-1)*(fz2 - f0) * (fz2 - fy1z2) * ( (fz2 - fx1z2)^2 + (fz2 - fy1z2)^2 + (fz2 - f0)^2 + epsilon )^-1.5;
%             
%             result(x,y,z) = tmp_result;
%         end;
%     end;
% end;

return;