(() => {
  const stack = document.getElementById('notification-stack');
  const styles = {
    xp: {label: 'Experience gained', icon: '✦', duration: 4000},
    level: {label: 'Level up', icon: '✪', duration: 5600},
    success: {label: 'ClassBound', icon: '✓', duration: 4200},
    warning: {label: 'Attention', icon: '!', duration: 5200},
    info: {label: 'ClassBound', icon: '✧', duration: 4200}
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
    icon.textContent = style.icon;
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
