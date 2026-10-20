function optimize_costs()
% MATLAB Optimization Script for Supply Chain Analysis
filename = 'industrial_supply_telemetry.csv';

% Check if the Python telemetry data file exists
if ~isfile(filename)
    error('❌ Error: %s not found. Run the Python simulator first to generate data.', filename);
end

fprintf('📊 Ingesting CSV telemetry data into MATLAB...\n');

% Ingest the data as a table structure
opts = detectImportOptions(filename);
data = readtable(filename, opts);

% Extract critical telemetry arrays for matrix calculations
costs = data.commodity_cost_zar;
efficiency = data.efficiency_rate;
materials = data.material_type;

% Mathematical Modeling: Performance Score Matrix
% Formula: Score = Efficiency / Normalized Cost
% Higher efficiency and lower cost maximize this operational vector
normalized_costs = costs / max(costs);
performance_scores = efficiency ./ normalized_costs;

% Find the absolute mathematical maximum threshold
[maxScore, optimalIdx] = max(performance_scores);

% Display calculated optimization metrics to terminal
fprintf('\n==================================================\n');
fprintf('💡 MATHEMATICAL OPTIMIZATION ENGINE RESULTS:\n');
fprintf('==================================================\n');
fprintf('Total Logs Evaluated   : %d rows\n', height(data));
fprintf('Optimal Material Strategy: %s\n', materials{optimalIdx});
fprintf('Optimized Cost Factor   : R%.2f\n', costs(optimalIdx));
fprintf('System Operational Efficiency: %.2f%%\n', efficiency(optimalIdx) * 100);
fprintf('Peak Performance Matrix Index: %.4f\n', maxScore);
fprintf('==================================================\n');

% Output the optimized matrix profile to a simple bridge file for C++
fileID = fopen('optimized_strategy.txt', 'w');
fprintf(fileID, '%s\n%.2f\n%.4f', materials{optimalIdx}, costs(optimalIdx), efficiency(optimalIdx));
fclose(fileID);
fprintf('✅ Optimization matrix successfully exported to optimized_strategy.txt\n');
end
