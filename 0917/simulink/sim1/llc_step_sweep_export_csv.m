function llc_step_sweep_export_csv()
% 扫频 + 导出 CSV
% 依赖：
%   - 模型 llc_ctrl_model
%   - base 里有 cfg_ol = llc_build_step_sweep_cfg();
%   - 模型 Data Import/Export 里勾选 Output=yout, Format=Dataset

    mdl = 'llc_ctrl_model';

    if ~bdIsLoaded(mdl)
        load_system(mdl);
    end

    % ---- 读开环扫频配置 ----
    cfg_ol = evalin('base','cfg_ol');   % f_start_Hz / f_stop_Hz / n_step / dwell_s / Ts_s

    f_list   = linspace(cfg_ol.f_start_Hz, cfg_ol.f_stop_Hz, cfg_ol.n_step);
    T_total  = cfg_ol.dwell_s * cfg_ol.n_step;    % 总仿真时间
    Ts       = cfg_ol.Ts_s;
    Ns_step  = round(cfg_ol.dwell_s / Ts);        % 每个频点对应多少个采样点

    fprintf('拟扫 %d 个频点，总仿真时间约 %.4f s\n', cfg_ol.n_step, T_total);

    % 强制开环：open_loop_en Constant=1
    try
        set_param([mdl '/open_loop_en'],'Value','1');
    catch
        warning('找不到 Constant block "open_loop_en"，请确认顶层块名一致。');
    end

    % ---- 跑一次长仿真 ----
    simOut = sim(mdl, ...
        'StopTime', num2str(T_total,'%.6g'), ...
        'ReturnWorkspaceOutputs','on');

    % =====================================================
    %  1) 先在 simOut 里找 Dataset：优先 logsout，没有就退回 yout
    % =====================================================
    ds = [];
    if isprop(simOut,'logsout') && ~isempty(simOut.logsout)
        ds = simOut.logsout;
    elseif isprop(simOut,'yout') && ~isempty(simOut.yout)
        ds = simOut.yout;
    else
        error(['仿真输出中既没有 "logsout" 也没有 "yout"，' ...
               '请在 Model Configuration Parameters → Data Import/Export ' ...
               '勾选 Output，并把 Format 设为 Dataset。']);
    end

    if ~isa(ds,'Simulink.SimulationData.Dataset')
        error('期望 Dataset 类型输出，但当前类型是：%s', class(ds));
    end

    % =====================================================
    %  2) 在 Dataset 里找到 Vout 信号
    % =====================================================
    names = ds.getElementNames;

    idx = find(strcmp(names,'Vout'),1);
    if isempty(idx)
        % 找不到名为 Vout 的元素，就退回第一个
        if isempty(names)
            error('Dataset 里没有任何元素，请检查顶层 Outport / Signal Logging 设置。');
        end
        warning('DataSet 中找不到名为 "Vout" 的元素，退回使用第一个元素 "%s"。', names{1});
        idx = 1;
    end

    sig = ds{idx};

    % 无论是 Signal 还是 timeseries，都统一成 timeseries 变量 ts
    if isa(sig,'Simulink.SimulationData.Signal')
        ts = sig.Values;
    elseif isa(sig,'timeseries')
        ts = sig;
    else
        error('不认识的信号类型: %s', class(sig));
    end

    t = ts.Time(:);
    y = ts.Data(:);

    % 简单健壮性检查
    if numel(y) < Ns_step*cfg_ol.n_step
        warning('采样点数量 (%d) 少于预期 Ns_step*n_step (%d)，索引会被自动截断。', ...
            numel(y), Ns_step*cfg_ol.n_step);
    end

    % =====================================================
    %  3) 对每个频点，取对应时间窗的“后半段均值”作为稳态 Vout
    % =====================================================
    Vout_ss = zeros(cfg_ol.n_step,1);

    for k = 1:cfg_ol.n_step
        idx_start = (k-1)*Ns_step + 1;
        idx_end   = min(k*Ns_step, numel(y));
        if idx_start > numel(y)
            Vout_ss(k) = NaN;
            continue;
        end

        seg = y(idx_start:idx_end);
        n_seg = numel(seg);
        if n_seg <= 2
            Vout_ss(k) = seg(end);
        else
            % 把每段前 50% 当作过渡，后 50% 做平均
            seg_ss = seg(round(n_seg/2):end);
            Vout_ss(k) = mean(seg_ss);
        end
    end

    % 归一化增益（假设 Vin 常数是 400V，可以按需改）
    Vin_nom   = 400.0;
    gain_norm = Vout_ss / Vin_nom;

    % =====================================================
    %  4) 导出到 CSV
    % =====================================================
    Ttbl = table( ...
        f_list(:), ...
        Vout_ss(:), ...
        gain_norm(:), ...
        'VariableNames', {'freq_Hz','Vout_V','gain_norm'});

    fname = fullfile(pwd,'llc_step_sweep.csv');
    writetable(Ttbl, fname);

    fprintf('✅ 已导出扫频结果到: %s\n', fname);
end
