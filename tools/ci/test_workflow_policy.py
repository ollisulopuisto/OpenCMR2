"""Regression checks for the intentionally small build-event policy.

Run with Python3 and PyYAML when reviewing workflow edits. Native CI retains
its existing coverage; this test is a local workflow-change review check.
"""
from pathlib import Path
import unittest
import yaml

ROOT=Path(__file__).resolve().parents[2]
class Policy(unittest.TestCase):
 def setUp(self):
  self.w=yaml.safe_load((ROOT/'.github/workflows/build.yml').read_text())
  self.events=self.w.get('on',self.w.get(True)) # YAML1.1 recognizes on as bool
 def test_feature_push_deduplicated(self):
  self.assertEqual(self.events['push']['branches'],['main'])
  self.assertEqual(self.events['push']['tags'],['v*'])
  self.assertIn('pull_request',self.events)
 def test_conservative_docs_filter_and_explicit_full_path(self):
  self.assertEqual(self.events['push']['paths-ignore'],self.events['pull_request']['paths-ignore'])
  self.assertEqual(set(self.events['push']['paths-ignore']),{'docs/**','**/*.md','LICENSE*'})
  self.assertIn('workflow_dispatch',self.events)
  # No inverted source allowlist: unknown code paths must still run all checks.
  self.assertNotIn('paths',self.events['pull_request'])
 def test_no_platform_coverage_removed(self):
  jobs=self.w['jobs'];self.assertEqual(set(jobs),{'linux-x86','linux','macos','windows','android','release'})
  self.assertEqual([x['arch']for x in jobs['linux']['strategy']['matrix']['include']],['x86_64','arm64'])
  for name in ('linux-x86','linux','macos','windows'):
   runs=[s.get('run','')for s in jobs[name]['steps']]
   self.assertTrue(any('cmake --build --preset' in r for r in runs))
   self.assertTrue(any('ctest --timeout 120 --preset' in r for r in runs))
  self.assertTrue(any('./gradlew assembleRelease' in s.get('run','')for s in jobs['android']['steps']))
 def test_only_pr_runs_cancel(self):
  c=self.w['concurrency'];self.assertEqual(c['cancel-in-progress'],"${{ github.event_name == 'pull_request' }}")
  self.assertIn('github.event_name',c['group']);self.assertIn('github.event.pull_request.number',c['group'])
 def test_release_dependencies_and_artifact_steps_preserved(self):
  release=self.w['jobs']['release'];self.assertEqual(release['if'],"startsWith(github.ref, 'refs/tags/v')")
  self.assertEqual(release['needs'],['linux-x86','linux','macos','windows','android'])
  self.assertTrue(any('gh release create' in s.get('run','')for s in release['steps']))
  for name in release['needs']:
   steps=self.w['jobs'][name]['steps'];self.assertTrue(any(s.get('uses','').startswith('actions/upload-artifact@')for s in steps))
 def test_jobs_bounded_and_no_added_storage_or_permissions(self):
  for job in self.w['jobs'].values():self.assertTrue(1<=job['timeout-minutes']<=90)
  self.assertEqual(self.w['permissions'],{'contents':'read'})
  self.assertEqual(self.w['jobs']['release']['permissions'],{'contents':'write'})
  self.assertNotIn('actions/cache@',(ROOT/'.github/workflows/build.yml').read_text())
if __name__=='__main__':unittest.main()
