function cfg_ol = llc_build_step_sweep_cfg()
% llc_build_step_sweep_cfg
%  配置开环“步进扫频”的参数，并把 cfg_ol 写到 base workspace 里
%
%  扫描参数:
%     f_start_Hz : 起始频率
%     f_stop_Hz  : 结束频率
%     n_step     : 频点个数
%     dwell_s    : 每个频点驻留时间
%     Ts_s       : 控制周期(和 llc_ctrl_step / 软启动保持一致)

    cfg_ol = struct();

    cfg_ol.f_start_Hz = 100;      % 起点 70 kHz
    cfg_ol.f_stop_Hz  = 30e3;     % 终点 150 kHz
    cfg_ol.n_step     = 360;        % 360 个频点

    % 总仿真时间想要 ≈ 0.082 s → 每点驻留时间:
    cfg_ol.dwell_s    = 0.82 / cfg_ol.n_step;   % ≈ 2.28 ms

    % 控制环 10 kHz，对应 Ts = 0.0001 s
    cfg_ol.Ts_s       = 1/10e3;

    % 丢到 base workspace，给 llc_step_sweep_export_csv 用
    assignin('base','cfg_ol',cfg_ol);

    fprintf('cfg_ol 已创建: %.0f → %.0f Hz, %d 点, 每点驻留 %.3f ms, 总时长 %.3f s\n', ...
        cfg_ol.f_start_Hz, cfg_ol.f_stop_Hz, cfg_ol.n_step, ...
        cfg_ol.dwell_s*1e3, cfg_ol.dwell_s*cfg_ol.n_step);
end
