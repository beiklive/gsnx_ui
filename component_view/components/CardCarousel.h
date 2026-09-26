// CardCarousel：游戏卡牌行（横排卡片 + 选中卡居中 + 焦点缩放/流光框）。
//
// 学习来源：GBAStation `src/ui/view/SwitchLayout.cpp` 的「游戏卡片行」
//   * 卡片 = 封面方块 + 标题（上方，超长跑马灯）+ 平台徽标 + 游玩时长 / 上次游玩（下方）；
//     空位画成虚线感的占位卡；封面按 Covers 裁剪进圆角块，解码中先画 shimmer 骨架。
//   * 一行固定若干槽位，选中卡永远往条带中心滚（`_updateTargetScroll`），滚动是插值不是瞬移。
//   * ←/→ 切卡 + 长按连发（HOLD_DELAY 0.30s / HOLD_REPEAT 0.085s），上下不连发；
//     卡片选中时放大、并有一圈渐变流光框，标题才滚动。
//   * 入场：整页一起从下方淡入，卡片之间再错开一点点（stagger）。
//
// 与 GBAStation 的差异（落进本组件库的约定）：
//   * 不自己写 nanovg：封面复用现成的 `Image`（Cover 裁剪 / 占位 / 圆角都现成），
//     文字用 `Draw::MarqueeText` / `Draw::Text`，焦点框用 `Draw::FlowingRing`
//     —— 形状与颜色全部来自 `Global::component_style` + `Theme`，切主题自动跟随。
//   * 交互补齐「手柄 / 触摸 / 鼠标」三条路：L/R（PageLeft/PageRight）整屏跳、
//     ←/→（capture_horizontal 自己吃掉）相邻跳 + 长按连发、触摸点击选中/再点激活、
//     横向拖动滚卡并在松手后吸附到最近一张、滚轮滚卡。
//   * 平台徽标文字与底色直接查 `PlatformBadgeInfoOf()`（与 Badge 组件同一张表）。
//
// 典型用法：
//   CardCarousel* row = panel.Emplace<CardCarousel>();
//   row->SetCards({{EmuPlatform::GBA, "黄金太阳", "12.5 小时 · 昨天", "img/a.png"}, ...});
//   connect(row, &CardCarousel::cardActivated, this, [](int i) { Launch(i); });
#pragma once

#include <string>
#include <vector>

#include "component_view/Global.h"
#include "component_view/Widget.h"
#include "component_view/components/Badge.h" // EmuPlatform / PlatformBadgeInfoOf

namespace gui_dev::cv {

class Image;

class CardCarousel : public Widget {
public:
    // 一张卡片的全部内容（都来自数据层，组件不猜）
    struct Card {
        EmuPlatform platform = EmuPlatform::Unknown; // 徽标：文字 + 底色查 PlatformBadgeInfoOf
        std::string title;                           // 主标题（放不下时跑马灯）
        std::string meta;                            // 副行：游玩时长 / 上次游玩
        std::string cover_path;                      // 封面路径（宿主注册 Global::image_source 后才有图）
        bool favourite = false;                      // 右上角小红心
        bool empty = false;                          // 占位卡（不可选中，只用来占满一行）
    };

    // 尺寸与动效（默认值 = 按 1080x600 逻辑画布调到和 Switch 版同比例）
    struct Style {
        float card_width = 148.0f;    // 卡宽（封面宽）
        float cover_height = 106.0f;  // 封面高（约 4:3，Switch 版是正方形）
        float title_height = 22.0f;   // 标题行高
        float meta_height = 20.0f;    // 副行行高
        float card_gap = 16.0f;       // 卡间距
        float cover_radius = 8.0f;    // 封面圆角
        float focus_scale = 1.055f;   // 选中卡放大比例
        float title_size = Theme::kFontBody;
        float meta_size = Theme::kFontSmall;
        float enter_speed = 3.2f;     // 入场速度（1/s）
        float enter_stagger = 0.05f;  // 每张卡错开多少秒
        float scroll_speed = 11.0f;   // 滚动插值速度（1/s）
        float hold_delay = 0.30f;     // 长按连发延迟
        float hold_repeat = 0.085f;   // 连发间隔
    };

    CardCarousel();
    ~CardCarousel() override;

    // ---- 内容 --------------------------------------------------------------
    CardCarousel& SetCards(std::vector<Card> cards, int start_index = 0);
    CardCarousel& AddCard(Card card);
    CardCarousel& SetEmptySlots(int count); // 末尾补 N 张占位卡（不进焦点）
    CardCarousel& SetStyle(const Style& value);
    CardCarousel& PlayEntrance(); // 重新播一次入场

    // ---- 选中项 ------------------------------------------------------------
    CardCarousel& SetIndex(int index, bool notify = true);
    int index() const { return index_; }
    int count() const { return static_cast<int>(cards_.size()); }
    int dataCount() const { return data_count_; }
    const Card* cardAt(int index) const;
    const Card* current() const { return cardAt(index_); }
    Rect currentCardRect() const { return CardRect(index_); }
    // 键盘/手柄 A 或触摸点击选中的卡（等价于发 cardActivated）
    CardCarousel& ActivateCurrent();
    // 该卡是否已滚到可见区（测试 / 宿主判断用）
    bool IsCardVisible(int index) const;

signals:
    Signal<int> selectionChanged;       // 选中项变了（带下标）
    Signal<int> cardActivated;          // A / 点击：启动这张卡
    Signal<int> favouriteToggled;       // X / 点右上角红心：收藏状态变了（需宿主自己保存）

protected:
    ImVec2 MeasureContent(const ImVec2& available) override;
    void OnDrawOverlay(ImDrawList* dl, const Rect& content) override; // 卡片视觉要盖住封面子节点
    void OnUpdate(float dt) override;
    void Activate() override;
    bool OnPadAction(InputAction action) override;
    void OnThemeChanged() override;

private:
    void RebuildChildren();                // 卡片数据变了：重建封面 Image 子节点
    void LayoutCards(const ImVec2& inner); // 每帧算卡片矩形（MeasureContent 里调，同帧生效）
    Rect CardRect(int index) const;        // 卡片矩形（绝对屏幕坐标；未布局时为空）
    void Step(int direction, bool wrap = true);
    void Page(int direction);
    void SnapScrollToIndex();
    int CardAtPoint(const ImVec2& point) const;
    float RowHeight() const;
    void EnsureCovers();                 // 懒加载封面（走 Global::image_source）

    std::vector<Card> cards_;
    int data_count_ = 0;
    int empty_slots_ = 0;
    int index_ = 0;
    Style style;

    // 每张卡的动画/几何状态（与 cards_ 一一对应）
    std::vector<Image*> covers_;   // 封面控件（占位卡为空指针）
    std::vector<float> focus_;     // 选中缩放进度 0..1
    std::vector<float> enter_;     // 入场进度 0..1
    // 卡片矩形存「内容区局部坐标」：Measure 阶段还不知道本帧的绝对原点，
    // 绘制/命中时再加上当帧的 content_rect.min 换算（同帧准确，不留一帧错位）。
    std::vector<Rect> rects_local_;

    float scroll_ = 0.0f;
    float scroll_target_ = 0.0f;
    float base_offset_ = 0.0f; // 内容比行短时整体居中用的偏移
    float hold_timer_ = 0.0f;
    int hold_dir_ = 0;

    // 触摸/鼠标手势
    bool drag_active_ = false;
    bool drag_moved_ = false;
    float drag_start_x_ = 0.0f;
    float drag_base_scroll_ = 0.0f;

    // 封面（路径 → 宿主纹理）。宿主缓存纹理会保活，这里只负责按路径取用 / 释放引用。
    struct Cover {
        std::string path;
        Global::ImageHandle handle;
        bool tried = false;
    };
    std::vector<Cover> loaded_;
};

} // namespace gui_dev::cv
