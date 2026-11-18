function summary = optimize_costs()
% OPTIMIZE_COSTS Compare current Qt inventory by material and metric unit.
projectRoot = fileparts(mfilename('fullpath'));
engineFolder = fullfile(projectRoot, 'MATLAB_Engine');
inputFile = fullfile(projectRoot, 'current_inventory.csv');
resultsFolder = fullfile(engineFolder, 'results');
if ~isfile(inputFile)
    error('Qt current inventory was not found. Capture or edit an item in the Qt app first: %s', inputFile);
end
addpath(fullfile(engineFolder, 'functions'));
currentInventory = readtable(inputFile);
summary = cost_efficiency_optimization(currentInventory, resultsFolder);
end
