function summary = cost_efficiency_optimization(currentInventory, resultsFolder)
% COST_EFFICIENCY_OPTIMIZATION Summarize current Qt inventory by material and unit.
%
% The relative score compares average efficiency with average unit cost only
% within the same metric unit. It is a screening measure, not a procurement
% recommendation or a comparison across unlike units.

requiredColumns = {'production_stream', 'material_type', 'product_name', ...
    'unit_cost_zar', 'efficiency_rate', 'metric_unit'};
if ~istable(currentInventory) || ...
        ~all(ismember(requiredColumns, currentInventory.Properties.VariableNames))
    error('Current inventory must contain material, unit cost, efficiency, and metric unit columns.');
end
if height(currentInventory) == 0
    error('Current inventory contains no items to compare.');
end

costs = currentInventory.unit_cost_zar;
efficiency = currentInventory.efficiency_rate;
materials = string(currentInventory.material_type);
streams = string(currentInventory.production_stream);
variants = string(currentInventory.product_name);
metricUnits = string(currentInventory.metric_unit);
streams(ismissing(streams) | strlength(strtrim(streams)) == 0) = "Unassigned";
variants(ismissing(variants) | strlength(strtrim(variants)) == 0) = "Unspecified variant";
metricUnits(ismissing(metricUnits) | strlength(strtrim(metricUnits)) == 0) = "Unspecified";
if ~isnumeric(costs) || ~isnumeric(efficiency) || ...
        any(~isfinite(costs)) || any(~isfinite(efficiency)) || ...
        any(costs <= 0) || any(efficiency < 0 | efficiency > 1) || ...
        any(ismissing(materials)) || any(strlength(strtrim(materials)) == 0)
    error('Current inventory contains invalid costs, efficiencies, or material categories.');
end

grouping = table(streams, materials, variants, metricUnits);
groups = unique(grouping, 'rows', 'stable');
summaryStream = groups.streams;
summaryMaterial = groups.materials;
summaryVariant = groups.variants;
summaryUnit = groups.metricUnits;
itemCounts = zeros(0, 1);
averageCosts = zeros(0, 1);
averageEfficiency = zeros(0, 1);
for m = 1:height(groups)
    idx = streams == summaryStream(m) & materials == summaryMaterial(m) & ...
        variants == summaryVariant(m) & metricUnits == summaryUnit(m);
    itemCounts(end + 1, 1) = sum(idx); %#ok<AGROW>
    averageCosts(end + 1, 1) = mean(costs(idx)); %#ok<AGROW>
    averageEfficiency(end + 1, 1) = mean(efficiency(idx)); %#ok<AGROW>
end

relativeScore = NaN(size(averageCosts));
highestInUnit = false(size(averageCosts));
comparableUnits = unique(summaryUnit(summaryUnit ~= "Unspecified"), 'stable');
for u = 1:numel(comparableUnits)
    unit = comparableUnits(u);
    idx = summaryUnit == unit;
    relativeScore(idx) = averageEfficiency(idx) ./ (averageCosts(idx) ./ max(averageCosts(idx)));
    localRows = find(idx);
    [~, winner] = max(relativeScore(idx));
    highestInUnit(localRows(winner)) = true;
end

summary = table(summaryStream, summaryMaterial, summaryVariant, summaryUnit, itemCounts, averageCosts, ...
    averageEfficiency, relativeScore, highestInUnit, ...
    'VariableNames', {'ProductionStream', 'Material', 'Variant', 'MetricUnit', 'InventoryItems', ...
    'AverageUnitCost_ZAR', 'AverageEfficiency', 'RelativeScoreWithinUnit', ...
    'HighestScoreInUnit'});

if ~isfolder(resultsFolder)
    mkdir(resultsFolder);
end
summaryFile = fullfile(resultsFolder, 'material_performance_summary.csv');
writetable(summary, summaryFile);

textFile = fullfile(resultsFolder, 'optimized_strategy.txt');
fileID = fopen(textFile, 'w');
if fileID == -1
    error('Could not create material comparison summary: %s', textFile);
end
cleanup = onCleanup(@() fclose(fileID));
fprintf(fileID, 'Qt current-inventory variant screening\n');
fprintf(fileID, 'Scores compare variants only within the same metric unit.\n');
fprintf(fileID, 'They are not procurement recommendations.\n\n');
for k = 1:height(summary)
    fprintf(fileID, '%s / %s / %s (%s): items=%d, average unit cost=R%.2f, average efficiency=%.2f%%', ...
        char(summary.ProductionStream(k)), char(summary.Material(k)), char(summary.Variant(k)), ...
        char(summary.MetricUnit(k)), ...
        summary.InventoryItems(k), summary.AverageUnitCost_ZAR(k), ...
        summary.AverageEfficiency(k) * 100);
    if isfinite(summary.RelativeScoreWithinUnit(k))
        fprintf(fileID, ', relative score=%.4f', summary.RelativeScoreWithinUnit(k));
        if summary.HighestScoreInUnit(k)
            fprintf(fileID, ' [highest in this unit]');
        end
    else
        fprintf(fileID, ', score unavailable (unit unspecified)');
    end
    fprintf(fileID, '\n');
end
clear cleanup;

fprintf('Material comparison exported to: %s\n', summaryFile);
fprintf('Unit-grouped screening summary exported to: %s\n', textFile);
end
