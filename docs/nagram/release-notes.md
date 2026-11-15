# Release notes

`docs/nagram/releases/<tag>.md` holds the notes of one release, in Chinese, with an
optional `<tag>.en.md`. `nagram-release.yml` uses the file as the GitHub release
body and posts it to the channels: the changelog is quoted under "更新日志 |
Changelog", `## 上游` is folded inside the quotation, `## 安装包` is left out (the
files replace it) and `## 安装` is folded after the quotation.

- Write only what changed since the previous stable release, beta content
  included. No opening paragraph (base version, testing status, where to report).
- Top-level sections, in this order, empty ones left out: `## 新增` (features or
  settings that did not exist before), `## 改善` (changes to existing features or
  behaviour: defaults, layout, renames, packaging), `## 修复`, then `## 上游`,
  `## 安装包`, `## 安装`.
- Group a section with many items under `### <group>` (翻译, 设置与界面, 安装与发行,
  ...). The post shows a section as a bold label and a group as a plain one.
- One item describes one action. Split an item that does several things or
  fixes several problems; no "；同时", no explanatory tail.
- A new setting is `<u>设置-Nagram-<page>[-<subpage>]</u> <setting>`, following the
  pages of the app; when the path is not certain, write `「<feature>」 | <what>`.
- A fix is `「<feature>」 | <symptom>的问题`: what the user saw, not how it was fixed.
  Without a clear feature name, write the symptom alone.
- Wish and feedback ids stay at the end of their item, as `（#WD0001）`.
- The last item of a group ends with `。`, the others with `；`.
- `## 上游` follows the same rules, one upstream change per item.
- English notes mirror the Chinese ones section by section, group by group and
  item by item (`## New`, `## Improvements`, `## Fixes`); fixes are noun
  phrases naming the symptom, items end with `;` and the last with `.`.
