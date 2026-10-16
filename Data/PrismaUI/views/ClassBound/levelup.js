(() => {
  const label = document.getElementById('level-up');
  const text = document.getElementById('level-up-text');
  let hideTimer = 0;
  let activeEvent = '';
  window.classboundShowLevelUp = (x, y, message, eventToken = 'legacy') => {
    label.style.left = `${x * 100}%`;
    label.style.top = `${y * 100}%`;
    text.textContent = message || 'LEVEL UP!';
    const isNewEvent = String(eventToken) !== activeEvent;
    if (isNewEvent) {
      activeEvent = String(eventToken);
      window.clearTimeout(hideTimer);
      label.classList.remove('enter');
      label.classList.add('visible', 'enter');
      window.setTimeout(() => label.classList.remove('enter'), 450);
      hideTimer = window.setTimeout(() => window.classboundHideLevelUp(), 2600);
    } else if (!label.classList.contains('visible')) {
      // Reappear at the tracked position if the actor temporarily leaves the viewport.
      label.classList.add('visible');
    }
  };
  window.classboundHideLevelUp = () => label.classList.remove('visible', 'enter');
})();
