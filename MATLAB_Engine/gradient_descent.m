function [solution, objectiveValue, history] = gradient_descent(objective, gradient, initialPoint, stepSize, tolerance, maxIterations)
%GRADIENT_DESCENT Minimize a differentiable objective with backtracking.
% This is a reusable numerical routine. Callers provide their objective and
% gradient; it does not invent plant cost or demand assumptions.
if nargin < 4 || isempty(stepSize), stepSize = 1; end
if nargin < 5 || isempty(tolerance), tolerance = 1e-6; end
if nargin < 6 || isempty(maxIterations), maxIterations = 1000; end
if ~isa(objective,'function_handle') || ~isa(gradient,'function_handle')
    error('objective and gradient must be function handles.');
end
if ~isnumeric(initialPoint) || isempty(initialPoint) || any(~isfinite(initialPoint(:)))
    error('initialPoint must contain finite numeric values.');
end
if ~isscalar(stepSize) || stepSize <= 0 || ~isfinite(stepSize) || ...
        ~isscalar(tolerance) || tolerance <= 0 || ~isfinite(tolerance) || ...
        ~isscalar(maxIterations) || maxIterations < 1 || maxIterations ~= floor(maxIterations)
    error('stepSize, tolerance, or maxIterations is invalid.');
end

solution = double(initialPoint);
iteration = zeros(maxIterations,1); value = NaN(maxIterations,1);
gradientNorm = NaN(maxIterations,1); acceptedStep = NaN(maxIterations,1);
objectiveValue = objective(solution);
if ~isscalar(objectiveValue) || ~isfinite(objectiveValue), error('Objective must return a finite scalar.'); end
count = 0;
for k = 1:maxIterations
    grad = gradient(solution);
    if ~isequal(size(grad),size(solution)) || any(~isfinite(grad(:)))
        error('Gradient must be finite and match the size of initialPoint.');
    end
    normGrad = norm(grad(:));
    count = k;
    iteration(k) = k; value(k) = objectiveValue; gradientNorm(k) = normGrad;
    if normGrad <= tolerance, acceptedStep(k) = 0; break; end
    step = stepSize;
    accepted = false;
    for backtrack = 1:40
        candidate = solution - step .* grad;
        candidateValue = objective(candidate);
        if isscalar(candidateValue) && isfinite(candidateValue) && ...
                candidateValue <= objectiveValue - 1e-4 * step * normGrad^2
            accepted = true;
            break;
        end
        step = step / 2;
    end
    if ~accepted, acceptedStep(k) = 0; break; end
    acceptedStep(k) = step;
    solution = candidate;
    objectiveValue = candidateValue;
    if step * normGrad <= tolerance, break; end
end
history = table(iteration(1:count),value(1:count),gradientNorm(1:count),acceptedStep(1:count), ...
    'VariableNames',{'Iteration','ObjectiveValue','GradientNorm','StepSize'});
end
