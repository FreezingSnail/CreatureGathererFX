#!/usr/bin/env node
// Generates docs/game-design-spec.html from the markdown section files in
// docs/game-design-spec/. Edit the markdown files, then run `make spec`.
// `make test-spec` (or --check) fails when the committed HTML is out of date.
//
// Supported markdown subset (keep content inside it):
//   # / ## / ###        section (h2 with anchor) / h3 / h4
//   - item, 1. item     unordered / ordered lists
//   | a | b |           tables (first row header, second row separator)
//   > note text         callout box
//   ``` ... ```         preformatted block
//   <...>               a line starting with < passes through as raw HTML
//   `code` **bold** *em* [text](href)  inline forms; nothing is escaped
//
// Files: 00-front.md and 99-footer.md are raw HTML passthrough; files named
// NN-*.md (sorted by name) are sections; README.md is ignored.

import { readdirSync, readFileSync, writeFileSync, existsSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const srcDir = join(root, "docs", "game-design-spec");
const outPath = join(root, "docs", "game-design-spec.html");

const escapeHtml = (s) =>
  s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");

function slug(text) {
  return text.toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-+|-+$/g, "");
}

function inline(text) {
  const codes = [];
  let s = text.replace(/`([^`]+)`/g, (_m, c) => {
    codes.push(c);
    return `\u0000${codes.length - 1}\u0000`;
  });
  s = s.replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2">$1</a>');
  s = s.replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>");
  s = s.replace(/\*([^*]+)\*/g, "<em>$1</em>");
  s = s.replace(/\u0000(\d+)\u0000/g, (_m, i) => `<code>${codes[Number(i)]}</code>`);
  return s;
}

function tableBlock(rows) {
  const cells = (row) =>
    row
      .replace(/^\||\|$/g, "")
      .split("|")
      .map((c) => c.trim());
  const body = [];
  let header = null;
  for (let r = 0; r < rows.length; r++) {
    const cols = cells(rows[r]);
    if (r === 1 && cols.every((c) => /^:?-{2,}:?$/.test(c))) continue;
    const tags = cols.map((c) => (r === 0 ? `<th>${inline(c)}</th>` : `<td>${inline(c)}</td>`)).join("");
    const line = `<tr>${tags}</tr>`;
    if (r === 0) header = line;
    else body.push(line);
  }
  return `<table>${header ?? ""}${body.join("")}</table>`;
}

function mdToHtml(md) {
  const lines = md.split("\n");
  const out = [];
  let i = 0;
  while (i < lines.length) {
    const line = lines[i];
    if (line.startsWith("<")) {
      out.push(line);
      i++;
      continue;
    }
    if (line.startsWith("```")) {
      i++;
      const buf = [];
      while (i < lines.length && !lines[i].startsWith("```")) buf.push(lines[i++]);
      i++;
      out.push(`<pre>${escapeHtml(buf.join("\n"))}</pre>`);
      continue;
    }
    if (line.startsWith("|")) {
      const rows = [];
      while (i < lines.length && lines[i].startsWith("|")) rows.push(lines[i++]);
      out.push(tableBlock(rows));
      continue;
    }
    if (line.startsWith("> ")) {
      const buf = [];
      while (i < lines.length && lines[i].startsWith("> ")) buf.push(lines[i++].slice(2));
      out.push(`<div class="note">${buf.map(inline).join(" ")}</div>`);
      continue;
    }
    if (/^- /.test(line)) {
      const buf = [];
      while (i < lines.length && /^- /.test(lines[i])) buf.push(lines[i++].slice(2));
      out.push(`<ul>${buf.map((x) => `<li>${inline(x)}</li>`).join("")}</ul>`);
      continue;
    }
    if (/^\d+\. /.test(line)) {
      const buf = [];
      while (i < lines.length && /^\d+\. /.test(lines[i]))
        buf.push(lines[i++].replace(/^\d+\. /, ""));
      out.push(`<ol>${buf.map((x) => `<li>${inline(x)}</li>`).join("")}</ol>`);
      continue;
    }
    if (line.startsWith("### ")) {
      out.push(`<h4>${inline(line.slice(4))}</h4>`);
      i++;
      continue;
    }
    if (line.startsWith("## ")) {
      out.push(`<h3>${inline(line.slice(3))}</h3>`);
      i++;
      continue;
    }
    if (line.startsWith("# ")) {
      throw new Error(`unexpected second-level '# ' heading: ${line}`);
    }
    if (line.trim() === "") {
      i++;
      continue;
    }
    const buf = [];
    while (
      i < lines.length &&
      lines[i].trim() !== "" &&
      !/^(#{1,3} |> |\d+\. |[-*] |\||```|<)/.test(lines[i])
    )
      buf.push(lines[i++]);
    out.push(`<p>${inline(buf.join(" "))}</p>`);
  }
  return out.join("\n");
}

function sectionHtml(file, body) {
  const lines = body.split("\n");
  const first = lines[0];
  if (!first.startsWith("# ")) throw new Error(`${file}: first line must be '# Title'`);
  const title = first.slice(2).trim();
  const id = slug(title.replace(/^\d+\.\s*/, ""));
  const html = mdToHtml(lines.slice(1).join("\n").trim());
  return {
    id,
    title,
    html: `<section id="${id}">\n<h2>${inline(title)}</h2>\n<p class="src">Source: <code>docs/game-design-spec/${file}</code> — edit that file and run <code>make spec</code>.</p>\n${html}\n</section>`,
  };
}

function build() {
  const files = readdirSync(srcDir).filter((f) => /^\d{2}-.*\.md$/.test(f)).sort();
  const front = files.find((f) => f.startsWith("00-"));
  const footer = files.find((f) => f.startsWith("99-"));
  if (!front || !footer) throw new Error("00-front.md and 99-footer.md are required");
  const sections = files
    .filter((f) => f !== front && f !== footer)
    .map((f) => sectionHtml(f, readFileSync(join(srcDir, f), "utf8")));

  const toc = sections
    .map((s) => `<a href="#${s.id}">${inline(s.title)}</a>`)
    .join("\n");

  return `<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>CreatureGathererFX — Game Design Spec</title>
<style>
:root{color-scheme:dark;--bg:#101010;--panel:#181818;--line:#555;--line2:#333;--fg:#eee;--dim:#bbb;--accent:#ffd75e;--ok:#7fe08a;--warn:#ffb454;--bad:#ff7b7b}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.55 system-ui,sans-serif}
main{max-width:1100px;margin:auto;padding:24px 20px 80px}
h1{font-size:clamp(28px,4.6vw,44px);margin:8px 0 4px}
h2{margin:44px 0 12px;padding-top:18px;border-top:1px solid var(--line);font-size:26px}
h3{margin:26px 0 8px;font-size:19px}
h4{margin:18px 0 6px;font-size:16px;color:var(--accent)}
p{color:var(--dim);max-width:92ch}
p.src{font-size:13px;color:#777;margin:2px 0 10px;max-width:none}
a{color:var(--accent)}
code,kbd{font:14px/1.4 ui-monospace,Menlo,Consolas,monospace;background:#242424;border:1px solid var(--line2);padding:1px 5px;border-radius:3px;color:#fff}
pre{background:var(--panel);border:1px solid var(--line2);padding:12px 14px;overflow:auto;font:13px/1.5 ui-monospace,Menlo,Consolas,monospace;color:#ddd}
table{border-collapse:collapse;margin:12px 0;width:100%;font-size:14px}
th,td{border:1px solid var(--line2);padding:5px 9px;text-align:left;vertical-align:top}
th{background:#202020;color:#fff;white-space:nowrap}
td code{font-size:12.5px}
.sub{color:#999;font-size:15px;max-width:92ch}
.meta{display:flex;flex-wrap:wrap;gap:10px;margin:14px 0 6px}
.meta span{border:1px solid var(--line);padding:3px 10px;font-size:13px;color:var(--dim)}
.tag{display:inline-block;font-size:11px;padding:1px 7px;border:1px solid var(--line);border-radius:2px;margin-left:6px;vertical-align:2px;color:var(--dim);white-space:nowrap}
.tag.live{color:var(--ok);border-color:#2f5b36}
.tag.plan{color:var(--accent);border-color:#5e5021}
.tag.ret{color:var(--bad);border-color:#5b2f2f}
.tag.cont{color:var(--warn);border-color:#5e4a21}
nav.toc{columns:2;gap:32px;border:1px solid var(--line);padding:14px 18px;background:var(--panel);margin:20px 0;max-width:900px}
nav.toc a{display:block;text-decoration:none;padding:2px 0;font-size:14px}
nav.toc a:hover{text-decoration:underline}
.note{border-left:3px solid var(--accent);background:#1c1a14;padding:8px 14px;color:var(--dim);margin:14px 0;max-width:92ch}
figure{margin:16px 0;max-width:640px}
figcaption{color:#999;font-size:13px;margin-top:6px}
svg{background:var(--panel);border:1px solid var(--line2)}
footer{border-top:1px solid var(--line);margin-top:48px;padding-top:16px;color:#888;font-size:14px}
.w{color:var(--ok);font-weight:600}.l{color:var(--bad);font-weight:600}.t{color:var(--warn);font-weight:600}
ul,ol{color:var(--dim);max-width:92ch}
li{margin:3px 0}
</style></head>
<body><main>

${readFileSync(join(srcDir, front), "utf8")}

<nav class="toc" aria-label="Table of contents">
${toc}
</nav>

${sections.map((s) => s.html).join("\n\n")}

${readFileSync(join(srcDir, footer), "utf8")}

</main></body></html>
`;
}

const html = build();
if (process.argv.includes("--check")) {
  if (!existsSync(outPath)) {
    console.error("design spec: docs/game-design-spec.html is missing; run make spec");
    process.exit(1);
  }
  const current = readFileSync(outPath, "utf8");
  if (current !== html) {
    console.error("design spec: docs/game-design-spec.html is out of date; run make spec");
    process.exit(1);
  }
  console.log("design spec: in sync");
} else {
  const current = existsSync(outPath) ? readFileSync(outPath, "utf8") : "";
  if (current === html) {
    console.log("design spec: unchanged");
  } else {
    writeFileSync(outPath, html);
    console.log(`design spec: wrote ${outPath}`);
  }
}
