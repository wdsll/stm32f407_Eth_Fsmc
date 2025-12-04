function [f_Hz, mag, phase_deg] = llc_plant_bode_export_csv(csv_file)
% llc_plant_bode_export_csv
%   使用 llc_init_cfg 中的 LLC_PLANT_F_RES / Q / K
%   直接对二阶模型 G_llc(s) 计算频率响应（幅频 + 相频），并导出 CSV。
%
%   调用示例：
%       llc_init_cfg;                           % 先把参数打进 base
%       [f, mag, phi] = llc_plant_bode_export_csv();   % 默认导出 llc_plant_bode.csv
%
%   CSV 列：
%       f_Hz      : 频率 (Hz)
%       mag       : 幅值 (线性)
%       mag_dB    : 幅值 (dB)
%       phase_deg : 相位 (度)

    % ---------- 读参数（从 base workspace） ----------
    F_RES = evalin('base', 'LLC_PLANT_F_RES');   % 共振频率 Hz
    Q     = evalin('base', 'LLC_PLANT_Q');       % 品质因数
    K     = evalin('base', 'LLC_PLANT_K');       % 直流增益

    w0    = 2*pi*F_RES;
    zeta  = 1/(2*Q);   % Q = 1/(2*zeta)

    num   = K * w0^2;
    % s^2 + 2*zeta*w0*s + w0^2
    den   = [1, 2*zeta*w0, w0^2];

    % ---------- 频率网格 ----------
    % 你可以按需要改频率范围和点数
    f_Hz = logspace(log10(10), log10(1e5), 300);   % 10 Hz ~ 100 kHz 共 300 点
    w    = 2*pi*f_Hz;
    s    = 1j*w;

    % ---------- 计算频响 ----------
    Gjw        = num ./ (s.^2 + 2*zeta*w0.*s + w0^2);
    mag        = abs(Gjw);
    mag_dB     = 20*log10(mag);
    phase_deg  = angle(Gjw) * 180/pi;

    % ---------- 打印前几行看一眼 ----------
    fprintf('  f_Hz        mag        mag_dB      phase_deg\n');
    fprintf('-----------------------------------------------\n');
    for k = 1:10  % 只打印前 10 行
        fprintf('%8.1f  %10.4g  %9.3f  %9.3f\n', ...
            f_Hz(k), mag(k), mag_dB(k), phase_deg(k));
    end
    fprintf('... 共 %d 个频点\n', numel(f_Hz));

    % ---------- 导出 CSV ----------
    if nargin < 1 || isempty(csv_file)
        csv_file = 'llc_plant_bode.csv';
    end

    T = table( ...
        f_Hz(:), ...
        mag(:), ...
        mag_dB(:), ...
        phase_deg(:), ...
        'VariableNames', {'f_Hz','mag','mag_dB','phase_deg'} ...
    );

    writetable(T, csv_file);
    fprintf('✅ 已导出 Bode 数据到 CSV: %s\n', csv_file);
end
