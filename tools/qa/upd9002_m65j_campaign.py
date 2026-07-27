#!/usr/bin/env python3
"""Generate deterministic M65j selector decomposition artifacts."""
# Copyright (c) 2026 Nakata Maho
import hashlib, json, pathlib
ROOT = pathlib.Path(__file__).resolve().parents[2]
BASE = "efd96b7e46717e7ee56e086f7d27ba42b04b49d3"
SRC = ROOT / "tests/ssts/evidence/g65/implementation_missing_inventory.json"
OUT = ROOT / "tests/ssts/campaigns/g65m/m65j"
def canon(x): return json.dumps(x, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()
def digest(xs): return hashlib.sha256(canon(sorted(xs))).hexdigest()
def main():
    data = json.loads(SRC.read_text())
    groups=[]; all_hashes=[]
    for i,e in enumerate(sorted(data["entries"], key=lambda x:x["selector_sha256"]), 1):
        hs=sorted(e["resolved_test_hashes"]); all_hashes.extend(hs)
        groups.append({"internal_id":f"M65j.{i:02d}","parent_milestone":"M65j","campaign_base_gate":"G65","campaign_base_sha":BASE,"selector":e["selector"],"selector_sha256":e["selector_sha256"],"hashes":hs,"count":len(hs),"hash_set_sha256":digest(hs),"classification_before":"known_target_gap","gap_kind_before":"implementation_missing","target_authority":e["selector"].get("support_target"),"corpus_availability":"resolved v20 hashes","disposition":"internal_semantic_work_package","canonical_task_assignment":None,"status":"complete_pending_campaign_gate"})
    assert len(all_hashes)==5908 and len(all_hashes)==len(set(all_hashes))
    OUT.mkdir(parents=True, exist_ok=True)
    def write(name,obj): (OUT/name).write_text(json.dumps(obj,sort_keys=True,indent=2)+"\n")
    write("selector_decomposition.json",{"schema":"vaeg-upd9002-m65j-decomposition-v1","groups":groups})
    write("ownership_mapping.json",{"schema":"vaeg-upd9002-m65j-ownership-v1","groups":[{"internal_id":g["internal_id"],"selector_sha256":g["selector_sha256"],"hashes":g["hashes"],"count":g["count"],"hash_set_sha256":g["hash_set_sha256"],"disposition":g["disposition"]} for g in groups]})
    write("work_package_manifest.json",{"schema":"vaeg-upd9002-m65j-work-packages-v1","work_packages":groups})
    write("disposition_summary.json",{"schema":"vaeg-upd9002-m65j-disposition-v1","group_count":len(groups),"hash_count":len(all_hashes),"hash_set_sha256":digest(all_hashes),"unresolved":[]})
    write("dependency_update.json",{"schema":"vaeg-upd9002-m65j-dependency-update-v1","edges":[{"from":"M65j","to":g["internal_id"],"type":"campaign_serialization_dependency"} for g in groups],"canonical_next":"M65a","m65m_terminal":True})
    write("coverage_proof.json",{"schema":"vaeg-upd9002-m65j-coverage-v1","expected_count":5908,"actual_count":len(all_hashes),"expected_digest":data["hash_set_sha256"],"actual_digest":digest(all_hashes),"pairwise_disjoint":True,"unowned":[]})
    write("manifest.json",{"schema":"vaeg-upd9002-m65j-manifest-v1","approved_g65_sha":BASE,"campaign_branch":"topic/m65-residue-campaign","implementation_missing_count":5908,"implementation_missing_digest":data["hash_set_sha256"],"group_count":len(groups),"production_semantic_change":False})
    print(f"M65j groups={len(groups)} hashes={len(all_hashes)} digest={digest(all_hashes)}")
if __name__ == "__main__": main()
