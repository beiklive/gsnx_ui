// CapsuleTabs：胶囊标签条（横排标签 + 选中项背后一个会滑动的胶囊高亮）。
//
// 几何与动效取自 GBAStation 游戏库顶部的「L/R 切换机种」轮播（RecyclingGrid::_drawToolbar），
// 那套是纯手柄 UI，这里补上了触摸/鼠标点击与焦点语义，放进组件库当通用控件：
//   * 选中项永远停在条带中心，其它标签按中心距左右展开（间距 132）
//   * 胶囊 = 104 x 42、圆角 = 高的一半（21，完全胶囊形）
//       填充 白 22~44 alpha、描边 1px 白 70~135 alpha、阴影 offset(3,3) / blur 5
//       ——alpha 都随「离中心的距离」prominence 线性插值
//   * 标签字号 17→22、透明度 0.42→1.0 也由 prominence 推出来；超出 1.55 个间距的直接不画
//   * 切项时整条横滑：t 线性推进（速度 8/s ≈ 125ms），位置用 easeOutCubic(t)，
//     所以是「新选中项从相邻格滑进来」，胶囊跟着滑入并淡入
//
// 交互（Focus 优先，和库里其它控件一致）：
//   * L / R（PageLeft / PageRight）切上一项 / 下一项
//   * 触摸 / 鼠标点哪个标签就选哪个；点已选中的那个（或手柄 A）发 activated
//   * 两套主题都取调色板：浅色主题下胶囊换成黑色半透明（Theme::kCapsuleFill）
#pragma once

#include <string>
#include <vector>

#include "component_view/Widget.h"

namespace gui_dev::cv {

class CapsuleTabs : public Widget {
public:
    // 尺寸 / 动效参数（默认值 = GBAStation 平台轮播那套；改这里就能整体调）
    struct Style {
        float spacing = 132.0f;       // 相邻标签中心距
        float capsule_width = 104.0f; // 胶囊宽
        float capsule_height = 42.0f; // 胶囊高（圆角取一半 → 胶囊形）
        float font_min = 17.0f;       // 最靠边的标签字号
        float font_max = 22.0f;       // 中心标签字号
        float alpha_min = 0.42f;      // 最靠边的标签透明度
        float fade_span = 1.55f;      // 超过这么多个间距（|相对格位|）就不画
        float slide_speed = 8.0f;     // 横滑速度（1/s，8 ≈ 125ms 走完一格）
        float fill_alpha = 22.0f / 255.0f;       // 胶囊填充基准 alpha（prominence=0）
        float fill_alpha_max = 44.0f / 255.0f;   // 中心处填充 alpha
        float stroke_alpha = 70.0f / 255.0f;     // 描边基准 alpha
        float stroke_alpha_max = 135.0f / 255.0f;// 中心处描边 alpha
        ImVec2 shadow_offset{3.0f, 3.0f};        // 胶囊阴影偏移
        float shadow_blur = 5.0f;                // 胶囊阴影柔化半径
    };

    CapsuleTabs();
    explicit CapsuleTabs(std::vector<std::string> values);

    std::vector<std::string> labels;
    int index = 0;    // 当前选中项
    bool wrap = true; // L/R 到头是否绕回
    Style style;

    CapsuleTabs& setLabels(std::vector<std::string> values, int start_index = 0);
    CapsuleTabs& setIndex(int value, bool notify = true);
    const char* currentLabel() const;
    int count() const { return static_cast<int>(labels.size()); }

signals:
    Signal<int> selectionChanged; // 选中项变了（带新下标）
    Signal<int> activated;        // 手柄 A / 再点一次已选中项

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawContent(ImDrawList* dl, const Rect& content) override;
    void OnUpdate(float dt) override;
    void Activate() override;
    bool OnPadAction(InputAction action) override;

private:
    // 当前动画的格位偏移：0 = 已归位，±1 = 刚切完还停在相邻格
    float SlotOffset() const;
    float LabelCenterX(float center_x, int item_index) const;
    // 这个 x 落在哪个标签上（-1 = 离所有标签都太远）
    int ItemAtX(float x) const;

    float slide_ = 1.0f; // 横滑进度 0..1（1 = 归位，第一帧不播动画）
    int slide_dir_ = 0;  // +1 = 往右切（下一个），-1 = 往左切
};

} // namespace gui_dev::cv
