import { createServer } from 'node:http';
import { mkdir, readdir, stat, unlink, writeFile, readFile } from 'node:fs/promises';
import { join } from 'node:path';
import { gzipSync, gunzipSync } from 'node:zlib';
import { pathToFileURL } from 'node:url';
import { isIP } from 'node:net';

const MAX = 262144;
const fail = () => { throw new Error('invalid_schema'); };
const object = (x, keys) => { if (!x || Array.isArray(x) || typeof x !== 'object' || Object.keys(x).sort().join() !== [...keys].sort().join()) fail(); };
const integer = (x, min, max) => { if (!Number.isSafeInteger(x) || x < min || x > max) fail(); };
const bool = x => { if (typeof x !== 'boolean') fail(); };
const array = (x, max) => { if (!Array.isArray(x) || x.length > max) fail(); };
const build = b => {
 object(b, ['class','skills','mutations']); integer(b.class,1,8); array(b.skills,54); array(b.mutations,54);
 const ids = new Set(); for (const s of b.skills) { array(s,2); if(s.length!==2)fail(); integer(s[0],0,53);integer(s[1],1,300);if(ids.has(s[0]))fail();ids.add(s[0]); }
 const mutations=new Set();for(const id of b.mutations){integer(id,0,53);if(mutations.has(id))fail();mutations.add(id);}
};
const metrics=new Set(['level','xp','healthRemaining','maxHealth','shieldRemaining','money','syringes','roundDeaths','damageTaken','timeAlive','deadTime','shotsFired','projectilesFired']);
export function validate(d) {
 object(d,['schema','round_id','round','game_build','collector','model','start_observed','truncated','duration_ms','end_observed','events']);
 if(d.schema!==1||d.collector!==1||d.model!==1||typeof d.round_id!=='string'||!/^[a-f0-9]{32}$/.test(d.round_id))fail();
 integer(d.round,1,100);integer(d.game_build,1,100000000);integer(d.duration_ms,0,86400000);bool(d.start_observed);bool(d.truncated);bool(d.end_observed);array(d.events,512);
 let previous=0;
 for(let index=0;index<d.events.length;index++){
  const e=d.events[index];integer(e?.type,0,5);integer(e.ms,previous,d.duration_ms);previous=e.ms;
  if(e.type===0){object(e,['type','ms','build']);build(e.build);}
  if(e.type===1){object(e,['type','ms','kind','build','candidates','best','api_available','api_used']);integer(e.kind,0,1);build(e.build);bool(e.api_available);bool(e.api_used);if(e.api_used&&!e.api_available)fail();array(e.candidates,54);const ids=new Set();
   for(const c of e.candidates){array(c,3);if(c.length!==3)fail();integer(c[0],0,53);if(typeof c[1]!=='number'||!Number.isFinite(c[1])||Math.abs(c[1])>100000)fail();integer(c[2],0,1000000);if(ids.has(c[0]))fail();ids.add(c[0]);}if(!ids.has(e.best))fail();
  }
  if(e.type===2){object(e,['type','ms','skill','kind','followed','advice_event']);
   if(e.skill===null){if(e.kind!==null||e.followed!==null||e.advice_event!==null)fail();}
   else {integer(e.skill,0,53);integer(e.kind,0,1);bool(e.followed);integer(e.advice_event,0,index-1);const a=d.events[e.advice_event];if(a.type!==1||a.kind!==e.kind||!a.candidates.some(c=>c[0]===e.skill)||e.followed!==(a.best===e.skill))fail();}
  }
  if(e.type===3){object(e,['type','ms','values']);if(!e.values||Array.isArray(e.values)||typeof e.values!=='object')fail();for(const [k,v]of Object.entries(e.values))if(!metrics.has(k)||typeof v!=='number'||!Number.isFinite(v)||v<0||v>100000000)fail();}
  if(e.type===4){object(e,['type','ms','offers','previous','reroll_counter']);integer(e.reroll_counter,0,1024);for(const a of [e.offers,e.previous]){array(a,3);for(const id of a)integer(id,-1,53);}}
  if(e.type===5){
   object(e,['type','ms','build','points','counter','flag','seeds','spent','bonus','allocations','progression','offers','previous','history']);build(e.build);
   integer(e.points,0,1024);integer(e.counter,0,1024);integer(e.flag,0,1);
   const raw=v=>integer(v,-2147483648,2147483647);
   array(e.seeds,2);if(e.seeds.length!==2)fail();e.seeds.forEach(raw);raw(e.spent);raw(e.bonus);
   array(e.allocations,512);e.allocations.forEach(raw);
   for(const [key,width,limit] of [['progression',2,512],['offers',3,3],['previous',3,3],['history',5,512]]){
    array(e[key],limit);for(const row of e[key]){array(row,width);if(row.length!==width)fail();row.forEach(raw);if(key==='offers'||key==='previous')integer(row[0],-1,53);}
   }
  }
 }
 if(Buffer.byteLength(JSON.stringify(d))>MAX)fail();return d;
}
// Anonymous submissions are untrusted research, never automatic model updates.
// No request bodies, IPs, user agents, account IDs or timestamps are persisted.
export class Store {
 constructor(directory,{maxBytes=1024*1024*1024,days=90}={}){this.directory=directory;this.maxBytes=maxBytes;this.days=days;this.tail=Promise.resolve();}
 put(d){validate(d);const operation=this.tail.then(()=>this.save(d));this.tail=operation.catch(()=>{});return operation;}
 async save(d){
  await mkdir(this.directory,{recursive:true});const path=join(this.directory,d.round_id+'.json.gz');const raw=JSON.stringify(d);
  try{const existing=gunzipSync(await readFile(path)).toString();if(existing!==raw)throw new Error('id_conflict');return false;}catch(e){if(e.code!=='ENOENT')throw e;}
  let bytes=0,count=0;for(const name of await readdir(this.directory)){if(!/^[a-f0-9]{32}\.json\.gz$/.test(name))continue;const p=join(this.directory,name),s=await stat(p);if(Date.now()-s.mtimeMs>this.days*86400000)await unlink(p);else{bytes+=s.size;count++;}}
  const compressed=gzipSync(raw,{level:6});if(bytes+compressed.length>this.maxBytes||count>=100000)throw new Error('storage_full');
  await writeFile(path,compressed,{flag:'wx'});return true;
 }
}
export function server(store,{trustProxy=false}={}){
 const buckets=new Map();let total=0,window=Date.now();
 return createServer({requestTimeout:5000,headersTimeout:5000,maxHeaderSize:4096},(req,res)=>{
  const reply=(code,data)=>{if(res.writableEnded)return;res.writeHead(code,{'Content-Type':'application/json','Cache-Control':'no-store'});res.end(JSON.stringify(data));};
  if(req.method==='GET'&&req.url==='/api/draft-training/v1/status')return reply(200,{schema:1,ready:true,anonymous:true,untrusted:true});
  if(req.method!=='POST'||req.url!=='/api/draft-training/v1/round'){req.resume();return reply(404,{code:'not_found'});}
  let source=req.socket.remoteAddress;
  if(trustProxy){if(!['127.0.0.1','::1','::ffff:127.0.0.1'].includes(source)||!isIP(req.headers['x-real-ip']||'')){req.resume();return reply(400,{code:'invalid_proxy'});}source=req.headers['x-real-ip'];}
  const now=Date.now();if(now-window>=60000){window=now;total=0;buckets.clear();}
  const n=(buckets.get(source)||0)+1;if(++total>120||n>12||buckets.size>=1024&&!buckets.has(source)){req.resume();return reply(429,{code:'rate_limited'});}buckets.set(source,n);
  const length=req.headers['content-length'];
  if(req.headers['content-type']!=='application/json'||req.headers['content-encoding']||req.headers['transfer-encoding']||!/^\d+$/.test(length||'')||Number(length)>MAX){req.resume();return reply(413,{code:'invalid_body'});}
  let size=0;const chunks=[];req.setTimeout(5000,()=>req.destroy());req.on('error',()=>{});
  req.on('data',chunk=>{size+=chunk.length;if(size>MAX){reply(413,{code:'too_large'});req.destroy();}else chunks.push(chunk);});
  req.on('end',async()=>{if(res.writableEnded)return;try{const d=validate(JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(Buffer.concat(chunks))));const fresh=await store.put(d);reply(fresh?201:200,{accepted:true});}catch(e){reply(e.message==='storage_full'?503:e.message==='id_conflict'?409:400,{code:'not_accepted'});}});
 });
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
 if(!process.argv[2])throw new Error('Data directory required');server(new Store(process.argv[2]),{trustProxy:process.argv.includes('--trust-proxy')}).listen(18744,'127.0.0.1');
}
