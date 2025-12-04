function llc_build_91k_cfg()
    cfg_ol = struct();
    cfg_ol.f_start_Hz = 91e3;       % 从 91 kHz 开始
    cfg_ol.f_stop_Hz  = 91e3;       % 到 91 kHz 结束 => 实际就是恒定 91 kHz
    cfg_ol.t_sweep_s  = 0.05;       % 扫描总时长(随便给个够长的，50 ms)
    cfg_ol.Ts_s       = 1/10e3;     % 你的控制周期 10 kHz
    assignin('base','cfg_ol',cfg_ol);
end
