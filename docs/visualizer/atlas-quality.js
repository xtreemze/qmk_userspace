(()=>{'use strict';
const $=id=>document.getElementById(id);
function layer(){const m=/L(\d+)/.exec($('layerNumber')?.textContent||'');return m?Number(m[1]):1}
function groupLayers(){
 const host=$('layers');if(!host||host.dataset.grouped)return;
 const nodes=[...host.children];host.replaceChildren();let group=null;
 for(const node of nodes){
  if(node.classList.contains('family-label')){
   group=document.createElement('div');group.className='layer-family';group.setAttribute('role','group');group.setAttribute('aria-label',node.textContent+' layers');
   const label=document.createElement('span');label.className='layer-family-name';label.textContent=node.textContent;group.append(label);host.append(group);
  }else if(group)group.append(node);else host.append(node);
 }
 host.dataset.grouped='1';
 host.addEventListener('keydown',event=>{
  if(!['ArrowLeft','ArrowRight','Home','End'].includes(event.key))return;
  const buttons=[...host.querySelectorAll('.layer-button:not(:disabled)')],index=buttons.indexOf(document.activeElement);if(index<0||!buttons.length)return;
  event.preventDefault();let next=index;
  if(event.key==='ArrowLeft')next=(index-1+buttons.length)%buttons.length;
  if(event.key==='ArrowRight')next=(index+1)%buttons.length;
  if(event.key==='Home')next=0;if(event.key==='End')next=buttons.length-1;buttons[next].focus();
 });
}
function sync(){
 const status=$('status'),failed=status?.classList.contains('error'),ready=!failed&&status?.textContent.startsWith('Live source:');
 document.querySelectorAll('.layer-button').forEach(button=>{button.disabled=!ready;const active=Number(button.dataset.layer)===layer();active?button.setAttribute('aria-current','true'):button.removeAttribute('aria-current')});
 document.querySelectorAll('.layer-family').forEach(group=>group.classList.toggle('has-active',!!group.querySelector('[aria-current="true"]')));
 for(const id of ['hideAlpha','rawMode'])if($(id))$(id).disabled=!ready;
 $('atlasWorkspace')?.setAttribute('aria-busy',String(!ready&&!failed));
 const detail=document.querySelector('.detail'),panel=$('behaviorPanel');if(detail&&panel&&detail.nextElementSibling!==panel)detail.after(panel);
 collapseCombos();
}
function collapseCombos(){
 const host=$('comboList');if(!host||host.querySelector('.other-combos'))return;
 const items=[...host.children].filter(node=>node.classList.contains('behavior-item')),other=items.filter(node=>!node.classList.contains('is-current'));if(!other.length)return;
 if(items.length===other.length){const empty=document.createElement('div');empty.className='behavior-empty';empty.textContent='No combo chord is fully present on this layer.';host.prepend(empty)}
 const details=document.createElement('details');details.className='other-combos';const summary=document.createElement('summary');summary.textContent=other.length+' other configured combos';const list=document.createElement('div');list.className='behavior-list';other.forEach(item=>list.append(item));details.append(summary,list);host.append(details);
}
function select(target){const key=target?.closest?.('.half .key[data-coord],.encoder-map .key');if(!key)return;document.querySelectorAll('.key.is-selected').forEach(item=>item.classList.remove('is-selected'));key.classList.add('is-selected')}
function refresh(){groupLayers();sync()}
function init(){
 refresh();const observer=new MutationObserver(refresh);
 for(const id of ['left','right','comboList','behaviorPanel','status']){const node=$(id);if(node)observer.observe(node,{childList:true,subtree:true,characterData:true,attributes:id==='status'})}
 document.addEventListener('click',event=>{select(event.target);if(event.target.closest?.('.layer-button'))setTimeout(refresh,0)});
 setTimeout(refresh,0);
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init,{once:true});else init();
})();