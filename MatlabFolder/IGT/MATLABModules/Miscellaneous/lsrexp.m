function fl = lsrexp(fpath,rexp)
%% fl = lsrexp(fpath,rexp)
% ------------------------------------------
% FILE   : lsrexp.m
% AUTHOR : Andy Shieh, School of Physics, The University of Sydney
% DATE   : 2013-03-04  Created
% ------------------------------------------
% PURPOSE
%   This code list the files having the specified regular expression in the
%   specified path. This function is required because ITKRegularexpression
%   seems to perform differently on Linux, i.e. the file sequence is not
%   properly sorted.
%
%   lsrexp(fpath,rexp)
% ------------------------------------------
% INPUT
%   fpath:      The full path to the directory containing the files.
%   rexp:       The regular expression, e.g. 'SPI'.
% ------------------------------------------
% OUTPUT
%   fl:         The file list.
% ------------------------------------------

%% 

system(['rtkprintfiles -p ',fpath,' -r ',rexp,' -o tmpfl.txt']);

flid = fopen('tmpfl.txt','r');
NEXTLINE = fgetl(flid);
line = 1;
flline = textscan(NEXTLINE,'%c');
flline = flline{1}';
[pdir,fname,fext] = fileparts(flline);
fl(line,:) = [fname,fext];

% Get next line:
line = line + 1;
NEXTLINE = fgetl(flid);

while NEXTLINE ~= -1
    flline = textscan(NEXTLINE,'%c');
    flline = flline{1}';
    [pdir,fname,fext] = fileparts(flline);
    fl(line,:) = [fname,fext];
    line = line + 1;
    NEXTLINE = fgetl(flid);

end
fclose(flid);

delete('tmpfl.txt');

return;