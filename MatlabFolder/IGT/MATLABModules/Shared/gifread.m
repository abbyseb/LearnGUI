function outpict=gifread(filepath,method,coalesce)
%   GIFREAD(FILEPATH, {METHOD}, {COALESCE})
%       reads all frames of an animated gif into a 4-D RGB image array
%       
%   FILEPATH: full path and filename
%   METHOD: file read method, 'native' or 'imagemagick' (default = 'native')
%       'imagemagick' method is a workaround for bug 813126 present in
%       R14SP3-2012a versions.  Bug consists of an OBOE in reading LCT data.
%       A patch does exist for these versions:
%       https://www.mathworks.com/support/bugreports/813126
%   COALESCE: 0 or 1, Specifies whether to coalesce the image sequence prior to
%       importing.  Used when loading optimized gifs. Requires imagemagick.
%       (optional, default 0)

if nargin<3;
    coalesce=0;
end
if nargin<2;
    method='native';
end


if coalesce==1
    system(sprintf('convert %s -layers coalesce /dev/shm/gifreadcoalescetemp.gif',filepath));
    filepath='/dev/shm/gifreadcoalescetemp.gif';
end

if strcmpi(method,'native')
    % use imread() directly (requires patched imgifinfo.m)
    [images map]=imread(filepath, 'gif','Frames','all');
    infostruct=imfinfo(filepath);

    s=size(images);
    numframes=s(4);

    outpict=zeros([s(1:2) 3 numframes],'uint8');
    for n=1:1:numframes;
        LCT=infostruct(1,n).ColorTable;
        outpict(:,:,:,n)=ind2rgb8(images(:,:,:,n),LCT);
    end
else
    % split the gif using imagemagick instead
    system(sprintf('convert %s /dev/shm/%%03d_gifreadtemp.gif',filepath));
    [~,numframes]=system('ls -1 /dev/shm/*gifreadtemp.gif | wc -l');

    numframes=str2num(numframes);
    [image map]=imread('/dev/shm/000_gifreadtemp.gif', 'gif');
    s=size(image);

    outpict=zeros([s(1:2) 3 numframes],'uint8');
    for n=1:1:numframes;
        [image map]=imread(sprintf('/dev/shm/%03d_gifreadtemp.gif',n-1), 'gif');
        outpict(:,:,:,n)=ind2rgb8(image,map);
    end

    system('rm /dev/shm/*gifreadtemp.gif');
end

if coalesce==1
    system(sprintf('rm %s',filepath));
end

return





