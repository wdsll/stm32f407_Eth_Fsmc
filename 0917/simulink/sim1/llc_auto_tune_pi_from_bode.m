function [kp_d, ki_d, info] = llc_auto_tune_pi_from_bode( ...
    f_Hz, mag, phase_deg, fc_target_Hz, PM_target_deg)
% llc_auto_tune_pi_from_bode
%   根据 plant 的 Bode 数据 (f_Hz, mag, phase_deg) 自动计算电压环 PI 参数。
%
%   输入：
%       f_Hz         : 频率向量 (Hz)，长度 N
%       mag          : 对应的幅值 |G(jw)|（线性，不是 dB），长度 N
%       phase_deg    : 对应的相位 (deg)，长度 N
%       fc_target_Hz : 期望电压环交叉频率 (Hz)，比如 1e3
%       PM_target_deg: 期望相位裕度 (deg)，比如 60
%
%   输出：
%       kp_d, ki_d   : 离散 PI 参数（给 llc_ctrl_cfg.kp / .ki 用）
%       info         : 结构体，里面带连续域 Kp, Ki，实际相位裕度等信息
%
%   用法示例：
%       % 你前面已经跑完：
%       % [f_sim, mag_sim, phi_sim] = llc_plant_bode_export_simscape;
%       fc = 1e3;
%       PM = 60;
%       [kp, ki, info] = llc_auto_tune_pi_from_bode(f_sim, mag_sim, phi_sim, fc, PM);
%
%       % 然后在 llc_init_cfg 里更新：
%       % llc_ctrl_cfg.kp = kp;
%       % llc_ctrl_cfg.ki = ki;

    % -------- 一些默认参数处理 --------
    if nargin < 4 || isempty(fc_target_Hz)
        fc_target_Hz = 10e3;  % 默认 1 kHz 电压环交叉
    end
    if nargin < 5 || isempty(PM_target_deg)
        PM_target_deg = 60;  % 默认 60° 相位裕度
    end

    % 尝试从 base 里拿控制周期 Ts（你之前在 llc_init_cfg 里有 1 kHz 电压环）
    Ts_ctrl = [];
    try
        Ts_ctrl = 1 / evalin('base', 'CONTROL_LOOP_HZ');
    catch
        % 如果没有，就默认 1 ms
        Ts_ctrl = 1e-4;
        warning('在 base workspace 找不到 CONTROL_LOOP_HZ，默认 Ts = 1e-3 s。');
    end

    % -------- 在 fc_target 处插值 plant 频响 --------
    % 为了更平滑，用 log10 轴插值
    w_log  = log10(f_Hz(:));
    mag    = mag(:);
    phase_deg = phase_deg(:);

    fc = fc_target_Hz;
    fc_log = log10(fc);

    % 幅值插值（用 dB 再转回线性，避免极端值）
    mag_dB = 20*log10(mag);
    mag_c_dB = interp1(w_log, mag_dB, fc_log, 'linear', 'extrap');
    mag_c    = 10.^(mag_c_dB/20);

    % 相位插值（度）
    phi_p_deg = interp1(w_log, phase_deg, fc_log, 'linear', 'extrap');

    fprintf('\n=== Plant 在 f_c = %.1f Hz 处的频响 ===\n', fc);
    fprintf('  |G(jw_c)| = %.4g (%.2f dB)\n', mag_c, mag_c_dB);
    fprintf('  ∠G(jw_c) = %.2f deg\n\n', phi_p_deg);

    % -------- 计算 PI 零点位置 ωz --------
    % 条件：φ_p + φ_c = -180° + PM_target
    % 而 φ_c = -atan(ωz/ωc)，所以：
    % atan(ωz/ωc) = 180° - PM_target + φ_p
    phi_p_rad      = deg2rad(phi_p_deg);
    PM_target_rad  = deg2rad(PM_target_deg);

    a = pi - PM_target_rad + phi_p_rad;   % = 180° - PM + φ_p

    if a <= 0 || a >= pi/2
        warning(['在目标交叉频率和相位裕度下，简单 PI 已经没法精确满足相位条件，', ...
                 '将零点位置夹在 [fc/10 , fc] 内做近似设计。']);

        % 简单 heuristic：把零点放在 fc/3 处
        w_c  = 2*pi*fc;
        w_z  = w_c/3;
    else
        r    = tan(a);          % ωz / ωc
        w_c  = 2*pi*fc;
        w_z  = r * w_c;
    end

    f_z = w_z / (2*pi);
    fprintf('设计得到 PI 零点频率 f_z = %.1f Hz\n', f_z);

    % -------- 连续域 Kp, Ki --------
    r = w_z / w_c;
    Kp_c = 1 / (mag_c * sqrt(1 + r^2));
    Ki_c = Kp_c * w_z;

    fprintf('\n=== 连续域 PI 参数 ===\n');
    fprintf('  Kp_c = %.6g\n', Kp_c);
    fprintf('  Ki_c = %.6g  (单位 ~ 1/s)\n', Ki_c);

    % -------- 换算到离散域（你现在电压环 PI 的实现） --------
    % 假设积分器实现：I[k] = I[k-1] + ki_d * e[k]
    % 则 ki_d ≈ Ki_c * Ts，kp_d ≈ Kp_c
    kp_d = Kp_c;
    ki_d = Ki_c * Ts_ctrl;

    fprintf('\n=== 离散域（Ts = %.6g s）PI 参数，准备写到 llc_ctrl_cfg ===\n', Ts_ctrl);
    fprintf('  kp = %.6g\n', kp_d);
    fprintf('  ki = %.6g\n', ki_d);

    % -------- 验证一下在交叉频率处的相位裕度 --------
    % 在 ωc 处，L(jw_c) = C(jw_c) * G(jw_c)
    % C(jw_c) = Kp (1 + w_z/(j w_c)) = Kp (1 - j w_z/w_c)
    Cjw_c = Kp_c * (1 - 1j * (w_z / w_c));
    Gjw_c = mag_c * exp(1j * phi_p_rad);
    Ljw_c = Cjw_c .* Gjw_c;

    PM_ach_deg = 180 + rad2deg(angle(Ljw_c));   % phase margin = 180 + ∠L(jw_c)
    fprintf('\n=== 校验 ===\n');
    fprintf('  交叉处 |L| = %.3f (应该接近 1)\n', abs(Ljw_c));
    fprintf('  实际相位裕度 ≈ %.2f deg (目标 %.2f deg)\n', PM_ach_deg, PM_target_deg);

    % -------- 输出 info 结构体 --------
    info = struct();
    info.fc_target_Hz   = fc_target_Hz;
    info.PM_target_deg  = PM_target_deg;
    info.mag_c          = mag_c;
    info.mag_c_dB       = mag_c_dB;
    info.phi_p_deg      = phi_p_deg;
    info.f_z_Hz         = f_z;
    info.Kp_c           = Kp_c;
    info.Ki_c           = Ki_c;
    info.kp_d           = kp_d;
    info.ki_d           = ki_d;
    info.PM_ach_deg     = PM_ach_deg;
end
