// Shared x/y matchup table for the mock pages.
// Usage: <div class="matrix" data-matrix="wheel|stances" data-highlight="on"></div>
// Rows attack columns. ▲ row wins, ▼ row loses, · neutral or mirror.
// With data-highlight="on" the table tints the chosen creatures' row/column
// from window.__battleMock (the running mock) once per poll.
(function (root) {
  'use strict';

  function source(kind) {
    const data = root.CGFX_SPIKE_DATA;
    if (kind.startsWith('spike:')) {
      const id = kind.slice('spike:'.length);
      const entry = root.CGFX_MATRICES && root.CGFX_MATRICES.matrices
        ? root.CGFX_MATRICES.matrices.find((matrix) => matrix.id === id) : null;
      return entry ? { order: entry.order, chart: entry.chart } : null;
    }
    return kind === 'stances' ? data.merged : data.wheel;
  }

  function mark(value, diagonal) {
    if (diagonal) return '<span class="m-mirror">&middot;</span>';
    if (value === 2) return '<span class="m-win">&#9650;</span>';
    if (value === 0.5) return '<span class="m-lose">&#9660;</span>';
    return '<span class="m-flat">&middot;</span>';
  }

  // Header sprites: draw the shared 8x8 icon art to a data-URI so the headers
  // use the same sprites as the badges. Falls back to text when no canvas.
  const iconCache = new Map();
  function iconImage(type) {
    if (iconCache.has(type)) return iconCache.get(type);
    const art = (root.CGFX_ICONS && (root.CGFX_ICONS.ICONS[type] || root.CGFX_ICONS.MERGED_ICONS[type])) || null;
    let html = type;
    if (art && typeof document.createElement === 'function') {
      try {
        const canvas = document.createElement('canvas');
        canvas.width = 8;
        canvas.height = 8;
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = '#fff';
        for (let y = 0; y < 8; y++) for (let x = 0; x < 8; x++) if (art[y][x] === '#') ctx.fillRect(x, y, 1, 1);
        html = `<img class="m-icon" src="${canvas.toDataURL()}" alt="${type}" title="${type}">`;
      } catch (error) {
        html = type; // canvas unavailable: keep the text header
      }
    }
    iconCache.set(type, html);
    return html;
  }

  function highlight(kind) {
    const state = root.__battleMock;
    if (!state) return {};
    const data = root.CGFX_SPIKE_DATA;
    const you = data.creatures[state.youIndex];
    const foe = data.creatures[state.foeIndex];
    if (!you || !foe) return {};
    if (kind === 'stances') return { row: you.merged[0], col: foe.merged[0] };
    const matrix = kind.startsWith('spike:') ? source(kind) : null;
    const fold = (type) => (matrix && matrix.map ? (matrix.map[type] || type) : type);
    return { row: fold(you.types[0]), col: fold(foe.types[0]) };
  }

  function render(container) {
    let kind = container.dataset.matrix || 'wheel';
    const state = root.__battleMock;
    // Duel windows follow the chosen type-count table live.
    if (container.dataset.followDuel === 'on' && state && state.duel) {
      kind = state.duel.kind === 'stances' ? 'stances' : `spike:${state.duel.matrixId || '8'}`;
    }
    const matrix = source(kind);
    if (!matrix) return;
    const { order, chart } = matrix;
    const marks = container.dataset.highlight === 'on' ? highlight(kind) : {};
    const labels = kind === 'stances' ? order : order;
    let html = '<table><tr><th class="corner">atk \\ def</th>'
      + labels.map((type) => {
        const cls = marks.col === type ? ' class="hl-col"' : '';
        return `<th${cls}>${iconImage(type)}</th>`;
      }).join('') + '</tr>';
    order.forEach((type, row) => {
      const rowClass = marks.row === type ? ' class="hl-row"' : '';
      html += `<tr><th${rowClass}>${iconImage(type)}</th>`
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
