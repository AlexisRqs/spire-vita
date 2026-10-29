from test_inputs import ROOT, engine_path, elf_path, output
"""Replay alternating color/scale variants through the compiled ARM cache."""
from pathlib import Path
import sys
fixture=(ROOT/'tests/test_text_memo_arm.py').read_text().split("q=get('Damage 6');")[0]
# The existing fixture loads the current built ELF. Baseline is run before edits.
exec(compile(fixture,'tests/test_text_memo_arm.py','exec'))
variants=12;rounds=24
for repeat in range(rounds):
 for i in range(variants):
  pf(color+8,(i+1)/16);pf(color+12,0.25);pf(color+16,0.75);pf(color+20,1)
  pf(data+60,0.8 if i%2 else 1.1)
  q=get('Damage 6');assert q
  expected=sum(word(letters[c]+52) for c in map(ord,'Damage 6'))*gf(data+60)
  assert abs(gf(q+12)-expected)<0.001
  runs=word(q+8);items=word(runs+8);storage=word(items+8);run=word(storage);rc=word(run+28)
  assert bytes(u.mem_read(rc+8,16))==bytes(u.mem_read(color+8,16))
report={'calls':variants*rounds,'variants':variants,'hits':word(app['memo_hits']),'misses':word(app['memo_misses']),'evictions':word(app['memo_evict']),'checks':word(app['memo_checks']),'mismatches':word(app['memo_mismatch']),'layouts':len(created),'limits':['Modelled managed layout and GPU','No measured Vita FPS gain']}
if '--baseline' not in sys.argv:
 assert report['misses']==variants and report['evictions']==0 and report['mismatches']==0,report
 assert report['hits']==variants*(rounds-1),report
output('text-memo-variants-'+('baseline' if '--baseline' in sys.argv else 'validation')+'.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
