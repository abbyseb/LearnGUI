function [hProjTrj,hBG,hError] = PlotIVTResults(inStruct)
%% [hProjTrj,hBG,hError] = PlotIVTResults(inStruct)
% ------------------------------------------
% FILE   : PlotIVTResults.m
% AUTHOR : Andy Shieh, The University of Sydney
% DATE   : 2016-08-12   Creation
% ------------------------------------------
% PURPOSE
%   Plot in-vivo tracking results.
%
% ------------------------------------------
% INPUT
%   inStruct:        The input struct read from the In-vivo Tracking result
%                    .csv file.
% ------------------------------------------
% OUTPUT
%   hProjTrj:        The figure handle to the projection-space trajectory
%                    plot.
%   hBG:             The figure handle to the background-related plot.
%   hError:          The figure handle to the error-related plot.

%%
screensize = get( 0, 'Screensize' );

hProjTrj = figure('Position',[0,0,screensize(3)/3.05,screensize(4)*0.8]);
movegui(hProjTrj,'west');

for k = 1:3
    subplot(3,1,k); plot(inStruct.Frame,inStruct.IVTProjTrj(:,k),'b');
    hold on;
    subplot(3,1,k); plot(inStruct.Frame,inStruct.PriorProjTrj(:,k),'r');
    subplot(3,1,k); plot(inStruct.Frame,inStruct.GTProjTrj(:,k),'g');
    legend('Tracking','Prior','Reference');
    switch k
        case 1,
            title('Lateral trajectory');
        case 2,
            title('SI trajectory');
        case 3,
            title('Ray-direction trajectory');
    end
end

hBG = figure('Position',[0,0,screensize(3)/3.2,screensize(4)*0.8]);
movegui(hBG,'center');

for k = 1:2
    subplot(3,1,k); plot(inStruct.Frame,inStruct.BGShift(:,k),'b');
    hold on;
    subplot(3,1,k); plot(inStruct.Frame,...
        inStruct.IVTProjTrj(:,k) - inStruct.PriorProjTrj(:,k),'r');
    legend('BG shift','Tracking-Prior');
    switch k
        case 1,
            title('BG shift u');
        case 2,
            title('BG shift v');
    end
end
subplot(3,1,3); plot(inStruct.Frame,inStruct.BGMatchScore,'b');
title('BG match score');

hError = figure('Position',[0,0,screensize(3)/3.2,screensize(4)*0.8]);
movegui(hError,'east');

subplot(4,1,1); plot(inStruct.Frame,inStruct.IVTKVErrorNorms,'b');
hold on;
subplot(4,1,1); plot(inStruct.Frame,inStruct.PriorKVErrorNorms,'r');
legend('Tracking','Prior');
title('kV-plane errors');
subplot(4,1,2); plot(inStruct.Frame,inStruct.IVTMVErrorNorms,'b');
hold on;
subplot(4,1,2); plot(inStruct.Frame,inStruct.PriorMVErrorNorms,'r');
legend('Tracking','Prior');
title('MV-plane errors');
subplot(4,1,3); plot(inStruct.Frame,...
    abs(inStruct.IVTProjTrj(:,2) - inStruct.GTProjTrj(:,2)),'b');
hold on;
subplot(4,1,3); plot(inStruct.Frame,...
    abs(inStruct.PriorProjTrj(:,2) - inStruct.GTProjTrj(:,2)),'r');
legend('Tracking','Prior');
title('SI errors');
subplot(4,1,4); plot(inStruct.Frame,inStruct.TMMetric,'b');
hold on;
subplot(4,1,4); plot(inStruct.Frame,inStruct.BGMatchScore,'r');
legend('Target match','BG match');
title('Matching score');

return;