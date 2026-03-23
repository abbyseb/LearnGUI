function B = prcfilt3(A,siz,prcPos,padopt,CHUNKFACTOR)
%% B = prcfilt3(A,siz,prcPos,padopt,CHUNKFACTOR)
% ------------------------------------------
% FILE   : prcfilt3.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-10-05  Created.
% ------------------------------------------
% PURPOSE
%   Performs percentile filtering, which is similar to median filtering but
%   the output value can be at any percentile position within the filtering
%   window. Adapted from open source code "medfilt3.m".
% ------------------------------------------
% INPUT
%   A:          Input image.
%   siz:        Size of the filtering window.
%   prcPos:     The percentile position from 0 to 100.
%   padopt:     Padding option. See medfilt3.m for details.
%               'circular',' replicate' (default), or 'symmetric'
%   CHUNKFACTOR:Factor >1 to account for large arrays causing
%               memory issue. See medfilt3.m for details.
%
% OUTPUT
%   B:          The filtered image.

%% Note:
% If you work with large 3D arrays, an "Out of memory" error may appear.
% The chunk factor thus must be increased to reduce the size of the chunks.
if nargin~=5
    CHUNKFACTOR = 1;
end
if CHUNKFACTOR<1, CHUNKFACTOR = 1; end

%% Checking input arguments
if isscalar(A), B = A; return, end

if ndims(A)>3
    error('A must be a 1-D, 2-D or 3-D array.')
end

if all(isnan(A(:))), B = A; return, end

sizA = size(A);
if nargin==1
    % default kernel size is 3 or 3x3 or 3x3x3
    if isvector(A)
        siz = 3;
    else
        siz = 3*ones(1,numel(sizA));
    end
    padopt = 'replicate';
elseif nargin<=3
    % default padding option is "replicate"
    padopt = 'replicate';
end

%% Make SIZ a 3-element array
if numel(siz)==2
    siz = [siz 1];
elseif isscalar(siz)
    if sizA(1)==1
        siz = [1 siz 1];
    else
        siz = [siz 1 1];
    end
end

%% Chunks: the numerical process is split up in order to avoid large arrays
N = numel(A);
siz = ceil((siz-1)/2);
n = prod(siz*2+1);
if n==1, B = A; return, end
nchunk = (1:ceil(N/n/CHUNKFACTOR):N);
if nchunk(end)~=N, nchunk = [nchunk N]; end

%% Change to double if needed
class0 = class(A);
if ~isa(A,'float')
    A = double(A);
end

%% Padding along specified direction
% If PADARRAY exists (Image Processing Toolbox), this function is used.
% Otherwise the array is padded with scalars.
B = A;
sizB = sizA;
try
    A = padarray(A,siz,padopt);
catch
    if ~isscalar(padopt)
        padopt = 0;
        warning('MATLAB:medfilt3:InexistentPadarrayFunction',...
            ['PADARRAY function does not exist: '...
            'only scalar padding option is available.\n'...
            'If not specified, the scalar 0 is used as default.']);
    end
    A = ones(sizB+siz(1:ndims(B))*2)*padopt;
    A(siz(1)+1:end-siz(1),siz(2)+1:end-siz(2),siz(3)+1:end-siz(3)) = B;
end
sizA = size(A);

if numel(sizB)==2
    sizA = [sizA 1];
    sizB = [sizB 1];
end

%% Creating the index arrays (INT32)
inc = zeros([3 2*siz+1],'int32');
siz = int32(siz);
[inc(1,:,:,:) inc(2,:,:,:) inc(3,:,:,:)] = ndgrid(...
    [0:-1:-siz(1) 1:siz(1)],...
    [0:-1:-siz(2) 1:siz(2)],...
    [0:-1:-siz(3) 1:siz(3)]);
inc = reshape(inc,1,3,[]);

I = zeros([sizB 3],'int32');
sizB = int32(sizB);
[I(:,:,:,1) I(:,:,:,2) I(:,:,:,3)] = ndgrid(...
    (1:sizB(1))+siz(1),...
    (1:sizB(2))+siz(2),...
    (1:sizB(3))+siz(3));
I = reshape(I,[],3);

%% Check if NANMEDIAN exists
existNaNmedian = exist('nanmedian','file');

%% Filtering
for i = 1:length(nchunk)-1

    Im = repmat(I(nchunk(i):nchunk(i+1),:),[1 1 n]);
    Im = bsxfun(@plus,Im,inc);

    I0 = Im(:,1,:) +...
        (Im(:,2,:)-1)*sizA(1) +...
        (Im(:,3,:)-1)*sizA(1)*sizA(2);
    I0 = squeeze(I0);
    
    B(nchunk(i):nchunk(i+1)) = prctile(A(I0),prcPos,2);
%     if existNaNmedian
%         B(nchunk(i):nchunk(i+1)) = nanmedian(A(I0),2);
%     else
%         B(nchunk(i):nchunk(i+1)) = median(A(I0),2);
%     end
end
B = cast(B,class0);
    