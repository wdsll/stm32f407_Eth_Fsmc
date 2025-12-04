function auto_calib_llc_gain()
% AUTO_CALIB_LLC_GAIN
%   基于闭环仿真自动标定 LLC_PLANT_K。
%
% 使用前确认：
%   1) 模型名：llc_ctrl_model.slx
%   2) Configuration Parameters → Data Import/Export：
%        - 勾选 Output，名字 = yout
%        - Format = Dataset
%   3) 顶层 Outport 的信号名设为 "Vout"（右键 signal properties）
%   4) 有 llc_init_cfg.m，并能在当前路径/搜索路径找到

    mdl      = 'llc_ctrl_model';
    stopTime = '0.5';     % 50ms 足够看稳态

    %% 0. 确认模型与参数存在
    if isempty(which(mdl))
        error('找不到模型 "%s.slx"，请确认当前目录或 MATLAB path。', mdl);
    end

    % 确保 LLC_PLANT_K 在 base 里
    if evalin('base','exist(''LLC_PLANT_K'',''var'')')
        K_old = evalin('base','LLC_PLANT_K');
    else
        if exist('llc_init_cfg.m','file')
            fprintf('base 中没有 LLC_PLANT_K，先执行 llc_init_cfg...\n');
            llc_init_cfg;
            K_old = evalin('base','LLC_PLANT_K');
        else
            error('base 中没有 LLC_PLANT_K，且找不到 llc_init_cfg.m。');
        end
    end

    % 取目标电压 vref_target
    if evalin('base','exist(''llc_ss_cfg_default'',''var'')')
        vref_target = evalin('base','llc_ss_cfg_default.vref_target');
    else
        vref_target = 48.0;
        warning('base 中找不到 llc_ss_cfg_default，默认 vref_target = %.3f V', vref_target);
    end

    fprintf('当前 LLC_PLANT_K = %.6g, 目标 vref_target = %.3f V\n', K_old, vref_target);

    %% 1. 运行仿真
    load_system(mdl);

    simOut = sim(mdl, ...
        'StopTime',            stopTime, ...
        'SaveOutput',          'on', ...
        'OutputSaveName',      'yout', ...
        'ReturnWorkspaceOutputs','on');

    % 取 DataSet yout
    if isprop(simOut,'yout')
        ds = simOut.yout;
    else
        ds = simOut.get('yout');
    end

    if isempty(ds)
        error('仿真输出 yout 为空，请检查 Data Import/Export 设置（Output=yout, Format=Dataset）。');
    end

    %% 2. 从 DataSet 中找到 Vout 信号
    voutElem = [];

    if isa(ds, 'Simulink.SimulationData.Dataset')
        n = ds.numElements;
        if n == 0
            error('DataSet yout 里没有任何元素。');
        end

        % 优先按名字找 "Vout"
        for k = 1:n
            el = ds{k};
            if strcmp(el.Name, 'Vout')
                voutElem = el;
                break;
            end
        end

        if isempty(voutElem)
            % 找不到就退回第一个
            voutElem = ds{1};
            if ~isempty(voutElem.Name)
                warning('DataSet 中找不到名为 "Vout" 的元素，退回使用第一个元素："%s"。', voutElem.Name);
            else
                warning('DataSet 中找不到名为 "Vout" 的元素，退回使用第一个匿名元素。');
            end
        end
    else
        % 没用 Dataset 的极端情况
        voutElem = ds;
    end

    % 取 timeseries
    if isa(voutElem, 'Simulink.SimulationData.Signal')
        ts = voutElem.Values;
    else
        ts = voutElem;   % 假定本身就是 timeseries
    end

    t = ts.Time;
    y = ts.Data;

    if numel(t) < 10
        error('Vout 样本点太少 (<10)，无法做稳态平均。');
    end

    %% 3. 计算稳态 Vout（最后 20% 时间窗口均值）
    tend = t(end);
    idx  = t > tend * 0.8;
    if ~any(idx)
        % 极端情况：直接取最后 10 点
        idx = max(1, numel(t)-9) : numel(t);
    end

    Vout_final = mean(y(idx));
    fprintf('测得 Vout_final = %.4f V（最后 20%%%% 时间窗口平均）\n', Vout_final);

    if Vout_final <= 0
        error('Vout_final ≤ 0，无法计算比例，请检查模型是否发散或参数是否异常。');
    end

    %% 4. 计算新的 K（温和收敛版本）
    ratio_raw  = vref_target / Vout_final;  % 理想一次性比例
    ratio_step = sqrt(ratio_raw);          % 开根号：每次只走一半

    % 再夹紧，单次不要超过 2 倍
    ratio_step = min(max(ratio_step, 0.5), 2.0);

    fprintf('原始比例 vref/Vout = %.3f, 单步修正因子 ratio_step = %.3f\n', ...
        ratio_raw, ratio_step);

    K_new = K_old * ratio_step;
    fprintf('K_old = %.8g  →  K_new = %.8g\n', K_old, K_new);

    %% 5. 写回 llc_init_cfg.m
    cfg_path = which('llc_init_cfg.m');
    if isempty(cfg_path)
        warning('找不到 llc_init_cfg.m，无法自动写回 K_new，请手动把 LLC_PLANT_K 改为 %.8g', K_new);
        return;
    end

    % 先备份一份
    backup_path = [cfg_path '.bak'];
    copyfile(cfg_path, backup_path);
    fprintf('已备份原文件到：%s\n', backup_path);

    % 读原文件
    txt = fileread(cfg_path);

    % 正则替换 LLC_PLANT_K = xxx
    pattern = 'LLC_PLANT_K\s*=\s*([0-9eE\.\+\-]+)';

    % 注意：你的 MATLAB 版本在使用 'once' 时只支持一个输出参数，
    % 所以这里只用 new_txt 一个输出，之后通过 strcmp 判断是否替换成功
    repl    = sprintf('LLC_PLANT_K     = %.15g', K_new);
    new_txt = regexprep(txt, pattern, repl, 'once');

    if strcmp(new_txt, txt)
        warning('在 llc_init_cfg.m 中没有找到 "LLC_PLANT_K = ..." 这行，请手动更新。');
        return;
    end

    % 写回文件
    fid = fopen(cfg_path, 'w');
    if fid < 0
        warning('打开 llc_init_cfg.m 失败，无法写回，请手动把 LLC_PLANT_K 改为 %.8g。', K_new);
        return;
    end
    fwrite(fid, new_txt);
    fclose(fid);

    fprintf('✅ 已将 LLC_PLANT_K 更新写回：%s\n', cfg_path);

    %% 6. 重新执行 init，把新 K 推到 base
    llc_init_cfg;
    fprintf('✅ 已重新执行 llc_init_cfg，新 K 已载入 base workspace。\n');
end
