import copy,json,tempfile,unittest
from pathlib import Path
from consolidate_recordings import merge,plan,apply,recover_legacy

def fixture(segment):
    stamp={'match_id':'a'*32,'segment':segment,'match_elapsed_ms':(segment-1)*2000}
    return [{'schema':3,'event':'recording_started','timeline':stamp,'mod_version':'fixture','game_sha256':'b'*64},
            {'event':'observed_state','elapsed_ms':0,'timeline':stamp,'sequence':1,'tracking':{'round':segment}},
            {'event':'recording_stopped','elapsed_ms':1000,'timeline':stamp,'tracking':{'round':segment,'round_end_observed':True}}]

class ConsolidationTests(unittest.TestCase):
    def test_merge_preserves_rounds_and_clock(self):
        rows=merge([fixture(3),fixture(1),fixture(2)])
        self.assertEqual(rows[0]['schema'],4)
        self.assertEqual([r['tracking']['round'] for r in rows if r['event']=='round_ended'],[1,2,3])
        self.assertEqual(rows[-1]['elapsed_ms'],5000)
        self.assertEqual([r['sequence'] for r in rows if 'sequence' in r],[1,2,3])
    def test_refuses_ambiguous_data(self):
        for segments in [[fixture(1),fixture(1)],[fixture(1),fixture(3)]]:
            with self.assertRaises(ValueError):merge(segments)
        second=fixture(2);second[0]['timeline']=dict(second[0]['timeline'],match_elapsed_ms=500)
        with self.assertRaises(ValueError):merge([fixture(1),second])
        second=fixture(2);second[-1]['event']='heartbeat'
        with self.assertRaises(ValueError):merge([fixture(1),second])
        second=fixture(2);second[-1]['timeline']=dict(second[-1]['timeline'],match_id='c'*32)
        with self.assertRaises(ValueError):merge([fixture(1),second])
    def test_legacy_requires_multiple_continuity_checks(self):
        files=[]
        for rn in (1,2,3):
            rows=fixture(rn);rows[0]['schema']=2
            for row in rows:row.pop('timeline',None)
            for row,level in [(rows[1],(rn-1)*4),(rows[-1],rn*4)]:
                row['tracking']={'round':rn,'full_round_timing':True,'round_end_observed':row['event']=='recording_stopped',
                    'metrics':{'className':'Caster','level':level},'availability':{'className':'observed','level':'observed'}}
            path=Path(f'session-20261004-15000{rn}-{rn*2000}-123-{rn}.jsonl')
            files.append((path,json.dumps(rows).encode(),rows))
        result=recover_legacy(files)
        self.assertIn('inferred',result[0]['grouping_basis'])
        self.assertEqual([r['tracking']['round'] for r in result if r['event']=='round_ended'],[1,2,3])
        wrong=copy.deepcopy(files);wrong[1][2][1]['tracking']['metrics']['level']=0
        with self.assertRaises(ValueError):recover_legacy(wrong)
        wrong=copy.deepcopy(files);wrong[1]=(Path('session-20261004-150002-4000-999-2.jsonl'),wrong[1][1],wrong[1][2])
        with self.assertRaises(ValueError):recover_legacy(wrong)
        wrong=copy.deepcopy(files);wrong[2][2][-1]['tracking']['full_round_timing']=False
        with self.assertRaises(ValueError):recover_legacy(wrong)

    def test_backup_and_no_duplicate_files(self):
        with tempfile.TemporaryDirectory() as root:
            folder=Path(root)/'recordings';folder.mkdir();originals={}
            for i in (1,2,3):
                data='\n'.join(json.dumps(r) for r in fixture(i)).encode();originals[f'{i}.jsonl']=data;(folder/f'{i}.jsonl').write_bytes(data)
            (folder/'legacy.jsonl').write_text('{"schema":2,"event":"recording_started"}')
            self.assertEqual(len(plan(folder)),1)
            backup=Path(root)/'backup';self.assertEqual(apply(folder,backup)['matches_merged'],1)
            self.assertEqual(len(list(folder.glob('*.jsonl'))),2)
            for name,data in originals.items():self.assertEqual((backup/name).read_bytes(),data)
            self.assertEqual(plan(folder),[])
