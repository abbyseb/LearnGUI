function [full_kspace_data,reduced_kspace_data,Sampling,Image_size,Receiver]...
         =get_kspace_data(Files,Receiver,Sampling,Image_size)


    image=Files.data;
        [Image_size.FEs, Image_size.PEs]=size(image);

%--------------------------------------------------------------------------      
%sensitivity generation 
%                 disp('coil sensitivity simulation: normal probability density function...')!!!!!;
                Receiver.max_sensitivity=1;
                sigma=Image_size.PEs/Receiver.num_coils*ones(Receiver.num_coils,1)/(2-(Receiver.variance_factor-1)/10); 
                %centers of the coil
                Receiver.center = [Image_size.PEs/Receiver.num_coils/2:Image_size.PEs/Receiver.num_coils:Image_size.PEs]; 
                %generate gaussian sensitivity profiles
%                 [profiles]=profile1d_lp(sigma,Image_size, Receiver); 
%                 sensitivity_temp = repmat(profiles,[1,1,Image_size.FEs]); 
%                 Receiver.sensitivity = permute(sensitivity_temp,[3 1 2]);%coil sensitivities
                Receiver.sensitivity=sensitivy_map([Image_size.FEs, Image_size.PEs],Receiver.num_coils); %coil sensitivities
                clear sensitivity_temp;

%--------------------------------------------------------------------------
[full_kspace_data, reduced_kspace_data, Sampling]=kspacedata_sim(Receiver, image, Sampling);  
save parameters;
return;

