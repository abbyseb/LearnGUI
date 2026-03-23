% PULSAR for GRAPPA ver 1.0


% PARAMETERS TO CHANGE

% k-space datafile selection. The dimension of matrix in the file is [PE, FE, COIL]. The file should include only one matrix and the name of matrix in the file can be arbitrary.

[header.Npe, header.Nfe, header.num_coils] = size(full_kspace_data);

% Acceleration factor for GRAPPA reconstruction
% header.subsampling_factor = input('Subsampling factor? [default: 2] ');
% if (isempty(header.subsampling_factor))
		header.subsampling_factor = 2;
% end

% Number of reference blocks for GRAPPA reconstruction
% header.blocks = input('Block size? [default: 4] ');
% if (isempty(header.blocks))
		header.blocks = 2;
% end

% Number of ACS lines (AutoCalibratingSignal)
% header.central_kspace=input(['Number of ACS lines? [default: 32]']);
% if (isempty(header.central_kspace))
		header.central_kspace = 32;
% end

% Number of coils used for GRAPPA reconstruction
% header.consider_coils = input( ['Number of coils for GRAPPA reconstruction? [default: ' num2str(header.num_coils) '] ' ]);
% if (isempty(header.consider_coils))
		header.consider_coils = header.num_coils;
% end

% Type of coil's geometric distribution
% disp('Type of coil distribution?');
% header.coil_type = input( ['1: Linear, 2:circularly symmetric coils [default: 2] ' ]);
% if (isempty(header.coil_type))
		header.coil_type = 2;
% end

% The method of applying Fourier transform
% disp('The method of applying Fourier transform? ');
% disp('1: 1D IFFT along FE -> GRAPPA Recon -> 1D IFFT along PE ');
% header.fft = input( ['2: GRAPPA Recon -> 2D IFFT [default]']);
% if (isempty(header.fft))
		header.fft = 2;
% end

% The percentage of k-space along frequency-encoding direction to be used for fitting
% header.percent = input( ['The percentage of k-space along frequency-encoding direction to be used for fitting [default:90]']);disp(' ');
% if (isempty(header.percent))
    header.percent = 65;
% end

% Subsampling k-space data along row-direction
[subsampled_full_kspace_data, header] = kspace_subsample(full_kspace_data, header);

% return

% GRAPPA reconstruction
[I_recon, subsampled_full_kspace_data]  = grappa(subsampled_full_kspace_data, header);

