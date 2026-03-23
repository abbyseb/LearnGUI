function [full_kspace_data,reduced_kspace_data, Sampling] = kspacedata_sim(Receiver,image,Sampling)

%-----------------------------------------------------
%input=coils sensitivity maps->sensitivity
%image data->image
%reduction/subsampling factor=subsampling_factor
%reference SNR=SNR
%-----------------------------------------------------
%output=full kspace/fourier data ->full_kspace_data
%undersampled data->reducd_kspace_data
%sampling locations along the phase encodign direction->sampling_location.
%-----------------------------------------------------
%INDIVIDUAL COIL IMAGES ARE OBTAINED BY MULTIPLYING IMAGE WITH EACH COIL
%MAP. FOURIER DATA IS THEN OBTAINED FROM THESE COIL IMAGES. SUBSAMPLING
%APPROPRIATELY GIVES THE REDUCED KSPACE DATA. CARE IS TAKEN TO ENSURE THAT
%THE CENTRAL DC PHASE ENCODING LINE IS COLLECTED NO MATTER WHAT THE
%subsampling_factor IS. REFERENCE SNR IS USED TO ADD NOISE USING THE
%addnoise.m.
%--------------------------------------------------------
% function [reduced_kspace_data, sampling_location] = kspacedata_sim(sensitivity,image,reduction_factor, SNR)
% simulate subsampled k-space data from each coil; reduction in the second
% direction
% sensitivity(Nfe,Npe,Num_coils)
% image(Nfe,Npe) is the whole image without sensitivity modulation
% reduction_factor: R, subsampling factor
% SNR: the signal-to-noise ratio, in dB

[Ro,C]=size(image);

% the sampling location is the subsampling location; need to take care of
% the even/odd reduction factor; make sure that the central k-space
% (C/2+1) is sampled and the subsmapled phase-encodings is an even number.
temp=Sampling.subsampling_factor;
tmp1 = (C/2+1-temp):(-temp):1;
tmp2 = (C/2+1):Sampling.subsampling_factor:C;
Sampling.sampling_location = [flipud(tmp1(:)); tmp2(:)]; 
if length(tmp1)>length(tmp2)
    Sampling.sampling_location=Sampling.sampling_location(2:length(Sampling.sampling_location));
    else if length(tmp1)<length(tmp2)
        Sampling.sampling_location=Sampling.sampling_location(1:length(Sampling.sampling_location)-1);
    end
end

% disp('a.coil image(i)=image*coil_map(i)');
% disp('b.full k_space (add noise: according to SNR)');
% disp('c.reduced k-space: full k-space with subsamplig factor');
for coil = 1:size(Receiver.sensitivity,3)
    surfimag = image.* Receiver.sensitivity(:,:,coil); 
%     figure;imshow(surfimag,[]);
    full_kspace_data(:,:,coil) = fftshift(fft2(fftshift(surfimag)));
    full_kspace_data(:,:,coil)=full_kspace_data(:,:,coil) + addnoise(full_kspace_data(:,:,coil), Receiver); %add noise gaussian random noise
    reduced_kspace_data(:,:,coil) = full_kspace_data(:,Sampling.sampling_location,coil);
end

return