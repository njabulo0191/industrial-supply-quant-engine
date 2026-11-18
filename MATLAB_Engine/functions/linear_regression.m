function regressionResults = linear_regression(data, resultsFolder)

% LINEAR REGRESSION
% Analyses the price trend of each material independently.
%
% Inputs:
%   data          - Validated telemetry table
%   resultsFolder - Destination for exported results
%
% Output:
%   regressionResults - Table containing regression statistics

    fprintf('\n========================================\n');
    fprintf(' LINEAR REGRESSION ANALYSIS\n');
    fprintf('========================================\n');

    % Validate input columns
    requiredColumns = {
        'production_stream'
        'material_type'
        'product_name'
        'unit_cost_zar'
        'elapsed_minutes'
    };

    if ~istable(data) || ...
            ~all(ismember(requiredColumns, ...
            data.Properties.VariableNames))
        error('Input table is missing required columns.');
    end

    if ~isfolder(resultsFolder)
        mkdir(resultsFolder);
    end

    % Track price histories by production stream and exact material variant.
    streamNames = string(data.production_stream);
    materialNames = string(data.material_type);
    productNames = string(data.product_name);
    streamNames(ismissing(streamNames) | strlength(strtrim(streamNames)) == 0) = "Unassigned";
    materialKeys = streamNames + " / " + materialNames + " / " + productNames;

    validNames = ~ismissing(materialNames) & strlength(strtrim(materialNames)) > 0 & ...
        ~ismissing(productNames) & strlength(strtrim(productNames)) > 0;

    materials = unique(materialKeys(validNames), 'stable');

    % Prepare result arrays
    resultMaterial = strings(numel(materials), 1);
    observations = zeros(numel(materials), 1);

    slopes = zeros(numel(materials), 1);
    intercepts = zeros(numel(materials), 1);
    rSquaredValues = zeros(numel(materials), 1);

    startTimes = zeros(numel(materials), 1);
    endTimes = zeros(numel(materials), 1);
    fittedEndCosts = zeros(numel(materials), 1);

    resultCount = 0;

    % Prepare regression plot
    fig = figure('Name', 'Material Price Regression');

    hold on;
    grid on;

    % Analyse each material independently
    for k = 1:numel(materials)

        % Select observations for current material
        idx = materialKeys == materials(k);

        x = data.elapsed_minutes(idx);
        y = data.unit_cost_zar(idx);

        % Remove invalid observations
        valid = isfinite(x) & isfinite(y);

        x = x(valid);
        y = y(valid);

        % Require at least two distinct time values
        if numel(x) < 2 || numel(unique(x)) < 2

            warning('Insufficient time observations for %s.', ...
                char(materials(k)));

            continue;
        end

        % Calculate linear regression coefficients
        coefficients = polyfit(x, y, 1);

        slope = coefficients(1);
        intercept = coefficients(2);

        % Calculate fitted prices
        fittedPrices = polyval(coefficients, x);

        % Calculate coefficient of determination (R-squared)
        residualSum = sum((y - fittedPrices).^2);

        totalSum = sum((y - mean(y)).^2);

        if totalSum > eps
            rSquared = 1 - residualSum / totalSum;
        else
            rSquared = NaN;
        end

        % Store results
        resultCount = resultCount + 1;

        resultMaterial(resultCount) = materials(k);
        observations(resultCount) = numel(x);

        slopes(resultCount) = slope;
        intercepts(resultCount) = intercept;
        rSquaredValues(resultCount) = rSquared;

        startTimes(resultCount) = min(x);
        endTimes(resultCount) = max(x);

        fittedEndCosts(resultCount) = ...
            polyval(coefficients, max(x));

        % Plot actual observations
        scatter(x, y, 35, 'filled', ...
            'DisplayName', char(materials(k) + " observed"));

        % Plot fitted regression line
        xFit = linspace(min(x), max(x), 100);

        yFit = polyval(coefficients, xFit);

        plot(xFit, yFit, '--', 'LineWidth', 1.5, ...
            'DisplayName', char(materials(k) + " trend"));

        % Display numerical results
        fprintf('\nMaterial: %s\n', char(materials(k)));

        fprintf('Observations: %d\n', numel(x));

        fprintf('Price trend: R%.6f per minute\n', slope);

        fprintf('Intercept: R%.4f\n', intercept);

        fprintf('R-squared: %.4f\n', rSquared);

        fprintf('Fitted price at end: R%.4f\n', ...
            fittedEndCosts(resultCount));

    end

    % Check whether any regression was completed
    if resultCount == 0

        close(fig);

        warning('No valid material regressions could be calculated.');

        regressionResults = table();

        return;
    end

    % Create results table
    regressionResults = table( ...
        resultMaterial(1:resultCount), ...
        observations(1:resultCount), ...
        slopes(1:resultCount), ...
        intercepts(1:resultCount), ...
        rSquaredValues(1:resultCount), ...
        startTimes(1:resultCount), ...
        endTimes(1:resultCount), ...
        fittedEndCosts(1:resultCount), ...
        'VariableNames', { ...
            'Material', ...
            'Observations', ...
            'Slope_ZAR_per_minute', ...
            'Intercept_ZAR', ...
            'R_squared', ...
            'Start_time_minutes', ...
            'End_time_minutes', ...
            'Fitted_cost_at_end_ZAR' ...
        });

    % Label graph
    xlabel('Elapsed time (minutes)');

    ylabel('Material cost (ZAR)');

    title('Linear Regression: Material Price Trends');

    legend('Location', 'best');

    % Export results
    csvFile = fullfile(resultsFolder, ...
        'linear_regression_results.csv');

    writetable(regressionResults, csvFile);

    % Export graph
    imageFile = fullfile(resultsFolder, ...
        'linear_regression.png');

    saveas(fig, imageFile);

    fprintf('\nRegression analysis completed.\n');

    fprintf('Results exported to:\n%s\n', csvFile);

    fprintf('Graph exported to:\n%s\n', imageFile);

    fprintf('\n========================================\n');

end
