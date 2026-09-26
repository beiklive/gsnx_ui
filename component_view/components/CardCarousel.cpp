#include "component_view/components/CardCarousel.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "component_view/Anim.h"
#include "component_view/Draw.h"
#include "component_view/Global.h"
#include "component_view/components/Content.h"
#include "ui/Icons.h"

namespace gui_dev::cv {
namespace {

constexpr float kHeartSize = 16.0f;    // 收藏红心图标
constexpr float kBadgeHeight = 20.0f;  // 平台徽标高
constexpr float kBadgePadX = 7.0f;     // 徽标左右内边距
constexpr float kDragThreshold = 8.0f; // 超过这个横向位移才算拖动（与 Page 的滚动阈值一致）
constexpr float kEnterOffsetY = 18.0f; // 入场时从下方滑上来的距离

} // namespace

CardCarousel::CardCarousel() : Widget("card_carousel") {
    // 复合控件：整行只作一个焦点停靠点，←/→ 自己吃掉（切卡），离开这一行用 ↑/↓ 或 B。
    focusable = true;
    focus_only_self = true;
    capture_horizontal = true;
    focus_frame = false;         // 焦点指示画在选中那张卡上（不给整行再画一圈）
    overflow = Overflow::Hidden; // 卡片超出可视区要裁掉
    background = 0;
    background_follows_theme = false;
}

CardCarousel::~CardCarousel() {
    // 封面是宿主给的纹理：这里只把引用还回去（宿主自己决定缓存的生死）
    if (Global::image_source.release != nullptr) {
        for (Cover& slot : loaded_) {
            if (slot.handle.texture.GetTexID() != ImTextureID_Invalid) {
                Global::image_source.release(slot.handle);
            }
        }
    }
}

// ---------------------------------------------------------------- 内容 ----

CardCarousel& CardCarousel::SetCards(std::vector<Card> cards, int start_index) {
    cards_ = std::move(cards);
    data_count_ = 0;
    for (const Card& card : cards_) {
        if (!card.empty) {
            ++data_count_;
        }
    }
    // 占位卡统一排到数据卡后面（学习 GBAStation：一行固定槽位，空位留在尾部）
    std::stable_partition(cards_.begin(), cards_.end(), [](const Card& card) { return !card.empty; });
    index_ = data_count_ <= 0
                 ? 0
                 : static_cast<int>(Clampf(static_cast<float>(start_index), 0.0f, static_cast<float>(data_count_ - 1)));
    scroll_ = 0.0f;
    scroll_target_ = 0.0f;
    hold_timer_ = 0.0f;
    hold_dir_ = 0;
    RebuildChildren();
    PlayEntrance();
    return *this;
}

CardCarousel& CardCarousel::AddCard(Card card) {
    const bool empty = card.empty;
    cards_.push_back(std::move(card));
    if (!empty) {
        ++data_count_;
        if (data_count_ == 1) {
            index_ = 0;
        }
    }
    RebuildChildren();
    return *this;
}

CardCarousel& CardCarousel::SetEmptySlots(int count) {
    empty_slots_ = count < 0 ? 0 : count;
    cards_.erase(std::remove_if(cards_.begin(), cards_.end(), [](const Card& card) { return card.empty; }),
                 cards_.end());
    for (int i = 0; i < empty_slots_; ++i) {
        Card placeholder;
        placeholder.empty = true;
        cards_.push_back(std::move(placeholder));
    }
    RebuildChildren();
    return *this;
}

CardCarousel& CardCarousel::SetStyle(const Style& value) {
    style = value;
    for (std::size_t i = 0; i < covers_.size(); ++i) {
        if (covers_[i] != nullptr) {
            covers_[i]->setRadius(style.cover_radius);
        }
    }
    return *this;
}

CardCarousel& CardCarousel::PlayEntrance() {
    enter_.assign(cards_.size(), 0.0f);
    focus_.assign(cards_.size(), 0.0f);
    return *this;
}

const CardCarousel::Card* CardCarousel::cardAt(int index) const {
    if (index < 0 || index >= static_cast<int>(cards_.size())) {
        return nullptr;
    }
    return &cards_[static_cast<std::size_t>(index)];
}

CardCarousel& CardCarousel::SetIndex(int index, bool notify) {
    if (data_count_ <= 0) {
        return *this;
    }
    const int next = static_cast<int>(Clampf(static_cast<float>(index), 0.0f, static_cast<float>(data_count_ - 1)));
    if (next == index_) {
        return *this;
    }
    index_ = next;
    SnapScrollToIndex();
    if (notify) {
        emit selectionChanged(index_);
    }
    return *this;
}

CardCarousel& CardCarousel::ActivateCurrent() {
    if (index_ >= 0 && index_ < static_cast<int>(cards_.size()) && !cards_[static_cast<std::size_t>(index_)].empty) {
        emit cardActivated(index_);
    }
    return *this;
}

bool CardCarousel::IsCardVisible(int index) const {
    if (index < 0 || index >= static_cast<int>(rects_local_.size())) {
        return false;
    }
    const Rect& card = rects_local_[static_cast<std::size_t>(index)];
    return card.Valid() && card.max.x > 0.0f && card.min.x < content_rect.Width();
}

Rect CardCarousel::CardRect(int index) const {
    if (index < 0 || index >= static_cast<int>(rects_local_.size())) {
        return Rect{};
    }
    return rects_local_[static_cast<std::size_t>(index)].Translate(content_rect.min);
}

// ---------------------------------------------------------------- 子节点 ----

void CardCarousel::RebuildChildren() {
    Clear();
    covers_.clear();
    loaded_.clear();
    covers_.resize(cards_.size(), nullptr);
    loaded_.resize(cards_.size());
    focus_.assign(cards_.size(), 0.0f);
    enter_.assign(cards_.size(), 0.0f);
    rects_local_.assign(cards_.size(), Rect{});

    for (std::size_t i = 0; i < cards_.size(); ++i) {
        if (cards_[i].empty) {
            continue; // 占位卡整张自己画（描边 + 图标 + 「空位」），不需要封面控件
        }
        Image* cover = Emplace<Image>();
        cover->SetName("card_cover");
        cover->setFit(Image::Fit::Cover); // 等比放大填满 + 裁掉超出（= GBAStation 的封面裁剪）
        cover->setRadius(style.cover_radius);
        cover->interactive = false; // 命中测试落在 CardCarousel 自己身上
        cover->focusable = false;
        covers_[i] = cover;
    }
}

void CardCarousel::EnsureCovers() {
    if (Global::image_source.load == nullptr) {
        return; // 宿主没注册 image_source：封面走 Image 自己的占位框，不报错
    }
    for (std::size_t i = 0; i < cards_.size() && i < covers_.size(); ++i) {
        Image* cover = covers_[i];
        if (cover == nullptr) {
            continue;
        }
        Cover& slot = loaded_[i];
        const std::string& path = cards_[i].cover_path;
        if (slot.tried && slot.path == path) {
            continue; // 同一路径只取一次
        }
        if (slot.handle.texture.GetTexID() != ImTextureID_Invalid && Global::image_source.release != nullptr) {
            Global::ImageHandle old = slot.handle;
            Global::image_source.release(old); // 换图：先把旧的还回去
        }
        slot = Cover{};
        slot.path = path;
        slot.tried = true;
        if (!path.empty()) {
            slot.handle = Global::image_source.load(path.c_str());
        }
        if (slot.handle.Valid()) {
            cover->setTexture(slot.handle.texture, static_cast<float>(slot.handle.width),
                              static_cast<float>(slot.handle.height));
        }
    }
}

// ---------------------------------------------------------------- 几何 ----

float CardCarousel::RowHeight() const {
    return style.cover_height + style.title_height + style.meta_height + 10.0f;
}

ImVec2 CardCarousel::MeasureContent(const ImVec2& available) {
    const float width = size.x > 0.0f ? size.x : Maxf(available.x, 1.0f);
    const float height = size.y > 0.0f ? size.y : RowHeight();
    const float chrome_x = padding.Horizontal() + (border.width + border.inset) * 2.0f;
    const float chrome_y = padding.Vertical() + (border.width + border.inset) * 2.0f;
    LayoutCards(ImVec2(Maxf(width - chrome_x, 1.0f), Maxf(height - chrome_y, 1.0f)));
    return ImVec2(width, height);
}

// 内容区局部坐标：card i 的左边界 = base_offset + i*pitch - scroll
void CardCarousel::LayoutCards(const ImVec2& inner) {
    if (cards_.empty()) {
        return;
    }
    const float pitch = style.card_width + style.card_gap;
    const float total = static_cast<float>(cards_.size()) * pitch - style.card_gap;
    const float row_h = style.cover_height + style.title_height + style.meta_height;
    base_offset_ = total <= inner.x ? (inner.x - total) * 0.5f : 0.0f; // 内容比行短 → 整行居中
    const float scroll_max = Maxf(total - inner.x, 0.0f);
    scroll_ = Clampf(scroll_, 0.0f, scroll_max);
    const float top = Maxf((inner.y - row_h) * 0.5f, 0.0f);

    for (std::size_t i = 0; i < cards_.size(); ++i) {
        const float scale = 1.0f + (style.focus_scale - 1.0f) * focus_[i];
        const float w = style.card_width * scale;
        const float cover_h = style.cover_height * scale;
        const float text_h = (style.title_height + style.meta_height) * scale;
        const float left = base_offset_ + static_cast<float>(i) * pitch - scroll_;
        const float center_x = left + style.card_width * 0.5f;
        const float top_y = top + (1.0f - Anim::EaseOutCubic(enter_[i])) * kEnterOffsetY;
        const Rect card{ImVec2(center_x - w * 0.5f, top_y), ImVec2(center_x + w * 0.5f, top_y + cover_h + text_h)};
        rects_local_[i] = card;

        if (covers_[i] != nullptr) {
            covers_[i]->position = ImVec2(card.min.x, top_y); // 相对本控件内容区
            covers_[i]->size = ImVec2(w, cover_h);
            covers_[i]->opacity = Anim::EaseOutCubic(enter_[i]);
        }
    }
}

// ---------------------------------------------------------------- 每帧 ----

void CardCarousel::OnUpdate(float dt) {
    if (cards_.empty()) {
        return;
    }
    EnsureCovers();

    for (std::size_t i = 0; i < cards_.size(); ++i) {
        if (enter_[i] < 1.0f) {
            const float delay = Minf(0.30f, static_cast<float>(i) * style.enter_stagger);
            enter_[i] = Anim::SmoothTo(enter_[i], 1.0f, style.enter_speed / Maxf(1.0f - delay, 0.2f), dt);
        }
        const float target = (static_cast<int>(i) == index_) ? 1.0f : 0.0f;
        focus_[i] = Anim::SmoothTo(focus_[i], target, 14.0f, dt);
    }
    scroll_ = Anim::SmoothTo(scroll_, scroll_target_, style.scroll_speed, dt);

    // 手柄长按连发（只做左右，和 GBAStation 一致；上下不连发）
    if (focused) {
        const bool left = Global::pad.Held(InputAction::Left);
        const bool right = Global::pad.Held(InputAction::Right);
        const int dir = (left && !right) ? -1 : ((right && !left) ? 1 : 0);
        if (dir == 0) {
            hold_timer_ = 0.0f;
            hold_dir_ = 0;
        } else if (dir != hold_dir_) {
            hold_dir_ = dir;
            hold_timer_ = -style.hold_delay; // 首个重复之前先等 hold_delay
        } else {
            hold_timer_ += dt;
            while (hold_timer_ >= style.hold_repeat) {
                hold_timer_ -= style.hold_repeat;
                Step(dir);
            }
        }
    }

    // ---- 触摸 / 鼠标：点选、再点激活、横向拖动滚卡 ----
    const bool inside = Global::mouse_available && content_rect.Contains(Global::mouse);
    if (Global::mouse_pressed[0] && inside) {
        drag_active_ = true;
        drag_moved_ = false;
        drag_start_x_ = Global::mouse.x;
        drag_base_scroll_ = scroll_;
        drag_base_index_ = index_;
        // 横向拖动由这一行自己处理（别让外层页面同时纵向滚）；
        // 一旦真的拖起来就置 pointer_dragging —— 基类看到它才不会把这次手势当成点击。
        Global::pointer_drag_host = nullptr;
    }
    if (drag_active_) {
        if (!Global::mouse_down[0]) {
            drag_active_ = false;
            if (drag_moved_) {
                // 松手：选中项 = 按下时那张 + 拖过的卡数，再把它吸附到行中心
                // （差值不到半张卡宽，所以不会「弹回原来的焦点位置」）。
                SyncIndexToDrag();
                SnapScrollToIndex();
                Global::pointer_dragging = false;
            }
            // 没拖动的手指点击交给基类的 Activate()（见下面 Activate）：
            // 这里再选一次会和它重复，表现为「点一下既选中又启动」。
        } else {
            const float delta = Global::mouse.x - drag_start_x_;
            if (Absf(delta) > kDragThreshold) {
                drag_moved_ = true;
                Global::pointer_dragging = true; // 取消这次点击
            }
            if (drag_moved_) {
                const float total = static_cast<float>(cards_.size()) * (style.card_width + style.card_gap) -
                                    style.card_gap;
                scroll_ = Clampf(drag_base_scroll_ - delta, 0.0f, Maxf(total - content_rect.Width(), 0.0f));
                scroll_target_ = scroll_;
                SyncIndexToDrag(); // 选中跟着手指走（只改下标与信号，不动 scroll_target_）
            }
        }
    }

    // 滚轮：横向滚卡（鼠标 / 触控板）
    if (inside && Global::mouse_wheel != 0.0f) {
        const float total =
            static_cast<float>(cards_.size()) * (style.card_width + style.card_gap) - style.card_gap;
        scroll_target_ = Clampf(scroll_target_ - Global::mouse_wheel * 60.0f, 0.0f,
                                Maxf(total - content_rect.Width(), 0.0f));
    }

    // 有焦点且没在拖：选中卡始终往行中心靠（= GBAStation 的 _updateTargetScroll）
    if (focused && !drag_active_) {
        SnapScrollToIndex();
    }
}

void CardCarousel::Activate() {
    // 触摸 / 鼠标：点哪张选哪张；点已选中的那张才是「启动」
    if (Global::mouse_available && Global::hovered == this) {
        const int hit = CardAtPoint(Global::mouse);
        if (hit >= 0) {
            if (hit == index_) {
                ActivateCurrent();
            } else {
                SetIndex(hit);
            }
            return;
        }
    }
    ActivateCurrent();
}

bool CardCarousel::OnPadAction(InputAction action) {
    switch (action) {
    case InputAction::Left: // ← 上一张（长按连发在 OnUpdate 里推进）
        Step(-1);
        return true;
    case InputAction::Right: // → 下一张
        Step(1);
        return true;
    case InputAction::PageLeft: // L = 上一屏
        Page(-1);
        return true;
    case InputAction::PageRight: // R = 下一屏
        Page(1);
        return true;
    case InputAction::ActionX: { // X = 收藏 / 取消收藏当前卡
        if (index_ >= 0 && index_ < static_cast<int>(cards_.size()) && !cards_[static_cast<std::size_t>(index_)].empty) {
            cards_[static_cast<std::size_t>(index_)].favourite = !cards_[static_cast<std::size_t>(index_)].favourite;
            emit favouriteToggled(index_);
            return true;
        }
        return false;
    }
    case InputAction::Confirm: // A = 启动当前卡（触摸路径走 Activate）
        ActivateCurrent();
        return true;
    default:
        return false; // B / 其它按键不消费，留给页面
    }
}

// ---------------------------------------------------------------- 导航 ----

void CardCarousel::Step(int direction, bool wrap) {
    if (data_count_ <= 0) {
        return;
    }
    int next = index_ + direction;
    if (wrap) {
        next = ((next % data_count_) + data_count_) % data_count_; // 首尾相接
    } else {
        next = static_cast<int>(Clampf(static_cast<float>(next), 0.0f, static_cast<float>(data_count_ - 1)));
    }
    if (next == index_) {
        return;
    }
    index_ = next;
    SnapScrollToIndex();
    emit selectionChanged(index_);
}

void CardCarousel::Page(int direction) {
    if (data_count_ <= 0) {
        return;
    }
    const float pitch = style.card_width + style.card_gap;
    const int per_page = static_cast<int>(Maxf(std::floor(Maxf(content_rect.Width(), pitch) / pitch), 1.0f));
    SetIndex(index_ + direction * per_page);
}

void CardCarousel::SnapScrollToIndex() {
    const float pitch = style.card_width + style.card_gap;
    const float total = static_cast<float>(cards_.size()) * pitch - style.card_gap;
    const float selected_center = base_offset_ + static_cast<float>(index_) * pitch + style.card_width * 0.5f;
    const float view = Maxf(content_rect.Width(), 1.0f);
    scroll_target_ = Clampf(selected_center - view * 0.5f, 0.0f, Maxf(total - view, 0.0f));
    if (content_rect.Width() <= 0.0f) {
        scroll_ = scroll_target_; // 还没布局过（第一帧）：直接对齐，别看到滑动
    }
}

// 拖动时选中第几张：按下时那张 + 拖过的卡数（位移 / pitch，四舍五入）
int CardCarousel::IndexFromDrag() const {
    if (data_count_ <= 0) {
        return 0;
    }
    const float pitch = style.card_width + style.card_gap;
    const float shift = (scroll_ - drag_base_scroll_) / Maxf(pitch, 1.0f);
    const int index = drag_base_index_ + static_cast<int>(std::lround(shift));
    return static_cast<int>(Clampf(static_cast<float>(index), 0.0f, static_cast<float>(data_count_ - 1)));
}

// 选中项跟着手指走（只改 index_ 与信号，不动 scroll_target_，免得和手指打架）
void CardCarousel::SyncIndexToDrag(bool notify) {
    const int next = IndexFromDrag();
    if (next == index_) {
        return;
    }
    index_ = next;
    if (notify) {
        emit selectionChanged(index_);
    }
}

int CardCarousel::CardAtPoint(const ImVec2& point) const {
    const ImVec2 local(point.x - content_rect.min.x, point.y - content_rect.min.y);
    for (std::size_t i = 0; i < rects_local_.size(); ++i) {
        if (rects_local_[i].Valid() && rects_local_[i].Contains(local)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ---------------------------------------------------------------- 绘制 ----

// 注意：卡片上的徽标 / 红心 / 焦点框要盖在「封面 Image 子节点」之上，
// 所以画在 OnDrawOverlay（子节点之后），不能放 OnDrawContent。
void CardCarousel::OnDrawOverlay(ImDrawList* dl, const Rect& content) {
    if (dl == nullptr || cards_.empty()) {
        return;
    }
    const Global::ComponentStyle& cs = Global::component_style;
    const float opacity = EffectiveOpacity();
    const float phase = Global::time * cs.focus_flow_speed;

    for (std::size_t i = 0; i < cards_.size(); ++i) {
        const Card& card = cards_[i];
        const Rect card_rect = rects_local_[i].Translate(content.min);
        if (!card_rect.Valid()) {
            continue;
        }
        if (card_rect.max.x < content.min.x - 4.0f || card_rect.min.x > content.max.x + 4.0f) {
            continue; // 完全在可视区外：不画
        }
        const float enter = Anim::EaseOutCubic(enter_[i]);
        const float card_opacity = opacity * enter;
        if (card_opacity <= 0.004f) {
            continue;
        }

        const bool selected = static_cast<int>(i) == index_;
        const float focus_mix = focus_[i];
        const float scale = 1.0f + (style.focus_scale - 1.0f) * focus_mix;
        const Rect cover{card_rect.min, ImVec2(card_rect.max.x, card_rect.min.y + style.cover_height * scale)};
        const float radius = style.cover_radius;

        if (card.empty) {
            // 占位卡：底 + 描边 + 居中图标 + 「空位」（学习 GBAStation 的 _drawEmptyCard）
            Draw::RoundedRectFilled(dl, cover, Theme::U32(Theme::kBgWidget, 0.30f * card_opacity), radius, radius,
                                    radius, radius);
            Draw::RoundedRectOutline(dl, cover, Theme::U32(Theme::kBorderStrong, 0.75f * card_opacity),
                                     Maxf(cs.border_width, 1.0f), radius, radius, radius, radius);
            const float icon_size = 30.0f;
            const char* glyph = Icons::Glyph(Icons::Material::ImagePlaceholder);
            const ImVec2 glyph_extent = Draw::MeasureText(nullptr, icon_size, glyph, 0.0f);
            Draw::Text(dl, nullptr, icon_size,
                       ImVec2(cover.Center().x - glyph_extent.x * 0.5f,
                              cover.Center().y - glyph_extent.y * 0.5f - icon_size * 0.35f),
                       Theme::U32(Theme::kTextMuted, 0.65f * card_opacity), glyph);
            const char* empty_label = "空位";
            const ImVec2 label_extent = Draw::MeasureText(nullptr, style.meta_size, empty_label, 0.0f);
            Draw::Text(dl, nullptr, style.meta_size,
                       ImVec2(cover.Center().x - label_extent.x * 0.5f,
                              cover.Center().y + icon_size * 0.35f - label_extent.y * 0.5f),
                       Theme::U32(Theme::kTextMuted, 0.65f * card_opacity), empty_label);
            continue;
        }

        // ---- 封面上的平台徽标（查表结果与 Badge 组件同源） ----
        const PlatformBadgeInfo info = PlatformBadgeInfoOf(card.platform);
        if (info.text != nullptr && info.text[0] != '\0') {
            const float badge_font = style.meta_size - 2.0f;
            const ImVec2 text_extent = Draw::MeasureText(nullptr, badge_font, info.text, 0.0f);
            const float badge_w = Maxf(text_extent.x + kBadgePadX * 2.0f, kBadgeHeight + 14.0f);
            const Rect badge{ImVec2(cover.min.x + 6.0f, cover.max.y - kBadgeHeight - 6.0f),
                             ImVec2(cover.min.x + 6.0f + badge_w, cover.max.y - 6.0f)};
            Draw::RoundedRectFilled(dl, badge, Theme::U32(info.background, card_opacity), 4.0f, 4.0f, 4.0f, 4.0f);
            Draw::Text(dl, nullptr, badge_font,
                       ImVec2(badge.Center().x - text_extent.x * 0.5f, badge.Center().y - text_extent.y * 0.5f),
                       Theme::U32(Theme::kWhite, card_opacity), info.text);
        }
        // ---- 收藏红心 ----
        if (card.favourite) {
            const char* heart = Icons::Glyph(Icons::Material::Favorite);
            const ImVec2 heart_extent = Draw::MeasureText(nullptr, kHeartSize, heart, 0.0f);
            Draw::Text(dl, nullptr, kHeartSize, ImVec2(cover.max.x - heart_extent.x - 7.0f, cover.min.y + 6.0f),
                       Theme::U32(Theme::kError, card_opacity), heart);
        }

        // ---- 标题 / 副行（封面下方；选中卡才跑马灯） ----
        const float text_y = cover.max.y + 4.0f;
        const Rect title_slot{ImVec2(card_rect.min.x, text_y),
                              ImVec2(card_rect.max.x, text_y + style.title_height * scale)};
        const Rect meta_slot{ImVec2(card_rect.min.x, title_slot.max.y),
                             ImVec2(card_rect.max.x, title_slot.max.y + style.meta_height * scale)};
        const float title_alpha = card_opacity * (selected ? 1.0f : 0.78f);
        if (selected && focused) {
            Draw::MarqueeText(dl, nullptr, style.title_size, title_slot,
                              Theme::U32(Theme::kTextPrimary, title_alpha), card.title.c_str(), Global::time,
                              cs.marquee_speed);
        } else {
            const char* shown = Draw::Ellipsize(nullptr, style.title_size, card.title.c_str(), title_slot.Width());
            const ImVec2 extent = Draw::MeasureText(nullptr, style.title_size, shown, 0.0f);
            Draw::Text(dl, nullptr, style.title_size,
                       ImVec2(title_slot.Center().x - extent.x * 0.5f, title_slot.Center().y - extent.y * 0.5f),
                       Theme::U32(Theme::kTextPrimary, title_alpha), shown);
        }
        if (!card.meta.empty()) {
            const char* shown = Draw::Ellipsize(nullptr, style.meta_size, card.meta.c_str(), meta_slot.Width());
            const ImVec2 extent = Draw::MeasureText(nullptr, style.meta_size, shown, 0.0f);
            Draw::Text(dl, nullptr, style.meta_size,
                       ImVec2(meta_slot.Center().x - extent.x * 0.5f, meta_slot.Center().y - extent.y * 0.5f),
                       Theme::U32(Theme::kTextMuted, card_opacity), shown);
        }

        // ---- 选中卡：焦点框（形状 / 粗细 / 颜色 / 是否 pause 风格都跟 Button 同一套全局约定） ----
        if (selected && focus_mix > 0.01f) {
            const float margin = cs.focus_margin;
            const Rect ring = cover.Expanded(margin);
            const float ring_radius = radius + margin;
            const float alpha = card_opacity * focus_mix * (focused ? 1.0f : 0.45f);
            if (Global::pause_focus_frame) {
                Draw::RoundedRectOutline(dl, ring, Theme::U32(Theme::kError, alpha * 0.55f),
                                         Maxf(cs.focus_width, 1.0f), ring_radius, ring_radius, ring_radius, ring_radius);
            } else {
                Draw::FlowingRing(dl, ring, cs.focus_width, phase, cs.focus_saturation, cs.focus_brightness, alpha,
                                  3.0f, ring_radius);
            }
        }
    }
}

void CardCarousel::OnThemeChanged() {
    background = 0;
    background_follows_theme = false;
}

} // namespace gui_dev::cv
