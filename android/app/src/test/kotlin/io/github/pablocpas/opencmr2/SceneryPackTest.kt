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
}
