// Production updater and actual installer click routing, with the existing wire fake.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
import { Updater, pack7, unpack7 } from '../web/fm1ota.js';
const eq = (a,b) => a.length === b.length && a.every((v,i) => v === b[i]);
const source = readFileSync('web/test_web.mjs','utf8');
const FakeFM1 = vm.runInNewContext(source.slice(source.indexOf('const HS ='), source.indexOf('async function updater()')) + ';FakeFM1',
  { pack7, unpack7, eq, setTimeout: (fn,ms) => setTimeout(fn,Math.min(ms,1)), TextEncoder, Uint8Array });
const image = new Uint8Array(8192);
const html = readFileSync('web/index_pkg.html','utf8');
async function click(fake, stock = false) {
  let handler, failure = null, done = false;
  const start = html.indexOf(`$("${stock ? 'stock-go' : 'go'}").addEventListener("click"`);
  const end = stock ? html.indexOf('</script>', start) : html.indexOf('/* back to the official', start);
  vm.runInNewContext(html.slice(start,end), { Updater, OUR_LOADER: id => /^ota-FM-1_9\d\d$/.test(id.text),
    writing:false, stock:{image,product:'FM-1_015'}, image, meta:{product:'FM-1_900'}, log:()=>{}, step:()=>{},
    navigator:{requestMIDIAccess:async()=>fake.access}, confirm:()=>true,t:x=>x,
    $:()=>({checked:true,addEventListener:(_type,fn)=>{handler=fn}}), lockAll:()=>{},
    sayK:key=>{if(key==='done')done=true}, failed:e=>{failure=e} });
  await handler();
  return {failure,done};
}
for (const stock of [false,true]) {
  const fake = new FakeFM1(image, {finalIdentity:stock?'FM-1_015':'FM-1_900'});
  fake.boot(stock?'ota-FM-1_015':'ota-FM-1_900','USB MIDI Device');
  const result = await click(fake,stock);
  assert.equal(result.failure,null); assert(result.done); assert.equal(fake.bad,0);
}
console.log('page routing: generic own and official-V15 loader recovery passed');
{
  const fake = new FakeFM1(image);
  const boot = fake.boot.bind(fake);
  fake.boot = (id,name) => boot(id.startsWith('ota-')?'ota-FM-1_015':id,name);
  const error = await new Updater(fake.access).install(image,'FM-1_900').then(()=>null,e=>e);
  assert.equal(error.code,'foreign'); assert.equal(fake.served,4);
  assert.equal(fake.input.onmidimessage,null);
}
console.log('post-stage-one foreign loader rejected before data/write');
{
  const fake = new FakeFM1(image,{finalIdentity:'FM-1_015'});
  const boot = fake.boot.bind(fake);
  fake.boot = (id,name) => boot(id.startsWith('ota-')?'ota-FM-1_015':id,name);
  assert.equal(await new Updater(fake.access).install(image,'FM-1_015'),'FM-1_015');
  assert.equal(fake.bad,0);
}
console.log('full official-V15 stage transition remains supported');
{
  const a=new FakeFM1(image), b=new FakeFM1(image);
  b.boot('ota-FM-1_900','USB MIDI Device');
  const access={inputs:new Map([...a.access.inputs,...b.access.inputs]),outputs:new Map([...a.access.outputs,...b.access.outputs])};
  await assert.rejects(new Updater(access).find(id=>!id.model.startsWith('ota-'),true),{code:'ambiguous'});
  assert.equal(a.served+b.served,0);
  assert.equal(a.input.onmidimessage,null);assert.equal(b.input.onmidimessage,null);
}
console.log('multiple FM-1 modes rejected and discovery handlers released');
{
  const fake=new FakeFM1(image);fake.boot('ota-FM-1_900','Felucca Update');
  const up=new Updater(fake.access);
  up.serve=async()=>{throw Object.assign(new Error('out of image'),{code:'badreq'})};
  await assert.rejects(up.resume(image),{code:'badreq'});
  assert.equal(fake.input.onmidimessage,null);
  const ota=await up.find(id=>id.model.startsWith('ota-'));
  await assert.rejects(up.write(ota,image,()=>{throw new Error('callback')}),{code:'badreq'});
  assert.equal(fake.input.onmidimessage,null);
}
{
  const fake=new FakeFM1(image);fake.boot('ota-FM-1_900','Felucca Update');
  await assert.rejects(new Updater(fake.access).resume(image,()=>{throw new Error('progress callback')}),/progress callback/);
  assert.equal(fake.input.onmidimessage,null);
}
{
  const fake=new FakeFM1(image);fake.boot('ota-FM-1_900','Felucca Update');
  await assert.rejects(new Updater(fake.access).resume(image,null,{product:'FM-1_015'}),{code:'foreign'});
  assert.equal(fake.served,0);
}
console.log('error cleanup and official-loader policy passed');

// UI functions are outside PROTO; exercise the actual lifetime boundary too.
const editor=readFileSync('web/editor.html','utf8'), noop=()=>{};
const disconnect=editor.slice(editor.indexOf('function disconnect(reasonKey'),editor.indexOf('function onState(e)'));
const load=editor.slice(editor.indexOf('async function load(d)'),editor.indexOf('async function getDescOrNull'));
const context={stopPreview:noop,renderPanels:noop,renderMixer:noop,renderSysinfo:noop,renderLive:noop,renderLib:noop,
  renderBank:noop,setLabel:noop,sayK:noop,t:x=>x,$:()=>({}),req:{info:()=>[]},parse:{},CMD:{INFO:1}};
await vm.runInNewContext(`let busy=0,needReload=false,connectionEpoch=0,reject;
let dev={input:{onmidimessage(){}},link:{request:()=>new Promise((a,b)=>reject=b),close(){reject(new Error('closed'))}}};
${disconnect}\n${load}
(async()=>{const pending=load(dev);disconnect('lost');await pending.catch(()=>{});
if(busy!==0)throw new Error('busy='+busy);})()`,context);
console.log('disconnect during pending load releases busy to zero');
const connect=editor.slice(editor.indexOf('async function connect()'),editor.indexOf('function disconnect(reasonKey'));
await vm.runInNewContext(`let busy=0,needReload=false,connectionEpoch=0,connectingEpoch=null,dev=null,wantConnected=false;
const MOCK=false,access={},sets={clear(){}},onPush=()=>{},rewatch=false;
const port={input:{open:async()=>{}},output:{send(){}}};
const fm1Ports=()=>[port];class Link{async request(){return []}close(){}}
let loads=0,rejectOld;const load=()=>{busy++;return (++loads===1?new Promise((a,b)=>rejectOld=b):Promise.resolve()).finally(()=>busy--)};
${connect}\n${disconnect}
(async()=>{const old=connect();while(!rejectOld)await Promise.resolve();disconnect('lost');
await connect();const replacement=dev;rejectOld(new Error('old connection failed'));await old;
if(!replacement||dev!==replacement||busy!==0)throw new Error('old load changed replacement session');})()`,
{...context,console:{error:()=>{}},navigator:{requestMIDIAccess:async()=>({})}});
console.log('late old-session failure preserves the replacement connection and balanced counter');
const proto=editor.slice(editor.indexOf('/*PROTO-BEGIN*/'),editor.indexOf('/*PROTO-END*/'));
const E=vm.runInNewContext(proto+';({makeMockDevice,req,parse,CMD,crc32,frame,unframe})',
  {setTimeout,clearTimeout,setInterval,clearInterval,console,TextEncoder,TextDecoder,Blob,Response,DecompressionStream});
{
  const mock=E.makeMockDevice({auto:false}), data=mock.state.settings.slice();
  const input=[...mock.access.inputs.values()][0], output=[...mock.access.outputs.values()][0];
  const request=([cmd,args])=>new Promise(resolve=>{input.onmidimessage=event=>resolve(E.parse[E.CMD.BK_PUT](E.unframe(event.data).a));output.send(E.frame(cmd,args))});
  mock.state.playing=true;
  assert.equal((await request(E.req.bkBegin(1,data.length,E.crc32(data)))).rc,0);
  for(let offset=0;offset<data.length;offset+=256)assert.equal((await request(E.req.bkData(1,offset,data.subarray(offset,offset+256)))).rc,0);
  assert.equal((await request(E.req.bkCommit(1))).rc,3);
  assert.deepEqual(mock.state.settings,data);mock.stop();
}
console.log('mock/firmware parity: unchanged settings restore is refused during playback');
