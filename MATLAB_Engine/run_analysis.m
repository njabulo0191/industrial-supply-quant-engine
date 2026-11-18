function run_analysis()
% RUN_ANALYSIS Analyze Qt event history and current-inventory data.

clc;
fprintf('\n========================================\n');
fprintf(' INDUSTRIAL SUPPLY QUANT ENGINE\n');
fprintf(' Qt inventory data analysis\n');
fprintf('========================================\n');

matlabFolder = fileparts(mfilename('fullpath'));
projectFolder = fileparts(matlabFolder);
addpath(matlabFolder, '-begin');
addpath(fullfile(matlabFolder, 'functions'), '-end');
resultsFolder = fullfile(matlabFolder, 'results');
if ~isfolder(resultsFolder)
    mkdir(resultsFolder);
end

historyFile = fullfile(projectFolder, 'historical_material_tracking.csv');
currentFile = fullfile(projectFolder, 'current_inventory.csv');
if ~isfile(historyFile) || ~isfile(currentFile)
    error(['Qt data files were not found. Capture or edit an item in the Qt app first. ' ...
        'Expected files: %s and %s'], historyFile, currentFile);
end

data = readtable(historyFile);
currentInventory = readtable(currentFile);
historyColumns = {'timestamp', 'action', 'item_id', 'production_stream', 'material_type', ...
    'product_name', 'unit_cost_zar', 'inventory_quantity', 'metric_unit', 'efficiency_rate'};
currentColumns = {'timestamp', 'item_id', 'production_stream', 'material_type', 'product_name', ...
    'unit_cost_zar', 'inventory_quantity', 'metric_unit', 'efficiency_rate', ...
    'annual_demand_metric_units', 'ordering_cost_zar_per_order', ...
    'holding_cost_zar_per_metric_unit_year'};
if ~all(ismember(historyColumns, data.Properties.VariableNames))
    error('Qt event-history CSV is missing required canonical columns. Re-run telemetry_processor.py.');
end
if ~all(ismember(currentColumns, currentInventory.Properties.VariableNames))
    error('Qt current-inventory CSV is missing required canonical columns. Re-run telemetry_processor.py.');
end
if height(data) == 0
    error('Qt event history is empty. Capture or edit an item in the Qt app first.');
end
if height(currentInventory) == 0
    error('Qt current inventory is empty. Capture an item in the Qt app first.');
end

validateTelemetryNumbers(data, 'event history');
validateTelemetryNumbers(currentInventory, 'current inventory');
currentInventory.annual_demand_metric_units = optionalNumericColumn(currentInventory.annual_demand_metric_units);
currentInventory.ordering_cost_zar_per_order = optionalNumericColumn(currentInventory.ordering_cost_zar_per_order);
currentInventory.holding_cost_zar_per_metric_unit_year = optionalNumericColumn(currentInventory.holding_cost_zar_per_metric_unit_year);
data.timestamp = parseQtTimestamps(data.timestamp);
currentInventory.timestamp = parseQtTimestamps(currentInventory.timestamp);
if any(isnat(data.timestamp)) || any(isnat(currentInventory.timestamp))
    error('The Qt CSV contains an invalid timestamp.');
end

data = sortrows(data, 'timestamp');
data.elapsed_minutes = minutes(data.timestamp - data.timestamp(1));

fprintf('\nDATA IMPORT SUCCESSFUL\n');
fprintf('Qt history events: %d\n', height(data));
fprintf('Current inventory items: %d\n', height(currentInventory));
fprintf('Production streams: %d\n', numel(unique(string(currentInventory.production_stream))));
fprintf('Material categories: %d\n', numel(unique(string(currentInventory.material_type))));
fprintf('Average recorded unit cost: R%.2f\n', mean(currentInventory.unit_cost_zar));
fprintf('Average recorded efficiency: %.2f%%\n', mean(currentInventory.efficiency_rate) * 100);
fprintf('Throughput: not recorded by the Qt form; no throughput result is calculated.\n');

validatedFile = fullfile(resultsFolder, 'validated_telemetry.csv');
writetable(data, validatedFile);
fprintf('Validated Qt event history exported to: %s\n', validatedFile);

fprintf('\nRUNNING VARIANT-LEVEL PRODUCTION REGRESSION...\n');
production_regression(data, resultsFolder);

fprintf('\nPREDICTING MATERIAL PRICES BY VARIANT...\n');
predict_material_prices(data, resultsFolder, 1);

fprintf('\nRUNNING EFFICIENCY PROCESS CONTROL...\n');
process_control(data, resultsFolder, 'efficiency_rate');

fprintf('\nCALCULATING VARIANT-LEVEL MOVING AVERAGES...\n');
writeVariantMovingAverages(data, resultsFolder, 3);

% EOQ is calculated only from the actual planning inputs entered in Qt.
eoqMatrix = [currentInventory.annual_demand_metric_units, ...
    currentInventory.ordering_cost_zar_per_order, ...
    currentInventory.holding_cost_zar_per_metric_unit_year];
eoqComplete = all(isfinite(eoqMatrix) & eoqMatrix > 0, 2);
eoqAny = any(isfinite(eoqMatrix) & eoqMatrix ~= 0, 2);
assignedStream = ismember(strtrim(string(currentInventory.production_stream)), ...
    ["PET", "RGB", "RefPET", "Can"]);
if any(eoqAny & ~eoqComplete & assignedStream)
    error('Some Qt items have incomplete EOQ inputs. Fill all three EOQ fields or leave all three blank.');
end
eoqEligible = eoqComplete & assignedStream;
fprintf('\nQt items with complete EOQ inputs and an assigned production stream: %d of %d\n', ...
    sum(eoqEligible), height(currentInventory));
fprintf('Unassigned legacy items excluded from EOQ: %d\n', sum(eoqComplete & ~assignedStream));
if any(eoqEligible)
    eoqItems = currentInventory(eoqEligible, :);
    materialLabel = makeEoqDisplayLabels(eoqItems.production_stream, ...
        eoqItems.material_type, eoqItems.product_name);
    eoqParameters = table(materialLabel, ...
        string(eoqItems.production_stream), string(eoqItems.material_type), ...
        string(eoqItems.product_name), ...
        eoqItems.annual_demand_metric_units, ...
        eoqItems.ordering_cost_zar_per_order, ...
        eoqItems.holding_cost_zar_per_metric_unit_year, ...
        'VariableNames', {'Material', 'ProductionStream', 'MaterialCategory', 'Variant', ...
        'AnnualDemand_metric_units', ...
        'OrderingCost_ZAR_per_order', 'HoldingCost_ZAR_per_metric_unit_year'});
    fprintf('RUNNING EOQ FROM QT ITEM INPUTS...\n');
    calculate_EOQ(eoqParameters, resultsFolder);
else
    staleEoqFiles = {fullfile(resultsFolder, 'eoq_results.csv'), ...
        fullfile(resultsFolder, 'eoq_analysis.png')};
    for k = 1:numel(staleEoqFiles)
        if isfile(staleEoqFiles{k})
            delete(staleEoqFiles{k});
        end
    end
    warning(['EOQ skipped: enter annual demand, ordering cost, and holding cost ' ...
        'for Qt items that need EOQ analysis. Old EOQ output files were removed.']);
end

fprintf('\nRUNNING CURRENT-INVENTORY MATERIAL COMPARISON...\n');
cost_efficiency_optimization(currentInventory, resultsFolder);

fprintf('\nAll available Qt-data analyses completed. Results are in:\n%s\n', resultsFolder);
end

function labels = makeEoqDisplayLabels(stream, category, variant)
baseLabels = strtrim(string(stream)) + " | " + strtrim(string(category)) + " | " + strtrim(string(variant));
labels = baseLabels;
for k = 1:numel(baseLabels)
    matchingRows = find(baseLabels == baseLabels(k));
    if numel(matchingRows) > 1
        occurrence = find(matchingRows == k, 1);
        labels(k) = baseLabels(k) + " (" + string(occurrence) + ")";
    end
end
end

function smoothedData = writeVariantMovingAverages(data, resultsFolder, windowSize)
streams = string(data.production_stream);
materials = string(data.material_type);
variants = string(data.product_name);
streams(ismissing(streams) | strlength(strtrim(streams)) == 0) = "Unassigned";
materials(ismissing(materials) | strlength(strtrim(materials)) == 0) = "Unspecified material";
variants(ismissing(variants) | strlength(strtrim(variants)) == 0) = "Unspecified variant";
groups = unique(table(streams,materials,variants), 'rows', 'stable');
n = height(data);
rowStream = strings(n,1); rowMaterial = strings(n,1); rowVariant = strings(n,1);
movingCost = NaN(n,1);
costs = double(data.unit_cost_zar);
for k = 1:height(groups)
    idx = streams == groups.streams(k) & materials == groups.materials(k) & variants == groups.variants(k);
    rows = find(idx); [~,order] = sort(data.timestamp(rows)); rows = rows(order);
    movingCost(rows) = moving_average(costs(rows),windowSize);
    rowStream(rows) = groups.streams(k);
    rowMaterial(rows) = groups.materials(k);
    rowVariant(rows) = groups.variants(k);
end
smoothedData = table(data.timestamp,rowStream,rowMaterial,rowVariant,costs,movingCost, ...
    'VariableNames',{'Timestamp','ProductionStream','Material','Variant', ...
    'UnitCost_ZAR','MovingAverageUnitCost_ZAR'});
outputFile = fullfile(resultsFolder,'moving_average.csv');
writetable(smoothedData,outputFile);
fprintf('Moving-average series saved to: %s\n',outputFile);
end

function validateTelemetryNumbers(data, label)
requiredNumeric = {'unit_cost_zar', 'efficiency_rate'};
for k = 1:numel(requiredNumeric)
    values = data.(requiredNumeric{k});
    if ~isnumeric(values) || any(~isfinite(values)) || any(ismissing(values))
        error('Invalid %s data in %s.', requiredNumeric{k}, label);
    end
end
if any(data.unit_cost_zar <= 0)
    error('Unit costs in %s must be positive.', label);
end
if any(data.efficiency_rate < 0 | data.efficiency_rate > 1)
    error('Efficiency values in %s must be between 0 and 1.', label);
end
if any(ismissing(string(data.material_type)) | strlength(strtrim(string(data.material_type))) == 0)
    error('Material categories in %s cannot be empty.', label);
end
end

function timestamps = parseQtTimestamps(values)
if isdatetime(values)
    timestamps = values;
else
    timestamps = datetime(string(values), 'InputFormat', 'yyyy-MM-dd HH:mm:ss.SSS');
end
end

function values = optionalNumericColumn(raw)
if isnumeric(raw)
    values = double(raw(:));
else
    values = str2double(string(raw(:)));
end
end
