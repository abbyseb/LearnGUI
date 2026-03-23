function [noisedata] = addnoise(kdata, Receiver)
%input kspace data, Reference SNR
%-------------------------------------
%output=noisy kspace data
%------------------------------------

%function [noisedata] = addnoise(kdata, snr)
%this function generate a noise matrix (only) with the same size as kdata

amp_signal = norm(kdata(:))/sqrt(prod(size(kdata)));
%image = abs(ifftshift(ifft2(ifftshift(kdata))));
%peakimg = max(image(:));
sigman = amp_signal*power(10,-(Receiver.SNR/20)) / sqrt(2); % real and imaginary 

noisedata = normrnd(0,sigman,prod(size(kdata)),1) + sqrt(-1)*normrnd(0,sigman,prod(size(kdata)),1);
noisedata = reshape(noisedata,size(kdata));

return
