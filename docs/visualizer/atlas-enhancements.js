(()=>{'use strict';
const PROFILE='https://raw.githubusercontent.com/xtreemze/qmk_userspace/halcyon/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil';
const RAW_USAGE_PAGE=0xFF60;
const RAW_USAGE=0x61;
const RGB_COMMAND=0xF0;
const OP={CAPABILITIES:0x01,GET_PROFILE:0x02,GET_COMBO_DURATION:0x06};
const SCOPE={GLOBAL:0,LAYER:1,MODIFIER:2,COMBO:3};
const UNASSIGNED=0xFF;
const TAP_FIELDS=['tap','hold','double tap','tap + hold'];
const MOD_PROFILE_NAMES=['Ctrl','GUI','Shift','Alt'];
const CUSTOM={USER10:'Save active-layer RGB',USER11:'Save modifier RGB',USER12:'Save chord RGB',USER13:'Trigger chord RGB',USER14:'Redo',USER15:'Paste',USER16:'Copy',USER17:'Cut',USER18:'Undo',USER19:'Select all',USER20:'OS trace'};
const SIMPLE={
 KC_SPACE:'Space',KC_BSPACE:'Backspace',KC_TAB:'Tab',KC_ENTER:'Enter',KC_ESCAPE:'Escape',KC_DELETE:'Delete',KC_HOME:'Home',KC_END:'End',KC_PGUP:'Page Up',KC_PGDOWN:'Page Down',
 KC_UP:'Up',KC_DOWN:'Down',KC_LEFT:'Left',KC_RIGHT:'Right',KC_QUOTE:"'",KC_SCOLON:';',KC_COMMA:',',KC_DOT:'.',KC_SLASH:'/',KC_GRAVE:'`',KC_MINUS:'−',KC_EQUAL:'=',KC_BSLASH:'\\',KC_LBRACKET:'[',KC_RBRACKET:']',KC_KP_EQUAL:'Num =',KC_KP_COMMA:'Num ,',
 KC_BTN1:'Mouse 1',KC_BTN2:'Mouse 2',KC_BTN3:'Mouse 3',KC_BTN4:'Mouse 4',KC_BTN5:'Mouse 5',KC_MS_U:'Mouse ↑',KC_MS_D:'Mouse ↓',KC_MS_L:'Mouse ←',KC_MS_R:'Mouse →',KC_WH_U:'Scroll ↑',KC_WH_D:'Scroll ↓'
};
const SHIFTED={KC_1:'!',KC_2:'@',KC_3:'#',KC_4:'$',KC_5:'%',KC_6:'^',KC_7:'&',KC_8:'*',KC_9:'(',KC_0:')',KC_MINUS:'_',KC_EQUAL:'+',KC_LBRACKET:'{',KC_RBRACKET:'}',KC_BSLASH:'|',KC_SCOLON:':',KC_QUOTE:'"',KC_COMMA:'<',KC_DOT:'>',KC_SLASH:'?',KC_GRAVE:'~'};
let profile=null;
let hidDevice=null;
let hidReportId=0;
let hidPacketLength=32;
let rgbState=null;
let refreshQueued=false;
const $=id=>document.getElementById(id);

function cleanKeycode(k){return typeof k==='string'?k:String(k)}
function shortText(s,max=28){s=String(s);return s.length>max?s.slice(0,max-1)+'…':s}
function macroText(index,depth=0){
 if(!profile||depth>3)return 'Macro '+index;
 const steps=profile.macro?.[index];
 if(!Array.isArray(steps)||steps.length===0)return 'Macro '+index+' (empty)';
 const parts=[];
 for(const step of steps){
   if(!Array.isArray(step)||step.length===0)continue;
   if(step[0]==='text')parts.push('type “'+String(step[1]??'').replace(/\n/g,'↵')+'”');
   else if(step[0]==='tap')parts.push(step.slice(1).map(k=>actionText(k,depth+1)).join(' then '));
   else parts.push(step[0]+' '+step.slice(1).join(' '));
 }
 return parts.join(', ')||('Macro '+index);
}
function actionText(k,depth=0){
 k=cleanKeycode(k);
 if(k==='KC_NO'||k==='-1')return 'no action';
 if(k==='KC_TRNS')return 'inherit';
 if(CUSTOM[k])return CUSTOM[k];
 if(SIMPLE[k])return SIMPLE[k];
 let m=/^KC_([A-Z])$/.exec(k);if(m)return m[1];
 m=/^KC_([0-9])$/.exec(k);if(m)return m[1];
 m=/^KC_F(\d+)$/.exec(k);if(m)return 'F'+m[1];
 m=/^M(\d+)$/.exec(k);if(m)return macroText(Number(m[1]),depth+1);
 m=/^TD\((\d+)\)$/.exec(k);if(m)return 'tap dance '+m[1];
 m=/^MO\((\d+)\)$/.exec(k);if(m)return 'hold Layer '+m[1];
 m=/^TG\((\d+)\)$/.exec(k);if(m)return 'toggle Layer '+m[1];
 m=/^TO\((\d+)\)$/.exec(k);if(m)return 'switch to Layer '+m[1];
 m=/^LT(\d+)\((.+)\)$/.exec(k);if(m)return actionText(m[2],depth+1)+' / hold Layer '+m[1];
 m=/^OSM\((.+)\)$/.exec(k);if(m)return 'one-shot '+modifierText(m[1]);
 m=/^(LSFT|RSFT)\((.+)\)$/.exec(k);if(m){if(SHIFTED[m[2]])return SHIFTED[m[2]];return (m[1]==='LSFT'?'Left Shift':'Right Shift')+' + '+actionText(m[2],depth+1)}
 m=/^(LCTL|RCTL|LALT|RALT|LGUI|RGUI)\((.+)\)$/.exec(k);if(m)return modifierToken(m[1])+' + '+actionText(m[2],depth+1);
 m=/^HYPR\((.+)\)$/.exec(k);if(m)return 'Hyper + '+actionText(m[1],depth+1);
 m=/^C_S\((.+)\)$/.exec(k);if(m)return 'Ctrl + Shift + '+actionText(m[1],depth+1);
 m=/^SGUI\((.+)\)$/.exec(k);if(m)return 'Shift + GUI + '+actionText(m[1],depth+1);
 return k.replace(/^KC_/,'').replaceAll('_',' ');
}
function modifierToken(token){return ({LCTL:'Left Ctrl',RCTL:'Right Ctrl',LALT:'Left Alt',RALT:'Right Alt',LGUI:'Left GUI',RGUI:'Right GUI',LSFT:'Left Shift',RSFT:'Right Shift'})[token]||token}
function modifierText(mask){return mask.replace(/MOD_/g,'').split('|').map(modifierToken).join(' + ')}
function tapDanceInfo(index){
 const td=profile?.tap_dance?.[index];
 if(!Array.isArray(td))return null;
 return {index,actions:td.slice(0,4).map(k=>actionText(k)),term:Number(td[4]||0),raw:td};
}
function comboInfo(index){
 const combo=profile?.combo?.[index];
 if(!Array.isArray(combo))return null;
 const inputs=combo.slice(0,4).filter(k=>k&&k!=='KC_NO');
 const output=combo[4];
 if(inputs.length===0||!output||output==='KC_NO')return null;
 return {index,inputs,output,outputText:actionText(output)};
}
function currentLayer(){const m=/L(\d+)/.exec($('layerNumber')?.textContent||'');return m?Number(m[1]):1}
function visibleKeycodes(layer){
 if(!profile?.layout?.[layer])return new Set();
 const result=new Set();
 for(const row of [0,1,2,3,5,6,7,8])for(const k of profile.layout[layer][row]||[])if(k&&k!==-1&&k!=='KC_NO'&&k!=='KC_TRNS')result.add(k);
 return result;
}
function ensurePanel(){
 if($('behaviorPanel'))return;
 const detail=document.querySelector('.detail');
 const panel=document.createElement('section');panel.id='behaviorPanel';panel.className='behavior-panel';panel.setAttribute('aria-label','Configured Vial behaviors');
 panel.innerHTML='<div class="behavior-head"><div><h2>Configured Vial behaviors</h2><p>Functional definitions from the canonical profile. Tap dances show their four gestures; combos show trigger chords and resulting actions. Saved RGB profiles can be read from the connected keyboard.</p></div></div><div class="behavior-grid"><section class="behavior-card"><h3>Tap dances on this layer</h3><div id="tapDanceList" class="behavior-list"></div></section><section class="behavior-card"><h3>Combos</h3><div id="comboList" class="behavior-list"></div></section><section class="behavior-card"><h3>RGB profiles</h3><div id="rgbProfileInfo"></div></section></div>';
 detail?.parentNode?.insertBefore(panel,detail);
}
function ensureRgbButton(){
 if($('connectRgb'))return;
 const switches=document.querySelector('.switches');if(!switches)return;
 const b=document.createElement('button');b.type='button';b.id='connectRgb';b.className='switch rgb-connect';b.dataset.state='idle';b.textContent='Connect keyboard RGB';b.addEventListener('click',connectRgb);switches.append(b);
}
function renderTapDances(layer){
 const host=$('tapDanceList');if(!host)return;
 const items=[];
 const seen=new Set();
 for(const row of [0,1,2,3,5,6,7,8])for(const k of profile.layout[layer][row]||[]){const m=/^TD\((\d+)\)$/.exec(String(k));if(m&&!seen.has(m[1])){seen.add(m[1]);const info=tapDanceInfo(Number(m[1]));if(info)items.push(info)}}
 host.replaceChildren();
 if(items.length===0){host.innerHTML='<div class="behavior-empty">No tap-dance keycodes are placed on this layer.</div>';return}
 for(const info of items){
   const div=document.createElement('div');div.className='behavior-item is-current';
   div.innerHTML='<div class="behavior-title">TD('+info.index+')<span class="behavior-chip">'+info.term+' ms term</span></div><div class="behavior-meta">'+TAP_FIELDS.map((name,i)=>'<strong>'+name+':</strong> '+escapeHtml(info.actions[i])).join(' · ')+'</div>';
   host.append(div);
 }
}
function renderCombos(layer){
 const host=$('comboList');if(!host)return;
 const visible=visibleKeycodes(layer);host.replaceChildren();
 const combos=[];for(let i=0;i<(profile.combo?.length||0);i++){const info=comboInfo(i);if(info)combos.push(info)}
 if(combos.length===0){host.innerHTML='<div class="behavior-empty">No active combos are configured.</div>';return}
 for(const info of combos){
   const isCurrent=info.inputs.every(k=>visible.has(k));
   const div=document.createElement('div');div.className='behavior-item'+(isCurrent?' is-current':'');
   const rgb=rgbState?.combos?.[info.index];const swatch=rgb&&assigned(rgb)?swatchHtml(rgb):'';
   div.innerHTML='<div class="behavior-title"><span class="behavior-sequence">'+info.inputs.map(k=>escapeHtml(actionText(k))).join(' + ')+'</span><span class="behavior-arrow">→</span> '+escapeHtml(info.outputText)+(isCurrent?'<span class="behavior-chip">on this layer</span>':'')+'</div>'+(rgb&&assigned(rgb)?'<div class="behavior-meta">'+swatch+'combo RGB override · '+profileText(rgb)+'</div>':'');
   host.append(div);
 }
}
function decorateKeys(layer){
 const visible=visibleKeycodes(layer);
 const currentCombos=[];for(let i=0;i<(profile.combo?.length||0);i++){const info=comboInfo(i);if(info&&info.inputs.every(k=>visible.has(k)))currentCombos.push(info)}
 document.querySelectorAll('.half .key[data-coord]').forEach(key=>{
   const [r,c]=(key.dataset.coord||'').split(',').map(Number);const k=profile.layout?.[layer]?.[r]?.[c];if(k===undefined)return;
   key.classList.remove('semantic-tap-dance');key.querySelector('.combo-badge')?.remove();
   const face=key.querySelector('.key-face');const primary=key.querySelector('.primary');const secondary=key.querySelector('.secondary');
   const tdMatch=/^TD\((\d+)\)$/.exec(String(k));
   if(tdMatch){
     const info=tapDanceInfo(Number(tdMatch[1]));if(info){
       key.classList.add('semantic-tap-dance');
       if(!$('rawMode')?.checked){primary.textContent='tap '+shortText(info.actions[0],14);secondary.textContent='hold '+shortText(info.actions[1],11)+' · 2× '+shortText(info.actions[2],11)}
       const full='TD('+info.index+'): '+TAP_FIELDS.map((name,i)=>name+' '+info.actions[i]).join('; ')+'; tapping term '+info.term+' ms';key.title=full;key.setAttribute('aria-label',full);
       if(!key.dataset.semanticBound){key.dataset.semanticBound='1';key.addEventListener('click',()=>setTimeout(()=>showTapDanceDetail(info),0))}
     }
   }
   const hits=currentCombos.filter(combo=>combo.inputs.includes(k));
   if(hits.length&&face){const badge=document.createElement('span');badge.className='combo-badge';badge.textContent=String(hits.length);badge.title=hits.map(combo=>combo.inputs.map(actionText).join(' + ')+' → '+combo.outputText).join('\n');face.append(badge)}
 });
 applyRgb(layer);
}
function showTapDanceDetail(info){
 const main=$('detailMain'),hint=$('detailHint'),raw=$('detailRaw');if(!main||!hint||!raw)return;
 main.textContent='Tap dance '+info.index+': tap → '+info.actions[0];
 hint.textContent='Hold → '+info.actions[1]+' · Double tap → '+info.actions[2]+' · Tap + hold → '+info.actions[3]+' · '+info.term+' ms tapping term';
 raw.textContent='TD('+info.index+') = ['+info.raw.slice(0,4).join(', ')+']';
}
function renderRgbInfo(layer){
 const host=$('rgbProfileInfo');if(!host)return;
 if(!('hid' in navigator)){host.innerHTML='<div class="behavior-empty">WebHID is unavailable in this browser. Saved layer/modifier/combo RGB profiles live in keyboard EEPROM and are not included in the .vil export.</div>';return}
 if(!rgbState){host.innerHTML='<div class="behavior-empty">Saved RGB profiles live in keyboard EEPROM, not in the .vil export. Use <strong>Connect keyboard RGB</strong> above to read the exact active layer, modifier, and combo profiles without changing them.</div>';return}
 const layerProfile=rgbState.layers[layer];const resolved=assigned(layerProfile)?layerProfile:rgbState.global;const source=assigned(layerProfile)?'Layer '+layer:'Global fallback';
 let html='<div class="rgb-profile-line">'+swatchHtml(resolved)+'<div class="rgb-profile-copy"><strong>'+source+'</strong><span>'+escapeHtml(profileText(resolved))+'</span></div></div>';
 const mods=rgbState.mods.map((p,i)=>assigned(p)?'<div class="rgb-profile-line">'+swatchHtml(p)+'<div class="rgb-profile-copy"><strong>'+MOD_PROFILE_NAMES[i]+'</strong><span>'+escapeHtml(profileText(p))+'</span></div></div>':'').join('');
 html+=mods||'<div class="rgb-note">No modifier-specific RGB profiles are assigned.</div>';
 if(rgbState.comboDuration)html+='<div class="rgb-note">Combo RGB override duration: '+rgbState.comboDuration+' ms. Colored combo rows indicate assigned combo profiles.</div>';
 html+='<div class="rgb-note">The atlas renders the stored HSV/brightness as a key glow. It reports the exact QMK effect mode and speed but does not simulate every matrix animation pattern.</div>';
 host.innerHTML=html;
}
function assigned(p){return p&&p.mode!==UNASSIGNED}
function profileText(p){if(!p)return 'unavailable';if(p.mode===UNASSIGNED)return 'unassigned';if(p.mode===0)return 'RGB off';return 'effect #'+p.mode+' · H '+p.h+' · S '+p.s+' · V '+p.v+' · speed '+p.speed}
function hsvRgb(h,s,v){h=(h/255)*360;s/=255;v/=255;const c=v*s,x=c*(1-Math.abs(((h/60)%2)-1)),m=v-c;let r=0,g=0,b=0;if(h<60){r=c;g=x}else if(h<120){r=x;g=c}else if(h<180){g=c;b=x}else if(h<240){g=x;b=c}else if(h<300){r=x;b=c}else{r=c;b=x}return [r,g,b].map(n=>Math.round((n+m)*255))}
function swatchHtml(p){if(!assigned(p)||p.mode===0)return '<span class="rgb-swatch" style="--swatch-rgb:70 76 84"></span>';const rgb=hsvRgb(p.h,p.s,255);return '<span class="rgb-swatch" style="--swatch-rgb:'+rgb.join(' ')+'"></span>'}
function applyRgb(layer){
 document.querySelectorAll('.half .key').forEach(k=>{k.classList.remove('rgb-lit');k.style.removeProperty('--rgb');k.style.removeProperty('--rgb-strength')});
 if(!rgbState)return;
 const p=assigned(rgbState.layers[layer])?rgbState.layers[layer]:rgbState.global;if(!assigned(p)||p.mode===0)return;
 const rgb=hsvRgb(p.h,p.s,255);const max=Math.max(1,rgbState.maxBrightness||255);const strength=Math.max(.08,Math.min(1,p.v/max));
 document.querySelectorAll('.half .key:not(.disabled)').forEach(k=>{k.classList.add('rgb-lit');k.style.setProperty('--rgb',rgb.join(' '));k.style.setProperty('--rgb-strength',String(strength))});
}
function escapeHtml(value){return String(value).replace(/[&<>"']/g,ch=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[ch]))}
function scheduleRefresh(){if(refreshQueued)return;refreshQueued=true;queueMicrotask(()=>{refreshQueued=false;refresh()})}
function refresh(){if(!profile)return;ensurePanel();ensureRgbButton();const layer=currentLayer();decorateKeys(layer);renderTapDances(layer);renderCombos(layer);renderRgbInfo(layer)}

function rawCollection(device){return device.collections?.find(c=>c.usagePage===RAW_USAGE_PAGE&&c.usage===RAW_USAGE)||device.collections?.[0]}
function configureReport(device){const collection=rawCollection(device);const report=collection?.outputReports?.[0];hidReportId=report?.reportId??0;const bits=report?.items?.reduce((sum,item)=>sum+(item.reportSize||0)*(item.reportCount||0),0)||0;hidPacketLength=Math.max(32,Math.ceil(bits/8)||32)}
async function sendRaw(bytes){
 if(!hidDevice?.opened)throw new Error('keyboard is not open');
 const packet=new Uint8Array(hidPacketLength);packet.set(bytes.slice(0,hidPacketLength));
 return await new Promise(async(resolve,reject)=>{
   let timer;
   const cleanup=()=>{clearTimeout(timer);hidDevice.removeEventListener('inputreport',onReport)};
   const onReport=e=>{if(e.device!==hidDevice||e.reportId!==hidReportId)return;cleanup();resolve(new Uint8Array(e.data.buffer,e.data.byteOffset,e.data.byteLength))};
   hidDevice.addEventListener('inputreport',onReport);
   timer=setTimeout(()=>{cleanup();reject(new Error('keyboard response timed out'))},1800);
   try{await hidDevice.sendReport(hidReportId,packet)}catch(err){cleanup();reject(err)}
 });
}
async function rgbCommand(bytes){const response=await sendRaw(bytes);if(response[0]!==RGB_COMMAND)throw new Error('RGB profile protocol unavailable');return response}
async function getRgbProfile(scope,index){const d=await rgbCommand([RGB_COMMAND,OP.GET_PROFILE,scope,index]);return {mode:d[4],h:d[5],s:d[6],v:d[7],speed:d[8]}}
async function readRgbState(){
 const caps=await rgbCommand([RGB_COMMAND,OP.CAPABILITIES]);
 const layerCount=caps[4],modCount=caps[5],comboCount=caps[6];
 const result={version:caps[2],maxBrightness:caps[7],maxEffect:caps[8],global:await getRgbProfile(SCOPE.GLOBAL,0),layers:[],mods:[],combos:Array(comboCount).fill(null),comboDuration:0};
 for(let i=0;i<layerCount;i++)result.layers.push(await getRgbProfile(SCOPE.LAYER,i));
 for(let i=0;i<modCount;i++)result.mods.push(await getRgbProfile(SCOPE.MODIFIER,i));
 const activeComboIndices=[];for(let i=0;i<(profile.combo?.length||0)&&i<comboCount;i++)if(comboInfo(i))activeComboIndices.push(i);
 for(const i of activeComboIndices)result.combos[i]=await getRgbProfile(SCOPE.COMBO,i);
 const duration=await rgbCommand([RGB_COMMAND,OP.GET_COMBO_DURATION,0,0]);result.comboDuration=(duration[2]<<8)|duration[3];
 return result;
}
async function connectRgb(){
 const button=$('connectRgb');if(!('hid' in navigator)){button.dataset.state='error';button.textContent='WebHID unavailable';return}
 try{
   button.disabled=true;button.textContent='Connecting…';
   const devices=await navigator.hid.requestDevice({filters:[{usagePage:RAW_USAGE_PAGE,usage:RAW_USAGE}]});if(!devices.length)throw new Error('No keyboard selected');
   hidDevice=devices[0];if(!hidDevice.opened)await hidDevice.open();configureReport(hidDevice);
   rgbState=await readRgbState();button.dataset.state='connected';button.textContent='RGB profiles connected';scheduleRefresh();
 }catch(err){button.dataset.state='error';button.textContent='RGB connect failed';const host=$('rgbProfileInfo');if(host)host.innerHTML='<div class="behavior-empty">Could not read the keyboard RGB profiles: '+escapeHtml(err.message)+'. The atlas did not modify the device.</div>'}
 finally{button.disabled=false}
}
async function autoReconnect(){if(!('hid' in navigator))return;try{const devices=await navigator.hid.getDevices();const device=devices.find(d=>rawCollection(d)?.usagePage===RAW_USAGE_PAGE||d.collections?.some(c=>c.usagePage===RAW_USAGE_PAGE&&c.usage===RAW_USAGE));if(!device)return;hidDevice=device;if(!device.opened)await device.open();configureReport(device);rgbState=await readRgbState();const b=$('connectRgb');if(b){b.dataset.state='connected';b.textContent='RGB profiles connected'}scheduleRefresh()}catch(_){/* permission/device state is optional */}}

async function init(){
 ensurePanel();ensureRgbButton();
 try{const res=await fetch(PROFILE,{cache:'no-store'});if(!res.ok)throw new Error('HTTP '+res.status);profile=await res.json();scheduleRefresh()}catch(err){const panel=$('behaviorPanel');panel?.querySelector('.behavior-head p')?.append(' Behavior definitions unavailable: '+err.message)}
 const observer=new MutationObserver(scheduleRefresh);for(const id of ['left','right']){const root=$(id);if(root)observer.observe(root,{childList:true})}
 document.addEventListener('click',e=>{if(e.target.closest?.('.layer-button'))setTimeout(scheduleRefresh,0)});
 for(const id of ['hideAlpha','rawMode'])$(id)?.addEventListener('change',()=>setTimeout(scheduleRefresh,0));
 autoReconnect();
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init,{once:true});else init();
})();
