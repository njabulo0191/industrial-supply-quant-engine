function controlResults = process_control(eventHistory, resultsFolder, metricName)
%PROCESS_CONTROL Build Individuals-chart limits for a selected Qt metric.
% Default metric is efficiency_rate. Each stream/material/variant receives
% separate limits. Sparse groups are retained with NaN limits.
if nargin < 2, resultsFolder = ''; end
if nargin < 3 || isempty(metricName), metricName = 'efficiency_rate'; end
metricName = char(metricName);
required={'timestamp','production_stream','material_type','product_name',metricName};
if ~istable(eventHistory) || ~all(ismember(required,eventHistory.Properties.VariableNames))
    error('Event history is missing the selected process-control metric or grouping columns.');
end
streams=fillText(string(eventHistory.production_stream),"Unassigned");
materials=fillText(string(eventHistory.material_type),"Unspecified material");
variants=fillText(string(eventHistory.product_name),"Unspecified variant");
groups=unique(table(streams,materials,variants),'rows','stable');
times=asDatetime(eventHistory.timestamp);
values=numericColumn(eventHistory.(metricName));
n=height(eventHistory);
productionStream=strings(n,1); materialType=strings(n,1); variant=strings(n,1);
metric=strings(n,1); observedAt=times; observedValue=values;
centerLine=NaN(n,1); lowerLimit=NaN(n,1); upperLimit=NaN(n,1); outOfControl=false(n,1);
for k=1:height(groups)
    idx=streams==groups.streams(k) & materials==groups.materials(k) & variants==groups.variants(k);
    rows=find(idx); [~,order]=sort(times(rows)); rows=rows(order);
    y=values(rows); valid=isfinite(y);
    if sum(valid)>=2
        validRows=rows(valid); validY=y(valid);
        mr=abs(diff(validY)); sigma=mean(mr)/1.128;
        cl=mean(validY); lcl=cl-3*sigma; ucl=cl+3*sigma;
        if strcmp(metricName,'efficiency_rate')
            lcl=max(0,lcl); ucl=min(1,ucl);
        elseif strcmp(metricName,'unit_cost_zar')
            lcl=max(0,lcl);
        end
        centerLine(validRows)=cl; lowerLimit(validRows)=lcl; upperLimit(validRows)=ucl;
        outOfControl(validRows)=validY<lcl | validY>ucl;
    end
    productionStream(rows)=groups.streams(k);
    materialType(rows)=groups.materials(k);
    variant(rows)=groups.variants(k);
    metric(rows)=string(metricName);
end
controlResults=table(productionStream,materialType,variant,metric,observedAt,observedValue, ...
    centerLine,lowerLimit,upperLimit,outOfControl, ...
    'VariableNames',{'ProductionStream','Material','Variant','Metric','Timestamp', ...
    'ObservedValue','CenterLine','LowerControlLimit','UpperControlLimit','OutOfControl'});
if ~isempty(resultsFolder)
    if ~isfolder(resultsFolder), mkdir(resultsFolder); end
    writetable(controlResults,fullfile(resultsFolder,'process_control.csv'));
end
end

function values=fillText(values,fallback)
mask=ismissing(values)|strlength(strtrim(values))==0; values(mask)=fallback;
end
function values=numericColumn(raw)
if isnumeric(raw), values=double(raw(:)); else, values=str2double(string(raw(:))); end
end
function values=asDatetime(raw)
if isdatetime(raw), values=raw(:); else, values=datetime(string(raw(:)),'InputFormat','yyyy-MM-dd HH:mm:ss.SSS'); end
end
