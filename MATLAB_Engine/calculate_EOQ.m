
function eoqResults = calculate_EOQ(parameters, resultsFolder)

% CALCULATE_EOQ
% Economic Order Quantity calculations for industrial materials.
%
% Inputs:
%   parameters   - Table containing demand, ordering and holding costs
%   resultsFolder - Results output directory
%
% Output:
%   eoqResults - Calculated EOQ results

    fprintf('\n========================================\n');
    fprintf(' ECONOMIC ORDER QUANTITY ANALYSIS\n');
    fprintf('========================================\n');

    % Required input columns
    requiredColumns = {
        'Material'
        'AnnualDemand_metric_units'
        'OrderingCost_ZAR_per_order'
        'HoldingCost_ZAR_per_metric_unit_year'
    };

    % Validate table
    if ~istable(parameters)
        error('EOQ input must be a MATLAB table.');
    end

    if ~all(ismember(requiredColumns, ...
            parameters.Properties.VariableNames))
        error('EOQ input table is missing required columns.');
    end

    if height(parameters) == 0
        error('EOQ input table is empty.');
    end

    % Extract input values
    material = string(parameters.Material);

    D = parameters.AnnualDemand_metric_units;
    S = parameters.OrderingCost_ZAR_per_order;
    H = parameters.HoldingCost_ZAR_per_metric_unit_year;

    % Validate material names. Repeated variants are valid; the runner adds
    % a short numeric suffix when needed to keep chart labels distinguishable.
    if any(ismissing(material)) || ...
            any(strlength(strtrim(material)) == 0)
        error('Material names cannot be empty.');
    end

    % Validate numerical inputs
    if ~isnumeric(D) || ~isnumeric(S) || ~isnumeric(H)
        error('EOQ parameters must be numeric.');
    end

    if ~isvector(D) || ~isvector(S) || ~isvector(H) || ...
            numel(D) ~= height(parameters) || ...
            numel(S) ~= height(parameters) || ...
            numel(H) ~= height(parameters)
        error('EOQ numerical input dimensions are incorrect.');
    end

    if any(~isfinite(D)) || ...
       any(~isfinite(S)) || ...
       any(~isfinite(H))
        error('EOQ input contains invalid numerical values.');
    end

    if any(D <= 0) || any(S <= 0) || any(H <= 0)
        error('Demand, ordering cost and holding cost must be positive.');
    end

    % Calculate economic order quantity
    EOQ = sqrt((2 .* D .* S) ./ H);

    % Calculate number of orders per year
    numberOfOrders = D ./ EOQ;

    % Annual ordering cost
    annualOrderingCost = numberOfOrders .* S;

    % Annual holding cost
    annualHoldingCost = (EOQ ./ 2) .* H;

    % Relevant annual inventory cost
    totalInventoryCost = ...
        annualOrderingCost + annualHoldingCost;

    % Carry the Qt stream/category/variant fields through to the report.
    productionStream = optionalTextColumn(parameters, 'ProductionStream', height(parameters));
    materialCategory = optionalTextColumn(parameters, 'MaterialCategory', height(parameters));
    variant = optionalTextColumn(parameters, 'Variant', height(parameters));

    % Construct and rank results, largest order quantities first.
    eoqResults = table( ...
        material, productionStream, materialCategory, variant, ...
        D, ...
        S, ...
        H, ...
        EOQ, ...
        numberOfOrders, ...
        annualOrderingCost, ...
        annualHoldingCost, ...
        totalInventoryCost, ...
        'VariableNames', { ...
            'Material', ...
            'ProductionStream', ...
            'MaterialCategory', ...
            'Variant', ...
            'AnnualDemand_metric_units', ...
            'OrderingCost_ZAR', ...
            'HoldingCost_ZAR', ...
            'EOQ_metric_units', ...
            'Orders_per_year', ...
            'AnnualOrderingCost_ZAR', ...
            'AnnualHoldingCost_ZAR', ...
            'TotalInventoryCost_ZAR' ...
        });
    eoqResults = sortrows(eoqResults, 'EOQ_metric_units', 'descend');

    % Display results
    for k = 1:height(eoqResults)

        fprintf('\nMaterial: %s\n', eoqResults.Material(k));

        fprintf('Annual demand: %.2f metric units/year\n', eoqResults.AnnualDemand_metric_units(k));

        fprintf('Economic Order Quantity: %.2f metric units\n', eoqResults.EOQ_metric_units(k));

        fprintf('Orders per year: %.2f\n', eoqResults.Orders_per_year(k));

        fprintf('Annual ordering cost: R%.2f\n', ...
            eoqResults.AnnualOrderingCost_ZAR(k));

        fprintf('Annual holding cost: R%.2f\n', ...
            eoqResults.AnnualHoldingCost_ZAR(k));

        fprintf('Total inventory cost: R%.2f\n', ...
            eoqResults.TotalInventoryCost_ZAR(k));

    end

    % Export results
    if ~isfolder(resultsFolder)
        mkdir(resultsFolder);
    end

    outputFile = fullfile(resultsFolder, ...
        'eoq_results.csv');

    writetable(eoqResults, outputFile);

    % Generate a readable horizontal ranking. Keep the full sorted table in
    % the CSV while limiting the chart to the 15 highest EOQs.
    chartCount = min(15, height(eoqResults));
    chartResults = eoqResults(1:chartCount, :);
    fig = figure('Name', 'Economic Order Quantity', 'Color', 'white', ...
        'Position', [100, 100, 1100, max(520, 32 * chartCount + 180)]);
    ax = axes(fig);
    barh(ax, chartResults.EOQ_metric_units, 'FaceColor', [0.12, 0.48, 0.72]);
    ax.YTick = 1:chartCount;
    ax.YTickLabel = cellstr(chartResults.Material);
    ax.YDir = 'reverse';
    ax.FontSize = 10;
    xlabel(ax, 'EOQ (metric units)');
    ylabel(ax, 'Production stream / material / variant');
    title(ax, sprintf('Highest EOQ values by Qt material (top %d of %d)', ...
        chartCount, height(eoqResults)));
    grid(ax, 'on');
    ax.XGrid = 'on';
    ax.YGrid = 'off';
    xMax = max(chartResults.EOQ_metric_units);
    if isfinite(xMax) && xMax > 0
        xlim(ax, [0, xMax * 1.18]);
        text(ax, chartResults.EOQ_metric_units * 1.01, 1:chartCount, ...
            compose('%.2f', chartResults.EOQ_metric_units), ...
            'VerticalAlignment', 'middle', 'FontSize', 9);
    end

    imageFile = fullfile(resultsFolder, 'eoq_analysis.png');

    saveas(fig, imageFile);

    fprintf('\nEOQ analysis completed successfully.\n');

    fprintf('Results exported to:\n%s\n', outputFile);

    fprintf('Graph exported to:\n%s\n', imageFile);

    fprintf('\n========================================\n');

end

function values = optionalTextColumn(parameters, columnName, rowCount)
    if ismember(columnName, parameters.Properties.VariableNames)
        values = string(parameters.(columnName));
    else
        values = repmat("", rowCount, 1);
    end
end
