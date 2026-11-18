function predictions = predict_material_prices(eventHistory, resultsFolder, forecastPeriods)
%PREDICT_MATERIAL_PRICES Forecast the next unit cost for each Qt variant.
% The forecast is a simple linear trend extrapolation. It requires at least
% two different timestamps for a stream/category/variant combination.
if nargin < 3 || isempty(forecastPeriods), forecastPeriods = 1; end
if ~isscalar(forecastPeriods) || ~isfinite(forecastPeriods) || forecastPeriods <= 0
    error('forecastPeriods must be a finite positive scalar.');
end
required = {'timestamp','production_stream','material_type','product_name','unit_cost_zar'};
if ~istable(eventHistory) || ~all(ismember(required,eventHistory.Properties.VariableNames))
    error('Event history is missing columns required for price prediction.');
end
if isempty(eventHistory), error('Event history is empty.'); end
if ~isfolder(resultsFolder), mkdir(resultsFolder); end

streams = fillText(string(eventHistory.production_stream),"Unassigned");
materials = fillText(string(eventHistory.material_type),"Unspecified material");
variants = fillText(string(eventHistory.product_name),"Unspecified variant");
groups = unique(table(streams,materials,variants),'rows','stable');
timestamps = asDatetime(eventHistory.timestamp);
costs = numericColumn(eventHistory.unit_cost_zar);
n = height(groups);
productionStream = strings(n,1); materialType = strings(n,1); variant = strings(n,1);
observationCount = zeros(n,1); lastObservedAt = NaT(n,1);
lastObservedCost = NaN(n,1); forecastHorizonMinutes = NaN(n,1);
predictedCost = NaN(n,1); trendPerMinute = NaN(n,1); status = strings(n,1);

for k = 1:n
    productionStream(k) = groups.streams(k);
    materialType(k) = groups.materials(k);
    variant(k) = groups.variants(k);
    idx = streams == groups.streams(k) & materials == groups.materials(k) & variants == groups.variants(k);
    rows = find(idx);
    [times,order] = sort(timestamps(rows)); rows = rows(order);
    y = costs(rows);
    valid = ~isnat(times) & isfinite(y);
    times = times(valid); y = y(valid);
    observationCount(k) = numel(y);
    if isempty(y)
        status(k) = "No valid prices";
        continue;
    end
    lastObservedAt(k) = times(end);
    lastObservedCost(k) = y(end);
    distinctMinutes = unique(minutes(times-times(1)));
    if numel(distinctMinutes) < 2
        status(k) = "Insufficient time history";
        continue;
    end
    stepMinutes = diff(distinctMinutes);
    stepMinutes = stepMinutes(stepMinutes > 0);
    if isempty(stepMinutes)
        status(k) = "Insufficient time history";
        continue;
    end
    horizon = median(stepMinutes) * forecastPeriods;
    x = minutes(times-times(1));
    coefficients = polyfit(x,y,1);
    forecastHorizonMinutes(k) = horizon;
    trendPerMinute(k) = coefficients(1);
    predictedCost(k) = max(0,polyval(coefficients,x(end)+horizon));
    status(k) = "Linear trend forecast";
end

predictions = table(productionStream,materialType,variant,observationCount,lastObservedAt, ...
    lastObservedCost,forecastHorizonMinutes,trendPerMinute,predictedCost,status, ...
    'VariableNames',{'ProductionStream','Material','Variant','ObservationCount', ...
    'LastObservedAt','LastObservedUnitCost_ZAR','ForecastHorizon_minutes', ...
    'Trend_ZAR_per_minute','PredictedUnitCost_ZAR','ForecastStatus'});
outputFile = fullfile(resultsFolder,'price_predictions.csv');
writetable(predictions,outputFile);
fprintf('Variant-level price forecasts saved to: %s\n',outputFile);
end

function values = fillText(values,fallback)
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
    values = datetime(string(raw(:)),'InputFormat','yyyy-MM-dd HH:mm:ss.SSS');
end
end
