"""Validate Android's dependency catalog without starting Gradle or an emulator."""
from pathlib import Path
import unittest

try:
    import tomllib
except ImportError:
    tomllib = None

ROOT = Path(__file__).resolve().parents[1]

@unittest.skipIf(tomllib is None, 'TOML validation needs Python 3.11 or newer')
class AndroidCatalog(unittest.TestCase):
    def test_catalog_parses_and_keeps_test_libraries(self):
        catalog = tomllib.loads((ROOT / 'android/gradle/libs.versions.toml').read_text())
        for name in ('junit', 'robolectric'):
            library = catalog['libraries'][name]
            self.assertEqual(library['name'], name)
            self.assertIn(library['version']['ref'], catalog['versions'])

    def test_duplicate_key_regression_fixture(self):
        with self.assertRaises(tomllib.TOMLDecodeError):
            tomllib.loads('[libraries]\njunit = 1\njunit = 2\n')
        parsed = tomllib.loads('[versions]\njunit = 1\n[libraries]\njunit = 2\n')
        self.assertEqual(parsed['libraries']['junit'], 2)

    def test_junit_dependency_declared_once(self):
        script = (ROOT / 'android/app/build.gradle.kts').read_text()
        declarations = [line.strip() for line in script.splitlines()]
        self.assertEqual(declarations.count('testImplementation(libs.junit)'), 1)

if __name__ == '__main__':
    unittest.main()
