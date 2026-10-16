(() => {
  const stack = document.getElementById('notification-stack');
  const styles = {
    xp: {label: 'Experience gained', duration: 4000, icon: '<path d="M12 2.8 14.8 9l6.7.7-5 4.5 1.4 6.6L12 17.4l-5.9 3.4 1.4-6.6-5-4.5L9.2 9 12 2.8Z"/><path d="m12 6.8 1.7 3.6 3.9.4-2.9 2.6.8 3.8-3.5-2-3.5 2 .8-3.8-2.9-2.6 3.9-.4L12 6.8Z" fill="currentColor" stroke="none"/>'},
    level: {label: 'Level up', duration: 5600, icon: '<path d="M4 19h16M6 16l4-4 3 2 5-7M14 7h4v4"/><path d="M12 2.5 13.5 5l2.9.5-2.1 2 .5 2.9-2.8-1.4-2.6 1.4.5-2.9-2.1-2 2.9-.5L12 2.5Z"/>'},
    success: {label: 'ClassBound', duration: 4200, icon: '<circle cx="12" cy="12" r="9"/><path d="m7.5 12.2 3 3 6-6.4"/>'},
    warning: {label: 'Attention', duration: 5200, icon: '<path d="M12 3 22 20H2L12 3Z"/><path d="M12 9v5m0 3h.01"/>'},
    info: {label: 'ClassBound', duration: 4200, icon: '<circle cx="12" cy="12" r="9"/><path d="M12 11v5m0-9h.01"/>'}
  };
  const remove = (toast) => {
    if (!toast.isConnected) return;
    toast.classList.add('toast-leave');
    window.setTimeout(() => toast.remove(), 260);
  };

  window.classboundNotify = (message, category = 'info') => {
    const kind = styles[category] ? category : 'info';
    const style = styles[kind];
    const toast = document.createElement('section');
    toast.className = `toast toast-${kind}`;
    toast.setAttribute('role', kind === 'warning' ? 'alert' : 'status');
    const icon = document.createElement('span');
    icon.className = 'toast-icon';
    icon.setAttribute('aria-hidden', 'true');
    icon.innerHTML = `<svg viewBox="0 0 24 24" focusable="false">${style.icon}</svg>`;
    const copy = document.createElement('span');
    copy.className = 'toast-copy';
    const label = document.createElement('small');
    label.className = 'toast-label';
    label.textContent = style.label;
    const text = document.createElement('span');
    text.className = 'toast-message';
    text.textContent = String(message || '');
    copy.append(label, text);
    toast.append(icon, copy);
    stack.prepend(toast);

    while (stack.children.length > 5) remove(stack.lastElementChild);
    window.setTimeout(() => remove(toast), style.duration);
  };
})();
