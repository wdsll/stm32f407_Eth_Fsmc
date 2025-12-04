function llc_plot_best_vs_candidate()
% 从 llc_pi_sweep_results.csv 里挑出一组“最好”的 PI 参数，
% 再挑另外一组当对比，分别仿真 step 响应并叠加画在一张图上。

    %========== 0. 读扫参结果 ==========
    T = readtable('llc_pi_sweep_results.csv');

    % 这里的列名按你截图来：fc, Kp, Ki, rise_time, overshoot_pct, settle_time
    fc          = T.fc;
    Kp_list     = T.Kp;
    Ki_list     = T.Ki;
    rise_time   = T.rise_time;
    overshoot   = T.overshoot_pct;
    settle_time = T.settle_time;

    %========== 1. 定义一个“好坏打分” ==========
    % 举例：优先选 overshoot 小的，如果一样，再看 rise_time
    cost = overshoot + 0.5 * rise_time*1e3;   % 简单权重，你可以自己调

    [~, idx_best] = min(cost);

    % 这里随便挑一个“对比方案”：比如频率最高的那个
    % 你也可以手动指定 idx_alt = 1; 或者 idx_alt = fc==4000;
    idx_alt = length(fc);
    if idx_alt == idx_best
        idx_alt = 1;    % 防止和 best 一样
    end

    fprintf('Best: idx=%d, fc=%g Hz, Kp=%.3f, Ki=%.3g\n', ...
            idx_best, fc(idx_best), Kp_list(idx_best), Ki_list(idx_best));
    fprintf('Alt : idx=%d, fc=%g Hz, Kp=%.3f, Ki=%.3g\n', ...
            idx_alt,  fc(idx_alt),  Kp_list(idx_alt),  Ki_list(idx_alt));

    %========== 2. 跑两次仿真 ==========
    stopTime = '0.1';   % 100 ms 你可以改

    resBest = local_run_case(Kp_list(idx_best), Ki_list(idx_best), stopTime);
    resAlt  = local_run_case(Kp_list(idx_alt ), Ki_list(idx_alt ), stopTime);

    %========== 3. 画图对比 ==========
    figure; clf; hold on; grid on;
    plot(resBest.t, resBest.y, 'LineWidth', 1.8);
    plot(resAlt.t,  resAlt.y,  '--', 'LineWidth', 1.8);

    xlabel('Time (s)');
    ylabel('V_{out} (V)');
    title('LLC step response: Best PI vs. Alternate PI');

    leg1 = sprintf('Best  fc=%g Hz, Kp=%.1f, Ki=%.2g', ...
                   fc(idx_best), Kp_list(idx_best), Ki_list(idx_best));
    leg2 = sprintf('Alt   fc=%g Hz, Kp=%.1f, Ki=%.2g', ...
                   fc(idx_alt),  Kp_list(idx_alt),  Ki_list(idx_alt));

    legend(leg1, leg2, 'Location', 'best');
end
