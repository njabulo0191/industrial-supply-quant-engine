function smoothed = moving_average(values, windowSize)
%MOVING_AVERAGE Trailing moving average for a numeric vector.
% Partial windows are used at the beginning of the series. NaN values are
% ignored; an all-NaN window returns NaN.
if ~isnumeric(values) || ~isvector(values)
    error('values must be a numeric vector.');
end
if ~isscalar(windowSize) || ~isfinite(windowSize) || windowSize < 1 || windowSize ~= floor(windowSize)
    error('windowSize must be a positive integer.');
end
values = double(values(:));
smoothed = NaN(size(values));
for k = 1:numel(values)
    first = max(1,k-windowSize+1);
    window = values(first:k);
    window = window(isfinite(window));
    if ~isempty(window), smoothed(k) = mean(window); end
end
end
