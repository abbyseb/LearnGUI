function [I_recon, subsampled_kspace_data] = grappa(subsampled_kspace_data, header)
%function [I_recon, subsampled_kspace_data] = grappa(subsampled_kspace_data, header)
% MODIFICATION OF (Chiara):
% PULSAR for GRAPPA ver 1.1
%	
%	Jim Ji,	Texas A&M University, 2006
%   All rights reserved
%
% Usage			: Please refer to the manual, 'PULSAR USER MANUAL for GRAPPA.'
%
% References:   Generalized Autocalibrating Partially Parallel Acquisitions (GRAPPA), M. Griswold, Magnetic Resonance in Medicne, 47:1202-1210 (2002)


% Index of coils for GRAPPA reconstruction. Limited number of coils can be used for reconstruction. Moreover, it supports two ways of coil distributions: linear and circularly symmetric coils
if header.coil_type == 1
    for index = 1 : floor(header.num_coils / 2)
        COIL_index(index, :) = [max(1, index - floor((header.consider_coils-1) / 2)) : max(1, index - floor((header.consider_coils-1) / 2)) + header.consider_coils - 1];   
    end
    for index = header.num_coils : -1 : floor(header.num_coils / 2) + 1
        COIL_index(index, :) = fliplr([min(header.num_coils, index + floor(header.consider_coils / 2)) : -1 : min(header.num_coils, index + floor(header.consider_coils / 2)) - header.consider_coils + 1]);
    end
else
    for index = 1 : header.num_coils
        COIL_index(index, :) = 1 + mod([index - 1 - floor((header.consider_coils-1) / 2) : index - 1 - floor((header.consider_coils-1) / 2) + header.consider_coils - 1], header.num_coils);   
    end    
end    

% Index for reference blocks for GRAPPA reconstruction
index = 1;
while (1 - header.subsampling_factor + header.subsampling_factor * (max(1, 1 + index - floor(header.blocks / 2)) + header.blocks - 1) <= header.Npe)
    PE_index(index * header.subsampling_factor - header.subsampling_factor + 1:index * header.subsampling_factor, :) = repmat(1 - header.subsampling_factor + header.subsampling_factor * [max(1, 1 + index - floor(header.blocks / 2)) : max(1, 1 + index - floor(header.blocks / 2)) + header.blocks - 1] , [header.subsampling_factor  1]);
    index = index + 1;
end
for index = size(PE_index, 1) + 1:header.Npe
    PE_index(index, :) = PE_index(index - 1, :);
    PE_index(index, end) = header.Npe + 1;
end

% Two ways of applying Fourier transform is supported. 
% 1: 1D IFFT along FE -> GRAPPA Recon -> 1D IFFT along PE
% 2: GRAPPA Recon -> 2D IFFT
% In order to support the first option, 1D IFFT needs to be processed before GRAPPA reconstruction
if header.fft == 1
    for index = 1:header.num_coils
        subsampled_kspace_data(:,:,index) = ifftshift(ifft(ifftshift(subsampled_kspace_data(:,:,index),2), [], 2),2);
    end
end
if header.percent==100
    range=1:header.Nfe;
else
    range = ceil((100 - header.percent) * header.Nfe / 200):header.Nfe - ceil((100 - header.percent) * header.Nfe / 200);
end

% The core GRAPPA reconstruction code
I_recon = zeros(header.Npe, header.Nfe);
for z_index = 1:header.num_coils
%     disp(['coil = ' num2str(z_index)]);
    for index = 1:header.subsampling_factor - 1
        
        % Coefficient calculation using ACS lines
    	temp1 = subsampled_kspace_data(index + [header.temp(1):header.subsampling_factor:header.temp(length(header.temp) - 1)], range, z_index);
    	[x y z] = size(temp1);
        temp1 = reshape(temp1, 1, x*y*z); %ACS lines(:)
        temp2 = [];
        for window_index = range
        		for x_index = [header.temp(1):header.subsampling_factor:header.temp(length(header.temp) - 1)]
            		temp3 = subsampled_kspace_data(PE_index(x_index,:), window_index, COIL_index(z_index, :));
            		[x y z] = size(temp3);
            		temp2 = [temp2; reshape(temp3, 1, x*y*z)];
        		end
        end
        coefficient = pinv(temp2) * transpose(temp1);
        
        % GRAPPA  reconstruction with acquired coefficients.
    	  for y_index = 1: header.Nfe
            for x_index = 1:header.subsampling_factor:header.Npe-header.subsampling_factor+1
                if ismember(x_index + index, header.sampling_location) < 1 %is not sampled
                    temp5 = subsampled_kspace_data(PE_index(index + x_index, :), y_index, COIL_index(z_index, :));
                    [x y z] = size(temp5);
                    temp6 = reshape(temp5, 1, x*y*z);
                    subsampled_kspace_data(x_index + index, y_index, z_index) =  temp6 * coefficient;
                end
            end
        end
    end
    
    % Image reconstruction with Fourier transform
    if header.fft == 1
        kspace = fftshift(fft(subsampled_kspace_data(1:header.Npe, :, z_index), [], 2), 2);
        ima = abs(ifftshift(ifft(ifftshift(subsampled_kspace_data(1:header.Npe, :, z_index),1),[],1),1));
    else
        kspace = subsampled_kspace_data(1:header.Npe, :, z_index);
        ima = abs(ifftshift(ifft2(ifftshift(subsampled_kspace_data(1:header.Npe, :, z_index)))));
    end
    I_recon = I_recon + ima .^ 2;
end
I_recon = sqrt(I_recon);
return;