function frames = imgFilesToMovie(fileList,addText)
%% frames = imgFilesToMovie(fileList,addText)
% ------------------------------------------
% FILE   : imgFilesToMovie.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-07-14   Creation
% ------------------------------------------
% PURPOSE
%   Convert a list of image files (.png, .jpg...etc, formats that can be
%   read by imread) to a MATLAB movie.
%
% ------------------------------------------
% INPUT
%   fileList:     The list of image files in cell format. (See "lscell.m")
%                 If left in specified, the program will automatically use
%                 all the files in the current directory to create the
%                 movie.
%   addText:      A boolean tag specifying whether a file name text label
%                 is added to the output movie. Default is true.
% ------------------------------------------
% OUTPUT
%   frames:       The output MATLAB movie variable.

%%

if nargin < 1
    fileList = lscell(fullfile(pwd,'*'));
end

if nargin < 2
    addText = true;
end

h = figure('InvertHardcopy','off','Color',[1 1 1]);
for k = 1:length(fileList)
    img = imread(fileList{k});
    figure(h);
    image(img); axis image; axis off;
    if addText
        [~,fileName] = fileparts(fileList{k});
        text(2,6,fileName,'color','blue','FontSize',14);
    end
    frames(k) = getframe;
end

close(h);