import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync, readdirSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..", "..");
const srcDir = join(root, "docs", "game-design-spec");
const html = readFileSync(join(root, "docs", "game-design-spec.html"), "utf8");

const sectionFiles = () =>
  readdirSync(srcDir)
    .filter((f) => /^\d{2}-.*\.md$/.test(f) && !f.startsWith("00-") && !f.startsWith("99-"))
    .sort();

test("generated html is in sync with the markdown sources", () => {
  execFileSync(process.execPath, ["tools/build-design-spec.mjs", "--check"], {
    cwd: root,
    stdio: "pipe",
  });
});

test("every section file is rendered and links back to its source", () => {
  const files = sectionFiles();
  assert.ok(files.length >= 10, `expected 10+ section files, found ${files.length}`);
  for (const f of files) {
    assert.ok(
      html.includes(`docs/game-design-spec/${f}`),
      `section file ${f} has no source reference in the html`,
    );
  }
});

test("table of contents links match section anchors in order", () => {
  const ids = [...html.matchAll(/<section id="([^"]+)"/g)].map((m) => m[1]);
  assert.ok(ids.length >= 10);
  const toc = html.match(/<nav class="toc"[\s\S]*?<\/nav>/)?.[0] ?? "";
  const hrefs = [...toc.matchAll(/href="#([^"]+)"/g)].map((m) => m[1]);
  assert.deepEqual(hrefs, ids);
});
