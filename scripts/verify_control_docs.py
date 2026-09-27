#!/usr/bin/env python3
"""校验 docs/controls/*.md 里提到的 API 名是否真实存在（且尽量是文档声称的那个文件里的）。

用法：python3 scripts/verify_control_docs.py

判定分三档：
  ok      —— 文档里出现的代码标识符都能在「它对应的源文件 + 公共头」里找到
  [warn]  —— 能在仓库别处找到，但不在对应源文件里（可能引用了别的组件/公共设施，也可能文档写错了文件）
  [?]     —— 全仓库都找不到（疑似虚构，人工确认；示例里的局部变量名/字符串通常落在这里）
"""
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# 文档 -> 它应该描述的源文件（可有多个）
DOC_SOURCES = {
    "Widget.md": ["component_view/Widget.h"],
    "WidgetTree.md": ["component_view/Widget.h", "component_view/Object.h"],
    "Box.md": ["component_view/components/Box.h"],
    "Label.md": ["component_view/components/Content.h"],
    "Separator.md": ["component_view/components/Content.h"],
    "ProgressBar.md": ["component_view/components/Content.h"],
    "Image.md": ["component_view/components/Content.h"],
    "Button.md": ["component_view/components/Button.h"],
    "Badge.md": ["component_view/components/Badge.h"],
    "Header.md": ["component_view/components/Header.h"],
    "TabColumn.md": ["component_view/components/TabColumn.h"],
    "CapsuleTabs.md": ["component_view/components/CapsuleTabs.h"],
    "CardCarousel.md": ["component_view/components/CardCarousel.h"],
    "FunctionBar.md": ["component_view/components/FunctionBar.h"],
    "RichText.md": ["component_view/components/RichText.h"],
    "ImageViewer.md": ["component_view/components/ImageViewer.h"],
    "Popup.md": ["component_view/popup/Popup.h", "component_view/popup/PopupManager.h"],
    "Toast.md": ["component_view/Toast.h"],
    "Page.md": ["component_view/pages/Page.h"],
    "FocusRing.md": ["component_view/FocusRing.h"],
    "UILayer.md": ["component_view/UILayer.h"],
}

# 公共设施：示例代码里合理出现的东西
SHARED = [
    "component_view/Theme.h", "component_view/Types.h", "component_view/Global.h", "component_view/Draw.h",
    "component_view/Anim.h", "component_view/Format.h", "framework/ui/Icons.h", "framework/ui/UiContext.h",
    "component_view/popup/PopupManager.h", "component_view/Widget.h", "component_view/Object.h",
]

WHITELIST = set("""
int float double bool void const constexpr auto struct class enum namespace std string vector size_t uint32_t
uint8_t uint16_t uint64_t intptr_t nullptr true false return if else for while new delete public private
protected virtual override static inline using template typename char unsigned signed long short
ImVec2 ImVec4 ImU32 ImDrawList ImFont ImFontConfig ImTextureID ImGuiIO ImGui ImGuiStyle imvec2
Object Widget Box Label Separator ProgressBar Image Button Badge Header TabColumn CapsuleTabs CardCarousel
FunctionBar ImageViewer RichText Popup PopupManager Toast Page FocusRing UILayer Global Theme Draw Anim
Types Format FocusManager TextureRef Connection Emplace AddTo Root
""".split())


def read(rel: str) -> str:
    try:
        return open(os.path.join(ROOT, rel), errors="ignore").read()
    except OSError:
        return ""


def code_spans(text: str):
    for m in re.finditer(r"`([^`\n]+)`", text):
        yield m.group(1)
    for m in re.finditer(r"```[a-z]*\n(.*?)```", text, flags=re.S):
        yield m.group(1)


def main() -> int:
    docs = sorted(glob.glob(os.path.join(ROOT, "docs/controls/*.md")))
    if not docs:
        print("没有 docs/controls/*.md")
        return 1

    corpus_files = [p for p in glob.iglob("**/*", root_dir=ROOT, recursive=True)
                    if p.endswith((".h", ".cpp", ".md")) and "/build" not in p and "third_party" not in p]
    all_corpus = "\n".join(read(p) for p in corpus_files)
    shared_text = "\n".join(read(p) for p in SHARED)

    clean = 0
    warn_total = 0
    unknown_total = 0
    for doc in docs:
        name = os.path.basename(doc)
        text = open(doc, errors="ignore").read()
        tokens = set()
        for span in code_spans(text):
            for m in re.finditer(r"[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*", span):
                for piece in m.group(0).split("::"):
                    tokens.add(piece)
        own = "\n".join(read(p) for p in DOC_SOURCES.get(name, []))
        own_or_shared = own + "\n" + shared_text
        warn, unknown = [], []
        for tok in sorted(tokens):
            if tok in WHITELIST or len(tok) <= 2:
                continue
            if re.search(r"\b" + re.escape(tok) + r"\b", own_or_shared):
                continue
            (warn if re.search(r"\b" + re.escape(tok) + r"\b", all_corpus) else unknown).append(tok)
        if warn or unknown:
            line = f"{name}:"
            if warn:
                line += f" [warn] {' '.join(warn)}"
            if unknown:
                line += f" [?] {' '.join(unknown)}"
            print(line)
        else:
            clean += 1
            print(f"{name}: ok")
        warn_total += len(warn)
        unknown_total += len(unknown)

    print(f"--- 完全命中 {clean}/{len(docs)} 篇；[warn] {warn_total} 个（仓库别处有）；"
          f"[?] {unknown_total} 个（全仓库没有，需人工确认）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
