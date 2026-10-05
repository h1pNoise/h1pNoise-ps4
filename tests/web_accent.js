// Run the shipped page script: colour changes stay local and must not disrupt pairing.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync(process.argv[2],'utf8'),script=fs.readFileSync(path.join(path.dirname(process.argv[2]),'app.js'),'utf8');
const ids=[...html.matchAll(/\bid="([^"]+)"/g)].map(m=>m[1]);
function fixture(saved=null,blocked=false){
 const memory=new Map(saved===null?[]:[['h1pNoise.accent.v1',saved]]),css=new Map();let requests=0,focus='';
 const nodes=Object.fromEntries(ids.map(id=>[id,{id,value:'',textContent:'',open:false,hidden:false,setAttribute(k,v){this[k]=v;},addEventListener(){},focus(){focus=id;}}]));
 const context=vm.createContext({document:{getElementById:id=>{assert.ok(nodes[id],id);return nodes[id];},documentElement:{style:{setProperty(k,v){assert.match(v,/^#[0-9a-f]{6}$/);css.set(k,v);}}}},
  localStorage:{getItem(k){if(blocked)throw Error('Unavailable');return memory.get(k)||null;},setItem(k,v){if(blocked)throw Error('Unavailable');memory.set(k,v);}},
  matchMedia:()=>({matches:false}),location:{hash:''},URLSearchParams,URL,AbortController,TextEncoder,setInterval(){},setTimeout(){},clearTimeout(){},fetch(){requests++;throw Error('Unexpected network request');}});
 vm.runInContext(script,context);
 return {nodes,css,memory,requests:()=>requests,focus:()=>focus};
}
function luminance(hex){const channels=hex.slice(1).match(/../g).map(c=>parseInt(c,16)/255).map(c=>c<=.04045?c/12.92:((c+.055)/1.055)**2.4);return channels[0]*.2126+channels[1]*.7152+channels[2]*.0722;}
function contrast(a,b){const x=luminance(a),y=luminance(b);return (Math.max(x,y)+.05)/(Math.min(x,y)+.05);}
const f=fixture();assert.equal(f.css.get('--mint'),'#9debcf');
for(const [name,color] of Object.entries({mint:'#9debcf',blue:'#8cc8ff',purple:'#c5b0ff',pink:'#ffadd2',orange:'#ffc08a',white:'#edf2f4'})){
 f.nodes['accent-'+name].onclick();assert.equal(f.css.get('--mint'),color);assert.equal(f.memory.get('h1pNoise.accent.v1'),color);assert.equal(f.nodes['accent-'+name]['aria-pressed'],'true');
 assert.equal(fixture(f.memory.get('h1pNoise.accent.v1')).css.get('--mint'),color,'colour must survive reload');
}
for(let n=0;n<256;n++){
 const color='#'+n.toString(16).padStart(2,'0').repeat(3);f.nodes['accent-custom'].value=color;f.nodes['accent-custom'].oninput();
 assert.ok(contrast(color,f.css.get('--mint-ink'))>=4.5,'button text contrast '+color);
 assert.ok(contrast(f.css.get('--accent-hover'),f.css.get('--accent-hover-ink'))>=4.5,'hover text contrast '+color);
 assert.ok(contrast(f.css.get('--accent-text'),'#14191e')>=4.5,'detail text contrast '+color);
}
f.nodes['accent-custom'].value='#A10fB8';f.nodes['accent-custom'].oninput();assert.equal(f.css.get('--mint'),'#a10fb8');
for(const invalid of ['red','#fff','#000000;display:none','url(https://example.invalid)','',{},null])assert.equal(fixture(invalid).css.get('--mint'),'#9debcf','invalid stored colours fall back safely');
const denied=fixture('#123456',true);denied.nodes['accent-blue'].onclick();assert.equal(denied.css.get('--mint'),'#8cc8ff');assert.match(denied.nodes['accent-status'].textContent,/não permite guardar/);
f.nodes['accent-reset'].onclick();assert.equal(f.css.get('--mint'),'#9debcf');assert.equal(f.memory.get('h1pNoise.accent.v1'),'#9debcf');
f.nodes['accent-picker'].open=true;f.nodes['accent-close'].onclick();assert.equal(f.nodes['accent-picker'].open,false);assert.equal(f.focus(),'accent-toggle');
f.nodes['accent-picker'].open=true;let prevented=false;f.nodes['accent-picker'].onkeydown({key:'Escape',preventDefault(){prevented=true;}});assert.equal(f.nodes['accent-picker'].open,false);assert.ok(prevented);
assert.equal(f.requests(),0);assert.equal(denied.requests(),0);console.log('Accent UI checks passed: presets/custom, reload, invalid/blocked storage, contrast, keyboard and no network requests.');
