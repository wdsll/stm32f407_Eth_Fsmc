function f_cmd = llc_freq_sweep_step(enable, cfg)
%#codegen
% 开环扫频频率发生器（离散）
%
% 输入:
%   enable : 1=开始扫频，0=复位到起始频率
%   cfg    : Bus LLC_OL_CFG
%            - f_start   : 起始频率 (Hz)
%            - f_end     : 终止频率 (Hz)
%            - t_sweep_s : 扫频总时间 (s)
%            - Ts_s      : 采样周期 (s)
% 输出:
%   f_cmd  : 当前时刻输出的频率 (Hz)

    % 进度 0~1
    persistent r
    if isempty(r)
        r = 0.0;
    end

    Ts      = cfg.Ts_s;
    t_sweep = cfg.t_sweep_s;
    f_start = cfg.f_start;
    f_end   = cfg.f_end;

    if enable ~= 0
        % 扫频开启
        if t_sweep <= 0
            % 扫频时间非法，直接一步到终点
            r = 1.0;
        else
            r = r + Ts / t_sweep;
            if r > 1.0
                r = 1.0;    % 扫完就停在终点频率
            end
        end
    else
        % 未使能：复位到起点
        r = 0.0;
    end

    % 线性插值：r=0 → f_start，r=1 → f_end
    f_cmd = f_start + (f_end - f_start) * r;
end
