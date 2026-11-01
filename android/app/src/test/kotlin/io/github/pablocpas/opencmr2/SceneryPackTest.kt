package io.github.pablocpas.opencmr2

import java.io.File
import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class SceneryPackTest {
    @get:Rule val folder = TemporaryFolder()

    @Test
    fun parseManifestCountsValidEntries() {
        val manifest = folder.newFile("manifest.tsv")
        manifest.writeText(
            "tree\tfin01hi.bfl\ttree01.dds\ttrees/a.obj\t\t5\n" +
            "spectator\tfin01hi.bfl\tspec.tga\tspec/a.tga\ta1.tga,a2.tga\t10\n" +
            "bush\tignored.kind\tbad\tbad\tbad\tbad\n" +
            "tree\tmissing-columns\n" +
            "\n",
        )
        assertEquals(2, SceneryPack.parseManifest(manifest))
    }

    @Test
    fun parseManifestMissingFileIsZero() {
        assertEquals(0, SceneryPack.parseManifest(File(folder.root, "absent.tsv")))
    }

    @Test
    fun manifestReferencesFlagEscapesOnly() {
        val manifest = folder.newFile("manifest.tsv")
        manifest.writeText(
            "tree\tfin01hi.bfl\ttree01.dds\ttrees/a.obj\t\t5\n" +
            "spectator\tfin01hi.bfl\tspec.tga\tspec/a.tga\ta1.tga,a2.tga\t10\n" +
            "spectator\tfin01hi.bfl\tspec.tga\t../escape.tga\ta.tga\t10\n" +
            "spectator\tfin01hi.bfl\tspec.tga\tspec/a.tga\t/abs.tga\t10\n" +
            "spectator\tfin01hi.bfl\tspec.tga\tspec/a.tga\tC:\\win.tga\t10\n",
        )
        assertEquals(listOf("../escape.tga", "/abs.tga", "C:\\win.tga"),
            SceneryPack.manifestReferences(manifest).sorted())
    }

    @Test
    fun manifestReferencesFlagNestedDotDot() {
        val manifest = folder.newFile("manifest.tsv")
        manifest.writeText(
            "tree\tfin01hi.bfl\ttree01.dds\ttrees/a.obj\t\t5\n" +
            "tree\tfin01hi.bfl\ttree01.dds\ttrees/../../escape.obj\t\t5\n" +
            "tree\tfin01hi.bfl\ttree01.dds\ttrees/./a.obj\t\t5\n",
        )
        assertEquals(listOf("trees/../../escape.obj", "trees/./a.obj"),
            SceneryPack.manifestReferences(manifest).sorted())
    }

    @Test
    fun symlinksBelowFindsLinksOnly() {
        val pack = folder.newFolder("pack")
        File(pack, "real.tga").writeText("x")
        java.nio.file.Files.createSymbolicLink(File(pack, "link.tga").toPath(), File("/etc/hostname").toPath())
        assertEquals(listOf("link.tga"), SceneryPack.symlinksBelow(pack))
    }

    @Test
    fun swapInRestoresInstalledPackWhenMoveFails() {
        val target = folder.newFolder("scenery")
        File(target, "manifest.tsv").writeText("tree\told\told\told.obj\t\t5\n")
        val missing = File(folder.root, "does-not-exist")
        try {
            SceneryPack.swapIn(missing, target)
            throw AssertionError("swapIn should have failed")
        } catch (_: java.io.IOException) {
            // expected
        }
        assertEquals("tree\told\told\told.obj\t\t5\n", File(target, "manifest.tsv").readText())
    }

    @Test
    fun swapInReplacesInstalledPack() {
        val target = folder.newFolder("scenery")
        File(target, "old.txt").writeText("old")
        val incoming = folder.newFolder("incoming")
        File(incoming, "manifest.tsv").writeText("tree\tnew\tnew\tnew.obj\t\t5\n")
        SceneryPack.swapIn(incoming, target)
        assertEquals("tree\tnew\tnew\tnew.obj\t\t5\n", File(target, "manifest.tsv").readText())
        assertEquals(false, File(target, "old.txt").exists())
        assertEquals(false, incoming.exists())
    }
}
