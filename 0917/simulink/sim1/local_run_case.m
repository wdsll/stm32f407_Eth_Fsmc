function res = local_run_case(Kp, Ki, stopTime)

    % === 0) 先清掉所有带 persistent 的控制函数 ===
    clear llc_ctrl_step llc_vref_softstart_step llc_freq_sweep_step

    % === 1) 初始化一次默认配置 ===
    llc_init_cfg;

    % 采样周期 Ts（在软启 cfg 里）
    ss_cfg = evalin('base','llc_ss_cfg_default');
    Ts = ss_cfg.Ts_s;

    % === 2) 把 Kp/Ki 改到控制配置结构里 ===
    ctrl_cfg      = evalin('base','llc_ctrl_cfg_default');
    ctrl_cfg.kp   = Kp;
    ctrl_cfg.ki   = abs(Ki) * Ts;   % 连续 Ki_c -> 离散 Ki_dis
    assignin('base','llc_ctrl_cfg_default', ctrl_cfg);

    % === 3) 跑仿真 ===
    simOut = sim('llc_ctrl_model', ...
        'StopTime',              stopTime, ...
        'ReturnWorkspaceOutputs','on');

    ds   = simOut.yout;
    vout = ds{1}.Values;

    res.t = vout.Time;
    res.y = vout.Data;
end


