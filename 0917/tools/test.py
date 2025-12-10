
from __future__ import annotations

import tkinter as tk
from dataclasses import dataclass
from typing import List

# Matplotlib 在部分最小化运行环境中并不总是可用, 因此这里尝试
# 延迟导入, 若失败则回退到纯命令行模式。
try:  # pragma: no cover - 简单依赖探测逻辑
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
    from matplotlib.figure import Figure
except ModuleNotFoundError:  # pragma: no cover - 依赖缺失时触发
    FigureCanvasTkAgg = None  # type: ignore[assignment]
    Figure = None  # type: ignore[assignment]
    HAS_MATPLOTLIB = False
else:
    HAS_MATPLOTLIB = True


@dataclass(frozen=True)
class LLCOpenLoopSegment:
   

    start_hz: float
    stop_hz: float
    slew_hz_per_ms: float
    hold_time_ms: int


class LLCOpenLoopController:
 

    def __init__(self, segments: List[LLCOpenLoopSegment]):
        if not segments:
            raise ValueError("segments must not be empty")
        self._segments = segments
        self.segment_count = len(segments)
        self.reset()

    # === 固件公开接口 ===
    def init(self) -> None:
        self.reset()
class LLCOpenLoopController:

        diff = seg.stop_hz - self.f_cmd
        step = abs(seg.slew_hz_per_ms)
        if step <= 1e-6 or abs(diff) <= step:
            self.f_cmd = seg.stop_hz
            self.holding = True
            self.hold_elapsed_ms = 0
            return

        self.f_cmd += step if diff > 0 else -step

    # === Python 帮助方法 ===
    def reset(self) -> None:
        self.current_index = 0
        self.f_cmd = self._segments[0].start_hz
        self.hold_elapsed_ms = 0
        self.running = False
        self.holding = False
        self.completed = False

    @property
    def current_segment(self) -> LLCOpenLoopSegment:
        return self._segments[self.current_index]


LLC_F_MIN_HZ = 85000.0
LLC_F_MAX_HZ = 130000.0
LLC_F_INIT_HZ = 90000.0
LLC_F_SLEW_HZ = 500.0


def create_default_controller() -> LLCOpenLoopController:
   

    segments = [
        LLCOpenLoopSegment(LLC_F_MAX_HZ, LLC_F_INIT_HZ, LLC_F_SLEW_HZ, 100),
        LLCOpenLoopSegment(LLC_F_INIT_HZ, LLC_F_INIT_HZ, 0.0, 0),
    ]
    return LLCOpenLoopController(segments)


class OpenLoopGUI:
   

    def __init__(self, root: tk.Tk):
        self.root = root
        root.title("LLC开环频率仿真")
        root.geometry("900x600")

        self.controller = create_default_controller()

        # 状态变量
        self.time_ms: int = 0
        self.time_data: List[float] = [0.0]
        self.freq_data: List[float] = [self.controller.f_cmd]
        self.steps_per_update = tk.IntVar(value=10)

        self._build_widgets()
        self._schedule_update()

    # GUI 构建
    def _build_widgets(self) -> None:
        ctrl_frame = tk.Frame(self.root)
        ctrl_frame.pack(fill=tk.X, padx=12, pady=8)

        start_btn = tk.Button(ctrl_frame, text="Start", width=10, command=self.start)
        start_btn.pack(side=tk.LEFT, padx=(0, 6))

        stop_btn = tk.Button(ctrl_frame, text="Stop", width=10, command=self.stop)
        stop_btn.pack(side=tk.LEFT, padx=6)

        reset_btn = tk.Button(ctrl_frame, text="Reset", width=10, command=self.reset)
        reset_btn.pack(side=tk.LEFT, padx=6)

        speed_label = tk.Label(ctrl_frame, text="速度倍数(ms/刷新):")
        speed_label.pack(side=tk.LEFT, padx=(24, 6))

        speed_spin = tk.Spinbox(
            ctrl_frame,
            from_=1,
            to=200,
            textvariable=self.steps_per_update,
            width=6,
            command=self._on_speed_change,
        )
        speed_spin.pack(side=tk.LEFT)

        self.status_var = tk.StringVar()
        status_label = tk.Label(ctrl_frame, textvariable=self.status_var)
        status_label.pack(side=tk.RIGHT)

        # Matplotlib 图表
        if not HAS_MATPLOTLIB:
            raise RuntimeError("Matplotlib backend is unavailable; GUI cannot be constructed")

        figure = Figure(figsize=(8.5, 4.5), dpi=100)
        self.ax = figure.add_subplot(111)
        self.ax.set_title("开环频率曲线")
        self.ax.set_xlabel("时间 (ms)")
        self.ax.set_ylabel("频率 (Hz)")
        self.ax.grid(True, linestyle="--", alpha=0.4)

        (self.line,) = self.ax.plot(self.time_data, self.freq_data, color="#0078D4", linewidth=2)
        self.ax.set_xlim(0, 200)
        self.ax.set_ylim(80000, 132000)

        canvas = FigureCanvasTkAgg(figure, master=self.root)
        canvas.draw()
        canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=12, pady=(0, 12))
        self.canvas = canvas

    def _on_speed_change(self) -> None:
        try:
            value = int(self.steps_per_update.get())
            if value < 1:
                self.steps_per_update.set(1)
        except tk.TclError:
            self.steps_per_update.set(1)

    # 控制逻辑
@@ -243,33 +261,82 @@ class OpenLoopGUI:
        status_parts = [
            f"Segment: {self.controller.current_index + 1}/{self.controller.segment_count}",
            f"Freq: {self.controller.f_cmd:.1f} Hz",
            "State: "
            + (
                "Running" if self.controller.running and not self.controller.holding else
                "Holding" if self.controller.holding else
                "Completed" if self.controller.completed else
                "Idle"
            ),
        ]
        if self.controller.holding:
            status_parts.append(f"Hold {self.controller.hold_elapsed_ms}/{seg.hold_time_ms} ms")
        self.status_var.set(" | ".join(status_parts))

        # 更新曲线范围: 自动扩展横轴
        if self.time_data:
            xmax = max(200, self.time_data[-1])
            self.ax.set_xlim(0, xmax)
        self.line.set_data(self.time_data, self.freq_data)
        self.ax.relim()
        self.ax.autoscale_view(scalex=False, scaley=True)
        self.canvas.draw_idle()


def run_cli_simulation(reason: str) -> None:
   

    controller = create_default_controller()
    controller.start()

    print("=" * 72)
    print("LLC开环频率仿真 (命令行模式)")
    print(f"触发原因: {reason}")
    if not HAS_MATPLOTLIB:
        print("提示: 运行 `pip install matplotlib` 后可获得带图形界面版本。")
    print("=" * 72)
    print("时间(ms)  频率(Hz)  状态")

    max_steps = 2000
    for step in range(max_steps):
        controller.tick()
        state = (
            "Holding"
            if controller.holding
            else "Running"
            if controller.running
            else "Completed"
            if controller.completed
            else "Idle"
        )
        if step % 10 == 0 or state == "Completed":
            print(f"{step:8d}  {controller.f_cmd:9.1f}  {state}")
        if controller.completed:
            break
    else:
        print("警告: 达到最大仿真步数仍未完成, 请检查段配置。")


def main() -> None:
    if HAS_MATPLOTLIB:
        try:
            root = tk.Tk()
        except tk.TclError as exc:
            run_cli_simulation(f"无法创建 Tk 窗口: {exc}")
            return

        try:
            OpenLoopGUI(root)
        except Exception as exc:  # pragma: no cover - GUI 初始化异常回退
            root.destroy()
            run_cli_simulation(f"GUI 初始化失败: {exc}")
            return

        root.mainloop()
        return

    run_cli_simulation("未安装 matplotlib 依赖")


if __name__ == "__main__":
    main()
