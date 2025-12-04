function tap_freq_to_scope()
    mdl = 'llc_ctrl_model';
    if ~bdIsLoaded(mdl)
        load_system(mdl);
    end

    % === 1) 确保 Scope 有 2 个输入口 ===
    scopeBlk = [mdl '/Scope1'];      % 如果你的名字是 "Scope" 就改成 '/Scope'
    set_param(scopeBlk, 'NumInputPorts', '2');

    % === 2) 拿端口句柄 ===
    hScope = get_param(scopeBlk,              'PortHandles');
    hCtrl  = get_param([mdl '/llc_ctrl_step'],'PortHandles');

    % === 3) 如果 Scope1 第 2 口还没连线，就从 f_cmd 分一支过来 ===
    lhScope = get_param(scopeBlk, 'LineHandles');
    if lhScope.Inport(2) == -1
        % 这里不会动原来连去 LLC_Plant 的那根线，只是新增一个分支
        add_line(mdl, hCtrl.Outport(1), hScope.Inport(2), 'autorouting','on');
        disp('✅ 已把 llc_ctrl_step 的 f_cmd 接到 Scope1 的第 2 个输入口。');
    else
        disp('ℹ️ Scope1 的第 2 个输入口已经有线，不再重复连接。');
    end

    save_system(mdl);
end
