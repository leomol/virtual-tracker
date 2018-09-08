% Tracker - Scan targets in an image.
% 
% Tracker methods:
%   track - Scan targets in an image. Identity of targets is preserved over
%   time.
% 
% Tracker properties:
%   area       - Target size relative to the screen area (0..1).
%   blobs      - Identity of isolated blobs.
%   hue        - Target hue.
%   population - Proportion of pixels to test.
%   quantity   - Number of targets to track.
%   roi        - Region of interest.
%   shrink     - Number of layers to peel off each test blob.
%   weights    - Match score assigned to each pixel.
% 
% Tracker Event list:
%   Area(area)
%   Hue(hue)
%   Population(population)
%   Position(position)
%   Quantity(quantity)
%   Roi(roi)
%   Shrink(shrink)
%   Weights(weights)
% 
% All of the above are reported after a change in the property with the same
% name.
% 
% See also Event, Tracker.GUI.

% 2016-11-23. Leonardo Molina.
% 2018-06-15. Last modified.