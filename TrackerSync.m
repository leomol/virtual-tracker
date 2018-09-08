% TrackerSync - Track position of a target synced to an external trigger.
% This is useful for tracking position with a webcam while synchronously
% acquiring electrophysiological, imaging, or behavioral data with another
% device.
% 
% Digital inputs from an Arduino will trigger an entry to a comma-separated
% log file with: time, x, y, pin, count
% Which means that, at a given time, the position of the target was x and y,
% and the pin had toggled low/high as the count indicates. Even numbers
% correspond to low states, odd numbers to high states. If the first state 
% is low, the count starts at 0, otherwise at 1.
% Data is saved to disk with every trigger, hence a decline in performance
% is expected with higher trigger frequencies.
% The location of the log file is Documents/TrackerSync/<timestamp>.csv
% 
% Data from the Arduino are expected in single bytes:
%   bits 1 to 6 indicate the pin number.
%   bit 7 indicates a positive or a negative change with 0 or 1, respectively.
%   bit 8 must be set to zero.
% An extended protocol (not provided here) is enabled when bit 8 is set to
% one. A minimalistic firmware -compliant with such protocol- is included
% and can be installed using the Arduino IDE. Such program enables pins 16
% to 19 as digital inputs by default.
% 
% See also VirtualTracker.GUI

% 2018-07-09. Leonardo Molina
% 2018-07-16. Last modified.
classdef TrackerSync < handle
    properties (SetAccess = private)
        % virtualTracker - VirtualTracker.GUI handle.
        virtualTracker
    end
    
    properties (Access = private)
        % comId - Serial port name.
        comId
        
        % count - Count of pin toggles.
        count = zeros(1, 64)
        
        % device - Serial device handle.
        device
        
        % frameOutput - Text GUI to count frames.
        frameOutput
        
        % frameText - Frame info text.
        frameText
        
        % inputs - Queue for input data.
        inputs = zeros(0, 1, 'uint8');
        
        % loopHandle - Handle for the scheduler loop.
        loopHandle
        
        % output - Output filename where data will be saved.
        output
        
        % settingsFilename - GUI settings filename.
        settingsFilename
        
        % setup - Whether this is the first report for a given pin.
        setup = true(1, 64)
        
        % startTime - Startup time.
        startTime
    end
    
    properties (Constant)
        % baudrate - Serial transmission speed.
        baudrate = 115200
        
        % timeout - Interval for read and write, not smaller than 0.001.
        timeout = 1e-3
    end
    
    methods
        function obj = TrackerSync(comId)
            % obj = TrackerSync(comId)
            % Connect to serial port with the given comId and log position
            % to disk with every pin toggle.
            
            % Start serial device.
            obj.comId = comId;
            try
                obj.device = serial(comId, 'BaudRate', obj.baudrate);
                obj.device.timeout = obj.timeout;
                fopen(obj.device);
            catch e
                fprintf(2, 'Could not open the provided serial device.\nIf synchronization is not required, use VirtualTracker.GUI instead.\n\n');
                rethrow(e);
            end
            
            % Start virtual tracker.
            obj.virtualTracker = VirtualTracker.GUI(1);
            width = obj.virtualTracker.window.Position(3);
            height = 20;
            panel = uipanel('Title', 'VirtualTracker');
            panel.Units = 'Pixels';
            panel.Position = [1, 1, width, 2 * height];
            obj.frameText   = uicontrol('Parent', panel, 'Style', 'Text', 'Position', [0.00 * width, 0.00, 0.19 * width, height], 'Units', 'Normalized', 'String', 'Frames:', 'HorizontalAlignment', 'right');
            obj.frameOutput = uicontrol('Parent', panel, 'Style', 'Text', 'Position', [0.20 * width, 0.00, 0.40 * width, height], 'Units', 'Normalized', 'String', '', 'HorizontalAlignment', 'right');
            
            obj.virtualTracker.pushPanel(panel);
            obj.virtualTracker.register('Close', @obj.delete);
            className = mfilename('class');
            obj.settingsFilename = sprintf('%s.settings.mat', className);
            obj.loadSettings(obj.settingsFilename);
            
            % Create log file.
            root = getenv('USERPROFILE');
            folder = fullfile(root, 'Documents', className);
            if exist(folder, 'dir') ~= 7
                mkdir(folder);
            end
            % Session name starts with VT and follows with a timestamp.
            session = sprintf('VT%s', datestr(now, 'yyyymmddHHMMSS'));
            obj.output = fullfile(folder, sprintf('%s.csv', session));
            % Write file header.
            fid = fopen(obj.output, 'a');
            fprintf(fid, 'time, x, y, pin, count\n');
            fclose(fid);
            
            % Read serial port regularly.
            obj.startTime = tic;
            obj.loopHandle = Scheduler.Repeat(@obj.loop, 2 * obj.timeout);
        end
        
        function delete(obj)
            % TrackerSync.delete()
            % Release serial device and save GUI settings.
            
            obj.saveSettings(obj.settingsFilename);
            delete(obj.loopHandle);
            fclose(obj.device);
            delete(obj.device);
        end
    end
    
    methods (Access = private)
        function loadSettings(obj, filename)
            % TrackerSync.loadSettings()
            % Apply previously saved settings, or else defaults.
            
            if exist(filename, 'file') == 2
                c = load(filename);
                settings = c.settings;
            else
                % Default settings.
                settings.camera.exposure = -5;
                settings.tracker.hue = -2;
                settings.tracker.population = 0.05;
                settings.tracker.area = 0.08;
            end
            % Only need to track one pointer.
            settings.tracker.quantity = 1;
            
            names = fieldnames(settings.camera);
            for i = 1:numel(names)
                obj.virtualTracker.camera.(names{i}) = settings.camera.(names{i});
            end
            names = fieldnames(settings.tracker);
            for i = 1:numel(names)
                obj.virtualTracker.tracker.(names{i}) = settings.tracker.(names{i});
            end
        end
        
        function loop(obj)
            % TrackerSync.loop()
            % Save position to disk when serial data is received.
            
            % Read a maximum number of bytes at a time.
            available = min(obj.device.BytesAvailable, 128);
            if available > 0
                recent = fread(obj.device, available, 'uint8');
                obj.inputs = [obj.inputs; recent];
            end
            
            % Process a maximum number of bytes at a time.
            nProcessed = 0;
            while numel(obj.inputs) > 0 && nProcessed <= 128
                head = obj.inputs(1);
                if bitand(head, 128) == 0
                    % 0xxxxxxx: 6-bit target and 1-bit state.
                    pin = bitand(head, 63);
                    state = bitand(head, 64) ~= 64;
                    % Update count.
                    id = pin + 1;
                    if obj.setup(id)
                        % A high state during start shifts the count by 1.
                        obj.setup(id) = false;
                        if state
                            obj.count(id) = 1;
                        end
                    else
                        obj.count(id) = obj.count(id) + 1;
                        ids = find(obj.count > 0);
                        counts = num2cell(obj.count(ids));
                        pins = num2cell(ids - 1);
                        obj.frameOutput.String = strjoin(Tools.compose('P%02i:%i', [pins(:) counts(:)]'), ' ');
                    end
                    
                    % Create an entry in the log file.
                    % For most applications, writing small volumes frequently will yield 
                    % better performance than writing larger volumes infrequently.
                    if state
                        fid = fopen(obj.output, 'a');
                        fprintf(fid, '%.2f,%.2f,%.2f,%i,%i\n', toc(obj.startTime), obj.virtualTracker.position(1), obj.virtualTracker.position(2), pin, obj.count(id));
                        fclose(fid);
                    end
                end
                % Pop the queue.
                obj.inputs(1) = [];
                nProcessed = nProcessed + 1;
            end
        end
        
        function saveSettings(obj, filename)
            % TrackerSync.saveSettings(filename)
            % Save camera and tracker settings.
            
            cameraSettings = {'resolution', 'exposure', 'mirror'};
            for i = 1:numel(cameraSettings)
                settings.camera.(cameraSettings{i}) = obj.virtualTracker.camera.(cameraSettings{i});
            end
            trackerSettings = {'hue', 'population', 'shrink', 'area', 'roi'};
            for i = 1:numel(trackerSettings)
                settings.tracker.(trackerSettings{i}) = obj.virtualTracker.tracker.(trackerSettings{i});
            end
            save(filename, 'settings');
        end
    end
end
%#ok<*STRNU>