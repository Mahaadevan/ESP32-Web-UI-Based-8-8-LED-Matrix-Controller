const char UI_HTML[] PROGMEM = R"rawliteral(<!doctype html><html><head><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1,user-scalable=no"><title>Matrix</title>
<style>
:root{--a:#ff3b6b;--b:#7c5cff;--bg:#0b0b14}
*{box-sizing:border-box}
body{margin:0;min-height:100vh;color:#e8e8f5;font:15px system-ui,sans-serif;background:radial-gradient(circle at 20% 0%,#2a1a55 0,transparent 50%),radial-gradient(circle at 90% 100%,#4a1030 0,transparent 50%),var(--bg);padding:14px;padding-bottom:90px}
.w{max-width:480px;margin:auto}
.card{background:#ffffff0d;border:1px solid #ffffff18;border-radius:18px;padding:14px;margin-bottom:12px;backdrop-filter:blur(8px)}
h1{font-size:18px;margin:0;letter-spacing:2px;background:linear-gradient(90deg,var(--a),var(--b));-webkit-background-clip:text;color:transparent}
.top{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px}
#s{font-size:12px;padding:4px 10px;border-radius:20px;background:#ffffff14}
#s.ok{background:#12633a;color:#8fffc0}
h4{margin:0 0 8px;font-size:11px;letter-spacing:2px;color:#8a8aa8;font-weight:600}
button,input[type=password]{font:inherit;color:#fff;background:#ffffff14;border:1px solid #ffffff22;border-radius:12px;padding:9px 13px;margin:2px}
button:active{transform:scale(.96)}
button.p{background:linear-gradient(135deg,var(--a),var(--b));border:0;font-weight:600}
input[type=password]{width:55%}
#g{display:grid;grid-template-columns:repeat(8,1fr);gap:6px;touch-action:none;padding:10px;background:#000;border-radius:14px}
#g div{aspect-ratio:1;border-radius:50%;background:#1b1b26;transition:.08s}
#g div.on{background:var(--a);box-shadow:0 0 10px var(--a),0 0 22px #ff3b6b88}
#st{display:flex;flex-wrap:wrap;gap:4px}
#st button{width:38px;padding:7px 0}
#st button.sel{border-color:var(--a);color:var(--a)}
.r{display:flex;align-items:center;gap:8px;margin:6px 0}.r span{width:48px;color:#8a8aa8;font-size:12px}
input[type=range]{flex:1;accent-color:var(--a)}
label{font-size:13px;margin-right:10px;color:#b8b8d0}
#bar{position:fixed;left:0;right:0;bottom:0;padding:10px;background:#0b0b14ee;border-top:1px solid #ffffff18;display:flex;justify-content:center;gap:6px}
#bar button{flex:1;max-width:150px}
</style></head><body><div class=w>
<div class=top><h1>LED MATRIX</h1><span id=s>offline</span></div>
<div class=card id=lg><div style="display:flex"><input id=pw type=password placeholder=password><button class=p onclick=conn()>Connect</button></div></div>
<div class=card><div id=g></div>
<div style="text-align:center;margin-top:8px"><button onclick=clr()>Clear</button><button onclick=inv()>Invert</button><button onclick=shift(1)>↑</button><button onclick=shift(2)>↓</button><button onclick=shift(3)>←</button><button onclick=shift(4)>→</button></div>
<div style="text-align:center;margin-top:6px"><label><input type=checkbox id=live checked> live</label><label><input type=checkbox id=mir> mirror</label><label><input type=checkbox id=flp> flip</label></div></div>
<div class=card><h4>FRAMES</h4><div id=st></div>
<div style="margin-top:6px"><button onclick=addF()>+ New</button><button onclick=dup()>Duplicate</button><button onclick=del()>Delete</button></div>
<div class=r><span>Speed</span><input id=fps type=range min=1 max=30 value=5></div>
<div class=r><span>Bright</span><input id=br type=range min=0 max=15 value=8></div></div>
<div class=card><h4>PATTERNS</h4><div id=pt></div></div>
</div>
<div id=bar><button class=p onclick=sendA()>▶ Play on LED</button><button onclick=prev()>👁 Preview</button><button onclick=send('S')>■ Stop</button></div>
<script>
const $=i=>document.getElementById(i);
let fr=[Array(8).fill(0)],cur=0,ws,paint=1,pv=null,want=0;
const g=$('g');
for(let i=0;i<64;i++){const d=document.createElement('div');
 d.onpointerdown=e=>{e.preventDefault();d.releasePointerCapture(e.pointerId);const x=i&7,y=i>>3;paint=fr[cur][y]&(128>>x)?0:1;tog(i)};
 d.onpointerenter=e=>{if(e.buttons)tog(i)};g.appendChild(d)}
function live(){if($('live').checked)send('F:'+enc(fr[cur]))}
function tog(i){const x=i&7,y=i>>3;if(paint)fr[cur][y]|=128>>x;else fr[cur][y]&=~(128>>x);draw();live()}
function draw(f){f=f||fr[cur];[...g.children].forEach((d,i)=>d.className=f[i>>3]&(128>>(i&7))?'on':'');
 $('st').innerHTML='';fr.forEach((_,i)=>{const b=document.createElement('button');b.textContent=i+1;b.className=i==cur?'sel':'';b.onclick=()=>{cur=i;draw();live()};$('st').appendChild(b)})}
function enc(f){let a=[...f];if($('flp').checked)a.reverse();
 return a.map(b=>{if($('mir').checked){let r=0;for(let i=0;i<8;i++)if(b>>i&1)r|=128>>i;b=r}return b.toString(16).padStart(2,'0')}).join('')}
function addF(){fr.splice(cur+1,0,Array(8).fill(0));cur++;draw();live()}
function dup(){fr.splice(cur+1,0,[...fr[cur]]);cur++;draw()}
function del(){if(fr.length>1){fr.splice(cur,1);cur=Math.min(cur,fr.length-1)}else fr[0].fill(0);draw();live()}
function clr(){fr[cur].fill(0);draw();live()}
function inv(){fr[cur]=fr[cur].map(b=>~b&255);draw();live()}
function shift(d){let f=fr[cur];if(d==1)f.push(f.shift());if(d==2)f.unshift(f.pop());
 if(d==3)fr[cur]=f.map(b=>(b<<1|b>>7)&255);if(d==4)fr[cur]=f.map(b=>(b>>1|b<<7)&255);draw();live()}
function conn(){want=1;try{localStorage.pw=$('pw').value}catch(e){}
 ws=new WebSocket('ws://'+location.hostname+':81');
 ws.onopen=()=>ws.send('AUTH:'+$('pw').value);
 ws.onmessage=e=>{const ok=e.data=='OK';$('s').textContent=ok?'connected':'wrong password';$('s').className=ok?'ok':'';if(ok)$('lg').style.display='none';if(!ok)want=0};
 ws.onclose=()=>{$('s').textContent='offline';$('s').className='';$('lg').style.display='';if(want)setTimeout(conn,2000)}}
function send(m){if(ws&&ws.readyState==1)ws.send(m)}
function sendA(){send('A:'+$('fps').value+':'+fr.map(enc).join(''))}
function prev(){if(pv){clearInterval(pv);pv=null;draw();return}let i=0;pv=setInterval(()=>{draw(fr[i%fr.length]);i++},1000/$('fps').value)}
$('br').oninput=()=>send('B:'+$('br').value);
const E=()=>Array(8).fill(0),P=(f,x,y)=>{if(x>=0&&x<8&&y>=0&&y<8)f[y]|=128>>x};
const pats={
 Heart:()=>[[0x66,0xFF,0xFF,0xFF,0x7E,0x3C,0x18,0x00]],
 Smiley:()=>[[0x3C,0x42,0xA5,0x81,0xA5,0x99,0x42,0x3C]],
 Checker:()=>[Array(8).fill(0).map((_,y)=>y&1?0xAA:0x55),Array(8).fill(0).map((_,y)=>y&1?0x55:0xAA)],
 Scan:()=>[...Array(8)].map((_,y)=>{const f=E();f[y]=255;return f}),
 Rain:()=>{let d=[...Array(8)].map(()=>-Math.floor(Math.random()*8)),o=[];
  for(let t=0;t<16;t++){const f=E();d.forEach((y,x)=>{P(f,x,y);P(f,x,y-1)});d=d.map(y=>y>=9?-Math.floor(Math.random()*5):y+1);o.push(f)}return o},
 Ripple:()=>[0,1,2,3].map(r=>{const f=E();for(let i=3-r;i<=4+r;i++){P(f,i,3-r);P(f,i,4+r);P(f,3-r,i);P(f,4+r,i)}return f}),
 Spiral:()=>{const o=[],f=E();let x=0,y=0,dx=1,dy=0,s=new Set();
  for(let i=0;i<64;i++){P(f,x,y);s.add(x+','+y);o.push([...f]);
   let nx=x+dx,ny=y+dy;if(nx<0||nx>7||ny<0||ny>7||s.has(nx+','+ny)){[dx,dy]=[-dy,dx];nx=x+dx;ny=y+dy}x=nx;y=ny}return o}
};
Object.keys(pats).forEach(k=>{const b=document.createElement('button');b.textContent=k;
 b.onclick=()=>{fr=pats[k]();cur=0;draw();fr.length>1?sendA():live()};$('pt').appendChild(b)});
try{if(localStorage.pw){$('pw').value=localStorage.pw;conn()}}catch(e){}
draw();
</script></body></html>)rawliteral";
