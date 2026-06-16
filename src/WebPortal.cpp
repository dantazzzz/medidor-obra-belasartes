// ============================================================================
//  WebPortal.cpp  -  Access Point WiFi + servidor HTTP.
//  Conecte o celular na rede "Medidor-Obra" (senha "belasartes") e abra
//  http://192.168.4.1 no navegador: ve a leitura ao vivo, a tabela das
//  medicoes salvas, baixa CSV, limpa, e acerta o relogio com a hora do celular.
// ============================================================================
#include <WiFi.h>
#include <WebServer.h>
#include "WebPortal.h"
#include "DataLog.h"
#include "LevelApp.h"
#include "RTC_PCF85063.h"
#include "Sun.h"

static WebServer server(80);

// ---- pagina (HTML+JS, servida ao celular) ---------------------------------
static const char PAGE[] PROGMEM = R"HTML(<!doctype html><html lang="pt-br"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Medidor de Obra</title><style>
*{box-sizing:border-box}body{margin:0;background:#0b0f17;color:#e5e7eb;
font-family:system-ui,Segoe UI,Roboto,Arial}
header{padding:16px;text-align:center;background:#111827;border-bottom:1px solid #1f2937}
h1{margin:0;font-size:18px}.sub{color:#94a3b8;font-size:12px;margin-top:4px}
.wrap{padding:14px;max-width:640px;margin:0 auto}
.card{background:#111827;border:1px solid #1f2937;border-radius:14px;padding:16px;margin-bottom:12px}
.live{display:flex;align-items:baseline;gap:10px;justify-content:center}
.big{font-size:46px;font-weight:700}.unit{font-size:18px;color:#94a3b8}
.mode{text-align:center;color:#38bdf8;font-size:13px;letter-spacing:1px}
.row{display:flex;gap:8px;flex-wrap:wrap}
button,a.btn{flex:1;min-width:130px;text-align:center;text-decoration:none;
border:0;border-radius:10px;padding:12px;font-size:14px;color:#fff;cursor:pointer}
.b1{background:#16a34a}.b2{background:#2563eb}.b3{background:#dc2626}
table{width:100%;border-collapse:collapse;font-size:14px}
th,td{padding:8px;border-bottom:1px solid #1f2937;text-align:left}
th{color:#94a3b8;font-weight:600}.empty{color:#64748b;text-align:center;padding:18px}
</style></head><body>
<header><h1>Medidor de Obra</h1><div class="sub">Belas Artes - FEBASP</div></header>
<div class="wrap">
<a class="btn b1" href="/sol" style="display:block;margin-bottom:12px;text-decoration:none;background:#b45309">Sol / Insolacao (carta solar offline)</a>
<div class="card"><div class="mode" id="mode">--</div>
<div class="live"><span class="big" id="val">--</span><span class="unit" id="unit"></span></div></div>
<div class="card"><div class="row">
<a class="btn b2" href="/data.csv">Baixar CSV</a>
<button class="b1" onclick="setClock()">Acertar relogio (hora do cel)</button>
<button class="b3" onclick="clr()">Limpar</button>
</div><div class="sub" id="msg" style="margin-top:8px"></div></div>
<div class="card"><table><thead><tr><th>Hora</th><th>Funcao</th><th>Valor</th></tr></thead>
<tbody id="tb"><tr><td colspan="3" class="empty">sem medicoes ainda</td></tr></tbody></table></div>
</div><script>
async function live(){try{let r=await fetch('/live');let j=await r.json();
mode.textContent=j.mode;val.textContent=j.value.toFixed(1);unit.textContent=j.unit;}catch(e){}}
async function load(){try{let r=await fetch('/data');let a=await r.json();
if(!a.length){tb.innerHTML='<tr><td colspan=3 class=empty>sem medicoes ainda</td></tr>';return;}
tb.innerHTML=a.map(x=>`<tr><td>${x.t}</td><td>${x.m}</td><td>${x.v.toFixed(1)} ${x.u}</td></tr>`).reverse().join('');}catch(e){}}
async function clr(){if(!confirm('Limpar todas as medicoes?'))return;await fetch('/clear');load();}
async function setClock(){let d=new Date();
let q=`y=${d.getFullYear()}&mo=${d.getMonth()+1}&d=${d.getDate()}&h=${d.getHours()}&mi=${d.getMinutes()}&s=${d.getSeconds()}&w=${d.getDay()}`;
await fetch('/settime?'+q);msg.textContent='Relogio acertado: '+d.toLocaleString();}
live();load();setInterval(live,1500);setInterval(load,3000);
</script></body></html>)HTML";

// ---- pagina SOL / INSOLACAO (calculo offline no navegador) ----------------
static const char PAGE_SOL[] PROGMEM = R"SOL(<!doctype html><html lang="pt-br"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Sol / Insolacao</title><style>
*{box-sizing:border-box}body{margin:0;background:#0b0f17;color:#e5e7eb;font-family:system-ui,Segoe UI,Roboto,Arial}
header{padding:14px;text-align:center;background:#111827;border-bottom:1px solid #1f2937}
h1{margin:0;font-size:18px}.sub{color:#94a3b8;font-size:12px;margin-top:4px}
.wrap{padding:14px;max-width:640px;margin:0 auto}
.card{background:#111827;border:1px solid #1f2937;border-radius:14px;padding:14px;margin-bottom:12px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
label{font-size:12px;color:#94a3b8;display:flex;flex-direction:column;gap:4px}
input,select{background:#0b0f17;border:1px solid #334155;color:#e5e7eb;border-radius:8px;padding:9px;font-size:15px}
.row{display:flex;gap:8px;margin-top:10px}
button,a.btn{flex:1;text-align:center;text-decoration:none;border:0;border-radius:10px;padding:12px;font-size:14px;color:#fff;cursor:pointer}
.b1{background:#16a34a}.b2{background:#b45309}
.kv{display:flex;justify-content:space-between;padding:6px 0;border-bottom:1px solid #1f2937;font-size:14px}
.kv span{color:#94a3b8}hr{border:0;border-top:1px solid #1f2937;margin:8px 0}
a.back{color:#38bdf8;font-size:13px;text-decoration:none}
</style></head><body>
<header><h1>Sol / Insolacao</h1><div class="sub">carta solar - calculo offline, sem internet</div></header>
<div class="wrap">
<div class="card"><a class="back" href="/">&lt; voltar aos dados</a>
<div class="grid" style="margin-top:10px">
<label>Latitude<input id="lat" type="number" step="any" value="-23.55"></label>
<label>Longitude<input id="lon" type="number" step="any" value="-46.63"></label>
<label>Fuso (h)<input id="tz" type="number" value="-3"></label>
<label>Data<input id="dt" type="date"></label>
<label>Fachada voltada p/<select id="fac">
<option value="0">Norte</option><option value="45">Nordeste</option><option value="90">Leste</option>
<option value="135">Sudeste</option><option value="180">Sul</option><option value="225">Sudoeste</option>
<option value="270">Oeste</option><option value="315">Noroeste</option></select></label>
<label>Altura p/ sombra (m)<input id="h" type="number" value="3"></label>
</div>
<div class="row"><button class="b2" onclick="calc()">Calcular</button>
<button class="b1" onclick="geo()">Usar GPS do celular</button></div>
<div class="row"><button class="b1" onclick="bussola()" style="background:#7c3aed">Bussola do cel</button>
<button class="b2" onclick="send()" style="background:#0f766e">Mostrar na tela do ESP</button></div>
<div class="sub" id="msg" style="margin-top:8px"></div></div>
<div class="card" id="out"></div>
<div class="card"><div class="sub">Carta solar (azimute x altura do sol)</div><div id="chart"></div></div>
</div><script>
const R=Math.PI/180,D=180/Math.PI,$=id=>document.getElementById(id);
function doy(d){return Math.floor((Date.UTC(d.getFullYear(),d.getMonth(),d.getDate())-Date.UTC(d.getFullYear(),0,0))/864e5);}
function decl(N){return 23.45*Math.sin(R*360*(284+N)/365);}
function eot(N){const B=R*360*(N-81)/364;return 9.87*Math.sin(2*B)-7.53*Math.cos(B)-1.5*Math.sin(B);}
function pos(lat,lon,tz,N,t){const dec=decl(N)*R,E=eot(N),TC=4*(lon-15*tz)+E,LST=t+TC/60,H=(15*(LST-12))*R,phi=lat*R;
const se=Math.sin(phi)*Math.sin(dec)+Math.cos(phi)*Math.cos(dec)*Math.cos(H);const el=Math.asin(Math.max(-1,Math.min(1,se)));
let cA=(Math.sin(dec)-Math.sin(phi)*Math.sin(el))/(Math.cos(phi)*Math.cos(el));cA=Math.max(-1,Math.min(1,cA));
let A=Math.acos(cA)*D;if(Math.sin(H)>0)A=360-A;return{el:el*D,az:A};}
function times(lat,lon,tz,N){const dec=decl(N)*R,phi=lat*R,E=eot(N),TC=4*(lon-15*tz)+E,noon=12-TC/60;
let c=-Math.tan(phi)*Math.tan(dec);if(c>1)return{none:'sol nao nasce hoje (noite polar)'};if(c<-1)return{none:'sol nao se poe hoje (dia polar)'};
const H0=Math.acos(c)*D/15;return{rise:noon-H0,set:noon+H0,noon,len:2*H0};}
function fmt(h){if(h==null)return'--';h=(h+24)%24;let m=Math.round(h*60);return String(Math.floor(m/60)).padStart(2,'0')+':'+String(m%60).padStart(2,'0');}
function chart(pts,fac){const W=340,H=200,x0=14,x1=W-8,y0=H-22,y1=10,X=a=>x0+(a/360)*(x1-x0),Y=e=>y0-(e/90)*(y0-y1);
let s=`<svg viewBox="0 0 ${W} ${H}" width="100%">`;
[[0,'N'],[90,'L'],[180,'S'],[270,'O'],[360,'N']].forEach(g=>{const x=X(g[0]);
s+=`<line x1="${x}" y1="${y1}" x2="${x}" y2="${y0}" stroke="#1f2937"/><text x="${x}" y="${H-6}" fill="#64748b" font-size="11" text-anchor="middle">${g[1]}</text>`;});
[30,60].forEach(e=>{const y=Y(e);s+=`<line x1="${x0}" y1="${y}" x2="${x1}" y2="${y}" stroke="#1f2937"/><text x="${x0+2}" y="${y-2}" fill="#475569" font-size="9">${e}&#176;</text>`;});
s+=`<line x1="${x0}" y1="${y0}" x2="${x1}" y2="${y0}" stroke="#334155"/>`;
const fx=X(((fac%360)+360)%360);s+=`<line x1="${fx}" y1="${y1}" x2="${fx}" y2="${y0}" stroke="#f59e0b" stroke-dasharray="3 3"/>`;
if(pts.length){let d=pts.map((p,i)=>(i?'L':'M')+X(p.az).toFixed(1)+' '+Y(p.el).toFixed(1)).join(' ');
s+=`<path d="${d}" fill="none" stroke="#38bdf8" stroke-width="2.5"/>`;}return s+'</svg>';}
function calc(){const lat=+$('lat').value,lon=+$('lon').value,tz=+$('tz').value,fac=+$('fac').value,hh=+$('h').value;
const d=new Date($('dt').value+'T12:00');if(isNaN(d)){$('msg').textContent='data invalida';return;}const N=doy(d),T=times(lat,lon,tz,N);
if(T.none){$('out').innerHTML='<b>'+T.none+'</b>';$('chart').innerHTML='';return;}
let pts=[],maxel=0;for(let t=T.rise;t<=T.set;t+=0.1){const p=pos(lat,lon,tz,N,t);if(p.el>=0){pts.push(p);if(p.el>maxel)maxel=p.el;}}
const eln=pos(lat,lon,tz,N,T.noon).el;let facH=0,win=[];
for(let t=T.rise;t<=T.set;t+=1/60){const p=pos(lat,lon,tz,N,t);if(p.el>0&&Math.cos((p.az-fac)*R)>0){facH+=1/60;win.push(t);}}
const sh=eln>1?hh/Math.tan(eln*R):null;
$('out').innerHTML=`<div class="kv"><span>Nascer do sol</span><b>${fmt(T.rise)}</b></div>
<div class="kv"><span>Por do sol</span><b>${fmt(T.set)}</b></div>
<div class="kv"><span>Meio-dia solar (sol a pino)</span><b>${fmt(T.noon)}</b></div>
<div class="kv"><span>Duracao do dia</span><b>${T.len.toFixed(1)} h</b></div>
<div class="kv"><span>Altura max. do sol</span><b>${maxel.toFixed(0)}&#176;</b></div><hr>
<div class="kv"><span>Sol direto na fachada</span><b>${facH.toFixed(1)} h</b></div>
<div class="kv"><span>Janela de sol na fachada</span><b>${win.length?fmt(win[0])+' - '+fmt(win[win.length-1]):'nunca'}</b></div>
<div class="kv"><span>Sombra ao meio-dia (h=${hh}m)</span><b>${sh!=null?sh.toFixed(2)+' m':'--'}</b></div>`;
$('chart').innerHTML=chart(pts,fac);}
function geo(){if(!navigator.geolocation){$('msg').textContent='navegador sem GPS';return;}
$('msg').textContent='pedindo GPS...';navigator.geolocation.getCurrentPosition(
p=>{$('lat').value=p.coords.latitude.toFixed(4);$('lon').value=p.coords.longitude.toFixed(4);$('msg').textContent='GPS ok';calc();},
e=>{$('msg').textContent='GPS bloqueado (pagina http) - digite lat/lon na mao. '+e.message;},{timeout:8000,enableHighAccuracy:true});}
function send(){const q=`lat=${$('lat').value}&lon=${$('lon').value}&tz=${$('tz').value}&fac=${$('fac').value}&h=${$('h').value}`;
fetch('/setsol?'+q).then(()=>{$('msg').textContent='Enviado! Veja a carta solar na tela do ESP (menu SOL).';}).catch(()=>{$('msg').textContent='falhou ao enviar';});}
function bussola(){function ok(e){let hd=(e.webkitCompassHeading!=null)?e.webkitCompassHeading:((e.alpha!=null)?(360-e.alpha):null);
if(hd!=null){$('fac').value=(Math.round(hd/45)*45)%360;$('msg').textContent='bussola: '+Math.round(hd)+' graus -> fachada ajustada';calc();
window.removeEventListener('deviceorientationabsolute',ok);window.removeEventListener('deviceorientation',ok);}}
if(typeof DeviceOrientationEvent!=='undefined'&&DeviceOrientationEvent.requestPermission){
DeviceOrientationEvent.requestPermission().then(p=>{if(p==='granted'){window.addEventListener('deviceorientationabsolute',ok);window.addEventListener('deviceorientation',ok);}else $('msg').textContent='permissao da bussola negada';}).catch(()=>{$('msg').textContent='bussola bloqueada (precisa HTTPS)';});
}else{window.addEventListener('deviceorientationabsolute',ok);window.addEventListener('deviceorientation',ok);setTimeout(()=>{if(!/bussola/.test($('msg').textContent))$('msg').textContent='bussola indisponivel nesta pagina (http)';},1500);}}
(function(){const n=new Date();$('dt').value=n.getFullYear()+'-'+String(n.getMonth()+1).padStart(2,'0')+'-'+String(n.getDate()).padStart(2,'0');calc();})();
</script></body></html>)SOL";

// ---- handlers -------------------------------------------------------------
static void handleRoot() { server.send_P(200, "text/html", PAGE); }
static void handleSol()  { server.send_P(200, "text/html", PAGE_SOL); }

static void handleLive() {
    char buf[96];
    snprintf(buf, sizeof(buf), "{\"mode\":\"%s\",\"value\":%.1f,\"unit\":\"%s\"}",
             LevelApp_ModeName(), (double)LevelApp_Value(), LevelApp_Unit());
    server.send(200, "application/json", buf);
}

static void handleData() {
    String j = "[";
    int n = DataLog_Count();
    for (int i = 0; i < n; i++) {
        const LogEntry *e = DataLog_Get(i);
        if (i) j += ",";
        j += "{\"t\":\""; j += e->when; j += "\",\"m\":\""; j += e->mode;
        j += "\",\"v\":"; j += String(e->value, 1);
        j += ",\"u\":\""; j += e->unit; j += "\"}";
    }
    j += "]";
    server.send(200, "application/json", j);
}

static void handleCsv() {
    String c = "hora,funcao,valor,unidade\n";
    int n = DataLog_Count();
    for (int i = 0; i < n; i++) {
        const LogEntry *e = DataLog_Get(i);
        c += e->when; c += ","; c += e->mode; c += ",";
        c += String(e->value, 1); c += ","; c += e->unit; c += "\n";
    }
    server.sendHeader("Content-Disposition", "attachment; filename=medicoes.csv");
    server.send(200, "text/csv", c);
}

static void handleClear() { DataLog_Clear(); server.send(200, "text/plain", "ok"); }

static void handleSetTime() {
    datetime_t t;
    t.year   = server.arg("y").toInt();
    t.month  = server.arg("mo").toInt();
    t.day    = server.arg("d").toInt();
    t.hour   = server.arg("h").toInt();
    t.minute = server.arg("mi").toInt();
    t.second = server.arg("s").toInt();
    t.dotw   = server.arg("w").toInt();
    PCF85063_Set_All(t);
    server.send(200, "text/plain", "ok");
}

static void handleSetSol() {                 // celular envia local/fachada p/ a tela do ESP
    if (server.hasArg("lat")) sunP.lat = server.arg("lat").toFloat();
    if (server.hasArg("lon")) sunP.lon = server.arg("lon").toFloat();
    if (server.hasArg("tz"))  sunP.tz  = server.arg("tz").toFloat();
    if (server.hasArg("fac")) sunP.facadeAz = server.arg("fac").toFloat();
    if (server.hasArg("h"))   sunP.objH = server.arg("h").toFloat();
    server.send(200, "text/plain", "ok");
}

// ---- API ------------------------------------------------------------------
void WebPortal_Init() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    server.on("/", handleRoot);
    server.on("/sol", handleSol);
    server.on("/live", handleLive);
    server.on("/data", handleData);
    server.on("/data.csv", handleCsv);
    server.on("/clear", handleClear);
    server.on("/settime", handleSetTime);
    server.on("/setsol", handleSetSol);
    server.begin();
}

void WebPortal_Loop() { server.handleClient(); }
