(() => {
  const label = document.getElementById('level-up');
  const text = document.getElementById('level-up-text');
  let hideTimer = 0;
  window.classboundShowLevelUp = (x, y, message) => {
    label.style.left = `${x * 100}%`;
    label.style.top = `${y * 100}%`;
    text.textContent = message || 'LEVEL UP!';
    if (!label.classList.contains('visible')) {
      label.classList.add('visible', 'enter');
      window.setTimeout(() => label.classList.remove('enter'), 450);
    }
    window.clearTimeout(hideTimer);
    hideTimer = window.setTimeout(() => window.classboundHideLevelUp(), 2600);
  };
  window.classboundHideLevelUp = () => label.classList.remove('visible', 'enter');
})();
