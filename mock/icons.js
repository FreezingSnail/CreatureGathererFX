// Shared 8x8 type icon art for the mock: used by battle-mock.js (badges, cells,
// centre strip) and matrix.js (x/y table headers). Browser: window.CGFX_ICONS;
// CommonJS: module.exports.
(function (root) {
  'use strict';
  const ICONS = {
    spirit: [
      '..####..', '.#....#.', '#.#..#.#', '#......#',
      '#......#', '#.#..#.#', '#.#..#.#', '.#.##.#.'],
    water: [
      '...#....', '..###...', '.#####..', '.#####..',
      '#######.', '#######.', '.#####..', '..###...'],
    wind: [
      '.#####..', '##...##.', '.....#..', '..####..',
      '.#......', '#....##.', '##...##.', '.#####..'],
    earth: [
      '........', '...#....', '..###...', '.#####..',
      '#######.', '#######.', '........', '#######.'],
    fire: [
      '...#....', '..##....', '..###...', '.#####..',
      '.#####..', '.#####..', '..###...', '...#....'],
    lightning: [
      '....###.', '...###..', '..###...', '.#####..',
      '...###..', '...##...', '..##....', '..#.....'],
    plant: [
      '....##..', '...###..', '..####..', '.#####..',
      '#####...', '####....', '###.....', '##......'],
    elder: [
      '..####..', '.#....#.', '#..##..#', '#.####.#',
      '#..##..#', '.#....#.', '..####..', '...##...'],
    status: [
      '........', '...#....', '...#....', '..###...',
      '...#....', '...#....', '........', '........'],
    none: [
      '........', '.######.', '.#....#.', '.#....#.',
      '.#....#.', '.######.', '........', '........'],
  };
  // 4-type cycle reuses the closest 8-type art as placeholder icons.
  const MERGED_ICONS = {
    flame: ICONS.fire, bloom: ICONS.plant, tide: ICONS.water, stone: ICONS.earth,
  };
  if (typeof module !== 'undefined' && module.exports) module.exports = { ICONS, MERGED_ICONS };
  else root.CGFX_ICONS = { ICONS, MERGED_ICONS };
})(typeof window !== 'undefined' ? window : this);
