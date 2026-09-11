(()=>{'use strict';
const PROFILE='https://raw.githubusercontent.com/xtreemze/qmk_userspace/halcyon/keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/xtreemzeVial.vil';
const TFT_GIF='data:image/gif;base64,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';
const META=[
 ['MOUSE','Pointer & control','Pointer'],['QWERTY','Primary typing','Typing'],['COLEMAK','Alternate typing','Typing'],['NUMSYMS','Numbers, keypad & repeat','Numeric'],['NUMFLIP','Mirrored numbers','Numeric'],['ONESHOT','One-shot modifiers','Editing'],['EDITING','Editing & shortcuts','Editing'],['FNSYMS','Functions, symbols & navigation','Programming'],['FNFLIP','Mirrored functions','Programming'],['SYMBOLS','Shifted number symbols','Programming'],['RGBHUE','RGB hue','Hardware'],['RGBVAL','RGB value','Hardware'],['BKLIGHT','TFT backlight','Hardware']];
const FAMILIES=['Typing','Pointer','Numeric','Editing','Programming','Hardware'];
const TFT_COLORS=['#7db7ff','#9dc4ff','#b5a4ff','#6de1c4','#ffcb7d','#ff9e84','#72d7ff','#be9fff','#79df9b','#ffd369','#ff82a8','#8ec5ff','#c9a6ff'];
const CUSTOM={USER0:'Macro 0',USER1:'Macro 1',USER2:'Macro 2',USER3:'Macro 3',USER4:'Macro 4',USER5:'Macro 5',USER6:'Macro 6',USER7:'Macro 7',USER8:'Macro 8',USER9:'Macro 9',USER10:'Save layer RGB',USER11:'Save mod RGB',USER12:'Save chord RGB',USER13:'Trigger chord RGB',USER14:'Redo',USER15:'Paste',USER16:'Copy',USER17:'Cut',USER18:'Undo',USER19:'Select all',USER20:'OS trace'};
const SIMPLE={KC_BSPACE:'Backspace',KC_SPACE:'Space',KC_ESCAPE:'Esc',KC_TAB:'Tab',KC_ENTER:'Enter',KC_MUTE:'Mute',KC_END:'End',KC_PGDOWN:'Page ↓',KC_MINUS:'−',KC_GRAVE:'`',KC_BSLASH:'\\',KC_LBRACKET:'[',KC_RBRACKET:']',KC_KP_PLUS:'Num +',KC_KP_MINUS:'Num −',KC_KP_ASTERISK:'Num ×',KC_KP_SLASH:'Num ÷',KC_KP_COMMA:'Num ,',KC_KP_DOT:'Num .',KC_BTN1:'Mouse 1',KC_BTN2:'Mouse 2',KC_BTN3:'Mouse 3',KC_BTN4:'Mouse 4',KC_BTN5:'Mouse 5',KC_MS_L:'Mouse ←',KC_MS_R:'Mouse →',KC_MS_U:'Mouse ↑',KC_MS_D:'Mouse ↓',KC_WH_U:'Scroll ↑',KC_WH_D:'Scroll ↓',KC_VOLD:'Volume −',KC_VOLU:'Volume +',QK_REPEAT_KEY:'Repeat',QK_ALT_REPEAT_KEY:'Alt repeat',QK_LAYER_LOCK:'Layer lock',QK_CAPS_WORD_TOGGLE:'Caps Word',KC_ASTG:'Auto Shift',BL_TOGG:'Backlight toggle',BL_BRTG:'Backlight breathe',BL_STEP:'Backlight step',BL_INC:'Backlight +',BL_DEC:'Backlight −',RGB_TOG:'RGB toggle',RGB_SAD:'Saturation −',RGB_SAI:'Saturation +',RGB_VAD:'Value −',RGB_VAI:'Value +',RGB_SPD:'Speed −',RGB_SPI:'Speed +',RGB_RMOD:'Mode ←',RGB_MOD:'Mode →',RGB_HUD:'Hue −',RGB_HUI:'Hue +',KC_BRID:'Display −',KC_BRIU:'Display +'};
const PHYSICAL=[
 {slot:'c4-r0',row:0,col:4},{slot:'c4-r1',row:1,col:4},{slot:'c4-r2',row:2,col:4},
 {slot:'c3-r0',row:0,col:3},{slot:'c3-r1',row:1,col:3},{slot:'c3-r2',row:2,col:3},
 {slot:'c2-r0',row:0,col:2},{slot:'c2-r1',row:1,col:2},{slot:'c2-r2',row:2,col:2},
 {slot:'c1-r0',row:0,col:1},{slot:'c1-r1',row:1,col:1},{slot:'c1-r2',row:2,col:1},
 {slot:'c0-r0',row:0,col:0},{slot:'c0-r1',row:1,col:0},{slot:'c0-r2',row:2,col:0},
 {slot:'thumb-outer',row:3,col:1},{slot:'thumb-inner',row:3,col:0}
];
let profile=null,current=1,selected=null;
const $=id=>document.getElementById(id);
function innerLabel(k){const v=describe(k);return v.primary}
function modifierName(s){return s.replace(/MOD_/g,'').replace(/LGUI|RGUI/g,'⌘').replace(/LCTL|RCTL/g,'Ctrl').replace(/LSFT|RSFT/g,'Shift').replace(/LALT|RALT/g,'Alt').replace(/\|/g,' + ')}
function describe(k){
 if(k===-1||k==='KC_NO')return{primary:'',secondary:'',disabled:true};
 if(k==='KC_TRNS')return{primary:'',secondary:'inherited',transparent:true};
 if(CUSTOM[k])return{primary:CUSTOM[k],secondary:'custom'};
 if(SIMPLE[k])return{primary:SIMPLE[k],secondary:''};
 let m=/^KC_([A-Z])$/.exec(k);if(m)return{primary:m[1],secondary:'',alpha:true};
 m=/^KC_([0-9])$/.exec(k);if(m)return{primary:m[1],secondary:''};
 m=/^KC_F(\d+)$/.exec(k);if(m)return{primary:'F'+m[1],secondary:''};
 m=/^KC_KP_([0-9])$/.exec(k);if(m)return{primary:'Num '+m[1],secondary:''};
 m=/^MO\((\d+)\)$/.exec(k);if(m)return{primary:'Layer '+m[1],secondary:'hold'};
 m=/^TO\((\d+)\)$/.exec(k);if(m)return{primary:'→ Layer '+m[1],secondary:'switch'};
 m=/^TG\((\d+)\)$/.exec(k);if(m)return{primary:'Layer '+m[1],secondary:'toggle'};
 m=/^LT(\d+)\((.+)\)$/.exec(k);if(m){const tap=describe(m[2]);return{primary:tap.primary||m[2],secondary:'hold L'+m[1]}}
 m=/^OSM\((.+)\)$/.exec(k);if(m)return{primary:modifierName(m[1]),secondary:'one-shot'};
 m=/^(LCTL_T|LGUI_T|LSFT_T|LALT_T|RCTL_T|RGUI_T|RSFT_T|RALT_T)\((.+)\)$/.exec(k);if(m)return{primary:innerLabel(m[2]),secondary:'hold '+modifierName('MOD_'+m[1].slice(0,4).replace('_T',''))};
 m=/^(LSFT|RSFT|LCTL|RCTL|LALT|RALT|LGUI|RGUI)\((.+)\)$/.exec(k);if(m)return{primary:modifierName('MOD_'+m[1])+' + '+innerLabel(m[2]),secondary:''};
 m=/^TD\((\d+)\)$/.exec(k);if(m)return{primary:'Tap dance '+m[1],secondary:'multi-tap'};
 m=/^M(\d+)$/.exec(k);if(m)return{primary:'Macro '+m[1],secondary:''};
 return{primary:String(k).replace(/^KC_/,'').replaceAll('_',' '),secondary:''};
}
function makeKey(k,coord,slot){
 const d=describe(k),b=document.createElement('button');
 b.type='button';
 b.className='key slot-'+slot+(d.transparent?' transparent':'')+(d.disabled?' disabled':'')+(d.alpha&&$('hideAlpha').checked?' alpha-hidden':'');
 b.disabled=!!d.disabled;b.dataset.coord=coord;
 const face=document.createElement('span');face.className='key-face';
 const primary=document.createElement('span');primary.className='primary';primary.textContent=$('rawMode').checked&&!d.transparent?String(k):d.primary;
 const secondary=document.createElement('span');secondary.className='secondary';secondary.textContent=d.secondary;
 face.append(primary,secondary);b.append(face);
 b.addEventListener('click',()=>selectControl(k,coord,d));
 return b;
}
function moduleMarkup(side,moduleRow){
 const shell=document.createElement('div');shell.className='module-shell';
 const inner=document.createElement('div');inner.className='module-inner module-screen';
 if(side==='left'){
   const img=document.createElement('img');img.src=TFT_GIF;img.alt='Animated TFT display activity emulator';
   const overlay=document.createElement('div');overlay.className='tft-overlay';
   const layer=document.createElement('div');layer.className='tft-layer';layer.textContent='L'+current;
   const name=document.createElement('div');name.className='tft-name';name.textContent=META[current][0];
   overlay.append(layer,name);inner.append(img,overlay);
 }else{
   inner.classList.add('encoder-module');
   const knob=document.createElement('div');knob.className='encoder-knob';inner.append(knob);
 }
 shell.append(inner);
 const caption=document.createElement('div');caption.className='module-caption';
 caption.textContent=side==='left'?'TFT display':'Encoder';
 const action=moduleValue(moduleRow);caption.title=action;
 return [shell,caption];
}
function renderHalf(target,rowOffset,moduleRow,side){
 const root=$(target);root.replaceChildren();
 const board=document.createElement('div');board.className='half-board';root.append(board);
 for(const p of PHYSICAL){
   const r=rowOffset+p.row;
   const k=profile.layout[current][r][p.col];
   root.append(makeKey(k,r+','+p.col,p.slot));
 }
 const [module,caption]=moduleMarkup(side,moduleRow);root.append(module,caption);
 root.style.setProperty('--tft-accent',TFT_COLORS[current]);
}
function encoder(id,pair,index){
 const host=$(id);host.replaceChildren();const [ccw,cw]=pair;
 const mk=(k,dir)=>{
   const b=document.createElement('button');b.type='button';b.className='key';
   const d=describe(k),face=document.createElement('span');face.className='key-face';
   const p=document.createElement('span');p.className='primary';p.textContent=$('rawMode').checked?String(k):d.primary;
   const s=document.createElement('span');s.className='secondary';s.textContent=dir;
   face.append(p,s);b.append(face);b.addEventListener('click',()=>selectControl(k,'encoder '+index+' '+dir,d));return b
 };
 host.append(mk(ccw,'CCW'));const dial=document.createElement('div');dial.className='dial';host.append(dial);host.append(mk(cw,'CW'));
}
function moduleValue(row){
 const vals=profile.layout[current][row].filter(k=>k!==-1&&k!=='KC_NO'&&k!=='KC_TRNS');
 return vals.length?vals.map(k=>$('rawMode').checked?k:describe(k).primary).join(' · '):'inherits / no action';
}
function render(){
 const [name,role]=META[current];
 $('layerTitle').textContent=name;$('layerRole').textContent=role;$('layerNumber').textContent='L'+current;
 document.querySelectorAll('.layer-button').forEach(b=>b.setAttribute('aria-pressed',String(Number(b.dataset.layer)===current)));
 renderHalf('left',0,4,'left');renderHalf('right',5,9,'right');
 $('leftModule').textContent=moduleValue(4);$('rightModule').textContent=moduleValue(9);
 encoder('enc0',profile.encoder_layout[current][0],0);encoder('enc1',profile.encoder_layout[current][1],1);
}
function selectControl(k,where,d){
 selected={k,where,d};$('detailMain').textContent=d.primary||'No direct action';
 $('detailHint').textContent=where+(d.secondary?' · '+d.secondary:'');$('detailRaw').textContent=String(k);
}
function buildLayers(){
 const host=$('layers');
 for(const family of FAMILIES){
   const label=document.createElement('div');label.className='family-label';label.textContent=family;host.append(label);
   META.forEach((m,i)=>{
     if(m[2]!==family)return;
     const b=document.createElement('button');b.type='button';b.className='layer-button';b.dataset.layer=i;b.textContent='L'+i+' '+m[0];
     b.setAttribute('aria-pressed',String(i===current));b.addEventListener('click',()=>{current=i;render()});host.append(b);
   });
 }
}
async function init(){
 buildLayers();$('hideAlpha').addEventListener('change',render);$('rawMode').addEventListener('change',render);
 try{
   const res=await fetch(PROFILE,{cache:'no-store'});if(!res.ok)throw new Error('HTTP '+res.status);profile=await res.json();
   if(!Array.isArray(profile.layout)||profile.layout.length!==13||!Array.isArray(profile.encoder_layout)||profile.encoder_layout.length!==13)throw new Error('Unexpected Vial profile shape');
   $('status').textContent='Live source: canonical xtreemzeVial.vil on halcyon · '+profile.layout.length+' layers · physical geometry derived from vial.json';
   render();
 }catch(err){
   $('layerTitle').textContent='Profile unavailable';$('status').classList.add('error');$('status').textContent='Could not load canonical Vial profile: '+err.message;
 }
}
init();
})();
