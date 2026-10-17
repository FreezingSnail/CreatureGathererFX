# Editing the design spec

`docs/game-design-spec.html` is **generated**. Do not edit it by hand.

- Edit the numbered markdown files in this directory.
- Run `make spec` to regenerate the HTML.
- `make test-spec` fails when the committed HTML is out of date (run it before committing).
- Every rendered section links back to its source file at the top, so reviewers can jump straight here.

File roles:

| File | Contents |
|---|---|
| `00-front.md` | Title, meta pills, status legend (raw HTML passthrough) |
| `01`…`15` | Spec sections, assembled in filename order; each starts with `# N. Title` |
| `99-footer.md` | Closing sources paragraph (raw HTML passthrough) |

Markdown subset the generator understands (keep content inside it):

- `# N. Title` — one per file; `## ` sub-heading; `### ` minor heading
- `- item` lists, `1. item` ordered lists
- `| a | b |` tables (first row header, second row separator)
- `> text` callout box
- fenced code blocks (triple backticks)
- inline: backtick code, `**bold**`, `*italic*`, `[text](href)`
- status pills: paste the raw span, e.g. `<span class="tag live">LIVE</span>`
- a line starting with `<` passes through as raw HTML (used for the type-web SVG)
- nothing is escaped: write `&amp;` / `&lt;` yourself where literal characters are needed

Keep each section a short, single-topic file so the owner can review and edit it independently.
