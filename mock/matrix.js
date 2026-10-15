// Shared x/y matchup table for the mock pages.
// Usage: <div class="matrix" data-matrix="wheel|stances" data-highlight="on"></div>
// Rows attack columns. ▲ row wins, ▼ row loses, · neutral or mirror.
// With data-highlight="on" the table tints the chosen creatures' row/column
// from window.__battleMock (the running mock) once per poll.
(function (root) {
  'use strict';

  function source(kind) {
    const data = root.CGFX_SPIKE_DATA;
    return kind === 'stances' ? data.merged : data.wheel;
  }

  function mark(value, diagonal) {
    if (diagonal) return '<span class="m-mirror">&middot;</span>';
    if (value === 2) return '<span class="m-win">&#9650;</span>';
    if (value === 0.5) return '<span class="m-lose">&#9660;</span>';
    return '<span class="m-flat">&middot;</span>';
  }

  function highlight(kind) {
    const state = root.__battleMock;
    if (!state) return {};
    const data = root.CGFX_SPIKE_DATA;
    const you = data.creatures[state.youIndex];
    const foe = data.creatures[state.foeIndex];
    if (!you || !foe) return {};
    return kind === 'stances'
      ? { row: you.merged[0], col: foe.merged[0] }
      : { row: you.types[0], col: foe.types[0] };
  }

  function render(container) {
    const kind = container.dataset.matrix || 'wheel';
    const { order, chart } = source(kind);
    const marks = container.dataset.highlight === 'on' ? highlight(kind) : {};
    const labels = kind === 'stances' ? order : order;
    let html = '<table><tr><th class="corner">atk \\ def</th>'
      + labels.map((type) => {
        const cls = marks.col === type ? ' class="hl-col"' : '';
        return `<th${cls}>${type}</th>`;
      }).join('') + '</tr>';
    order.forEach((type, row) => {
      const rowClass = marks.row === type ? ' class="hl-row"' : '';
      html += `<tr><th${rowClass}>${type}</th>`
        + chart[row].map((value, col) => {
          const classes = [];
          if (marks.row === type) classes.push('hl-row');
          if (marks.col === order[col]) classes.push('hl-col');
          if (marks.row === type && marks.col === order[col]) classes.push('hl-x');
          const cls = classes.length ? ` class="${classes.join(' ')}"` : '';
          return `<td${cls}>${mark(value, row === col)}</td>`;
        }).join('') + '</tr>';
    });
    html += '</table>';
    container.innerHTML = html;
  }

  function init() {
    for (const container of document.querySelectorAll('[data-matrix]')) {
      render(container);
      if (container.dataset.highlight === 'on') setInterval(() => render(container), 400);
    }
  }

  root.CGFXM = { render };
  if (typeof document !== 'undefined') {
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init);
    else init();
  }
})(typeof window !== 'undefined' ? window : this);
