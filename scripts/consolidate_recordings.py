"""Consolidate closed schema-three segments using explicit match IDs only.

Offline, no uploading. Originals are verified in a separate backup before
replacement. Ambiguous, overlapping, active and legacy files are left alone.
"""
import argparse
import copy
import hashlib
import json
import os
import re
from datetime import datetime, timezone
from pathlib import Path
import shutil


def merge(rows_by_file):
    items=[]
    for rows in rows_by_file:
        h=rows[0]
        if h.get('schema')!=3 or h.get('event')!='recording_started' or rows[-1].get('event')!='recording_stopped':
            raise ValueError('not_closed_segments')
        stamp=h['timeline']; ident=stamp['match_id'];segment=stamp['segment'];offset=stamp['match_elapsed_ms']
        if not isinstance(ident,str) or len(ident)!=32 or any(c not in '0123456789abcdef' for c in ident):raise ValueError('identity')
        if type(segment) is not int or type(offset) is not int or segment<1 or offset<0:raise ValueError('timeline')
        items.append((segment,offset,rows))
    items.sort(key=lambda x:x[0])
    if len(items)<2:raise ValueError('single_file')
    first=items[0][2][0];ident=first['timeline']['match_id'];origin=items[0][1]
    result=[copy.deepcopy(first)];result[0]['schema']=4;result[0]['consolidated_from_schema']=3
    previous=-1;sequence=0;last_segment=items[0][0]-1
    for segment,offset,rows in items:
        if segment!=last_segment+1:raise ValueError('ambiguous_segments')
        h=rows[0]
        if any(h.get(k)!=first.get(k) for k in ('game_sha256','mod_version')):raise ValueError('provenance')
        for row in rows[1:]:
            stamp=row.get('timeline',{})
            if stamp.get('match_id')!=ident or stamp.get('segment')!=segment:raise ValueError('identity')
            time=row.get('elapsed_ms')
            if type(time) is not int or time<0:raise ValueError('time')
            time+=offset-origin
            if time<previous:raise ValueError('overlapping_segments')
            previous=time;entry=copy.deepcopy(row);entry['elapsed_ms']=time
            if entry.get('event')=='recording_stopped':entry['event']='round_ended'
            if 'sequence' in entry:sequence+=1;entry['sequence']=sequence
            result.append(entry)
        last_segment=segment
    result.append({'event':'recording_stopped','elapsed_ms':previous,'tracking':None,
                   'timeline':copy.deepcopy(result[-1]['timeline']),
                   'dropped_events':result[-1].get('dropped_events',0)})
    return result


def recover_legacy(files):
    """Conservative reconstruction, explicitly marked as inferred, not official."""
    if len(files)!=3:raise ValueError('three_rounds_required')
    converted=[];prior=None;origin=None;cls=None;pid=None;serial=None;game=None
    identity=hashlib.sha256(b''.join(raw for _,raw,_ in files)).hexdigest()[:32]
    for expected,(path,raw,rows) in enumerate(files,1):
        m=re.fullmatch(r'session-(\d{8})-(\d{6})-(\d+)-(\d+)-(\d+)\.jsonl',path.name)
        if not m or rows[0].get('schema')!=2 or rows[-1].get('event')!='recording_stopped':raise ValueError('legacy_shape')
        tick,process,sequence=map(int,m.group(3,4,5));duration=rows[-1].get('elapsed_ms')
        if type(duration) is not int or duration<0:raise ValueError('duration')
        samples=[r['tracking'] for r in rows if isinstance(r.get('tracking'),dict)]
        if not samples or any(t.get('round')!=expected for t in samples):raise ValueError('round_sequence')
        first,last=samples[0],samples[-1]
        if not last.get('round_end_observed') or not last.get('full_round_timing'):raise ValueError('partial_round')
        def metric(t,key):
            if t.get('availability',{}).get(key)!='observed':raise ValueError('unobserved')
            return t['metrics'][key]
        start_level,end_level=metric(first,'level'),metric(last,'level');name=metric(first,'className')
        if metric(last,'className')!=name or not isinstance(start_level,(int,float)) or not isinstance(end_level,(int,float)):raise ValueError('class_or_level')
        if expected==1:
            if start_level!=0:raise ValueError('not_round_one_start')
            origin=tick;cls=name;pid=process;serial=sequence;game=rows[0].get('game_sha256')
        elif process!=pid or sequence!=serial+expected-1 or name!=cls or start_level!=prior[1] or not 0<=tick-prior[0]<=60000 or rows[0].get('game_sha256')!=game:
            raise ValueError('continuity_not_established')
        output=copy.deepcopy(rows)
        output[0]['schema']=3
        output[0]['started_utc_ms']=int(datetime.strptime(m.group(1)+m.group(2),'%Y%m%d%H%M%S').replace(tzinfo=timezone.utc).timestamp()*1000)
        for i,row in enumerate(output):
            elapsed=0 if i==0 else row.get('elapsed_ms')
            if type(elapsed) is not int or elapsed<0:raise ValueError('time')
            row['timeline']={'match_id':identity,'segment':expected,'match_elapsed_ms':tick-origin+elapsed}
        converted.append(output);prior=(tick+duration,end_level)
    result=merge(converted);result[0]['consolidated_from_schema']=2
    result[0]['grouping_basis']='inferred_consecutive_process_rounds_class_level_continuity'
    return result


def plan(folder,recover=False):
    folder=Path(folder).resolve();groups={};blocked=set();legacy=[]
    for p in sorted(folder.glob('*.jsonl')):
        if p.resolve().parent!=folder or p.is_symlink():raise ValueError('linked_recording')
        if p.stat().st_size>10*1024*1024:continue
        raw=p.read_bytes()
        try:
            rows=[json.loads(line) for line in raw.splitlines()]
            h=rows[0];ident=h.get('timeline',{}).get('match_id')
            if recover and h.get('schema')==2:legacy.append((p,raw,rows))
            if not isinstance(ident,str):continue
            if h.get('schema')!=3:blocked.add(ident);continue
            groups.setdefault(ident,[]).append((p,raw,rows))
        except (ValueError,TypeError,KeyError,IndexError):continue
    jobs=[]
    for ident,files in groups.items():
        if ident in blocked or len(files)<2:continue
        try:rows=merge([item[2] for item in files])
        except (ValueError,KeyError,TypeError):continue
        raw=('\n'.join(json.dumps(row,separators=(',',':'),allow_nan=False) for row in rows)+'\n').encode()
        if len(raw)>10*1024*1024:continue
        target=folder/('session-merged-'+ident+'.jsonl')
        if target.exists():raise ValueError('output_exists')
        jobs.append((files,target,raw))
    if recover:
        i=0
        while i+2<len(legacy):
            files=legacy[i:i+3]
            try:rows=recover_legacy(files)
            except (ValueError,TypeError,KeyError,IndexError):i+=1;continue
            ident=rows[0]['timeline']['match_id'];target=folder/('session-recovered-'+ident+'.jsonl')
            if target.exists():raise ValueError('output_exists')
            raw=('\n'.join(json.dumps(row,separators=(',',':'),allow_nan=False) for row in rows)+'\n').encode()
            if len(raw)<=10*1024*1024:jobs.append((files,target,raw))
            i+=3
    return jobs


def apply(folder,backup,recover=False):
    folder=Path(folder).resolve();backup=Path(backup).resolve()
    if backup==folder or folder in backup.parents:raise ValueError('backup_must_be_outside_recordings')
    jobs=plan(folder,recover);backup.mkdir(parents=True,exist_ok=False)
    for files,target,raw in jobs:
        for path,original,_ in files:
            if path.read_bytes()!=original:raise ValueError('recording_changed')
            shutil.copy2(path,backup/path.name)
            if (backup/path.name).read_bytes()!=original:raise ValueError('backup_failed')
    completed=[]
    try:
        for files,target,raw in jobs:
            for path,original,_ in files:
                if path.read_bytes()!=original:raise ValueError('recording_changed')
            staged=target.with_suffix('.jsonl.tmp')
            with staged.open('xb') as f:f.write(raw);f.flush();os.fsync(f.fileno())
            os.replace(staged,target);completed.append((files,target))
            for path,original,_ in files:
                if path.resolve().parent!=folder or (backup/path.name).read_bytes()!=original:raise ValueError('unsafe_removal')
                path.unlink()
    except BaseException:
        for files,target in completed:
            for path,original,_ in files:
                if not path.exists():path.write_bytes(original)
            if target.exists():target.unlink()
        raise
    return {'matches_merged':len(jobs),'original_files_backed_up':sum(len(j[0]) for j in jobs)}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('folder',type=Path);p.add_argument('--backup',type=Path);p.add_argument('--recover-legacy',action='store_true');a=p.parse_args()
    if a.backup:print(json.dumps(apply(a.folder,a.backup,a.recover_legacy)))
    else:
        jobs=plan(a.folder,a.recover_legacy);print(json.dumps({'eligible_matches':len(jobs),'eligible_files':sum(len(j[0]) for j in jobs)}))
