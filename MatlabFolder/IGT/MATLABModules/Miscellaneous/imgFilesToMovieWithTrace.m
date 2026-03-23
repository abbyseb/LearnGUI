function frames = imgFilesToMovieWithTrace(fileList,xData,yData,xLabelText,yLabelText,figSize,yRanges)
%% frames = mgFilesToMovieWithTrace(fileList,xData,yData,xLabelText,yLabelText,figSize,yRanges)
% ------------------------------------------
% FILE   : imgFilesToMovieWithTrace.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2017-11-09   Creation
% ------------------------------------------
% PURPOSE
%   Convert a list of image files (.png, .jpg...etc, formats that can be
%   read by imread) to a MATLAB movie with trace.
%
% ------------------------------------------
% INPUT
%   fileList:     The list of image files in cell format. (See "lscell.m")
%                 If left in specified, the program will automatically use
%                 all the files in the current directory to create the
%                 movie.
%   xData:        Data plotted as the x-axis of the trace. Can be in cell
%                 format for multiple trace subplot.
%   yData:        Dara plotted as the y-axis of the trace. Can be in cell
%                 format for multiple trace subplot.
%   xLabelText:   x-axis label as string. Can be in cell format for 
%                 multiple trace subplot.
%   yLabelText:   y-axis label as string. Can be in cell format for
%                 multiple trace subplot.
%   figSize:      Normalized figure position.
%   yRanges:      The lower and upper bounds for yData, Can be in cell
%                 format for multiple trace subplot.
% ------------------------------------------
% OUTPUT
%   frames:       The output MATLAB movie.

%%

if nargin < 3
    error('ERROR: fileList, xData, and yData must be provided.');
end;

if nargin < 6
    figSize = [0, 0, 1, 1];
end

if nargin < 7
    yRanges = [];
end;

if iscell(xData)
    for k = 1:length(xData)
        xMin{k} = min(xData{k});  xMax{k} = max(xData{k});
        if isempty(yRanges)
            yMin{k} = min(yData{k});  yMax{k} = max(yData{k});
            yCen = 0.5 * yMin{k} + 0.5 * yMax{k};
            yMin{k} = yCen - 1.1 * abs(yCen - yMin{k});
            yMax{k} = yCen + 1.1 * abs(yCen - yMax{k});
        else
            yMin{k} = yRanges{k}(1);    yMax{k} = yRanges{k}(2);
        end
    end
else
    xMin{1} = min(xData);  xMax{1} = max(xData);
    if isempty(yRanges)
        yMin{1} = min(yData);  yMax{1} = max(yData);
        yCen = 0.5 * yMin{1} + 0.5 * yMax{1};
        yMin{1} = yCen - 1.1 * abs(yCen - yMin{1});
        yMax{1} = yCen + 1.1 * abs(yCen - yMax{1});
    else
        yMin{1} = yRanges(1);    yMax{1} = yRanges(2);
    end
end

N = length(fileList);
if isempty(fileList)
    if iscell(xData)
        N = length(xData{1});
    else
        N = length(xData);
    end
end

h = figure('InvertHardcopy','off','Color',[1 1 1],'units','normalized','outerposition',figSize);
for k = 1:N
    if isempty(fileList)
        img = 0;
    else
        img = imread(fileList{k});
    end
    figure(h);
    if iscell(xData)
        subplot(1+length(xData),1,1); image(img); axis image; axis off;
        for n = 1:length(xData)
            subplot(1+length(xData),1,1+n); plot(xData{n}(1:k),yData{n}(1:k),'b');
            xlabel(xLabelText{n}); ylabel(yLabelText{n});
            xlim([xMin{n}, xMax{n}]);
            ylim([yMin{n}, yMax{n}]);
            set(findall(gca, 'Type', 'Line'),'LineWidth',2);
        end
    else
        subplot(2,1,1); image(img); axis image; axis off;
        subplot(2,1,2); plot(xData(1:k),yData(1:k),'b');
        xlabel(xLabelText); ylabel(yLabelText);
        xlim([xMin{1}, xMax{1}]);
        ylim([yMin{1}, yMax{1}]);
        set(findall(gca, 'Type', 'Line'),'LineWidth',2);
    end
    frames(k) = getframe(h);
end

close(h);