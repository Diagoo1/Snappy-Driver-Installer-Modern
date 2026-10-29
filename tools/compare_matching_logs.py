"""Compare actual matching rows, including order, from two local SDI logs."""
import argparse
import json
import re
from collections import Counter
from pathlib import Path

def read_rows(path):
    text=Path(path).read_bytes().decode('cp1251',errors='replace')
    section=text.rsplit('{matcher_print[',1)[-1].split('}matcher_print',1)[0]
    rows=[]
    for line in section.splitlines():
        if not re.match(r'^\s*\d+\s*\|\s*[A-Fa-f0-9]{8}\s*\|',line):continue
        fields=[part.strip() for part in line.split('|')]
        if len(fields)<14:raise ValueError('Incomplete matching log row')
        # The repaired loader binds the package name to the current archive;
        # the legacy executable can log stale path strings from its cached index.
        rows.append(tuple(fields[i].casefold() for i in (1,5,6,9,11,12)))
    return rows

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('reference');parser.add_argument('actual');parser.add_argument('--output')
    args=parser.parse_args()
    reference,actual=read_rows(args.reference),read_rows(args.actual)
    differences=[i for i in range(min(len(reference),len(actual))) if reference[i]!=actual[i]]
    # Distinguish eligibility/rank changes from missing or invented driver candidates.
    identities=lambda rows:Counter((r[2],r[3],r[4],r[5]) for r in rows)
    statuses=lambda rows:Counter(rows)
    result=dict(referenceRows=len(reference),actualRows=len(actual),
                sameOrderedMatchingRows=bool(reference) and reference==actual,
                differingRowCount=len(differences),firstDifferingRows=differences[:10],
                sameCandidateIdentityMultiset=bool(reference) and identities(reference)==identities(actual),
                differingScoreOrStatusRows=sum((statuses(reference)-statuses(actual)).values()),
                comparedFields=['score','status','INF section','INF path','version','HWID'])
    print(json.dumps(result,indent=2))
    if args.output:Path(args.output).write_text(json.dumps(result,indent=2)+'\n')
    raise SystemExit(0 if result['sameOrderedMatchingRows'] else 1)
