function [root, result] = numerical_methods(method, fun, startValue, tolerance, maxIterations, derivative)
%NUMERICAL_METHODS Solve a scalar root problem with bisection or Newton's method.
% Bisection: [root,result] = numerical_methods('bisection',f,[a b])
% Newton:    [root,result] = numerical_methods('newton',f,x0,...,df)
if nargin < 4 || isempty(tolerance), tolerance = 1e-8; end
if nargin < 5 || isempty(maxIterations), maxIterations = 200; end
if nargin < 6, derivative = []; end
if ~isa(fun,'function_handle'), error('fun must be a function handle.'); end
if ~isscalar(tolerance) || tolerance <= 0 || ~isfinite(tolerance) || ...
        ~isscalar(maxIterations) || maxIterations < 1 || maxIterations ~= floor(maxIterations)
    error('tolerance or maxIterations is invalid.');
end
method = lower(string(method));
history = NaN(maxIterations,3);
converged = false;

if method == "bisection"
    if ~isnumeric(startValue) || numel(startValue) ~= 2 || ...
            any(~isfinite(startValue)) || startValue(1) >= startValue(2)
        error('Bisection requires an increasing finite interval [a b].');
    end
    left = startValue(1); right = startValue(2);
    fLeft = fun(left); fRight = fun(right);
    if ~isscalar(fLeft) || ~isscalar(fRight) || ~isfinite(fLeft) || ~isfinite(fRight) || fLeft*fRight > 0
        error('Bisection interval must have finite function values with opposite signs or a zero endpoint.');
    end
    if fLeft == 0, root=left; converged=true; count=0;
    elseif fRight == 0, root=right; converged=true; count=0;
    else
        root = (left+right)/2; count = 0;
        for k=1:maxIterations
            root = (left+right)/2; fRoot=fun(root); count=k;
            history(k,:)=[k,root,fRoot];
            if abs(fRoot)<=tolerance || (right-left)/2<=tolerance, converged=true; break; end
            if fLeft*fRoot < 0, right=root; fRight=fRoot; else, left=root; fLeft=fRoot; end
        end
    end
elseif method == "newton"
    if ~isa(derivative,'function_handle'), error('Newton method requires a derivative function handle.'); end
    if ~isscalar(startValue) || ~isfinite(startValue), error('Newton requires a finite scalar initial guess.'); end
    root = double(startValue); count=0;
    for k=1:maxIterations
        fValue=fun(root); dValue=derivative(root); count=k;
        if ~isscalar(fValue) || ~isscalar(dValue) || ~isfinite(fValue) || ~isfinite(dValue) || abs(dValue)<=eps
            error('Newton method encountered an invalid or near-zero derivative.');
        end
        history(k,:)=[k,root,fValue];
        if abs(fValue)<=tolerance, converged=true; break; end
        next=root-fValue/dValue;
        if ~isfinite(next), error('Newton method produced a non-finite iterate.'); end
        if abs(next-root)<=tolerance, root=next; converged=true; break; end
        root=next;
    end
else
    error('method must be ''bisection'' or ''newton''.');
end

if count > 0, history=history(1:count,:); else, history=zeros(0,3); end
result=struct('Method',char(method),'Converged',converged,'Iterations',count, ...
    'Residual',abs(fun(root)),'History',array2table(history, ...
    'VariableNames',{'Iteration','Estimate','FunctionValue'}));
if ~converged, warning('Numerical method reached its iteration limit before convergence.'); end
end
