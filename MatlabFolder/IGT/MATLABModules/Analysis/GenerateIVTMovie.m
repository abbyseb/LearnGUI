function m = GenerateIVTMovie(imgFiles,R,projInfo)
%% m = GenerateIVTMovie(imgFiles,R,projInfo)
% ------------------------------------------
% FILE   : GenerateIVTMovie.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-08-23   Creation
% ------------------------------------------
% PURPOSE
%   Generate In-vivo tracking movies.
%
% ------------------------------------------
% INPUT
%   imgFiles:   The list of .png files from where the movie will be
%               generated. The list should be in cell format (can be
%               obtained from lscell). If left un-specified, all the image
%               files in the current folder will be used.
%   R:          The tracking result read and processed by
%               ReadCSVFileWithHeader and ProcessIVTResult. If left
%               unspecified, the program will ask the user for the
%               tracking file.
%   projInfo:   Projection size and spacing in the format of
%               [projSizeX,projSizeY;projSpaceX,projSpaceY].
%               If left unspecified, the system will ask the user for the
%               DataProperties.txt file.
% ------------------------------------------
% OUTPUT
%   m:          The output movie.
%
%% Input check

if nargin < 1
    imgFiles = lscell(fullfile(pwd,'*'));
end

if nargin < 2
    R = ProcessIVTResults(ReadCSVFileWithHeader);
end

if nargin < 3
    projInfoFile = uigetfilepath({'*.txt','IVT data property file (*.txt)'},...
        'Select the IVT data property file', ...
        pwd);
    fid = fopen(projInfoFile,'r');
    t = fgetl(fid);
    while ischar(t)
        if strfind(t,'Projection size')
            parts = regexp(t,'[','split');
            parts = regexp(parts{2},']','split');
            parts = regexp(parts{1},',','split');
            projInfo(1,1) = str2double(parts{1});
            projInfo(1,2) = str2double(parts{2});
        elseif strfind(t,'Projection spacings')
            parts = regexp(t,'[','split');
            parts = regexp(parts{2},']','split');
            parts = regexp(parts{1},',','split');
            projInfo(2,1) = str2double(parts{1});
            projInfo(2,2) = str2double(parts{2});
        end
        t = fgetl(fid);
    end
    fclose(fid);
end

%%

screensize = get( groot, 'Screensize' );
figuresize(3) = screensize(3) * 0.8;
figuresize(4) = screensize(4) * 0.6;
figuresize(1) = screensize(3) * 0.1;
figuresize(2) = screensize(4) * 0.2;
h = figure('InvertHardcopy','off','Color',[1 1 1],'Position',figuresize);

% Find n0 and nend first
[~,fileName] = fileparts(imgFiles{1});
n = regexp(fileName,'Frame','split');
n = str2double(n{2});
n = find(R.Frame == n);
n0 = n;
[~,fileName] = fileparts(imgFiles{end});
n = regexp(fileName,'Frame','split');
n = str2double(n{2});
n = find(R.Frame == n);
nend = n;

% Find ylim
maxy = max([R.IVTTrj(:,2);R.GTTrj(:,2)])+ 0.5;
miny = min([R.IVTTrj(:,2);R.GTTrj(:,2)]) - 0.5;

for k = 1:length(imgFiles)
    [~,fileName] = fileparts(imgFiles{k});
    n = regexp(fileName,'Frame','split');
    n = str2double(n{2});
    n = find(R.Frame == n);
    
    img = imread(imgFiles{k});
    cw = round(mean(size(img(:,:,1))) * 0.02);
    
%     % Tracking centroid
%     point = R.IVTProjTrj(n,1:2); point(2) = -point(2);
%     point = round((point./projInfo(2,:) ...
%         + (projInfo(1,:)+1)/2) ...
%         .* size(img(:,:,1)') ./ projInfo(1,:));
%     img(max(point(2)-cw,1):min(point(2)+cw,size(img,1)),point(1),:) = 255;
%     img(point(2),max(point(1)-cw,1):min(point(1)+cw,size(img,2)),:) = 255;
%     
%     % Prior centroid
%     point = R.PriorProjTrj(n,1:2); point(2) = -point(2);
%     point = round((point./projInfo(2,:) ...
%         + (projInfo(1,:)+1)/2) ...
%         .* size(img(:,:,1)') ./ projInfo(1,:));
%     img(max(point(2)-cw,1):min(point(2)+cw,size(img,1)),point(1),1) = 255;
%     img(point(2),max(point(1)-cw,1):min(point(1)+cw,size(img,2)),1) = 255;
%     img(max(point(2)-cw,1):min(point(2)+cw,size(img,1)),point(1),2:3) = 0;
%     img(point(2),max(point(1)-cw,1):min(point(1)+cw,size(img,2)),2:3) = 0;
%     
%     % Ground truth centroid
%     point = R.GTProjTrj(n,1:2); point(2) = -point(2);
%     point = round((point./projInfo(2,:) ...
%         + (projInfo(1,:)+1)/2) ...
%         .* size(img(:,:,1)') ./ projInfo(1,:));
%     img(max(point(2)-cw,1):min(point(2)+cw,size(img,1)),point(1),2) = 255;
%     img(point(2),max(point(1)-cw,1):min(point(1)+cw,size(img,2)),2) = 255;
%     img(max(point(2)-cw,1):min(point(2)+cw,size(img,1)),point(1),[1,3]) = 0;
%     img(point(2),max(point(1)-cw,1):min(point(1)+cw,size(img,2)),[1,3]) = 0;
    
    sub1 = subplot(2,1,1);
    image(img,'Parent',sub1); axis image; axis off;
    text(2,6,fileName,'color','blue','FontSize',14);
    subsize = get(sub1,'Position');
    subsize(2) = subsize(2) - subsize(4)*0.38;
    subsize(4) = subsize(4)*1.7;
    set(sub1, 'Position', subsize);
    
    axes1 = subplot(2,1,2);
    subsize = get(axes1,'Position');
    subsize(2) = subsize(2) - subsize(4)*0.0;
    subsize(4) = subsize(4)*0.8;
    set(axes1, 'Position', subsize);
    hold(axes1,'on');
    tp = plot(...
        ((R.Frame(n0):R.Frame(n)) - R.Frame(n0)) * 0.2,...
        R.IVTTrj(n0:n,2),'b','LineWidth',2,'Parent',axes1,'DisplayName','Markerless tumor tracking');
    gp = plot(...
        ((R.Frame(n0):R.Frame(n)) - R.Frame(n0)) * 0.2,...
        R.GTTrj(n0:n,2),'g','LineWidth',2,'Parent',axes1,'DisplayName','Ground truth');
    xlim(axes1,[0,(R.Frame(nend) - R.Frame(n0)) * 0.2]);
    xlabel('Time (seconds)');
    ylim(axes1,[miny maxy]);
    ylabel('SI motion (mm)');
    box(axes1,'on');
    set(axes1,'FontSize',14,'FontWeight','bold','LineWidth',2,'YMinorTick','on','YGrid','on');
    l = legend(axes1,'show');
    set(l,'Location','northeast');
    lsize = get( l, 'Position' );
    lsize(2) = lsize(2) + 1.4 * lsize(4);
    set(l,'Position',lsize);
    
    m(k) = getframe(h);
end
%close(h);