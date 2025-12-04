function [f_Hz, mag, phase_deg] = llc_plant_bode_export_simscape(arg1, arg2)
% llc_plant_bode_export_simscape
%   不再用 frestimate / Sinestream，而是：
%   对 Simscape LLC_PowerStage 做多次“固定频率开环仿真”，
%   每个频点上用时间域方法估算 |G(jw)| 和相位。
%
% 约定（请根据你模型实际情况核对块名）：
%   顶层模型： llc_ctrl_model
%   - Constant: 'open_loop_en'     （1=开环，用 f_cmd_ol_const 驱动 LLC_PowerStage）
%   - Constant: 'f_cmd_ol_const'   （频率指令 Hz，接到 LLC_PowerStage/f_cmd 这条链路）
%   - 顶层唯一 Outport: 'Vout'    （已经在 Config 里设置为输出 yout，Format=Dataset）
%
% 用法：
%   % 最简单：用默认 fc=800Hz、CSV 名称
%   [f, mag, phi] = llc_plant_bode_export_simscape;
%
%   % 指定目标带宽 fc（比如 1 kHz），其他默认：
%   [f, mag, phi] = llc_plant_bode_export_simscape(1000);
%
%   % 同时指定 fc 和 CSV 文件名：
%   [f, mag, phi] = llc_plant_bode_export_simscape(800, 'llc_plant_bode_simscape_time.csv');
%
% 输出：
%   f_Hz      : 频率 (Hz)
%   mag       : 线性幅值（对应 Vout 小信号幅度）
%   phase_deg : 相位（度）
%
%   同时会导出 CSV: llc_plant_bode_simscape_time.csv （或你传入的 csv_file）

    mdl = 'llc_ctrl_model';

    % ---------- 参数解析 ----------
    fc_default = 800;  % 默认希望的闭环带宽（Hz），可以按需要改

    fc = [];
    csv_file = [];

    if nargin == 0
        % 全部默认
    elseif nargin == 1
        if ischar(arg1) || isstring(arg1)
            csv_file = char(arg1);
        else
            fc = arg1;
        end
    else
        % nargin == 2
        fc = arg1;
        csv_file = char(arg2);
    end

    if isempty(fc)
        fc = fc_default;
    end

    if isempty(csv_file)
        csv_file = 'llc_plant_bode_simscape_time.csv';
    end

    % ---------- 控制环采样频率（如果有的话，用来限制最高扫频） ----------
    fs_ctrl = [];
    if evalin('base','exist(''CONTROL_LOOP_HZ'',''var'')')
        fs_ctrl = evalin('base','CONTROL_LOOP_HZ');
    end

    % ---------- 频率网格设置：围绕 fc 展开 ----------
    % 建议范围：f_min ≈ fc/20， f_max ≈ min(fc*20, fs_ctrl/5)
    f_min = fc/20;      % 例如 fc=800 -> 40 Hz
    f_max = fc*20;      % 例如 fc=800 -> 16 kHz

    if ~isempty(fs_ctrl)
        f_max = min(f_max, fs_ctrl/5);   % 离 Nyquist 远一点
    end

    % 防止写得太小或出错
    f_min = max(f_min, 1);       % 不低于 1 Hz
    if f_max <= f_min
        f_max = f_min*10;
    end

    % 每个 decade 点数
    n_per_dec = 30; % 30 点/decade，已经比较细
    n_pts = round(n_per_dec * log10(f_max/f_min));

    f_Hz  = logspace(log10(f_min), log10(f_max), n_pts);

    % ---------- 每个频点仿真多少周期 ----------
    cycles_total  = 60;   % 总周期数（可以根据仿真速度适当减小/增大）
    cycles_ignore = 30;   % 前多少周期丢掉（作为稳定时间）

    % ---------- 输出数组预分配 ----------
    mag        = zeros(size(f_Hz));
    phase_deg  = zeros(size(f_Hz));

    fprintf('使用模型: %s\n', mdl);
    fprintf('开环时域扫频，fc=%.1f Hz，%d 个频点，%.1f Hz ~ %.1f Hz ...\n', ...
            fc, n_pts, f_min, f_max);

    % 如果你有 llc_init_cfg，就先刷新一遍参数
    if exist('llc_init_cfg','file')
        llc_init_cfg;
    end

    % 确保模型已加载
    if ~bdIsLoaded(mdl)
        load_system(mdl);
    end

    % ---------- 主循环：逐频点仿真 ----------
    for k = 1:n_pts
        f  = f_Hz(k);
        T  = 1/f;
        t_total  = cycles_total  * T;
        t_ignore = cycles_ignore * T;

        fprintf('  [%2d/%2d] f = %8.1f Hz, 仿真 %.5f s...\n', ...
            k, n_pts, f, t_total);

        % 1) 设置开环 & 频率常数
        % NOTE: 如果 Constant 块名字不一样，请改这里两行
        set_param([mdl '/open_loop_en'],   'Value', '1');           % 开环
        set_param([mdl '/f_cmd_ol_const'],'Value', num2str(f));     % 频率命令 Hz

        % 2) 仿真
        simOut = sim(mdl, ...
            'StopTime', num2str(t_total, '%.6g'), ...
            'ReturnWorkspaceOutputs', 'on');

        % 3) 取输出：假设 Config 里 Output=yout，Format=Dataset，唯一 Outport=Vout
        if isprop(simOut,'yout')
            ds = simOut.yout;
        else
            error('仿真结果中没有 yout，请在 Model Configuration → Data Import/Export 里把 Output 设为 ''yout''。');
        end

        if ds.numElements < 1
            error('yout 里没有任何元素，请确认顶层 Outport(Vout) 已连接且 Format=Dataset。');
        end

        % 顶层只有一个 Outport，就直接取第一个
        voutElem = ds{1};
        voutSig  = voutElem.Values;
        t = voutSig.Time;
        y = voutSig.Data;

        % 4) 只取稳态时间窗：t > t_ignore
        idx = (t > t_ignore);
        if nnz(idx) < 10
            warning('f=%.1f Hz: 有效点太少，可能 dwell 时间不够，结果不可靠。', f);
            t_win = t;
            y_win = y - mean(y);
        else
            t_win = t(idx);
            y_win = y(idx) - mean(y(idx));  % 去掉 DC
        end

        % 5) 用积分法投影到 cos/sin，估计振幅和相位
        w  = 2*pi*f;
        cw = cos(w * t_win);
        sw = sin(w * t_win);

        % 连续时间近似积分（考虑变步长的 t_win，用 trapz）
        T_win = t_win(end) - t_win(1);
        if T_win <= 0
            warning('f=%.1f Hz: 窗口时间长度为 0，跳过此点。', f);
            continue;
        end

        a = (2 / T_win) * trapz(t_win, y_win .* cw);   % cos 分量
        b = (2 / T_win) * trapz(t_win, y_win .* sw);   % sin 分量

        A   = sqrt(a^2 + b^2);       % 振幅（线性）
        phi = atan2(-b, a);          % 相位（按 C(jw)=a - j b 的约定）

        mag(k)       = A;
        phase_deg(k) = phi * 180/pi;
    end

    % ---------- 打印前几行看看 ----------
    fprintf('\n  f_Hz        mag        mag_dB      phase_deg\n');
    fprintf('------------------------------------------------\n');
    for k = 1:min(10, numel(f_Hz))
        fprintf('%8.1f  %10.4g  %9.3f  %9.3f\n', ...
            f_Hz(k), mag(k), 20*log10(mag(k)), phase_deg(k));
    end
    fprintf('... 共 %d 个频点\n', numel(f_Hz));

    % ---------- 导出 CSV ----------
    Ttbl = table( ...
        f_Hz(:), ...
        mag(:), ...
        20*log10(mag(:)), ...
        phase_deg(:), ...
        'VariableNames', {'f_Hz','mag','mag_dB','phase_deg'} ...
    );
    writetable(Ttbl, csv_file);
    fprintf('✅ 已导出 Simscape Plant 的 Bode 数据到: %s\n', csv_file);
end
