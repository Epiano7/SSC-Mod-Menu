import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtemp, rm, readdir, readFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { gunzipSync } from 'node:zlib';
import { validate, Store, server } from './service.mjs';
const fixture=()=>({schema:1,round_id:'a'.repeat(32),round:1,game_build:26047692,collector:1,model:1,start_observed:false,truncated:false,duration_ms:1000,end_observed:true,events:[{type:0,ms:0,build:{class:1,skills:[],mutations:[]}}]});
test('strict anonymous schema rejects extra identifiers and arbitrary metrics',()=>{
 validate(fixture());const native=fixture();native.events.push({type:4,ms:10,offers:[2,7,-1],previous:[0,1,-1],reroll_counter:1});validate(native);native.events[1].offers[0]=54;assert.throws(()=>validate(native));for(const key of ['steam_id','name','computer','chat']){const d=fixture();d[key]='private';assert.throws(()=>validate(d));}
 const d=fixture();d.events.push({type:3,ms:10,values:{username:'private'}});assert.throws(()=>validate(d));
 const c=fixture();c.events.push({type:2,ms:10,skill:1,kind:0,followed:true,advice_event:0});assert.throws(()=>validate(c));
});
test('compressed store is idempotent, bounded and preserves exact payload',async()=>{
 const dir=await mkdtemp(join(tmpdir(),'ssc-training-'));try{const store=new Store(dir);const d=fixture();assert.equal(await store.put(d),true);assert.equal(await store.put(d),false);d.round=2;await assert.rejects(store.put(d),/id_conflict/);const files=await readdir(dir);assert.equal(files.length,1);assert.deepEqual(JSON.parse(gunzipSync(await readFile(join(dir,files[0])))),fixture());
 const capped=new Store(join(dir,'cap'),{maxBytes:1});await assert.rejects(capped.put(fixture()),/storage_full/);
 }finally{await rm(dir,{recursive:true,force:true});}
});
test('HTTP accepts valid round, rejects PII, oversized and repeated requests',async()=>{
 const dir=await mkdtemp(join(tmpdir(),'ssc-training-'));const s=server(new Store(dir));await new Promise(r=>s.listen(0,'127.0.0.1',r));const url=`http://127.0.0.1:${s.address().port}/api/draft-training/v1/round`;
 const send=d=>fetch(url,{method:'POST',headers:{'content-type':'application/json'},body:JSON.stringify(d)});
 try{assert.equal((await send(fixture())).status,201);assert.equal((await send(fixture())).status,200);assert.equal((await send({...fixture(),name:'private'})).status,400);assert.equal((await send({junk:'a'.repeat(270000)})).status,413);for(let i=0;i<9;i++)await send(fixture());assert.equal((await send(fixture())).status,429);}finally{await new Promise(r=>s.close(r));await rm(dir,{recursive:true,force:true});}
});
if(process.env.SSC_TRAINING_FIXTURE)test('client fixture matches service schema',async()=>validate(JSON.parse(await readFile(process.env.SSC_TRAINING_FIXTURE,'utf8'))));

test('bounded draft context accepts numeric observations and rejects identifiers and malformed rows',()=>{
 const d=fixture();const e={type:5,ms:10,build:d.events[0].build,points:1,counter:2,flag:0,seeds:[790369,1357],spent:0,bonus:0,allocations:[1],progression:[[1,2]],offers:[[11,1,0],[23,1,0]],previous:[[26,1,0],[2,1,0]],history:[[26,2,10,1,1]]};d.events.push(e);validate(d);
 for(const change of [x=>x.name='private',x=>x.seeds.push(3),x=>x.offers[0][0]=54,x=>x.history[0].pop(),x=>x.allocations=Array(513).fill(0),x=>x.spent=2147483648]){const bad=structuredClone(d);change(bad.events[1]);assert.throws(()=>validate(bad));}
});
