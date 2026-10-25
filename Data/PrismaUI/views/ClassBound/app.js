(() => {
  window.classboundPlayLevelUpSound=()=>{
    const audio=document.getElementById('classbound-level-up-audio');
    if(!audio)return;
    audio.pause();audio.currentTime=0;audio.volume=.9;
    const playback=audio.play();
    if(playback&&typeof playback.catch==='function')playback.catch(error=>console.error('ClassBound level-up audio playback failed:',error));
  };
  const categoryNames={1:'Melee',2:'Ranged',3:'Magic',4:'Social'};
  const accents={1:'#d9a26f',2:'#89bd88',3:'#91a9db',4:'#d4bd72'};
  const sigils={1:'⚔',2:'➶',3:'✧',4:'⌂'};
  const pages=[['codex','Classes'],['change-class','Change Class'],['character','Character'],['universal-skills','Skill Tree'],['companion-skills','Companion Skills'],['followers','Followers'],['bonuses','Bonuses'],['statistics','Statistics'],['settings','Settings'],['test-tools','Cheat Menu']];
  const titles={'change-class':'Change Class',character:'Character Overview','universal-skills':'Universal Skill Tree','companion-skills':'Companion Skill Tree',followers:'Follower Company',bonuses:'Class Bonuses',statistics:'Class Statistics',settings:'ClassBound Settings','test-tools':'Progression Cheat Menu'};
  const subtitles={'change-class':'Choose a new calling and retrain with a class trainer.',character:'Your calling, progression, and combat profile.','universal-skills':'Invest shared progression points in 16 attributes. Each rank improves your character.','companion-skills':'Every companion earns and spends skill points independently.',followers:'Review companions and assign them a ClassBound role.',bonuses:'A clear summary of your active class bonuses.',statistics:'Your class journey, recorded in deeds.',settings:'Adjust progression and notification preferences.','test-tools':'Grant test XP, skill points, or levels to yourself or an active companion.'};
  let state={classes:[],followers:[],stats:{},settings:{},universalSkills:{earned:0,spent:0,available:0,attributes:[]},currentClass:0,selectedClass:0,classLevel:1,classXP:0,requiredXP:100,hasTrainer:false,changeCost:0};
  let filter='All',page='codex',pendingSkillReset=null,selectedSkillFollower=0,selectedTestSubject=0;
  const followerClassDrafts={};
  const $=id=>document.getElementById(id);
  const send=(name,value='')=>{if(typeof window[name]==='function')window[name](String(value));};
  const esc=v=>String(v??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const followerClassMark=f=>{const role=state.classes.find(c=>String(c.name).toLowerCase()===String(f?.className||'').toLowerCase());return sigils[role?.category]||'✦';};
  const activeClass=()=>state.classes.find(c=>c.id===state.currentClass);
  const classImage=c=>`assets/classes/${String(c?.name||'').toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'')}.jpg`;
  const followerPortrait=f=>{const races=['nord','imperial','redguard','breton','high_elf','wood_elf','dark_elf','orc','argonian','khajiit'];const race=races.includes(f?.race)?f.race:'nord';const sex=f?.sex==='female'?'female':'male';return `assets/followers/${race}_${sex}.jpg`;};
  const classBannerImage=c=>`assets/classes/banners/${String(c?.name||'').toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/^-|-$/g,'')}.jpg`;
  function renderNav(){
    $('page-nav').innerHTML=pages.map(([id,label])=>`<button class="${page===id?'active':''}" data-page="${id}">${label.toUpperCase()}</button>`).join('');
  }
  $('page-nav').addEventListener('click',e=>{const b=e.target.closest('[data-page]');if(!b)return;page=b.dataset.page;renderApp();});

  function renderCategories(){
    const cats=['All','Melee','Ranged','Magic','Social'];
    $('category-nav').innerHTML=cats.map(c=>`<button class="category-tab ${filter===c?'active':''}" data-category="${c}">${c==='All'?'ALL PATHS':c.toUpperCase()}</button>`).join('');
    $('category-nav').querySelectorAll('button').forEach(b=>b.addEventListener('click',()=>{filter=b.dataset.category;const shown=state.classes.filter(c=>filter==='All'||categoryNames[c.category]===filter).sort((a,b)=>a.name.localeCompare(b.name));if(shown.length&&!shown.some(c=>c.id===state.selectedClass)){state.selectedClass=shown[0].id;send('cbClassSelected',state.selectedClass);}renderCodex();}));
  }
  function renderDetails(){
    const c=state.classes.find(x=>x.id===state.selectedClass);$('choose-button').disabled=!c;
    if(!c){$('detail-image').hidden=true;$('detail-image').removeAttribute('src');$('detail-art').classList.remove('has-art');$('detail-sigil').textContent='✦';$('detail-category').textContent='SELECT A CLASS';$('detail-name').textContent='Choose a class';$('detail-playstyle').textContent='';$('detail-description').textContent='Select a class card to see its role and begin your journey.';return;}
    const detailImage=$('detail-image');detailImage.src=classImage(c);detailImage.alt=`${c.name} class artwork`;detailImage.hidden=false;$('detail-art').classList.add('has-art');detailImage.onerror=()=>{detailImage.hidden=true;$('detail-art').classList.remove('has-art');};$('detail-sigil').textContent=c.icon||sigils[c.category]||'✦';$('detail-category').textContent=categoryNames[c.category]?.toUpperCase()||'ADVENTURER';$('detail-name').textContent=c.name;$('detail-playstyle').textContent=c.playstyle||categoryNames[c.category]||'Adventurer';$('detail-description').textContent=c.description||'A new path through Skyrim awaits.';
    const current=state.currentClass===c.id;$('choose-label').textContent=current?'CURRENT CLASS':state.currentClass&&!state.hasTrainer?'TRAINER REQUIRED':state.currentClass?'CONFIRM CLASS CHANGE':'CHOOSE THIS CLASS';$('choose-button').disabled=current||!!(state.currentClass&&!state.hasTrainer);$('selection-note').textContent=state.currentClass&&!current?'Your shared level and XP stay with you when changing class. Visit a trainer to unlock retraining.':'Your choice shapes your abilities.';$('trainer-note').textContent=state.currentClass&&!current&&!state.hasTrainer?'Trainer access required':'';
  }
  function renderCodex(){
    $('page-codex').hidden=page!=='codex';$('page-other').hidden=page==='codex';
    const current=activeClass();$('active-label').textContent=current?current.name.toUpperCase():'UNBOUND';$('level-label').textContent=`LEVEL ${state.classLevel||1}`;$('journey-class').textContent=current?current.name:'No class chosen';$('journey-sub').textContent=current?(current.playstyle||categoryNames[current.category]||'Adventurer'):'Choose a calling to begin';$('hero-title').innerHTML=current?`The path of<br>${esc(current.name)} begins.`:'Every legend begins<br>with a calling.';$('hero-description').textContent=current?`Level ${state.classLevel} · ${current.description||''}`:'Choose a class to define how you fight, grow, and survive in Skyrim.';$('xp-values').textContent=`${state.classXP||0} / ${state.requiredXP||100} shared XP`;$('xp-fill').style.width=`${Math.min(100,100*(state.classXP||0)/(state.requiredXP||100))}%`;$('change-cost').textContent=current?`${Number(state.changeCost||0).toLocaleString()} gold to retrain`:'';
    if(!state.selectedClass&&state.classes.length){state.selectedClass=state.currentClass||state.classes[0].id;send('cbClassSelected',state.selectedClass);}
    renderCategories();const list=state.classes.filter(c=>filter==='All'||categoryNames[c.category]===filter).sort((a,b)=>a.name.localeCompare(b.name));$('class-count').textContent=`${list.length} ${list.length===1?'CLASS':'CLASSES'}`;$('class-grid').innerHTML=list.map(c=>`<button class="class-card ${state.selectedClass===c.id?'selected':''} ${state.currentClass===c.id?'current':''}" data-id="${c.id}" style="--accent:${accents[c.category]||'#d8b66a'}"><img class="class-thumb" src="${classImage(c)}" alt="" loading="lazy"><span class="class-card-copy"><span class="class-icon">${esc(c.icon||sigils[c.category]||'✦')}</span><span class="class-name">${esc(c.name)}</span><span class="class-role">${esc(c.playstyle||categoryNames[c.category]||'Adventurer')}</span></span></button>`).join('');$('class-grid').querySelectorAll('.class-card').forEach(card=>card.addEventListener('click',()=>send('cbClassSelected',card.dataset.id)));if(!list.some(c=>c.id===state.selectedClass))state.selectedClass=list[0]?.id||0;renderDetails();
  }
  function card(title,body,extra=''){return `<article class="info-card ${extra}"><h3>${title}</h3>${body}</article>`;}
  function renderCharacter(c){
    if(!c)return card('No class chosen','Choose a class in Classes to start your progression.');
    const level=Number(state.classLevel||1),xp=Number(state.classXP||0),required=Number(state.requiredXP||100),pct=Math.min(100,100*xp/required),remaining=Math.max(0,required-xp);
    const statNames={combat:'Combat',defense:'Defense',magic:'Magic',stealth:'Stealth',trading:'Trading'};
    const stats=Object.entries(c.stats||{}).map(([key,value])=>{const score=Math.max(0,Math.min(10,Number(value)||0));return `<div class="character-stat"><span>${esc(statNames[key]||key)}</span><div><i style="width:${score*10}%"></i></div><b>${score.toFixed(0)}<small>/10</small></b></div>`;}).join('');
    const skillPoints=state.universalSkills||{};
    return `<div class="character-overview"><header class="character-banner" style="--character-accent:${accents[c.category]||'#d8b66a'}"><img src="${classBannerImage(c)}" alt="${esc(c.name)} artwork"><div class="character-banner-shade"></div><div class="character-banner-copy"><span class="eyebrow">CLASSBOUND CHARACTER RECORD</span><h3>${esc(c.name)}</h3><p>${esc(c.playstyle||categoryNames[c.category]||'Adventurer')}</p></div><div class="character-banner-sigil">${esc(sigils[c.category]||'✦')}</div></header><section class="character-level-card"><div class="character-level-seal"><span>LEVEL</span><b>${level}</b><i>✦</i></div><div class="character-level-info"><div class="character-level-heading"><div><span class="eyebrow gold">YOUR JOURNEY</span><h4>${level>=100?'Maximum level reached':`Level ${level} progression`}</h4></div><strong>${level>=100?'MAX':`${Math.floor(pct)}%`}</strong></div><div class="character-xp-caption"><span>${xp.toLocaleString()} XP earned</span><b>${level>=100?'Level cap reached':`${remaining.toLocaleString()} XP to next level`}</b><span>${required.toLocaleString()} XP</span></div><div class="character-xp-track"><i style="width:${pct}%"></i></div><div class="character-level-foot"><span>CLASS LEVEL <b>${level} / 100</b></span><span>SHARED XP <b>${xp.toLocaleString()} / ${required.toLocaleString()}</b></span></div></div></section><div class="character-detail-grid"><section class="character-profile-panel"><div class="character-panel-heading"><span class="character-panel-icon">⚔</span><div><span class="eyebrow">COMBAT PROFILE</span><h4>Class attributes</h4></div></div><div class="character-stat-list">${stats}</div><p class="character-description">${esc(c.description||'Your class defines your path through Skyrim.')}</p></section><section class="character-growth-panel"><div class="character-panel-heading"><span class="character-panel-icon">✧</span><div><span class="eyebrow">PERSONAL GROWTH</span><h4>Universal skill points</h4></div></div><div class="character-point-pair"><div><small>AVAILABLE</small><b>${Number(skillPoints.available||0)}</b></div><div><small>INVESTED</small><b>${Number(skillPoints.spent||0)}</b></div><div><small>EARNED</small><b>${Number(skillPoints.earned||0)}</b></div></div><p class="character-description">Earn 3 universal points each level and shape your own build in the Skill Tree.</p><div class="character-status-line"><span class="status-dot"></span>${state.hasTrainer?'A class trainer is available nearby.':state.currentClass?'Your class is active.':'Your first class is free.'}</div></section></div></div>`;
  }

  const focusDisplayNames={0:'⚔ Free Combat',1:'🛡 Protect Player',2:'🎯 Focus Player Target',3:'⚡ Prioritize Casters',4:'💀 Finish Weakest'};

  function renderFollowers(){
    if(!state.followers?.length)return card('No companions found','Recruit a humanoid follower and reopen this page while they are nearby.');
    return `<div class="follower-roster">${state.followers.map(f=>{
      const selected=Number(followerClassDrafts[f.formID]??f.classID??0);
      const role=state.classes.find(c=>Number(c.id)===selected);
      const xpPct=Math.min(100,100*Number(f.xp||0)/Number(f.requiredXP||100));
      const fFocus=Number(f.tacticalFocus||0);
      return `<article class="follower-card">
        <header class="follower-card-head">
          <img class="follower-portrait" src="${followerPortrait(f)}" alt="${esc(f.race||'Nord')} ${esc(f.sex||'male')} portrait">
          <span class="follower-identity">
            <b>${esc(f.name)}</b>
            <small>${esc(f.race||'Nord')} · ${esc(f.sex||'male')} · LEVEL ${Number(f.level||1)}</small>
          </span>
          <span class="follower-class-badge">${esc(f.className||'UNASSIGNED')}</span>
        </header>
        <div class="follower-role-preview">
          ${role?`<img src="${classImage(role)}" alt=""><span><small>CLASS</small><b>${esc(role.name)}</b></span>`:'<span class="role-unassigned">Select a class to view companion progression</span>'}
        </div>
        <div class="follower-progression">
          <div class="progress-caption"><span>ADVENTURER PROGRESS</span><b>${Number(f.xp||0)} / ${Number(f.requiredXP||100)} XP</b></div>
          <div class="progress-line"><span style="width:${xpPct}%"></span></div>
        </div>
        ${f.classID?`
        <div class="follower-tactics-box">
          <div class="tactics-box-header">
            <span class="tactics-box-icon">⚔</span>
            <span class="tactics-box-title">COMPANION TARGETING</span>
          </div>
          <div class="tactics-controls-grid">
            <div class="tactics-field">
              <span class="tactics-field-label">TARGET PRIORITY</span>
              <div class="tactics-dropdown" data-dropdown="focus-${f.formID}">
                <button type="button" class="tactics-dropdown-trigger" data-tactics-toggle="focus-${f.formID}">
                  <span class="tactics-selected-value">${esc(focusDisplayNames[fFocus]||'⚔ Free Combat')}</span>
                  <span class="tactics-dropdown-arrow">▾</span>
                </button>
                <div class="tactics-dropdown-menu" id="menu-focus-${f.formID}" hidden>
                  <button type="button" class="formation-btn ${fFocus===0?'active':''}" data-priority-focus="0" data-form-id="${f.formID}">
                    <span class="formation-icon">⚔</span>
                    <span class="formation-text"><b>Free Combat</b><small>Autonomous battle decisions</small></span>
                  </button>
                  <button type="button" class="formation-btn ${fFocus===1?'active':''}" data-priority-focus="1" data-form-id="${f.formID}">
                    <span class="formation-icon">🛡</span>
                    <span class="formation-text"><b>Protect Player</b><small>Prioritize enemies attacking you</small></span>
                  </button>
                  <button type="button" class="formation-btn ${fFocus===2?'active':''}" data-priority-focus="2" data-form-id="${f.formID}">
                    <span class="formation-icon">🎯</span>
                    <span class="formation-text"><b>Focus Player Target</b><small>Attack player's chosen target</small></span>
                  </button>
                  <button type="button" class="formation-btn ${fFocus===3?'active':''}" data-priority-focus="3" data-form-id="${f.formID}">
                    <span class="formation-icon">⚡</span>
                    <span class="formation-text"><b>Prioritize Casters</b><small>Mages & ranged archers first</small></span>
                  </button>
                  <button type="button" class="formation-btn ${fFocus===4?'active':''}" data-priority-focus="4" data-form-id="${f.formID}">
                    <span class="formation-icon">💀</span>
                    <span class="formation-text"><b>Finish Weakest</b><small>Execute lowest health enemies</small></span>
                  </button>
                </div>
              </div>
            </div>
          </div>
        </div>`:''}
        <div class="follower-assign-row">
          <label><small>ASSIGN CLASS</small>
            <div class="tactics-select-wrap">
              <select class="select-control" data-follower-class="${f.formID}">
                <option value="0" ${selected===0?'selected':''}>Unassigned</option>
                ${state.classes.map(c=>`<option value="${c.id}" ${selected===c.id?'selected':''}>${esc(c.name)}</option>`).join('')}
              </select>
            </div>
          </label>
          <button class="action-button follower-assign" data-follower-assign="${f.formID}">${f.classID?'Update class':'Assign class'} <span>→</span></button>
        </div>
        ${f.classID&&selected!==Number(f.classID)?'<p class="follower-hint">Changing this role requires a trainer and gold.</p>':''}
      </article>`;
    }).join('')}</div>`;
  }
  function renderBonuses(c){if(!c)return card('No class chosen','Choose a class to activate its bonuses.');const base=(c.bonuses||[]).map(x=>`<li>✦ ${esc(x)}</li>`).join('')||'<li>No starting bonuses listed.</li>';const av=(c.avBonuses||[]).map(x=>`<li>+${Number(x.base).toFixed(0)} ${esc(x.label)} at class level 1 · +${Number(x.perLevel).toFixed(0)} per rank</li>`).join('');return card(`${esc(c.name)} · Class bonuses`,`<ul>${base}${av}</ul>`);}
  function renderChangeClass(){
    const current=activeClass();
    const selected=state.classes.find(x=>Number(x.id)===Number(state.selectedClass));
    const canConfirm=!!selected&&Number(selected.id)!==Number(state.currentClass)&&(!state.currentClass||state.hasTrainer);
    const currentBody=current?`<div class="change-current"><img src="${classImage(current)}" alt=""><div><span class="eyebrow gold">CURRENT CALLING</span><h3>${esc(current.name)}</h3><p>Level ${Number(state.classLevel||1)} · ${Number(state.classXP||0)} / ${Number(state.requiredXP||100)} shared XP</p></div></div>`:'<p>You have not chosen a class yet. Your first class is free; choose a calling below.</p>';
    const choices=state.classes.map(c=>`<button class="change-class-choice ${Number(c.id)===Number(state.selectedClass)?'selected':''} ${Number(c.id)===Number(state.currentClass)?'current':''}" data-change-class="${c.id}" style="--accent:${accents[c.category]||'#d8b66a'}"><img src="${classImage(c)}" alt=""><span><b>${esc(c.name)}</b><small>${esc(c.playstyle||categoryNames[c.category]||'Adventurer')}</small></span>${Number(c.id)===Number(state.currentClass)?'<i>ACTIVE</i>':Number(c.id)===Number(state.selectedClass)?'<i>SELECTED</i>':''}</button>`).join('');
    const requirement=state.currentClass&&!state.hasTrainer?'Speak with a ClassBound trainer before changing your class.':state.currentClass?`Retraining costs ${Number(state.changeCost||0).toLocaleString()} gold. Your shared level and XP are preserved.`:'Your first class choice is free.';
    return card('Your current class',currentBody,'change-current-card')+card(current?'Choose a new class':'Choose your first class',`<div class="change-class-grid">${choices||'<p>No classes are available.</p>'}</div>`,'change-choices-card')+card('Retraining',`<p>${esc(requirement)}</p>${selected?`<p><b>Selected:</b> ${esc(selected.name)}</p>`:''}<button class="action-button change-class-confirm" data-change-confirm="1" ${canConfirm?'':'disabled'}>${current?'Confirm class change':'Choose this class'}</button>`,'change-confirm-card');
  }
  function renderStats(){const labels={enemiesKilled:'NPCs defeated',meleeKills:'Melee kills',archeryKills:'Archery kills',magicKills:'Magic kills',sneakKills:'Sneak kills',bossesDefeated:'Bosses defeated',spellsCast:'Spells cast',itemsSold:'Items sold',itemsCrafted:'Items crafted',locksPicked:'Locks picked',dungeonsCleared:'Quests completed'};return `<div class="stat-grid">${Object.entries(labels).map(([k,v])=>`<article class="stat-tile"><strong>${Number(state.stats?.[k]||0).toLocaleString()}</strong><span>${v}</span></article>`).join('')}<article class="stat-tile"><strong>${state.classLevel||1}</strong><span>Adventurer level</span></article><article class="stat-tile"><strong>${Number(state.classXP||0).toLocaleString()}</strong><span>Shared XP</span></article></div>`;}
  function renderSkillTree(progress,target,formID=0,hasSelectedClass=true){
    const p=progress||{earned:0,spent:0,available:0,attributes:[]};
    const attrs=p.attributes||[];
    const disabledClass=!hasSelectedClass;
    return `<div class="skill-tree">${disabledClass?`<div class="skill-unassigned-warning" style="background:rgba(217,83,79,0.15);border:1px solid #d9534f;padding:12px 16px;border-radius:4px;margin-bottom:16px;color:#f8d7da;display:flex;align-items:center;gap:10px;"><span style="font-size:18px;">⚠</span><span>${target==='player'?'You must choose a class before spending skill points.':'This companion must be assigned a class before spending skill points.'}</span></div>`:''}<section class="skill-summary"><div><small>POINTS EARNED</small><b>${Number(p.earned||0)}</b></div><div><small>INVESTED</small><b>${Number(p.spent||0)}</b></div><div class="available"><small>AVAILABLE</small><b>${Number(p.available||0)}</b></div><button class="action-button skill-reset" data-skill-reset="${target}" data-form-id="${formID}" ${Number(p.spent||0)?'':'disabled'}>Refund all</button></section><div class="skill-grid">${attrs.map(a=>{const rank=Number(a.rank||0),max=Number(a.maxRank||30);return `<article class="skill-card"><div class="skill-card-head"><span class="skill-glyph">${esc(a.icon||'✦')}</span><span class="skill-name">${esc(a.name)}</span><span class="skill-rank">${rank}<i>/${max}</i></span></div><p>${esc(a.description)}</p><div class="skill-effect">${esc(a.effect)}</div><div class="rank-track">${Array.from({length:10},(_,i)=>`<i class="${rank>=(i+1)*3?'filled':''}"></i>`).join('')}</div><button class="action-button skill-upgrade" data-skill-upgrade="${target}" data-form-id="${formID}" data-attribute-id="${a.id}" ${disabledClass||Number(p.available||0)===0||rank>=max?'disabled':''}>${rank>=max?'MAX RANK':'Improve · 1 point'}</button></article>`;}).join('')}</div></div>`;
  }
  function renderCompanionSkills(){
    if(!state.followers?.length)return card('No active companions','Recruit a humanoid follower and reopen this page to manage their individual skill points.');
    if(!state.followers.some(f=>Number(f.formID)===selectedSkillFollower))selectedSkillFollower=Number(state.followers[0].formID);
    const selected=state.followers.find(f=>Number(f.formID)===selectedSkillFollower);
    const hasClass=!!(selected?.classID && Number(selected.classID)!==0);
    return `<div class="companion-skill-layout"><nav class="skill-follower-list">${state.followers.map(f=>`<button class="skill-follower-choice ${Number(f.formID)===selectedSkillFollower?'selected':''}" data-skill-follower="${f.formID}"><img class="skill-follower-portrait" src="${followerPortrait(f)}" alt=""><span><b>${esc(f.name)}</b><small>${esc(f.className||'UNASSIGNED')} · LV ${Number(f.level||1)}</small></span></button>`).join('')}</nav><section class="companion-skill-detail"><header><span class="eyebrow gold">COMPANION SKILL PROFILE</span><h3>${esc(selected.name)}</h3><p>Points and attribute ranks belong to this companion.</p></header>${renderSkillTree(selected.universalSkills,'follower',Number(selected.formID),hasClass)}</section></div>`;
  }
  const keyMap = {
    'A': 0x1E, 'B': 0x30, 'C': 0x2E, 'D': 0x20, 'E': 0x12, 'F': 0x21, 'G': 0x22, 'H': 0x23,
    'I': 0x17, 'J': 0x24, 'K': 0x25, 'L': 0x26, 'M': 0x32, 'N': 0x31, 'O': 0x18, 'P': 0x19,
    'Q': 0x10, 'R': 0x13, 'S': 0x1F, 'T': 0x14, 'U': 0x16, 'V': 0x2F, 'W': 0x11, 'X': 0x2D,
    'Y': 0x15, 'Z': 0x2C, 'F1': 0x3B, 'F2': 0x3C, 'F3': 0x3D, 'F4': 0x3E, 'F5': 0x3F, 'F6': 0x40,
    'F7': 0x41, 'F8': 0x42, 'F9': 0x43, 'F10': 0x44, 'F11': 0x57, 'F12': 0x58
  };
  const scancodeToName = Object.fromEntries(Object.entries(keyMap).map(([k, v]) => [v, k]));
  const modifierNames = { 0: 'None', 1: 'Shift', 2: 'Ctrl', 3: 'Alt' };

  let isRecordingKeybind = false;

  function renderKeybindControl() {
    const s = state.settings || {};
    const mod = Number(s.menuModifier ?? 1);
    const key = Number(s.menuKey ?? 0x2F);
    const modName = modifierNames[mod] || 'None';
    const keyName = scancodeToName[key] || 'V';
    const comboText = mod === 0 ? keyName : `${modName} + ${keyName}`;

    return `<label class="setting-row"><span>Menu Hotkey (Shortcut)<br><small style="color:#8f9a8e">Click to record key or combo (e.g. Ctrl+I, Alt+U, C, V)</small></span><span><button class="action-button ${isRecordingKeybind ? 'recording' : ''}" id="btn-record-keybind" style="min-width:140px;${isRecordingKeybind ? 'border-color:#d4bd72;background:#d4bd7233;' : ''}">${isRecordingKeybind ? 'Press key combo...' : comboText}</button></span></label>`;
  }

  function renderSettings(){
    const s=state.settings||{};
    const toggle=(key,label)=>`<label class="setting-row">${label}<input type="checkbox" data-setting="${key}" ${s[key]?'checked':''}></label>`;
    return card('Progression',`<label class="setting-row">Shared XP multiplier <span><input class="range-control" type="range" min="0.1" max="5" step="0.1" data-setting="xpMultiplier" value="${Number(s.xpMultiplier||1)}"><b id="xp-multiplier-label">×${Number(s.xpMultiplier||1).toFixed(1)}</b></span></label>${toggle('automaticClassXP','Automatic shared XP gains')}${toggle('limitClassLevelToPlayerLevel','Limit adventurer level to character level')}`)+card('Interface & feedback',`${toggle('enableNotifications','Enable All Notifications (Master Switch)')}${toggle('notifyClassXP','Shared XP notifications')}${toggle('notifyClassLevel','Level up notifications')}${toggle('showDamageNumbers','Class damage numbers')}${toggle('confirmClassChange','Confirm class changes')}${renderKeybindControl()}`);
  }
  function renderTestTools(){
    const options=[`<option value="0" ${selectedTestSubject===0?'selected':''}>Player character · Level ${Number(state.classLevel||1)}</option>`,...(state.followers||[]).map(f=>`<option value="${Number(f.formID)}" ${selectedTestSubject===Number(f.formID)?'selected':''}>${esc(f.name)} · Level ${Number(f.level||1)}</option>`)];
    const companions=state.followers||[];
    if(!options.some((_,i)=>i>0&&Number(state.followers[i-1]?.formID)===selectedTestSubject))selectedTestSubject=0;
    return card('Progression test controls',`<p class="test-warning">These actions change the selected character’s current save. They are intended for testing ClassBound progression.</p><label class="test-subject-label"><span>TEST SUBJECT</span><select class="select-control" id="test-subject">${options.join('')}</select></label><div class="test-action-grid"><section class="test-action-group"><span class="eyebrow gold">ADD XP</span><p>Uses the normal level-up flow.</p><div>${[100,500,1000].map(n=>`<button class="action-button test-action" data-test-action="xp" data-amount="${n}">+${n} XP</button>`).join('')}</div></section><section class="test-action-group"><span class="eyebrow gold">ADD SKILL POINTS</span><p>Adds spendable Universal Skill Tree points.</p><div>${[1,5,10].map(n=>`<button class="action-button test-action" data-test-action="points" data-amount="${n}">+${n} SP</button>`).join('')}</div></section><section class="test-action-group"><span class="eyebrow gold">LEVEL UP PLAYER</span><p>Raises your ClassBound level directly.</p><div>${[1,5].map(n=>`<button class="action-button test-action" data-test-action="levels" data-amount="${n}">+${n} Level${n>1?'s':''}</button>`).join('')}</div></section></div><section class="follower-level-test"><div><span class="eyebrow gold">FOLLOWER LEVEL TEST</span><p>Select an active companion, then grant levels. Their regular level-up notification, sound, and visual effect will run.</p></div><label class="test-subject-label"><span>COMPANION</span><select class="select-control" id="test-follower-subject" ${companions.length?'':'disabled'}>${companions.length?companions.map(f=>`<option value="${Number(f.formID)}">${esc(f.name)} · Level ${Number(f.level||1)}</option>`).join(''):'<option value="">No active companions</option>'}</select></label><div class="follower-level-buttons">${[1,5].map(n=>`<button class="action-button test-action" data-test-action="follower-levels" data-amount="${n}" ${companions.length?'':'disabled'}>Follower +${n} Level${n>1?'s':''}</button>`).join('')}</div></section><small class="test-cap-note">Maximum: level 100 and 300 total skill points. Test points are saved with the character.</small>`,'test-tools-card');
  }
  function renderDynamic(){
    const c=activeClass();$('dynamic-title').textContent=titles[page]||'ClassBound';$('dynamic-subtitle').textContent=subtitles[page]||'';$('dynamic-footer').textContent=page.toUpperCase();
    if(page==='change-class')$('dynamic-content').innerHTML=renderChangeClass();
    else if(['character','bonuses'].includes(page)&&!c)$('dynamic-content').innerHTML=card('Choose your calling','Open Classes and select a class to unlock this page.');
    else if(page==='character')$('dynamic-content').innerHTML=renderCharacter(c);
    else if(page==='followers')$('dynamic-content').innerHTML=renderFollowers();
    else if(page==='universal-skills')$('dynamic-content').innerHTML=renderSkillTree(state.universalSkills,'player',0,!!c);
    else if(page==='companion-skills')$('dynamic-content').innerHTML=renderCompanionSkills();
    else if(page==='bonuses')$('dynamic-content').innerHTML=renderBonuses(c);
    else if(page==='statistics')$('dynamic-content').innerHTML=renderStats();
    else if(page==='settings')$('dynamic-content').innerHTML=renderSettings();
    else if(page==='test-tools')$('dynamic-content').innerHTML=renderTestTools();
  }
  function renderApp(){renderNav();renderCodex();renderDynamic();}
  $('dynamic-content').addEventListener('click',e=>{const change=e.target.closest('[data-change-class]');if(change){send('cbClassSelected',change.dataset.changeClass);return;}const followerChoice=e.target.closest('[data-skill-follower]');if(followerChoice){selectedSkillFollower=Number(followerChoice.dataset.skillFollower);renderDynamic();return;}if(e.target.closest('[data-change-confirm]')){window.showConfirmation();return;}const upgrade=e.target.closest('[data-skill-upgrade]');if(upgrade){send('cbUniversalSkillAction',JSON.stringify({target:upgrade.dataset.skillUpgrade,formID:Number(upgrade.dataset.formId||0),attributeID:Number(upgrade.dataset.attributeId),action:'upgrade'}));return;}const reset=e.target.closest('[data-skill-reset]');if(reset){pendingSkillReset={target:reset.dataset.skillReset,formID:Number(reset.dataset.formId||0)};$('confirm-title').textContent='Refund all skill points?';$('confirm-copy').textContent='All invested ranks for this character will be removed and the points returned. This cannot be undone.';$('confirm-modal').hidden=false;return;}const assign=e.target.closest('[data-follower-assign]');if(assign){const formID=Number(assign.dataset.followerAssign);const classID=Number(followerClassDrafts[formID]??state.followers.find(f=>Number(f.formID)===formID)?.classID??0);assign.disabled=true;assign.textContent='Applying…';send('cbFollowerAssign',JSON.stringify({formID,classID}));return;}});
  $('dynamic-content').addEventListener('click',e=>{
    const recBtn = e.target.closest('#btn-record-keybind');
    if (recBtn) {
      isRecordingKeybind = !isRecordingKeybind;
      renderDynamic();
      return;
    }
    const trigger=e.target.closest('[data-tactics-toggle]');
    if(trigger){
      const menuId='menu-'+trigger.dataset.tacticsToggle;
      const menu=document.getElementById(menuId);
      if(menu){
        const isHidden=menu.hidden;
        document.querySelectorAll('.tactics-dropdown-menu').forEach(m=>m.hidden=true);
        document.querySelectorAll('.tactics-dropdown').forEach(d=>d.classList.remove('open'));
        if(isHidden){
          menu.hidden=false;
          trigger.closest('.tactics-dropdown').classList.add('open');
        }
      }
      return;
    }
    const priorityBtn=e.target.closest('[data-priority-focus]');
    if(priorityBtn){
      const formID=Number(priorityBtn.dataset.formId);
      const val=Number(priorityBtn.dataset.priorityFocus);
      priorityBtn.parentElement.querySelectorAll('.formation-btn').forEach(b=>b.classList.remove('active'));
      priorityBtn.classList.add('active');
      const follower=state.followers.find(f=>Number(f.formID)===formID);
      if(follower)follower.tacticalFocus=val;
      const dropdown=priorityBtn.closest('.tactics-dropdown');
      if(dropdown){
        dropdown.querySelector('.tactics-selected-value').textContent=focusDisplayNames[val]||'⚔ Free Combat';
        dropdown.classList.remove('open');
        const menu=dropdown.querySelector('.tactics-dropdown-menu');
        if(menu)menu.hidden=true;
      }
      send('cbFollowerTacticsChanged',JSON.stringify({formID,tacticalFocus:val}));
      return;
    }
    const button=e.target.closest('[data-test-action]');if(!button)return;send('cbTestAction',JSON.stringify({action:button.dataset.testAction,amount:Number(button.dataset.amount),formID:Number((button.dataset.testAction==='follower-levels'?$('test-follower-subject'):$('test-subject'))?.value||0)}));});
  $('dynamic-content').addEventListener('change',e=>{if(e.target.id==='test-subject'){selectedTestSubject=Number(e.target.value||0);return;}});
  $('dynamic-content').addEventListener('change',e=>{
    const tactics=e.target.closest('[data-follower-tactics]');
    if(tactics){
      const formID=Number(tactics.dataset.formId);
      const val=Number(e.target.value);
      if(tactics.dataset.followerTactics==='focus'){
        send('cbFollowerTacticsChanged',JSON.stringify({formID,tacticalFocus:val}));
      }
      return;
    }
    const follower=e.target.closest('[data-follower-class]');
    if(follower){followerClassDrafts[follower.dataset.followerClass]=Number(follower.value);renderDynamic();return;}
    if(e.target.matches('[data-setting]')){if(e.target.dataset.setting==='xpMultiplier')$('xp-multiplier-label').textContent=`×${Number(e.target.value).toFixed(1)}`;const settings={...state.settings};document.querySelectorAll('[data-setting]').forEach(el=>settings[el.dataset.setting]=el.type==='checkbox'?el.checked:Number(el.value));send('cbSettingsChanged',JSON.stringify(settings));}
  });

  window.ClassBound={update(next){state={...state,...next};renderApp();}};
  window.showConfirmation=()=>{const c=state.classes.find(x=>x.id===state.selectedClass);if(!c)return;const retrain=!!state.currentClass&&state.selectedClass!==state.currentClass;$('confirm-title').textContent=retrain?'Retrain as '+c.name+'?':'Choose '+c.name+'?';$('confirm-copy').textContent=retrain?`Your shared level and XP are preserved. The trainer will charge ${Number(state.changeCost||0).toLocaleString()} gold.`:'Your class bonuses and starter equipment will be applied to your character.';$('confirm-modal').hidden=false;};
  window.hideConfirmation=()=>{$('confirm-modal').hidden=true;pendingSkillReset=null;};
  window.confirmSelection=()=>{$('confirm-modal').hidden=true;if(pendingSkillReset){send('cbUniversalSkillAction',JSON.stringify({...pendingSkillReset,action:'reset'}));pendingSkillReset=null;}else send('cbClassConfirmed');};
  window.closeClassBound=()=>send('cbCloseClassBound');
  window.addEventListener('keydown', e => {
    if (isRecordingKeybind) {
      e.preventDefault();
      e.stopPropagation();
      if (e.key === 'Escape') {
        isRecordingKeybind = false;
        renderDynamic();
        return;
      }
      let keyUpper = e.key.toUpperCase();
      if (keyUpper === 'CONTROL' || keyUpper === 'SHIFT' || keyUpper === 'ALT') return;
      if (keyUpper === 'I' || keyUpper === 'İ' || keyUpper === 'ı') keyUpper = 'I';
      const scancode = keyMap[keyUpper];
      if (scancode) {
        let mod = 0;
        if (e.shiftKey) mod = 1;
        else if (e.ctrlKey) mod = 2;
        else if (e.altKey) mod = 3;
        state.settings = { ...state.settings, menuModifier: mod, menuKey: scancode };
        isRecordingKeybind = false;
        send('cbSettingsChanged', JSON.stringify(state.settings));
        renderDynamic();
      }
      return;
    }
    if (e.key === 'Escape') {
      e.preventDefault();
      e.stopPropagation();
      if (!$('confirm-modal').hidden) window.hideConfirmation();
      else window.closeClassBound();
    }
  });
  renderApp();
})();
