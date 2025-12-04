function simOut = llc_run_closedloop(stopTime)
% 一键：清 persistent + 重新 init + 跑一次闭环仿真
    if nargin < 1
        stopTime = '1';   % 默认仿真 0.1 s
    else
        stopTime = num2str(stopTime);
    end

    % 1) 清掉所有有 persistent 的控制函数
    clear llc_ctrl_step llc_vref_softstart_step llc_freq_sweep_step

    % 2) 重新加载参数（含 K / Kp / Ki / Plant）
    llc_init_cfg;

    % 3) 强制关开环，跑闭环
    set_param('llc_ctrl_model/open_loop_en','Value','0');

    % 4) 仿真
    simOut = sim('llc_ctrl_model', ...
                 'StopTime', stopTime, ...
                 'ReturnWorkspaceOutputs','on');
end
