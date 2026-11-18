function [allocation, totalCost] = linear_programming(unitCosts, requiredQuantity, supplyLimits, itemNames)
%LINEAR_PROGRAMMING Solve a one-demand, bounded, divisible procurement LP.
% Minimize unitCosts'*x subject to sum(x)>=requiredQuantity and
% 0<=x<=supplyLimits. Items must be interchangeable and use the same unit.
% This helper is intentionally not run on mixed Qt categories because labels,
% syrup, closures, and packaging are not interchangeable.
if nargin < 4 || isempty(itemNames)
    itemNames = "Item " + string((1:numel(unitCosts))');
end
unitCosts = double(unitCosts(:)); supplyLimits = double(supplyLimits(:));
itemNames = string(itemNames(:));
if isempty(unitCosts) || numel(unitCosts) ~= numel(supplyLimits) || numel(unitCosts) ~= numel(itemNames)
    error('unitCosts, supplyLimits, and itemNames must have the same nonzero length.');
end
if any(~isfinite(unitCosts)) || any(unitCosts <= 0) || ...
        any(~isfinite(supplyLimits)) || any(supplyLimits < 0) || ...
        any(ismissing(itemNames) | strlength(strtrim(itemNames)) == 0)
    error('Costs, capacities, and names must contain valid values.');
end
if ~isscalar(requiredQuantity) || ~isfinite(requiredQuantity) || requiredQuantity <= 0
    error('requiredQuantity must be a finite positive scalar.');
end
if sum(supplyLimits) + eps < requiredQuantity
    error('Available supply is below the requested quantity.');
end

[~,order] = sort(unitCosts,'ascend');
allocated = zeros(size(unitCosts));
remaining = requiredQuantity;
for k=1:numel(order)
    i=order(k);
    allocated(i)=min(supplyLimits(i),remaining);
    remaining=remaining-allocated(i);
    if remaining <= eps, break; end
end
procurementCost = allocated .* unitCosts;
totalCost = sum(procurementCost);
allocation=table(itemNames,unitCosts,supplyLimits,allocated, ...
    supplyLimits-allocated,procurementCost, ...
    'VariableNames',{'Item','UnitCost','SupplyLimit','AllocatedQuantity', ...
    'RemainingSupply','ProcurementCost'});
end
