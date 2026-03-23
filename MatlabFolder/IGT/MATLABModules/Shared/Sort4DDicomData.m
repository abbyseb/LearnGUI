%% Sort Nepean 4DCT & SPECT datasets (provided by John)

% Set the tag values of interest:
Extension = 'dcm';
ModalityTagName = 'Modality';
SortTagName = 'SeriesDescription'; % or 'TriggerTime';

fn = dir(['*.' Extension]);
ModalityTagValues = cell(length(fn),1);
SortTagValues = cell(length(fn),1);
for count = 1:length(fn)
info = dicominfo(fn(count).name);
if (isfield(info,ModalityTagName))
ThisModalityTag = eval(sprintf('info.%s',ModalityTagName));
ModalityTagValues{count}=ThisModalityTag;
else
ModalityTagValues{count}='NA';
end
if (isfield(info,SortTagName))
ThisSortTag = eval(sprintf('info.%s',SortTagName));
SortTagValues{count}=ThisSortTag;
else
SortTagValues{count}='NA';
end
count
end
UniqueModalityValues = unique(ModalityTagValues,'stable')
UniqueSortValues = unique(SortTagValues,'stable')


% Now sort everything into folders by modality:
for sortfile = 1:length(fn)
ModalityNumber = find(ismember(UniqueModalityValues,ModalityTagValues{sortfile}));
SortNumber = find(ismember(UniqueSortValues,SortTagValues{sortfile}));
if isdir([pwd '/' UniqueModalityValues{ModalityNumber} sprintf('%.2i',SortNumber)])
movefile([pwd '/' fn(sortfile).name],[pwd '/' UniqueModalityValues{ModalityNumber} sprintf('%.2i',SortNumber) '/' fn(sortfile).name],'f');
else
mkdir([pwd '/' UniqueModalityValues{ModalityNumber} sprintf('%.2i',SortNumber)]);
movefile([pwd '/' fn(sortfile).name],[pwd '/' UniqueModalityValues{ModalityNumber} sprintf('%.2i',SortNumber) '/' fn(sortfile).name],'f');
end
end