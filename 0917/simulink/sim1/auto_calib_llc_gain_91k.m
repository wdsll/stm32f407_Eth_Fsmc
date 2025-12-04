function auto_calib_llc_gain_91k()
    % 目标输出电压
    Vref_target = 48.0;

    % 1) 初始化配置 & 91k 开环
    llc_init_cfg;           % 里面会给 LLC_PLANT_K 一个初值
    llc_build_91k_cfg;      % 上面那段脚本，配置 cfg_ol
    
    % 开环
    set_param('llc_ctrl_model/open_loop_en','Value','1');

    % 2) 仿真 50ms，看 91kHz 下的 Vout
    simOut = sim('llc_ctrl_model', ...
                 'StopTime','0.05', ...
                 'ReturnWorkspaceOutputs','on');

    ds   = simOut.yout;       % Data Import/Export 里的 Output: yout (Dataset)
    vout_s = ds{1}.Values;    % 顶层只有一个 Outport: Vout
    t    = vout_s.Time;
    y    = vout_s.Data;

    % 只拿最后 10ms 做平均
    idx        = t > (t(end)*0.8);
    Vout_91k   = mean(y(idx));

    % 3) 按比例修 K
    K_old  = evalin('base','LLC_PLANT_K');
    scale  = Vref_target / Vout_91k;
    K_new  = K_old * scale;

    fprintf('Vout@91k = %.3f V, K_old = %.6g, K_new = %.6g\n', ...
             Vout_91k, K_old, K_new);

    % 写回 base workspace（临时生效）
    assignin('base','LLC_PLANT_K', K_new);

    % 你愿意的话也可以在这里自动改 llc_init_cfg.m，
    % 先确认这个 K_new 看起来合理再说。
end
