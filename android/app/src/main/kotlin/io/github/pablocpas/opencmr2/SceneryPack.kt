package io.github.pablocpas.opencmr2

import android.content.Context
import android.net.Uri
import android.os.StatFs
import android.provider.DocumentsContract
import android.provider.DocumentsContract.Document
import java.io.File
import java.io.IOException
import java.nio.file.Files
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.ensureActive

/**
 * A prepared scenery pack (3D trees, animated spectators), imported from a
 * user-picked folder. Packs are prepared with the desktop build - preparing
 * runs Python tooling (src/platform/sys.cpp PrepareEnhancementPack) that the
 * Android build cannot run - then copied to the device. The game reads the
 * pack through --scenery-dir (game/SceneryPack.cpp, Sys_OpenSceneryFile in
 * src/platform/sys.cpp): read-only, independent of saves, and never combined
 * with the automatic --trees/--spectators preparation flags.
 *
 * Mirrors GameData's document-tree import, kept separate because a pack is
 * not a game installation: detection is a manifest.tsv at the pack root, not
 * the game-data marker files.
 */
object SceneryPack {
    private fun storage(context: Context): File = context.getExternalFilesDir(null) ?: context.filesDir

    /** The imported pack root; holds manifest.tsv and the pack assets. */
    fun directory(context: Context): File = File(storage(context), "scenery")

    /** Parsed manifest mappings, or 0 when no valid pack is installed. */
    fun mappings(context: Context): Int = parseManifest(File(directory(context), "manifest.tsv"))

    fun isInstalled(context: Context): Boolean = mappings(context) > 0

    fun remove(context: Context) {
        directory(context).deleteRecursively()
    }

    /** References in [manifest] that escape the pack; empty when all stay inside. */
    internal fun manifestReferences(manifest: File): List<String> {
        fun unsafe(path: String): Boolean {
            if (path.isEmpty() || path.startsWith('/') || path.contains('\\') ||
                path.contains(':') || path.endsWith('/')) return true
            return path.split('/').any { it.isEmpty() || it == "." || it == ".." }
        }
        if (!manifest.isFile) return emptyList()
        val bad = mutableListOf<String>()
        for (line in manifest.readLines()) {
            if (line.isBlank()) continue
            val columns = line.split('\t')
            if (columns.size != 6) continue
            if (unsafe(columns[3])) bad += columns[3]
            if (columns[4].isNotEmpty())
                for (frame in columns[4].split(',')) if (unsafe(frame)) bad += frame
        }
        return bad
    }

    /** Symlinks below [root] (relative paths); the importer writes regular files only. */
    internal fun symlinksBelow(root: File): List<String> {
        if (!root.isDirectory) return emptyList()
        return root.walkTopDown()
            .filter { Files.isSymbolicLink(it.toPath()) }
            .map { it.relativeTo(root).path }
            .toList()
    }

    /** Replace [target] with [temporary]; on failure the previous pack is restored. */
    internal fun swapIn(temporary: File, target: File) {
        val backup = File(target.parentFile, target.name + ".replaced")
        backup.deleteRecursively()
        if (target.exists() && !target.renameTo(backup))
            throw IOException("Cannot set aside the installed pack at $target")
        if (!temporary.renameTo(target)) {
            backup.renameTo(target)
            throw IOException("Cannot move the copied files to $target")
        }
        backup.deleteRecursively()
    }

    /** manifest.tsv columns: kind source_archive source_texture asset frames ticks. */
    internal fun parseManifest(manifest: File): Int {
        if (!manifest.isFile || manifest.length() > 16L shl 20) return 0
        var count = 0
        for (line in manifest.readLines()) {
            if (line.isBlank()) continue
            val columns = line.split('\t')
            if (columns.size == 6 && (columns[0] == "tree" || columns[0] == "spectator")) count++
        }
        return count
    }

    sealed interface Progress {
        data object Searching : Progress
        data class Copying(val copied: Long, val total: Long) : Progress
    }

    sealed class ImportException(message: String) : IOException(message) {
        class NotFound : ImportException("No scenery pack found")
        class Ambiguous(val folders: List<String>) : ImportException("More than one scenery pack found")
        class NoSpace(val needed: Long, val available: Long) : ImportException("Not enough space")
        class Unsafe(val reference: String) : ImportException("Pack references a file outside itself")
    }

    suspend fun import(context: Context, tree: Uri, onProgress: (Progress) -> Unit) {
        onProgress(Progress.Searching)
        val source = DocumentTree(context.contentResolver, tree)
        val found = source.findPacks()
        val root = when (found.size) {
            0 -> throw ImportException.NotFound()
            1 -> found.single()
            else -> throw ImportException.Ambiguous(found.map { it.path })
        }
        val files = source.listFiles(root.documentId)
        val total = files.sumOf { it.second.size }

        val target = directory(context)
        val temporary = File(storage(context), "scenery.import")
        temporary.deleteRecursively()
        // Peak usage holds the old pack and the full new copy at once; the old
        // pack's bytes are freed only after the copy completes, so they do not
        // count toward what the copy may use. (Packs are small; GameData's
        // looser replacement credit exists for gigabyte game installs.)
        val available = StatFs(storage(context).path).availableBytes
        val margin = 64L shl 20
        if (available < total + margin)
            throw ImportException.NoSpace(total + margin, available)

        try {
            var copied = 0L
            var reported = 0L
            val buffer = ByteArray(1 shl 20)
            onProgress(Progress.Copying(0, total))
            for ((path, entry) in files) {
                val file = File(temporary, path)
                file.parentFile!!.mkdirs()
                source.open(entry.documentId).use { input ->
                    file.outputStream().use { output ->
                        while (true) {
                            currentCoroutineContext().ensureActive()
                            val n = input.read(buffer)
                            if (n < 0) break
                            output.write(buffer, 0, n)
                            copied += n
                            if (copied - reported >= (4 shl 20)) {
                                onProgress(Progress.Copying(copied, total))
                                reported = copied
                            }
                        }
                    }
                }
            }
            onProgress(Progress.Copying(total, total))
            // Every manifest reference must resolve inside the pack: the
            // native loader enforces this at read time too, but a bad pack
            // should fail the import, not silently render original assets.
            // Runs before the swap, so a rejection keeps the installed pack.
            for (reference in manifestReferences(File(temporary, "manifest.tsv")))
                throw ImportException.Unsafe(reference)
            // The importer never writes links; one present anyway can point
            // anywhere on the device, so it fails the import the same way.
            for (link in symlinksBelow(temporary))
                throw ImportException.Unsafe(link)
            // Swap, never delete-then-move: a failed move restores the
            // installed pack instead of leaving the game without it.
            swapIn(temporary, target)
        } finally {
            temporary.deleteRecursively()
        }
    }

    /** Mirrors GameData.DocumentTree against a user-picked document tree. */
    private class DocumentTree(private val resolver: android.content.ContentResolver, private val tree: Uri) {
        data class Entry(val documentId: String, val name: String, val isDirectory: Boolean, val size: Long)
        data class Folder(val documentId: String, val path: String)

        fun children(documentId: String): List<Entry> {
            val uri = DocumentsContract.buildChildDocumentsUriUsingTree(tree, documentId)
            val projection = arrayOf(
                Document.COLUMN_DOCUMENT_ID, Document.COLUMN_DISPLAY_NAME, Document.COLUMN_MIME_TYPE, Document.COLUMN_SIZE,
            )
            val entries = mutableListOf<Entry>()
            resolver.query(uri, projection, null, null, null)?.use { cursor ->
                while (cursor.moveToNext()) {
                    entries += Entry(
                        documentId = cursor.getString(0),
                        name = cursor.getString(1) ?: continue,
                        isDirectory = cursor.getString(2) == Document.MIME_TYPE_DIR,
                        size = if (cursor.isNull(3)) 0 else cursor.getLong(3),
                    )
                }
            }
            return entries
        }

        fun open(documentId: String) =
            resolver.openInputStream(DocumentsContract.buildDocumentUriUsingTree(tree, documentId))
                ?: throw IOException("Cannot open $documentId")

        private fun hasManifest(documentId: String): Boolean =
            children(documentId).any { !it.isDirectory && it.name == "manifest.tsv" }

        /** The pack root itself or one level of folders holding a manifest.tsv. */
        fun findPacks(): List<Folder> {
            val root = DocumentsContract.getTreeDocumentId(tree)
            if (hasManifest(root)) return listOf(Folder(root, ""))
            return children(root).filter { it.isDirectory && hasManifest(it.documentId) }
                .map { Folder(it.documentId, it.name) }
        }

        /** Every file below [documentId], as relative slash paths to entries. */
        fun listFiles(documentId: String): List<Pair<String, Entry>> {
            val out = mutableListOf<Pair<String, Entry>>()
            fun walk(id: String, prefix: String) {
                for (entry in children(id)) {
                    if (entry.name.contains('/') || entry.name.contains('\\') || entry.name == ".." || entry.name == ".") continue
                    if (entry.isDirectory) walk(entry.documentId, prefix + entry.name + "/")
                    else out += (prefix + entry.name) to entry
                }
            }
            walk(documentId, "")
            return out
        }
    }
}
