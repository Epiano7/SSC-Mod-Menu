# SPDX-License-Identifier: GPL-3.0-or-later
"""Optional Piper worker. Private pipes only; network is used only by install/download."""
import argparse, base64, hashlib, io, json, math, os, re, shutil, struct, sys, time
from pathlib import Path, PurePosixPath
MAX_MODEL=256*1024*1024
MAX_CONFIG=1024*1024
MAX_AUDIO=2*1024*1024

def emit(**fields):
    print(json.dumps(fields,separators=(',',':'),ensure_ascii=True),flush=True)

def digest(path):
    h=hashlib.sha256()
    with open(path,'rb') as stream:
        for b in iter(lambda:stream.read(1024*1024),b''):h.update(b)
    return h.hexdigest()

def no_links(path):
    for p in (path,*path.parents):
        if p.is_symlink() or (hasattr(p,'is_junction') and p.is_junction()):raise ValueError('Linked paths are unsupported')

def remove_tree(root,target):
    no_links(root);no_links(target)
    if target.resolve().parent!=root.resolve():raise ValueError('Refusing to remove a path outside speech storage')
    shutil.rmtree(target)

def json_file(path,limit=MAX_CONFIG):
    no_links(path)
    if path.stat().st_size>limit:raise ValueError('Metadata is too large')
    with path.open(encoding='utf-8') as stream:return json.load(stream)

def safe_relative(name):
    if not isinstance(name,str) or not name or '\\' in name or ':' in name or '\x00' in name:raise ValueError('Invalid archive path')
    p=PurePosixPath(name)
    if p.is_absolute() or any(x in ('..','.') for x in p.parts):raise ValueError('Invalid archive path')
    return p

def extract_zip(path,target):
    import zipfile
    with zipfile.ZipFile(path) as archive:
        entries=archive.infolist()
        if len(entries)>20000 or sum(x.file_size for x in entries)>900*1024*1024:raise ValueError('Archive exceeds limits')
        seen=set()
        for entry in entries:
            relative=safe_relative(entry.filename.rstrip('/'))
            key=str(relative).casefold()
            if key in seen:raise ValueError('Duplicate archive path')
            seen.add(key)
            if entry.flag_bits&1 or ((entry.external_attr>>16)&0o170000)==0o120000:raise ValueError('Unsupported archive entry')
        for entry in entries:
            relative=safe_relative(entry.filename.rstrip('/'));out=target.joinpath(*relative.parts);no_links(out)
            if entry.is_dir():out.mkdir(parents=True,exist_ok=True);continue
            out.parent.mkdir(parents=True,exist_ok=True)
            with archive.open(entry) as src,out.open('wb') as dst:shutil.copyfileobj(src,dst,1024*1024)

def download(asset,path,done=0,total=1):
    import urllib.request
    if not asset['url'].startswith('https://') or not re.fullmatch('[0-9a-f]{64}',asset['sha256']) or not 0<asset['size']<100*1024*1024:raise ValueError('Invalid download manifest')
    if path.exists() and path.stat().st_size==asset['size'] and digest(path)==asset['sha256']:return
    tmp=path.with_suffix(path.suffix+'.part');tmp.parent.mkdir(parents=True,exist_ok=True)
    request=urllib.request.Request(asset['url'],headers={'User-Agent':'SSC-Mod-Menu-optional-local-tts/1.0'})
    h=hashlib.sha256();count=0;last=0
    try:
        with urllib.request.urlopen(request,timeout=25) as response,tmp.open('wb') as out:
            if not response.url.startswith('https://'):raise ValueError('Insecure redirect')
            while True:
                data=response.read(256*1024)
                if not data:break
                count+=len(data)
                if count>asset['size']:raise ValueError('Download exceeded expected size')
                h.update(data);out.write(data)
                now=time.monotonic()
                if now-last>.2:emit(type='progress',done=done+count,total=total);last=now
        if count!=asset['size'] or h.hexdigest()!=asset['sha256']:raise ValueError('Download checksum mismatch')
        os.replace(tmp,path)
    finally:
        if tmp.exists():tmp.unlink()

def runtime_files(root):
    files=[]
    for base in ('python','packages'):
        for path in sorted((root/base).rglob('*')):
            if path.is_file():
                no_links(path);files.append({'path':path.relative_to(root).as_posix(),'sha256':digest(path)})
    for name in ('piper_worker.py','piper-pins.json'):files.append({'path':name,'sha256':digest(root/name)})
    return sorted(files,key=lambda item:item['path'])

def install_runtime(root):
    pins=json_file(root/'piper-pins.json');cache=root/'downloads';cache.mkdir(exist_ok=True)
    total=sum(a['size'] for a in pins['assets'] if a['target']=='packages');done=0
    for asset in pins['assets']:
        if asset['target']!='packages':continue
        path=cache/asset['name'];download(asset,path,done,total);extract_zip(path,root/'packages');done+=asset['size'];emit(type='progress',done=done,total=total)
    # No user/global Python, pip invocation, startup hooks, or registry changes.
    manifest={'schema':1,'files':runtime_files(root)}
    (root/'ready.json').write_text(json.dumps(manifest,separators=(',',':'),sort_keys=True),encoding='utf-8')
    remove_tree(root,cache)
    emit(type='installed',runtime_manifest=digest(root/'ready.json'))

def read_varint(data,at):
    value=0
    for shift in range(0,70,7):
        if at>=len(data):raise ValueError('Truncated ONNX metadata')
        b=data[at];at+=1;value|=(b&127)<<shift
        if b<128:return value,at
    raise ValueError('Invalid ONNX varint')

def fields(data):
    at=0;count=0
    while at<len(data):
        count+=1
        if count>1000000:raise ValueError('ONNX metadata exceeds limits')
        key,at=read_varint(data,at);number,wire=key>>3,key&7
        if not number:raise ValueError('Invalid ONNX field')
        if wire==0:value,at=read_varint(data,at)
        elif wire in (1,5):
            size=8 if wire==1 else 4
            if at+size>len(data):raise ValueError('Truncated ONNX metadata')
            value=data[at:at+size];at+=size
        elif wire==2:
            size,at=read_varint(data,at)
            if size>len(data)-at:raise ValueError('Truncated ONNX metadata')
            value=data[at:at+size];at+=size
        else:raise ValueError('Unsupported ONNX wire type')
        yield number,wire,value

def check_message(data,kind,depth=0):
    if depth>48:raise ValueError('ONNX nesting exceeds limits')
    nested={'model':{7:'graph'},'graph':{1:'node',5:'tensor',15:'sparse'},'node':{5:'attribute'},'attribute':{5:'tensor',6:'graph',10:'tensor',11:'graph',22:'sparse',23:'sparse'},'sparse':{1:'tensor',2:'tensor'},'tensor':{}}
    graphs=0
    for number,wire,value in fields(data):
        if kind=='tensor' and ((number==13) or (number==14 and (wire!=0 or value!=0))):raise ValueError('External ONNX weights are unsupported')
        if kind=='model' and number==25:raise ValueError('ONNX local functions are unsupported')
        child=nested[kind].get(number)
        if child:
            if wire!=2:raise ValueError('Invalid ONNX message')
            check_message(value,child,depth+1)
            if kind=='model':graphs+=1
    if kind=='model' and graphs!=1:raise ValueError('Expected one embedded ONNX graph')

def validate_model(model,config):
    no_links(model);no_links(config)
    if model.suffix.lower()!='.onnx' or not 16<=model.stat().st_size<=MAX_MODEL:raise ValueError('Expected a Piper ONNX model, maximum 256 MiB')
    j=json_file(config)
    if not isinstance(j,dict) or j.get('phoneme_type','espeak') not in ('espeak','text'):raise ValueError('Only Piper espeak/text models are supported')
    def integer(value,low,high):return isinstance(value,int) and not isinstance(value,bool) and low<=value<=high
    if not integer(j.get('num_symbols'),1,4096) or not integer(j.get('num_speakers'),1,2048):raise ValueError('Invalid Piper dimensions')
    if not integer(j.get('audio',{}).get('sample_rate'),8000,48000):raise ValueError('Invalid sample rate')
    voice=j.get('espeak',{}).get('voice','')
    if not isinstance(voice,str) or not re.fullmatch('[A-Za-z0-9_-]{1,48}',voice):raise ValueError('Invalid phonemizer voice')
    ids=j.get('phoneme_id_map')
    if not isinstance(ids,dict) or not 1<=len(ids)<=4096:raise ValueError('Missing Piper phoneme map')
    for key,value in ids.items():
        if not isinstance(key,str) or not 1<=len(key)<=16 or not isinstance(value,list) or len(value)>32 or any(not integer(x,0,j['num_symbols']-1) for x in value):raise ValueError('Invalid phoneme map')
    for value in j.get('inference',{}).values():
        if isinstance(value,(int,float)) and (not math.isfinite(value) or abs(value)>10):raise ValueError('Invalid inference settings')
    with model.open('rb') as stream:check_message(memoryview(stream.read()),'model')
    return j

def import_model(models,source):
    source=Path(source);config=Path(str(source)+'.json');j=validate_model(source,config)
    models.mkdir(parents=True,exist_ok=True);model_hash=digest(source);config_hash=digest(config);identity='custom-'+hashlib.sha256((model_hash+config_hash).encode()).hexdigest()[:20]
    destination=models/identity
    existing=[p for p in models.iterdir() if p.is_dir() and not p.name.endswith('.partial')]
    if not destination.exists() and len(existing)>=16:raise ValueError('Maximum 16 imported voices')
    temp=models/(identity+'.partial');no_links(temp)
    if temp.exists():remove_tree(models,temp)
    temp.mkdir(exist_ok=False)
    try:
        shutil.copyfile(source,temp/'voice.onnx');shutil.copyfile(config,temp/'voice.onnx.json')
        if digest(temp/'voice.onnx')!=model_hash or digest(temp/'voice.onnx.json')!=config_hash:raise ValueError('Model changed during import')
        label=source.stem[:48];meta={'id':identity,'label':label,'model_sha256':model_hash,'config_sha256':config_hash,'speakers':j['num_speakers'],'sample_rate':j['audio']['sample_rate']}
        (temp/'voice.json').write_text(json.dumps(meta),encoding='utf-8')
        if destination.exists():remove_tree(models,temp)
        else:os.replace(temp,destination)
    finally:
        if temp.exists():remove_tree(models,temp)
    emit(type='imported',voice=meta)

def download_norman(root,models):
    pins=json_file(root/'piper-pins.json');destination=models/'norman';temp=models/'norman.partial';models.mkdir(parents=True,exist_ok=True);no_links(temp)
    if temp.exists():remove_tree(models,temp)
    temp.mkdir(exist_ok=False)
    try:
        total=sum(a['size'] for a in pins['norman']);done=0
        for a in pins['norman']:
            name='voice.onnx' if a['name'].endswith('.onnx') else 'voice.onnx.json' if a['name'].endswith('.json') else 'MODEL_CARD'
            download(a,temp/name,done,total);done+=a['size']
        j=validate_model(temp/'voice.onnx',temp/'voice.onnx.json')
        meta={'id':'norman','label':'Piper Norman','model_sha256':digest(temp/'voice.onnx'),'config_sha256':digest(temp/'voice.onnx.json'),'speakers':j['num_speakers'],'sample_rate':j['audio']['sample_rate']}
        (temp/'voice.json').write_text(json.dumps(meta),encoding='utf-8')
        if destination.exists():raise ValueError('Norman is already installed')
        os.replace(temp,destination);emit(type='downloaded',voice=meta)
    finally:
        if temp.exists():remove_tree(models,temp)

def serve(root,models,identity):
    if not re.fullmatch('(norman|custom-[0-9a-f]{20})',identity):raise ValueError('Invalid model identifier')
    directory=models/identity;meta=json_file(directory/'voice.json');model=directory/'voice.onnx';config=directory/'voice.onnx.json'
    if digest(model)!=meta['model_sha256'] or digest(config)!=meta['config_sha256']:raise ValueError('Model checksum changed; import again')
    j=validate_model(model,config)
    os.environ['OMP_NUM_THREADS']='2';os.environ['OPENBLAS_NUM_THREADS']='1'
    import onnxruntime
    onnxruntime.disable_telemetry_events()
    from piper import PiperVoice, SynthesisConfig
    from piper.config import PiperConfig
    options=onnxruntime.SessionOptions();options.intra_op_num_threads=2;options.inter_op_num_threads=1;options.execution_mode=onnxruntime.ExecutionMode.ORT_SEQUENTIAL
    options.add_session_config_entry("session.intra_op.allow_spinning","0");options.add_session_config_entry("session.inter_op.allow_spinning","0")
    session=onnxruntime.InferenceSession(str(model),sess_options=options,providers=['CPUExecutionProvider'])
    voice=PiperVoice(session=session,config=PiperConfig.from_dict(j),use_tashkeel=False,download_dir=directory)
    emit(type='ready',sample_rate=voice.config.sample_rate,speakers=voice.config.num_speakers)
    while True:
        line=sys.stdin.buffer.readline(8193)
        if not line:break
        if len(line)>8192 or not line.endswith(b'\n'):raise ValueError('Speech request exceeds limits')
        message=json.loads(line);identity=message.get('id');text=message.get('text');rate=message.get('rate',0);speaker=message.get('speaker',0)
        if not isinstance(identity,int) or not isinstance(text,str) or not 0<len(text)<=384 or not isinstance(rate,int) or not -5<=rate<=5 or not isinstance(speaker,int) or not 0<=speaker<voice.config.num_speakers:raise ValueError('Invalid speech request')
        output=bytearray();config=SynthesisConfig(speaker_id=speaker,length_scale=2**(-rate/6))
        for chunk in voice.synthesize(text,syn_config=config):
            if chunk.sample_width!=2 or chunk.sample_channels!=1:raise ValueError('Unsupported audio format')
            output.extend(chunk.audio_int16_bytes)
            if len(output)>MAX_AUDIO:raise ValueError('Speech audio exceeds limits')
        emit(type='audio',id=identity,sample_rate=voice.config.sample_rate,pcm=base64.b64encode(output).decode('ascii'))

def main():
    parser=argparse.ArgumentParser();parser.add_argument('action',choices=['install','norman','import','serve']);parser.add_argument('--root',type=Path,required=True);parser.add_argument('--models',type=Path);parser.add_argument('--source');parser.add_argument('--voice');args=parser.parse_args();no_links(args.root)
    if args.action=='install':install_runtime(args.root)
    else:
        if args.models is None:raise ValueError('Models directory required')
        no_links(args.models)
        if args.action=='norman':download_norman(args.root,args.models)
        elif args.action=='import':import_model(args.models,args.source)
        else:serve(args.root,args.models,args.voice)
if __name__=='__main__':
    try:main()
    except Exception as e:emit(type='error',message=str(e)[:180]);sys.exit(1)
