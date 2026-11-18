function analysis = production_regression(eventHistory, resultsFolder)
%PRODUCTION_REGRESSION Summarize Qt material records by stream and exact variant.
% This project records inventory events, cost, and efficiency; it does not
% record actual production throughput. The output therefore reports only
% observed inventory and material trends.

required = {'timestamp','production_stream','material_type','product_name', ...
    'unit_cost_zar','inventory_quantity','metric_unit','efficiency_rate'};
if ~istable(eventHistory) || ~all(ismember(required,eventHistory.Properties.VariableNames))
    error('Event history is missing columns required for production analysis.');
end
if isempty(eventHistory)
    error('Event history is empty.');
end
if ~isfolder(resultsFolder), mkdir(resultsFolder); end

streams = fillText(string(eventHistory.production_stream), "Unassigned");
materials = fillText(string(eventHistory.material_type), "Unspecified material");
variants = fillText(string(eventHistory.product_name), "Unspecified variant");
units = fillText(string(eventHistory.metric_unit), "Unspecified");
groups = unique(table(streams,materials,variants,units), 'rows', 'stable');

productionStream = strings(height(groups),1);
materialType = strings(height(groups),1);
variant = strings(height(groups),1);
metricUnit = strings(height(groups),1);
observationCount = zeros(height(groups),1);
currentQuantity = NaN(height(groups),1);
averageUnitCost = NaN(height(groups),1);
costTrendPerMinute = NaN(height(groups),1);
averageEfficiency = NaN(height(groups),1);
rSquared = NaN(height(groups),1);

timestamps = asDatetime(eventHistory.timestamp);
quantity = numericColumn(eventHistory.inventory_quantity);
cost = numericColumn(eventHistory.unit_cost_zar);
efficiency = numericColumn(eventHistory.efficiency_rate);
for k = 1:height(groups)
    productionStream(k) = groups.streams(k);
    materialType(k) = groups.materials(k);
    variant(k) = groups.variants(k);
    metricUnit(k) = groups.units(k);
    idx = streams == groups.streams(k) & materials == groups.materials(k) & ...
        variants == groups.variants(k) & units == groups.units(k);
    rows = find(idx);
    [groupTimes, order] = sort(timestamps(rows));
    rows = rows(order);
    observationCount(k) = numel(rows);
    validQuantity = quantity(rows(isfinite(quantity(rows))));
    validCost = cost(rows(isfinite(cost(rows))));
    validEfficiency = efficiency(rows(isfinite(efficiency(rows))));
    if ~isempty(validQuantity), currentQuantity(k) = validQuantity(end); end
    if ~isempty(validCost), averageUnitCost(k) = mean(validCost); end
    if ~isempty(validEfficiency), averageEfficiency(k) = mean(validEfficiency); end
    x = minutes(groupTimes - groupTimes(1));
    y = cost(rows);
    valid = isfinite(x) & isfinite(y);
    x = x(valid); y = y(valid);
    if numel(unique(x)) >= 2
        coefficients = polyfit(x,y,1);
        costTrendPerMinute(k) = coefficients(1);
        fitted = polyval(coefficients,x);
        total = sum((y-mean(y)).^2);
        if total > eps
            rSquared(k) = 1 - sum((y-fitted).^2)/total;
        else
            rSquared(k) = 1;
        end
    end
end

analysis = table(productionStream,materialType,variant,metricUnit,observationCount, ...
    currentQuantity,averageUnitCost,costTrendPerMinute,averageEfficiency,rSquared, ...
    'VariableNames',{'ProductionStream','Material','Variant','MetricUnit', ...
    'ObservationCount','LatestRecordedQuantity','AverageUnitCost_ZAR', ...
    'UnitCostTrend_ZAR_per_minute','AverageEfficiency','CostTrend_R_squared'});
outputFile = fullfile(resultsFolder,'production_analysis.csv');
writetable(analysis,outputFile);
fprintf('Variant-level production/material summary saved to: %s\n',outputFile);
end

function values = fillText(values, fallback)
mask = ismissing(values) | strlength(strtrim(values)) == 0;
values(mask) = fallback;
end

function values = numericColumn(raw)
if isnumeric(raw), values = double(raw(:)); else, values = str2double(string(raw(:))); end
end

function values = asDatetime(raw)
if isdatetime(raw)
    values = raw(:);
else
    values = datetime(string(raw(:)), 'InputFormat','yyyy-MM-dd HH:mm:ss.SSS');
end
end
