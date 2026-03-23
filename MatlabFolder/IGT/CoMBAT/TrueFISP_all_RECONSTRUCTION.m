
% clc;
% clear all;
% close all;
% startup;

Inputs_List_grappa;

for bin=1:num_bin
    disp(['frame: ' num2str(bin)]);
    Files.current_image=Files.image{bin,1};
    for slice=1:size(Files.current_image,3)
        disp(['slice: ' num2str(slice)]);
        Files.data=Files.current_image(:,:,slice);

         %------------------------------------
         %kspace data collection
%          disp('Loading files and generating kspace data...');
         [full_kspace_data,reduced_kspace_data,Sampling, Image_size,Receiver]...
             =get_kspace_data(Files,Receiver,Sampling,Image_size) ;
%          save full_kspace_data full_kspace_data

% disp('DOOOOONNNNEEEE');

         %------------------------------------
         % image reconstruction
         recon_new; %based on kspace_subsample and grappa  
         
          I_END{bin,1}(:,:,slice)=I_recon;
%          k_space_end{bin,1}(:,:,:,slice)=subsampled_full_kspace_data;
%         I_END(:,:,bin)=I_recon;
%          k_space_end{bin,1}(:,:,:,slice)=subsampled_full_kspace_data;
    end
end

% I_END_all=I_END;
% figure;imshow(I_END{1,1}(:,:,100),[]);

% save I_END_all I_END_all -v7.3
% save k_space_end k_space_end
         
         
         
         
         
         
